#include "mac_drv.h"
#include "standard_output/standard_output.h"
#include "stm32h7xx.h"

#include <memory.h>

#include <stm32h7xx_hal.h>

__weak uint32_t ETHHW_setupPHY(ETH_TypeDef *eth) {
    (void)eth;
    return MODEINIT_FULL_DUPLEX | MODEINIT_SPEED_100MBPS;
}

// ----------------

static void ETHHW_InitClocks() {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Ethernett MSP init: RMII Mode */

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Ethernet pins configuration ************************************************/
    /*
     RMII_REF_CLK ----------------------> PA1
     RMII_MDIO -------------------------> PA2
     RMII_MDC --------------------------> PC1
     RMII_MII_CRS_DV -------------------> PA7
     RMII_MII_RXD0 ---------------------> PC4
     RMII_MII_RXD1 ---------------------> PC5
     RMII_MII_RXER ---------------------> PG2
     RMII_MII_TX_EN --------------------> PG11
     RMII_MII_TXD0 ---------------------> PG13
     RMII_MII_TXD1 ---------------------> PB13
     */

    /* Configure PA1, PA2 and PA7 */
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure PB13 */
    GPIO_InitStructure.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure PC1, PC4 and PC5 */
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure PG2, PG11, PG13 and PG14 */
    GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_11 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* Enable the Ethernet global Interrupt */
    HAL_NVIC_SetPriority(ETH_IRQn, 0x7, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);

    /* Enable Ethernet clocks */
    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();

}

#ifndef CEIL_TO_4
#define CEIL_TO_4(x) ((((x) >> 2) + (((x) & 0b11) ? 1 : 0)) << 2)
#endif

static void ETHHW_InitPeripheral(ETH_TypeDef *eth, ETHHW_InitOpts *init) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_SYSCFG_ETHInterfaceSelect(SYSCFG_ETH_RMII);

    // reset all MAC internal registers and logic
    SET_BIT(eth->DMAMR, ETH_DMAMR_SWR);

    // wait for reset completion
    while (READ_BIT(eth->DMAMR, ETH_DMAMR_SWR)) {
        asm("nop");
    }

    /* ---- MAC initialization ---- */

    // initialize MDIO clock
    WRITE_REG(eth->MACMDIOAR, ETH_MACMDIOAR_CR_DIV102 << ETH_MACMDIOAR_CR_Pos); // for 150-250MHz HCLK

    // setup PHY
    uint32_t initMode = ETHHW_setupPHY(eth);

    // fill-in MAC address
    uint32_t hwaTmp = 0;
    memcpy(&hwaTmp, init->mac, 4);
    WRITE_REG(eth->MACA0LR, hwaTmp);
    hwaTmp = 0;
    memcpy(&hwaTmp, init->mac + 4, 2);
    WRITE_REG(eth->MACA0HR, hwaTmp);

    // receive using perfect matching and multicast matching
    SET_BIT(eth->MACPFR, ETH_MACPFR_PM | ETH_MACPFR_HPF);

    // create a mode bit subfield based on the ETHHW_setup() return value
    uint32_t mode = 0;
    if (initMode & MODEINIT_FULL_DUPLEX) { // duplex mode
        mode |= ETH_MACCR_DM;
    }
    if (initMode & MODEINIT_SPEED_100MBPS) { // Fast Ethernet
        mode |= ETH_MACCR_FES;
    }

    // set speed and duplex mode AND turn on checksum validation
    WRITE_REG(eth->MACCR, mode | ETH_MACCR_IPC);

    /* ---- MTL initialization ---- */
    WRITE_REG(eth->MTLTQOMR, (0b111 << 16) | ETH_MTLTQOMR_TSF | (0b10 << 2)); // transmit store and forward ON and enable transmit queue
    WRITE_REG(eth->MTLRQOMR, ETH_MTLRQOMR_RSF);                               // receive store and forward ON

    /* ---- DMA configuration ---- */

    // configure DMA system bus mode TODO
    // calculate aligned buffer size
    uint16_t alignedBufSize = CEIL_TO_4(init->blockSize);

    // create RX descriptors; use Extended Descriptors
    uint16_t byteSkip = sizeof(ETHHW_DescExt);                // calculate skip size in bytes
    uint16_t dwordSkip = byteSkip / 4;                        // calculate skip size in dwords (32-bit units), used by the DMA
    ETHHW_DescFull *ring = (ETHHW_DescFull *)init->rxRingPtr; // acquire RX ring begin pointer
    uint8_t *rxBuf = init->bufPtr;

    // fill RX descriptor area
    memset(init->rxRingPtr, 0, sizeof(ETHHW_DescFull) * init->rxRingLen); // clear descriptors
    for (uint16_t i = 0; i < init->rxRingLen; i++) {
        ETHHW_DescFull *bd = ring + i;                                                            // acquire descriptor at the beginning of the ring item
        bd->ext.bufAddr = ((uint32_t)(rxBuf)) + alignedBufSize * i;                               // compute buffer start and store it for subsequent use when the
                                                                                                  // descriptor field get overwritten by the DMA
        bd->desc.DES0 = bd->ext.bufAddr;                                                          // store Buffer 1 address
        bd->desc.DES3 = 0 | ETH_DMARXNDESCRF_OWN | ETH_DMARXNDESCRF_IOC | ETH_DMARXNDESCRF_BUF1V; // set flags: OWN, IOC, BUF1V
    }

    // write receive-related registers
    WRITE_REG(eth->DMACRDRLR, init->rxRingLen - 1);       // write ring length
    WRITE_REG(eth->DMACRDLAR, (uint32_t)init->rxRingPtr); // ring start address
    // WRITE_REG(eth->DMACRDTPR, ((uint32_t ) init->rxRingPtr) + (init->rxRingLen)
    // * byteSkip); // tail pointer
    WRITE_REG(eth->DMACRDTPR, 0); // tail pointer WON'T STOP

    // transmit descriptor initialization
    memset(init->txRingPtr, 0, sizeof(ETHHW_DescFull) * init->txRingLen); // clear everything
    ring = init->txRingPtr;
    uint8_t *txBuf = init->bufPtr + alignedBufSize * init->rxRingLen; // fill in later used buffer addresses
    for (uint16_t i = 0; i < init->txRingLen; i++) {
        ETHHW_DescFull *bd = ring + i;
        bd->ext.bufAddr = ((uint32_t)txBuf) + i * alignedBufSize;
    }

    // write transmit-related registers
    WRITE_REG(eth->DMACTDRLR, init->txRingLen - 1);       // write ring length
    WRITE_REG(eth->DMACTDLAR, (uint32_t)init->txRingPtr); // ring start address
    WRITE_REG(eth->DMACTDTPR, 0);                         // tail pointer WON'T STOP

    // set common DMA-related options
    WRITE_REG(eth->DMACCR, ((dwordSkip & 0b111) << ETH_DMACCR_DSL_Pos));                         // set skip to the size of the extension
    WRITE_REG(eth->DMACTCR, ETH_DMACTCR_TPBL_32PBL);                                             // 32 beats per DMA transfer (TX) [DO NOT START!]
    WRITE_REG(eth->DMACRCR, ETH_DMACRCR_RPBL_32PBL | (init->blockSize << ETH_DMACRCR_RBSZ_Pos)); // set beats per DMA transfer to 32 (RX) and write receive buffer size [DO NOT START!]
}

static ETHHW_State *ETHHW_GetState(ETH_TypeDef *eth) {
    return ((ETHHW_State *)eth->DMACRDLAR) - 1;
}

static void ETHHW_InitState(ETH_TypeDef *eth) {
    ETHHW_State *state = ETHHW_GetState(eth);
    state->nextTxDescIdx = 0;
    state->txCntSent = 0;
    state->txCntAcked = 0;
}

void ETHHW_Init(ETH_TypeDef *eth, ETHHW_InitOpts *init) {
    ETHHW_InitClocks();
    ETHHW_InitPeripheral(eth, init);
    ETHHW_InitState(eth);
}

void ETHHW_Start(ETH_TypeDef *eth) {
    SET_BIT(eth->DMACIER, ETH_DMACIER_NIE); // normal interrupt enable
    SET_BIT(eth->DMACIER, ETH_DMACIER_RIE); // receive interrupt enable
    SET_BIT(eth->DMACIER, ETH_DMACIER_TIE); // transmit interrupt enable

    // start DMA reception and transmission
    SET_BIT(eth->DMACRCR, ETH_DMACRCR_SR);
    SET_BIT(eth->DMACTCR, ETH_DMACTCR_ST);

    // ----------------------------------

    // start MAC transceiver
    SET_BIT(eth->MACCR, ETH_MACCR_RE);
    SET_BIT(eth->MACCR, ETH_MACCR_TE);
}

__weak int ETHHW_EventCallback(ETHHW_EventDesc *evt) {
    (void)evt;
    return 0;
}

__weak int ETHHW_ReadCallback(ETHHW_EventDesc *evt) {
    (void)evt;
    return ETHHW_RET_RX_PROCESSED;
}

ETHHW_DescFull *ETHHW_AdvanceDesc(ETHHW_DescFull *start, uint16_t n, ETHHW_DescFull *bd, int delta) {
    int16_t index = (((uint32_t)(bd)) - ((uint32_t)(start))) / sizeof(ETHHW_DescFull);
    // int16_t startIndex = index;
    index = ((int)index + delta) % n;
    index = (index < 0) ? (index + n) : index;
    // MSG("%d +(%d) = %d mod %u\n", startIndex, delta, index, n);
    return start + index;
}

#define ETHHW_DESC_PREV(s, n, p) ETHHW_AdvanceDesc((s), (n), (p), -1)
#define ETHHW_DESC_NEXT(s, n, p) ETHHW_AdvanceDesc((s), (n), (p), 1)

static void ETHHW_RestoreRXDesc(ETHHW_DescFull *bd) {
    bd->desc.DES0 = bd->ext.bufAddr;                                                          // store Buffer 1 address
    bd->desc.DES3 = 0 | ETH_DMARXNDESCRF_OWN | ETH_DMARXNDESCRF_IOC | ETH_DMARXNDESCRF_BUF1V; // set flags: OWN, IOC, BUF1V
}

typedef enum { ETHHW_RINGBUF_RX,
               ETHHW_RINGBUF_TX } ETHHW_RingBufId;

static void ETHHW_PrintRingBufStatus(ETH_TypeDef *eth,
                                     ETHHW_RingBufId ringBufId) {
    ETHHW_DescFull *ring = NULL;
    ETHHW_DescFull *currentPtr = NULL;
    uint16_t n = 0;

    // fetch pointers based on ringbuffer ID
    if (ringBufId == ETHHW_RINGBUF_RX) {
        ring = (ETHHW_DescFull *)eth->DMACRDLAR;
        n = eth->DMACRDRLR + 1;
        currentPtr = (ETHHW_DescFull *)eth->DMACCARDR;
        MSG("RX: ");
    } else if (ringBufId == ETHHW_RINGBUF_TX) {
        ring = (ETHHW_DescFull *)eth->DMACTDLAR;
        n = eth->DMACTDRLR + 1;
        currentPtr = (ETHHW_DescFull *)eth->DMACCATDR;
        MSG("TX: ");
    }

    uint16_t current = n + 1;              // set current to something non-possible
    uint32_t DES3 = ring[n - 1].desc.DES3; // extract DES3

#define IS_DESC_EMPTY(DES3)                                            \
    ((ringBufId == ETHHW_RINGBUF_RX) ? ((DES3) & ETH_DMARXNDESCRF_OWN) \
                                     : !((DES3) & ETH_DMARXNDESCRF_OWN))

    // packet spanning over multiple descriptors; examine last descriptor first to
    // check if first descriptor is an intermediate one
    bool multiDescPkt = !(IS_DESC_EMPTY(DES3)) && !(DES3 & ETH_DMARXNDESCWBF_LD);

    MSG("|");
    for (uint16_t i = 0; i < n; i++) {
        if ((ring + i) == currentPtr) { // search current descriptor (next descriptor to safe in)
            current = i;
        }

        DES3 = ring[i].desc.DES3;             // fetch DES3 dword
        bool descEmpty = IS_DESC_EMPTY(DES3); // determine if descriptor is empty

        if (descEmpty) { // descriptor empty (rx: "armed", tx: not yet passed to the DMA)
            if ((ringBufId == ETHHW_RINGBUF_TX) && (ring[i].ext.tsCbPtr != 0) && (DES3 & ETH_DMATXNDESCWBF_TTSS)) {
                MSG("t");
            } else {
                MSG("-");
            }
        } else { // descriptor non-empty (rx: packet stored, tx: packet passed to
                 // the DMA)
            // multidescriptor packets
            bool packetBoundary = false;

            // detect first descriptor
            if ((DES3 & ETH_DMARXNDESCWBF_FD) && !((DES3 & ETH_DMARXNDESCWBF_LD))) {
                multiDescPkt = true;
                packetBoundary = true;
            }

            // detect last descriptor
            if (!(DES3 & ETH_DMARXNDESCWBF_FD) && ((DES3 & ETH_DMARXNDESCWBF_LD))) {
                multiDescPkt = false;
                packetBoundary = true;
            }

            // print mark accordingly...
            if (DES3 & ETH_DMARXNDESCWBF_CTXT) { // context descriptor
                MSG("C");
            } else {                                  // normal descriptor
                if (packetBoundary && multiDescPkt) { // first descriptor
                    MSG(">");
                } else if (packetBoundary && !multiDescPkt) { // last descriptor
                    MSG("<");
                } else { // intermediate or non-multi descriptor
                    if ((ringBufId == ETHHW_RINGBUF_TX) && (ring[i].ext.tsCbPtr != 0) &&
                        (ring[i].desc.DES2 & ETH_DMATXNDESCRF_TTSE)) {
                        MSG("T");
                    } else {
                        MSG("x");
                    }
                }
            }
        }

        // print connection between descriptors
        if (multiDescPkt) { // multi-descriptor packets
            MSG("=");
        } else { // single descriptor packets
            MSG(" ");
        }
    }
    MSG("|\n");

    // mark current descriptor position
    MSG("%*c", (uint32_t)2 * current + 5, ' ');
    MSG("^\n");
}

#define ETHHW_DESC_OWNED_BY_APPLICATION(bd) \
    (!(((bd)->desc.DES3) & ETH_DMARXNDESCRF_OWN))

// process incoming packet
void ETHHW_ProcessRx(ETH_TypeDef *eth) {
    // ETHHW_PrintRingBufStatus(eth, ETHHW_RINGBUF_RX);

    ETHHW_DescFull *ring = (ETHHW_DescFull *)eth->DMACRDLAR;
    uint16_t ringLen = eth->DMACRDRLR + 1;

    // get current descriptor
    ETHHW_DescFull *bd = (ETHHW_DescFull *)eth->DMACCARDR;
    ETHHW_DescFull *bd_prev = ETHHW_DESC_PREV(ring, ringLen, bd);

    // get the first unprocessed descriptor (oldest one)
    while (ETHHW_DESC_OWNED_BY_APPLICATION(bd_prev)) {
        bd = bd_prev;
        bd_prev = ETHHW_DESC_PREV(ring, ringLen, bd);
    }

    // iterate over unprocessed descriptors
    while (ETHHW_DESC_OWNED_BY_APPLICATION(bd)) {
        ETHHW_DescFull *bd_next = ETHHW_DESC_NEXT(ring, ringLen, bd);

        ETHHW_EventDesc evt;
        evt.type = ETHHW_EVT_RX_READ;
        evt.data.rx.size = (bd->desc.DES3) & 0x3FFF;
        evt.data.rx.payload = (void *)bd->ext.bufAddr;

        // check if a timestamp had been captured for the packet as well
        bool tsFound = bd->desc.DES1 & ETH_DMARXNDESCWBF_TSA;
        ETHHW_DescFull *ctx_bd = NULL; // context descriptor holding the timestamp
        if (tsFound) {
            // fetch timestamp
            ctx_bd = bd_next;
            evt.data.rx.ts_s = ctx_bd->desc.DES1;
            evt.data.rx.ts_ns = ctx_bd->desc.DES0;

            // step next bd further
            bd_next = ETHHW_DESC_NEXT(ring, ringLen, bd_next);
        }

        int ret = ETHHW_ReadCallback(&evt);
        if (ret == ETHHW_RET_RX_PROCESSED) {
            ETHHW_RestoreRXDesc(bd);     // release buffer descriptor
            ETHHW_RestoreRXDesc(ctx_bd); // and context descriptor also
        }

        bd = bd_next;
    }

    // ETHHW_print_rx_ringbuf_status(eth, ETHHW_RINGBUF_RX);

    // (*(bd-1)).desc.DES3
    // MSG("TAIL: %p SIZE: %u\n", bd, size);
}

void ETHHW_ProcessTx(ETH_TypeDef *eth) {
    //    ETHHW_PrintRingBufStatus(eth, ETHHW_RINGBUF_TX);

    uint16_t ringLen = eth->DMACTDRLR + 1;

    ETHHW_State *state = ETHHW_GetState(ETH);
    uint16_t minIdx = 0;
    int32_t minDelta = 0;
    ETHHW_DescFull *ring = (ETHHW_DescFull *)eth->DMACTDLAR;

    uint16_t descsWithTimestamp = 0;

    do {
        // search for oldest timestamp-carrying descriptor
        bool firstIteration = true;
        for (uint16_t i = 0; i < ringLen; i++) {
            uint16_t txCntr = ring[i].ext.txCntr;
            uint32_t DES3 = ring[i].desc.DES3;
            if ((DES3 & ETH_DMATXNDESCWBF_TTSS) && !(DES3 & ETH_DMATXNDESCWBF_OWN)) { // examine only descriptors holding a timestamp
                descsWithTimestamp++;                                                 // desc found with timestamp
                int32_t delta = (int32_t)txCntr - (int32_t)state->txCntAcked;

                if (firstIteration) { // initialize variables with valid data
                    firstIteration = false;
                    minDelta = delta;
                    minIdx = i;
                    continue;
                }

                if (delta == 1) { // consecutive packets found, stop search
                    minIdx = i;
                    minDelta = 1;
                } else { // search for minimum "distance"
                    if (delta < minDelta) {
                        minIdx = i;
                        minDelta = delta;
                    }
                }
            }
        }

        if (descsWithTimestamp > 0) {
            // invoke callback (the descriptor certainly contains a valid callback
            // address, see transmit function why)
            ETHHW_DescFull *bd = ring + minIdx;
            uint32_t ts_s = bd->desc.DES1;
            uint32_t ts_ns = bd->desc.DES0;

            if (bd->ext.tsCbPtr != 0) {
                ((void (*)(uint32_t, uint32_t, uint32_t))(bd->ext.tsCbPtr))(ts_s, ts_ns, bd->ext.tsCbArg);
            }

            // clear descriptor
            memset((void *)bd, 0, sizeof(ETHHW_Desc));
            bd->ext.tsCbPtr = 0;
            bd->ext.tsCbArg = 0;

            // increment TX acknowledge counter
            state->txCntAcked = bd->ext.txCntr;

            // decrement number of remaining unprocessed descriptors carrying a
            // timestamp
            descsWithTimestamp--;
        }

    } while (descsWithTimestamp > 0);
}

void ETHHW_ISR(ETH_TypeDef *eth) {
    uint32_t csr = READ_REG(eth->DMACSR);

    // MSG("ETH [0x%X]\n", csr);

    if (csr & ETH_DMACSR_NIS) {    // Normal Interrupt Summary
        if (csr & ETH_DMACSR_RI) { // Receive Interrupt
            SET_BIT(ETH->DMACSR, ETH_DMACSR_RI);
            SET_BIT(ETH->DMACSR, ETH_DMACSR_NIS);

            ETHHW_EventDesc evt;
            evt.type = ETHHW_EVT_RX_NOTFY;
            ETHHW_EventCallback(&evt);
        } else if (csr & ETH_DMACSR_TI) { // Transmit Interrupt
            SET_BIT(ETH->DMACSR, ETH_DMACSR_TI);

            ETHHW_ProcessTx(eth);
        }
    }

    SET_BIT(ETH->DMACSR, ETH_DMACSR_NIS);
}

void ETHHW_Transmit(ETH_TypeDef *eth, const uint8_t *buf, uint16_t len, uint8_t txOpts, void *txOptArgs) {
    ETHHW_State *state = ETHHW_GetState(eth);                                // fetch state
    uint16_t nextTxDescIdx = state->nextTxDescIdx;                           // fetch index of descriptor to fill
    ETHHW_DescFull *bd = ((ETHHW_DescFull *)eth->DMACTDLAR) + nextTxDescIdx; // get descriptor being filled

    // MSG("%u\n", nextTxDesc);

    // ETHHW_PrintRingBufStatus(eth, ETHHW_RINGBUF_TX);

    while (!ETHHW_DESC_OWNED_BY_APPLICATION(bd)) {
    } // wait for descriptor to become released by the DMA (if needed)

    // erase possible old descriptor data
    memset(bd, 0, sizeof(ETHHW_Desc)); // DON'T erase extension

    uint32_t opts = 0;
    if (txOpts & ETHHW_TXOPT_INTERRUPT_ON_COMPLETION) {
        opts |= ETH_DMATXNDESCRF_IOC;
    }

    if ((txOpts == ETHHW_TXOPT_CAPTURE_TS) &&
        (txOptArgs != NULL)) { // arguments are mandatory
        opts |= ETH_DMATXNDESCRF_TTSE;
        ETHHW_OptArg_TxTsCap *arg = (ETHHW_OptArg_TxTsCap *)txOptArgs; // retrieve args
        bd->ext.tsCbPtr = arg->txTsCbPtr;                              // fill-in extension fields
        bd->ext.tsCbArg = arg->tag;
    }

    // fill-in identification field
    bd->ext.txCntr = ++state->txCntSent; // copy AFTER increase

    // copy payload to TX buffer
    memcpy((void *)bd->ext.bufAddr, buf, len);

    // fill in-descriptor fields
    bd->desc.DES0 = bd->ext.bufAddr;
    bd->desc.DES2 = opts | (len & 0x3FFF);                                                                                                  // {IOC|TTSE} and buffer length truncated to 14-bits
    bd->desc.DES3 = ETH_DMATXNDESCRF_OWN | ETH_DMATXNDESCRF_FD | ETH_DMATXNDESCRF_LD | ETH_DMATXNDESCRF_CIC_IPHDR_PAYLOAD_INSERT_PHDR_CALC; // pass desciptor to the DMA, set First Desc. and Last Desc. flags

    //ETHHW_PrintRingBufStatus(eth, ETHHW_RINGBUF_TX);

    state->nextTxDescIdx = (state->nextTxDescIdx + 1) % (eth->DMACTDRLR + 1); // advance index to next descriptor

    WRITE_REG(eth->DMACTDTPR, 0); // tail pointer WON'T STOP
}

// -----------------

void ETHHW_SetLinkProperties(ETH_TypeDef *eth, bool fastEthernet, bool fullDuplex) {
    // fetch register content
    uint32_t reg = eth->MACCR;

    // set Fast Ethernet state
    if (fastEthernet) {
        SET_BIT(reg, ETH_MACCR_FES);
    } else {
        CLEAR_BIT(reg, ETH_MACCR_FES);
    }

    // set duplex state
    if (fullDuplex) {
        SET_BIT(reg, ETH_MACCR_DM);
    } else {
        CLEAR_BIT(reg, ETH_MACCR_DM);
    }

    // write modified register value back
    eth->MACCR = reg;
}

uint32_t ETHHW_ReadPHYRegister(ETH_TypeDef *eth, uint32_t PHYAddr, uint32_t PHYReg, uint32_t *pRegValue) {
    uint32_t tmpreg;

    /* Check for the Busy flag */
    if (READ_BIT(eth->MACMDIOAR, ETH_MACMDIOAR_MB) != 0U) {
        return 1;
    }

    /* Get the  MACMDIOAR value */
    WRITE_REG(tmpreg, eth->MACMDIOAR);

    /* Prepare the MDIO Address Register value
     - Set the PHY device address
     - Set the PHY register address
     - Set the read mode
     - Set the MII Busy bit */

    MODIFY_REG(tmpreg, ETH_MACMDIOAR_PA, (PHYAddr << 21));
    MODIFY_REG(tmpreg, ETH_MACMDIOAR_RDA, (PHYReg << 16));
    MODIFY_REG(tmpreg, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_RD);
    SET_BIT(tmpreg, ETH_MACMDIOAR_MB);

    /* Write the result value into the MDII Address register */
    WRITE_REG(eth->MACMDIOAR, tmpreg);

    /* Wait for the Busy flag */
    while (READ_BIT(eth->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
    }

    /* Get MACMIIDR value */
    WRITE_REG(*pRegValue, (uint16_t)eth->MACMDIODR);

    return 0;
}

uint32_t ETHHW_WritePHYRegister(ETH_TypeDef *eth, uint32_t PHYAddr,
                                uint32_t PHYReg, uint32_t RegValue) {
    uint32_t tmpreg;

    /* Check for the Busy flag */
    if (READ_BIT(eth->MACMDIOAR, ETH_MACMDIOAR_MB) != 0U) {
        return 1;
    }

    /* Get the  MACMDIOAR value */
    WRITE_REG(tmpreg, eth->MACMDIOAR);

    /* Prepare the MDIO Address Register value
     - Set the PHY device address
     - Set the PHY register address
     - Set the write mode
     - Set the MII Busy bit */

    MODIFY_REG(tmpreg, ETH_MACMDIOAR_PA, (PHYAddr << 21));
    MODIFY_REG(tmpreg, ETH_MACMDIOAR_RDA, (PHYReg << 16));
    MODIFY_REG(tmpreg, ETH_MACMDIOAR_MOC, ETH_MACMDIOAR_MOC_WR);
    SET_BIT(tmpreg, ETH_MACMDIOAR_MB);

    /* Give the value to the MII data register */
    WRITE_REG(eth->MACMDIODR, (uint16_t)RegValue);

    /* Write the result value into the MII Address register */
    WRITE_REG(eth->MACMDIOAR, tmpreg);

    /* Wait for the Busy flag */
    while (READ_BIT(eth->MACMDIOAR, ETH_MACMDIOAR_MB) > 0U) {
    }

    return 0;
}

/* ---- PTP CAPABILITIES ---- */

// #define ETH_PTP_FLAG_TSARFE ((uint32_t)(1 << 8)) // enable timestamping for
// every received frame #define ETH_PTP_FLAG_TSSSR ((uint32_t)(1 << 9)) //
// subsecond rollover control (1 = rollover on 10^9-1 nsec) #define
// ETH_PTP_FLAG_TSE ((uint32_t)(1 << 0)) // global timestamp enable flag

void ETHHW_EnablePTPTimeStamping(ETH_TypeDef *eth) {
    __IO uint32_t tmpreg = eth->MACTSCR;

    tmpreg = 0b01 << ETH_MACTSCR_SNAPTYPSEL_Pos;
    tmpreg |= ETH_MACTSCR_TSIPV4ENA | ETH_MACTSCR_TSIPENA |
              ETH_MACTSCR_TSVER2ENA | ETH_MACTSCR_TSENA |
              ETH_MACTSCR_TSCTRLSSR; // turn on relevant flags

    eth->MACTSCR = tmpreg;
}

void ETHHW_DisablePTPTimeStamping(ETH_TypeDef *eth) {
    __IO uint32_t tmpreg = eth->MACTSCR;

    tmpreg &= ~ETH_MACTSCR_TSENA;

    eth->MACTSCR = tmpreg;
}

// #define ETH_PTP_FLAG_TSSTI ((uint32_t)(1 << 2)) // initialize PTP time with
// the values stored in Timestamp high and low registers

void ETHHW_InitPTPTime(ETH_TypeDef *eth, uint32_t sec, uint32_t nsec) {
    // fill registers with time components
    eth->MACSTSUR = sec;
    eth->MACSTNUR = nsec;

    // wait for TSINIT to clear
    while (eth->MACTSCR & (ETH_MACTSCR_TSINIT)) {
        __NOP();
    }

    // perform time initialization
    __IO uint32_t tmpreg = eth->MACTSCR;

    tmpreg |= ETH_MACTSCR_TSINIT;

    eth->MACTSCR = tmpreg;
}

// #define ETH_PTP_FLAG_TSFCU ((uint32_t)(1 << 1)) // flag controlling
// fine/coarse update methods

void ETHHW_EnablePTPFineCorr(ETH_TypeDef *eth, bool enFineCorr) {
    __IO uint32_t tmpreg = eth->MACTSCR;

    if (enFineCorr) {
        tmpreg |= ETH_MACTSCR_TSCFUPDT;
    } else {
        tmpreg &= ~ETH_MACTSCR_TSCFUPDT;
    }

    eth->MACTSCR = tmpreg;
}

// #define ETH_PTP_FLAG_TSSTU ((uint32_t)(1 << 3)) // flag initiating time
// update

void ETHHW_UpdatePTPTime(ETH_TypeDef *eth, uint32_t sec, uint32_t nsec,
                         bool add_substract) { // true = add
    if (add_substract) {
        nsec &= ~((uint32_t)(1 << 31));
    } else {
        nsec = 1000000000 - (nsec >> 1);
        nsec |= (1 << 31);
    }

    // fill registers with time components
    eth->MACSTSUR = sec;
    eth->MACSTNUR = nsec;

    // wait for TSUPDT and TSINIT to clear
    while (eth->MACTSCR & (ETH_MACTSCR_TSUPDT | ETH_MACTSCR_TSINIT)) {
        __NOP();
    }

    // perform time update
    __IO uint32_t tmpreg = eth->MACTSCR;

    tmpreg |= ETH_MACTSCR_TSUPDT;

    eth->MACTSCR = tmpreg;
}

// #define ETH_PTP_FLAG_TSARU ((uint32_t)(1 << 5)) // flag initiating addend
// register update

void ETHHW_SetPTPAddend(ETH_TypeDef *eth, uint32_t addend) {
    size_t i = 0;
    for (i = 0; i < 8; i++) {
        eth->MACTSAR = addend; // set addend

        // wait for TSADDREG to clear
        while (eth->MACTSCR & (ETH_MACTSCR_TSADDREG)) {
            __NOP();
        }

        // update PTP block internal register
        __IO uint32_t tmpreg = eth->MACTSCR;

        tmpreg |= ETH_MACTSCR_TSADDREG;

        eth->MACTSCR = tmpreg;
    }
}

uint32_t ETHHW_GetPTPAddend(ETH_TypeDef *eth) {
    return eth->MACTSAR;
}

void ETHHW_SetPTPSubsecondIncrement(ETH_TypeDef *eth, uint8_t increment) {
    eth->MACSSIR = increment << 16;
}

uint32_t ETHHW_GetPTPSubsecondIncrement(ETH_TypeDef *eth) {
    return eth->MACSSIR >> 16;
}

#define ETH_PTP_PPS_NO_INTERRUPT (0b11 << 5)
#define ETH_PTP_PPS_OUTPUT_MODE_SELECT (1 << 4)

void ETHHW_SetPTPPPSFreq(ETH_TypeDef *eth, uint32_t freqCode) {
    __IO uint32_t tmpreg = eth->MACPPSCR;

    tmpreg = ETH_PTP_PPS_NO_INTERRUPT | (freqCode & 0x0F);

    eth->MACPPSCR = tmpreg;
}

void ETHHW_AuxTimestampCh(ETH_TypeDef *eth, uint8_t ch, bool en) {
    __IO uint32_t tmpreg = eth->MACACR;

    ch = ch & 0b11;
    en = en & 0b1;

    tmpreg = (en) << (ch + 4);

    eth->MACACR = tmpreg;
}

void ETHHW_ReadLastAuxTimestamp(ETH_TypeDef *eth, uint32_t *ps, uint32_t *pns) {
    *ps = eth->MACATSSR;
    *pns = eth->MACATSNR;
}

void ETHHW_ClearAuxTimestampFIFO(ETH_TypeDef *eth) {
    eth->MACACR |= 1;
}

uint8_t ETHHW_GetAuxTimestampCnt(ETH_TypeDef *eth) {
    return ((eth->MACTSSR >> 25) & 0b11111);
}

#define ETH_PTP_PPS_PULSE_TRAIN_START (0b0010)
#define ETH_PTP_PPS_PULSE_TRAIN_STOP_IMM (0b0101)
#define ETH_PTP_PPSCMD_MASK (0x0F)

void ETHHW_StartPTPPPSPulseTrain(ETH_TypeDef *eth, uint32_t high_len, uint32_t period) {
    ETHHW_StopPTPPPSPulseTrain(eth);

    while (eth->MACPPSCR & ETH_PTP_PPSCMD_MASK) {
    };

    // delayed start of pulse train with (at least) 1s to ensure,
    // target timestamps point always in the future on return from this function
    eth->MACPPSTTSR = eth->MACSTSR + 2;

    // emit pulsetrain on nanoseconds rollover
    eth->MACPPSTTNR = 0;

    // get increment value
    uint32_t increment = ETHHW_GetPTPSubsecondIncrement(eth);

    // compute positive pulse width
    eth->MACPPSWR = (high_len / increment) - 1;

    // compute repeat period
    eth->MACPPSIR = (period / increment) - 1;

    __IO uint32_t tmpreg = eth->MACPPSCR;
    tmpreg |= ETH_PTP_PPS_NO_INTERRUPT | ETH_PTP_PPS_PULSE_TRAIN_START;
    eth->MACPPSCR = tmpreg;
}

void ETHHW_StopPTPPPSPulseTrain(ETH_TypeDef *eth) {
    if (!(eth->MACPPSCR & ETH_PTP_PPS_OUTPUT_MODE_SELECT)) {
        eth->MACPPSCR |= ETH_PTP_PPS_OUTPUT_MODE_SELECT; // switch to pulse-train mode
    } else {
        while (eth->MACPPSCR & ETH_PTP_PPSCMD_MASK) {
        };
    }

    eth->MACPPSCR |= ETH_PTP_PPS_PULSE_TRAIN_STOP_IMM;
}
