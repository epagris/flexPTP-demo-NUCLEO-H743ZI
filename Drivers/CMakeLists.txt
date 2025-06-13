set(H7HAL_ROOT ${CMAKE_CURRENT_LIST_DIR}/stm32h7xx-hal-driver)

set(H7HAL_SRC
    ${H7HAL_ROOT}/Src/Legacy/stm32h7xx_hal_eth.c
    ${H7HAL_ROOT}/Src/Legacy/stm32h7xx_hal_eth_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_adc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_adc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_cec.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_comp.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_cordic.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_cortex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_crc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_crc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_cryp.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_cryp_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dac.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dac_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dcmi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dfsdm.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dfsdm_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dma2d.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dma.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dma_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dsi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_dts.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_eth.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_eth_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_exti.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_fdcan.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_flash.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_flash_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_fmac.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_gfxmmu.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_gpio.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_hash.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_hash_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_hcd.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_hrtim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_hsem.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_i2c.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_i2c_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_i2s.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_i2s_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_irda.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_iwdg.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_jpeg.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_lptim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_ltdc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_ltdc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_mdios.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_mdma.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_mmc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_mmc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_msp_template.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_nand.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_nor.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_opamp.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_opamp_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_ospi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_otfdec.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_pcd.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_pcd_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_pssi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_pwr.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_pwr_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_qspi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_ramecc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rcc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rcc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rng.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rng_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rtc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_rtc_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sai.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sai_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sd.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sd_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sdio.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sdram.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_smartcard.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_smartcard_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_smbus.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_smbus_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_spdifrx.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_spi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_spi_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_sram.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_swpmi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_tim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_tim_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_uart.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_uart_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_usart.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_usart_ex.c
    ${H7HAL_ROOT}/Src/stm32h7xx_hal_wwdg.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_adc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_bdma.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_comp.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_cordic.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_crc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_crs.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_dac.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_delayblock.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_dma2d.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_dma.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_exti.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_fmac.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_fmc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_gpio.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_hrtim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_i2c.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_lptim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_lpuart.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_mdma.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_opamp.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_pwr.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_rcc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_rng.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_rtc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_sdmmc.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_spi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_swpmi.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_tim.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_usart.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_usb.c
    ${H7HAL_ROOT}/Src/stm32h7xx_ll_utils.c
)

add_library(H7HAL STATIC ${H7HAL_SRC})

target_compile_options(H7HAL PUBLIC ${cpu_PARAMS})
target_compile_definitions(H7HAL PUBLIC ${compile_DEFS})
target_include_directories(H7HAL PUBLIC 
    ${H7HAL_ROOT}/Inc
    ${CMAKE_CURRENT_LIST_DIR}/../Inc
    ${CMAKE_CURRENT_LIST_DIR}/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_CURRENT_LIST_DIR}/CMSIS/Include
)

target_include_directories(
    ${CMAKE_PROJECT_NAME} PUBLIC
    ${H7HAL_ROOT}/Inc
)

target_link_libraries(${CMAKE_PROJECT_NAME} H7HAL)

# Add CMSIS includes
add_subdirectory(CMSIS)

# Add custom Ethernet driver
add_subdirectory(EthDrv)
