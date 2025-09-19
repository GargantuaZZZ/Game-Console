/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gTIMER_KEYsBackup;
DL_TimerA_backupConfig gTIMER_LEDsBackup;
DL_SPI_backupConfig gSPI_FlashBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_TIMER_KEYs_init();
    SYSCFG_DL_TIMER_LEDs_init();
    SYSCFG_DL_UART_Debug_init();
    SYSCFG_DL_SPI_Flash_init();
    SYSCFG_DL_DMA_init();
    SYSCFG_DL_DAC12_init();
    /* Ensure backup structures have no valid state */
	gTIMER_KEYsBackup.backupRdy 	= false;
	gTIMER_LEDsBackup.backupRdy 	= false;

	gSPI_FlashBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(TIMER_KEYs_INST, &gTIMER_KEYsBackup);
	retStatus &= DL_TimerA_saveConfiguration(TIMER_LEDs_INST, &gTIMER_LEDsBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_Flash_INST, &gSPI_FlashBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(TIMER_KEYs_INST, &gTIMER_KEYsBackup, false);
	retStatus &= DL_TimerA_restoreConfiguration(TIMER_LEDs_INST, &gTIMER_LEDsBackup, false);
	retStatus &= DL_SPI_restoreConfiguration(SPI_Flash_INST, &gSPI_FlashBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(TIMER_KEYs_INST);
    DL_TimerA_reset(TIMER_LEDs_INST);
    DL_UART_Main_reset(UART_Debug_INST);
    DL_SPI_reset(SPI_Flash_INST);

    DL_DAC12_reset(DAC0);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(TIMER_KEYs_INST);
    DL_TimerA_enablePower(TIMER_LEDs_INST);
    DL_UART_Main_enablePower(UART_Debug_INST);
    DL_SPI_enablePower(SPI_Flash_INST);

    DL_DAC12_enablePower(DAC0);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_Debug_IOMUX_TX, GPIO_UART_Debug_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_Debug_IOMUX_RX, GPIO_UART_Debug_IOMUX_RX_FUNC);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_Flash_IOMUX_SCLK, GPIO_SPI_Flash_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_Flash_IOMUX_PICO, GPIO_SPI_Flash_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_Flash_IOMUX_POCI, GPIO_SPI_Flash_IOMUX_POCI_FUNC);

    DL_GPIO_initDigitalOutput(LEDs_LED1_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED2_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED3_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED4_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED5_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED6_IOMUX);

    DL_GPIO_initDigitalOutput(LEDs_LED7_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY1_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY2_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY3_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY4_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY5_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY6_IOMUX);

    DL_GPIO_initDigitalInput(KEYs_KEY7_IOMUX);

    DL_GPIO_initDigitalOutput(COMs_CS_Flash_IOMUX);

    DL_GPIO_initDigitalOutput(COMs_SCL_Screen_IOMUX);

    DL_GPIO_initDigitalOutput(COMs_SDA_Screen_IOMUX);

    DL_GPIO_clearPins(GPIOA, LEDs_LED2_PIN |
		LEDs_LED3_PIN |
		LEDs_LED4_PIN);
    DL_GPIO_enableOutput(GPIOA, LEDs_LED2_PIN |
		LEDs_LED3_PIN |
		LEDs_LED4_PIN);
    DL_GPIO_clearPins(GPIOB, LEDs_LED1_PIN |
		LEDs_LED5_PIN |
		LEDs_LED6_PIN |
		LEDs_LED7_PIN |
		COMs_CS_Flash_PIN |
		COMs_SCL_Screen_PIN |
		COMs_SDA_Screen_PIN);
    DL_GPIO_enableOutput(GPIOB, LEDs_LED1_PIN |
		LEDs_LED5_PIN |
		LEDs_LED6_PIN |
		LEDs_LED7_PIN |
		COMs_CS_Flash_PIN |
		COMs_SCL_Screen_PIN |
		COMs_SDA_Screen_PIN);

}



SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_enableMFPCLK();
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_SYSOSC);

}



/*
 * Timer clock configuration to be sourced by LFCLK /  (32768 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32768 Hz = 32768 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_KEYsClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_LFCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_KEYs_INST_LOAD_VALUE = (1ms * 32768 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_KEYsTimerConfig = {
    .period     = TIMER_KEYs_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_KEYs_init(void) {

    DL_TimerA_setClockConfig(TIMER_KEYs_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_KEYsClockConfig);

    DL_TimerA_initTimerMode(TIMER_KEYs_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_KEYsTimerConfig);
    DL_TimerA_enableInterrupt(TIMER_KEYs_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(TIMER_KEYs_INST);




}

/*
 * Timer clock configuration to be sourced by LFCLK /  (32768 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   32768 Hz = 32768 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_LEDsClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_LFCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_LEDs_INST_LOAD_VALUE = (2s * 32768 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_LEDsTimerConfig = {
    .period     = TIMER_LEDs_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_LEDs_init(void) {

    DL_TimerA_setClockConfig(TIMER_LEDs_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_LEDsClockConfig);

    DL_TimerA_initTimerMode(TIMER_LEDs_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_LEDsTimerConfig);
    DL_TimerA_enableInterrupt(TIMER_LEDs_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(TIMER_LEDs_INST);




}



static const DL_UART_Main_ClockConfig gUART_DebugClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_DebugConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_Debug_init(void)
{
    DL_UART_Main_setClockConfig(UART_Debug_INST, (DL_UART_Main_ClockConfig *) &gUART_DebugClockConfig);

    DL_UART_Main_init(UART_Debug_INST, (DL_UART_Main_Config *) &gUART_DebugConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(UART_Debug_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_Debug_INST, UART_Debug_IBRD_32_MHZ_115200_BAUD, UART_Debug_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(UART_Debug_INST);
}

static const DL_SPI_Config gSPI_Flash_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_Flash_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_Flash_init(void) {
    DL_SPI_setClockConfig(SPI_Flash_INST, (DL_SPI_ClockConfig *) &gSPI_Flash_clockConfig);

    DL_SPI_init(SPI_Flash_INST, (DL_SPI_Config *) &gSPI_Flash_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     8000000 = (32000000)/((1 + 1) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_Flash_INST, 1);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_Flash_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_Flash_INST);
}

static const DL_DMA_Config gDMA_CH0Config = {
    .transferMode   = DL_DMA_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_UNCHANGED,
    .srcIncrement   = DL_DMA_ADDR_INCREMENT,
    .destWidth      = DL_DMA_WIDTH_HALF_WORD,
    .srcWidth       = DL_DMA_WIDTH_HALF_WORD,
    .trigger        = DAC12_INST_DMA_TRIGGER,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_DMA_CH0_init(void)
{
    DL_DMA_setSrcIncrement(DMA, DMA_CH0_CHAN_ID, DL_DMA_ADDR_INCREMENT);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID , (DL_DMA_Config *) &gDMA_CH0Config);
}
SYSCONFIG_WEAK void SYSCFG_DL_DMA_init(void){
    SYSCFG_DL_DMA_CH0_init();
}


static const DL_DAC12_Config gDAC12Config = {
    .outputEnable              = DL_DAC12_OUTPUT_ENABLED,
    .resolution                = DL_DAC12_RESOLUTION_12BIT,
    .representation            = DL_DAC12_REPRESENTATION_BINARY,
    .voltageReferenceSource    = DL_DAC12_VREF_SOURCE_VDDA_VSSA,
    .amplifierSetting          = DL_DAC12_AMP_OFF_TRISTATE,
    .fifoEnable                = DL_DAC12_FIFO_ENABLED,
    .fifoTriggerSource         = DL_DAC12_FIFO_TRIGGER_SAMPLETIMER,
    .dmaTriggerEnable          = DL_DAC12_DMA_TRIGGER_ENABLED,
    .dmaTriggerThreshold       = DL_DAC12_FIFO_THRESHOLD_TWO_QTRS_EMPTY,
    .sampleTimeGeneratorEnable = DL_DAC12_SAMPLETIMER_ENABLE,
    .sampleRate                = DL_DAC12_SAMPLES_PER_SECOND_1M,
};
SYSCONFIG_WEAK void SYSCFG_DL_DAC12_init(void)
{
    DL_DAC12_init(DAC0, (DL_DAC12_Config *) &gDAC12Config);
    DL_DAC12_enableInterrupt(DAC0, (DL_DAC12_INTERRUPT_DMA_DONE));
    DL_DAC12_enable(DAC0);
}

