#include <memory.h>
#include <stdint.h>

#include <stdlib.h>
#include <stm32h7xx_hal.h>

#include <stm32h7xx_ll_rng.h>

#include "FreeRTOSConfig.h"


#include "cliutils/cli.h"
#include "cmds.h"
#include "ethernet/ethernet_lwip.h"

#include "flexptp/event.h"
#include "flexptp/logging.h"
#include "flexptp/profiles.h"
#include "flexptp/ptp_profile_presets.h"
#include "flexptp/settings_interface.h"
#include "standard_output/serial_io.h"
#include "standard_output/standard_output.h"

#include <cmsis_os2.h>


#define FLEXPTP_INITIAL_PROFILE ("gPTP")

// ------------------------

void Error_Handler(void);

// ------------------------

#define TARGET_SYSCLK_MHZ (configCPU_CLOCK_HZ / 1000000)

void init_pll() {
    RCC_OscInitTypeDef osc;
    RCC_ClkInitTypeDef clk;

    // clear the structures
    memset(&osc, 0, sizeof(RCC_OscInitTypeDef));
    memset(&clk, 0, sizeof(RCC_ClkInitTypeDef));

    // ---------------------

    // Configure the Main PLL.

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	osc.HSEState = RCC_HSE_ON; // TODO: turn HSE bypass OFF if using X3 instead of the board controller's clock output
	osc.HSIState = RCC_HSI_OFF;
	osc.CSIState = RCC_CSI_OFF;
	osc.PLL.PLLState = RCC_PLL_ON;
	osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;

	osc.PLL.PLLM = HSE_VALUE / 2000000;
	osc.PLL.PLLN = TARGET_SYSCLK_MHZ;
	osc.PLL.PLLFRACN = 0;
	osc.PLL.PLLP = 2;
	osc.PLL.PLLR = 2;
	osc.PLL.PLLQ = 4;

	osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;


    HAL_RCC_OscConfig(&osc);

    // initialize clock tree
    clk.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                     RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1);
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;
    clk.APB1CLKDivider = RCC_APB1_DIV2;
    clk.APB2CLKDivider = RCC_APB2_DIV2;
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;

    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4); // set a FLASH latency supporting maximum speed

    // ---------------------
}

void init_osc_and_clk() {
    // ensure that the MCU uses the internal LDO instead of anything else
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // configure internal voltage regulator to VOS0 to unlock main clock frequencies above 400MHz
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    // initialize the main PLL
    init_pll();

    // turn on additional clock sources and prepare GPIOs for high-speed operation
    __HAL_RCC_HSI48_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_CSI_ENABLE();
    HAL_EnableCompensationCell();

    // compute SYSCLK values (HSE_VALUE must be correct when invoking this)
    SystemCoreClockUpdate();

    // set tick frequency
    HAL_SetTickFreq(HAL_TICK_FREQ_1KHZ);
}

void print_welcome_message() {
    MSGraw("\033[2J\033[H");
    MSG(ANSI_COLOR_BGREEN "Hi!" ANSI_COLOR_BYELLOW " This is a flexPTP demo for the STMicroelectronics NUCLEO-H743ZI2 (STM32H743) board.\n\n"
                          "The application is built on FreeRTOS, flexPTP is currenty compiled against %s and uses the supplied example %s Network Stack Driver. "
                          "In this demo, the underlying Ethernet stack can be either lwip or EtherLib, the 'ETH_STACK' CMake variable (in the main CMakeLists.txt file) determines which one will be used. "
                          "The STM32H7xx PTP hardware module driver is also picked from the bundled ones. This flexPTP instance features a full CLI control interface, the help can be listed by typing '?' once the flexPTP has loaded. "
                          "The initial PTP preset that loads upon flexPTP initialization is the 'gPTP' (802.1AS) profile. It's a nowadays common profile, but we encourage "
                          "you to also try out the 'default' (plain IEEE 1588) profile and fiddle around with other options as well. The application will try to acquire an IP-address with DHCP. "
                          "Once the IP-address is secured, you might start the flexPTP module by typing 'flexptp'. 'Have a great time! :)'\n\n" ANSI_COLOR_RESET,
        ETH_STACK, ETH_STACK);

    MSG(ANSI_COLOR_BRED "By default, the MCU clock is sourced by the onboard (STLink) board controller on this devboard. According to our observations, this clock signal is loaded "
                        "with heavy noise rendering the clock synchronization unable to settle precisely. We highly recommend to solder a 8 MHz oscillator onto "
                        "the designated X3 pads to achieve the best results!\n\n" ANSI_COLOR_RESET);

    // MSG("Freq: %u\n", SystemCoreClock);
}


void task_startup(void *arg) {
    // initialize the CLI
    cli_init();

    // print greetings
    print_welcome_message();

    // initialize Ethernet
    init_ethernet();

    // initialize commands
    cmd_init();

    // -------------

    for (;;) {
        osDelay(1000);
    }
}

void btn_cb() {
}

void init_randomizer() {
    __HAL_RCC_RNG_CLK_ENABLE();

    LL_RNG_Enable(RNG);

    while (!LL_RNG_IsActiveFlag_DRDY(RNG)) {
    }

    srand(LL_RNG_ReadRandData32(RNG));
}

void init_mpu() {
    HAL_MPU_Disable();
}

int main(void) {
    // initialize FPU and several system blocks
    SystemInit();

    // initialize HAL library
    HAL_Init();

    // initialize oscillator and clocking
    init_osc_and_clk();

    // make random a bit more less deterministic
    init_randomizer();

    // initialize MPU
    init_mpu();

    // initialize standard output
    serial_io_init();

    // -------------

    // initialize the FreeRTOS kernel
    osKernelInitialize();

    // create startup thread
    osThreadAttr_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.stack_size = 2048;
    attr.name = "init";
    osThreadNew(task_startup, NULL, &attr);

    // start the FreeRTOS!
    osKernelStart();

    for (;;) {
    }
}

void flexptp_user_event_cb(PtpUserEventCode uev) {
    switch (uev) {
    case PTP_UEV_INIT_DONE:
        ptp_load_profile(ptp_profile_preset_get(FLEXPTP_INITIAL_PROFILE));
        ptp_print_profile();

        ptp_log_enable(PTP_LOG_DEF, true);
        ptp_log_enable(PTP_LOG_BMCA, true);
        break;
    default:
        break;
    }
}


// ------------------------

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".FreeRTOSHeapSection")));

// ------------------------

void vApplicationTickHook(void) {
    HAL_IncTick();
}

void vApplicationIdleHook(void) {
    return;
}

// --------
