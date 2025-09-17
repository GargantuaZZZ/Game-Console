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

#include "inc/ti_msp_dl_config.h"

DL_TimerA_backupConfig gTIMER_KEYsBackup;
DL_SPI_backupConfig gSPI_flashBackup;
DL_SPI_backupConfig gSPI_screenBackup;

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
    SYSCFG_DL_SPI_flash_init();
    SYSCFG_DL_SPI_screen_init();
    SYSCFG_DL_DMA_init();
    SYSCFG_DL_DAC12_init();
    /* Ensure backup structures have no valid state */
	gTIMER_KEYsBackup.backupRdy 	= false;
	gSPI_flashBackup.backupRdy 	= false;
	gSPI_screenBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(TIMER_KEYs_INST, &gTIMER_KEYsBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_flash_INST, &gSPI_flashBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_screen_INST, &gSPI_screenBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(TIMER_KEYs_INST, &gTIMER_KEYsBackup, false);
	retStatus &= DL_SPI_restoreConfiguration(SPI_flash_INST, &gSPI_flashBackup);
	retStatus &= DL_SPI_restoreConfiguration(SPI_screen_INST, &gSPI_screenBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(TIMER_KEYs_INST);
    DL_SPI_reset(SPI_flash_INST);
    DL_SPI_reset(SPI_screen_INST);

    DL_DAC12_reset(DAC0);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(TIMER_KEYs_INST);
    DL_SPI_enablePower(SPI_flash_INST);
    DL_SPI_enablePower(SPI_screen_INST);

    DL_DAC12_enablePower(DAC0);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_flash_IOMUX_SCLK, GPIO_SPI_flash_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_flash_IOMUX_PICO, GPIO_SPI_flash_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_flash_IOMUX_POCI, GPIO_SPI_flash_IOMUX_POCI_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_screen_IOMUX_SCLK, GPIO_SPI_screen_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_screen_IOMUX_PICO, GPIO_SPI_screen_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_screen_IOMUX_POCI, GPIO_SPI_screen_IOMUX_POCI_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_screen_IOMUX_CS0, GPIO_SPI_screen_IOMUX_CS0_FUNC);

    DL_GPIO_initDigitalOutput(Others_CS_flash_IOMUX);

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

    DL_GPIO_clearPins(GPIOA, LEDs_LED4_PIN |
		LEDs_LED5_PIN);
    DL_GPIO_enableOutput(GPIOA, LEDs_LED4_PIN |
		LEDs_LED5_PIN);
    DL_GPIO_clearPins(GPIOB, Others_CS_flash_PIN |
		LEDs_LED1_PIN |
		LEDs_LED2_PIN |
		LEDs_LED3_PIN |
		LEDs_LED6_PIN |
		LEDs_LED7_PIN);
    DL_GPIO_enableOutput(GPIOB, Others_CS_flash_PIN |
		LEDs_LED1_PIN |
		LEDs_LED2_PIN |
		LEDs_LED3_PIN |
		LEDs_LED6_PIN |
		LEDs_LED7_PIN);

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


static const DL_SPI_Config gSPI_flash_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_flash_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_flash_init(void) {
    DL_SPI_setClockConfig(SPI_flash_INST, (DL_SPI_ClockConfig *) &gSPI_flash_clockConfig);

    DL_SPI_init(SPI_flash_INST, (DL_SPI_Config *) &gSPI_flash_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     8000000 = (32000000)/((1 + 1) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_flash_INST, 1);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_flash_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_flash_INST);
}
static const DL_SPI_Config gSPI_screen_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO4_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
    .chipSelectPin = DL_SPI_CHIP_SELECT_0,
};

static const DL_SPI_ClockConfig gSPI_screen_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_screen_init(void) {
    DL_SPI_setClockConfig(SPI_screen_INST, (DL_SPI_ClockConfig *) &gSPI_screen_clockConfig);

    DL_SPI_init(SPI_screen_INST, (DL_SPI_Config *) &gSPI_screen_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     8000000 = (32000000)/((1 + 1) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_screen_INST, 1);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_screen_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_screen_INST);
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

