/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
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
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for TIMER_KEYs */
#define TIMER_KEYs_INST                                                  (TIMA0)
#define TIMER_KEYs_INST_IRQHandler                              TIMA0_IRQHandler
#define TIMER_KEYs_INST_INT_IRQN                                (TIMA0_INT_IRQn)
#define TIMER_KEYs_INST_LOAD_VALUE                                         (32U)



/* Defines for SPI_flash */
#define SPI_flash_INST                                                     SPI1
#define SPI_flash_INST_IRQHandler                               SPI1_IRQHandler
#define SPI_flash_INST_INT_IRQN                                   SPI1_INT_IRQn
#define GPIO_SPI_flash_PICO_PORT                                          GPIOA
#define GPIO_SPI_flash_PICO_PIN                                  DL_GPIO_PIN_18
#define GPIO_SPI_flash_IOMUX_PICO                               (IOMUX_PINCM40)
#define GPIO_SPI_flash_IOMUX_PICO_FUNC               IOMUX_PINCM40_PF_SPI1_PICO
#define GPIO_SPI_flash_POCI_PORT                                          GPIOA
#define GPIO_SPI_flash_POCI_PIN                                  DL_GPIO_PIN_16
#define GPIO_SPI_flash_IOMUX_POCI                               (IOMUX_PINCM38)
#define GPIO_SPI_flash_IOMUX_POCI_FUNC               IOMUX_PINCM38_PF_SPI1_POCI
/* GPIO configuration for SPI_flash */
#define GPIO_SPI_flash_SCLK_PORT                                          GPIOA
#define GPIO_SPI_flash_SCLK_PIN                                  DL_GPIO_PIN_17
#define GPIO_SPI_flash_IOMUX_SCLK                               (IOMUX_PINCM39)
#define GPIO_SPI_flash_IOMUX_SCLK_FUNC               IOMUX_PINCM39_PF_SPI1_SCLK
/* Defines for SPI_screen */
#define SPI_screen_INST                                                    SPI0
#define SPI_screen_INST_IRQHandler                              SPI0_IRQHandler
#define SPI_screen_INST_INT_IRQN                                  SPI0_INT_IRQn
#define GPIO_SPI_screen_PICO_PORT                                         GPIOA
#define GPIO_SPI_screen_PICO_PIN                                 DL_GPIO_PIN_14
#define GPIO_SPI_screen_IOMUX_PICO                              (IOMUX_PINCM36)
#define GPIO_SPI_screen_IOMUX_PICO_FUNC              IOMUX_PINCM36_PF_SPI0_PICO
#define GPIO_SPI_screen_POCI_PORT                                         GPIOA
#define GPIO_SPI_screen_POCI_PIN                                 DL_GPIO_PIN_10
#define GPIO_SPI_screen_IOMUX_POCI                              (IOMUX_PINCM21)
#define GPIO_SPI_screen_IOMUX_POCI_FUNC              IOMUX_PINCM21_PF_SPI0_POCI
/* GPIO configuration for SPI_screen */
#define GPIO_SPI_screen_SCLK_PORT                                         GPIOA
#define GPIO_SPI_screen_SCLK_PIN                                 DL_GPIO_PIN_11
#define GPIO_SPI_screen_IOMUX_SCLK                              (IOMUX_PINCM22)
#define GPIO_SPI_screen_IOMUX_SCLK_FUNC              IOMUX_PINCM22_PF_SPI0_SCLK
#define GPIO_SPI_screen_CS0_PORT                                          GPIOA
#define GPIO_SPI_screen_CS0_PIN                                   DL_GPIO_PIN_2
#define GPIO_SPI_screen_IOMUX_CS0                                (IOMUX_PINCM7)
#define GPIO_SPI_screen_IOMUX_CS0_FUNC                 IOMUX_PINCM7_PF_SPI0_CS0



/* Defines for DMA_CH0 */
#define DMA_CH0_CHAN_ID                                                      (0)
#define DAC12_INST_DMA_TRIGGER                          (DMA_DAC0_EVT_BD_1_TRIG)



/* Port definition for Pin Group Others */
#define Others_PORT                                                      (GPIOB)

/* Defines for CS_flash: GPIOB.13 with pinCMx 30 on package pin 1 */
#define Others_CS_flash_PIN                                     (DL_GPIO_PIN_13)
#define Others_CS_flash_IOMUX                                    (IOMUX_PINCM30)
/* Defines for LED1: GPIOB.14 with pinCMx 31 on package pin 2 */
#define LEDs_LED1_PORT                                                   (GPIOB)
#define LEDs_LED1_PIN                                           (DL_GPIO_PIN_14)
#define LEDs_LED1_IOMUX                                          (IOMUX_PINCM31)
/* Defines for LED2: GPIOB.15 with pinCMx 32 on package pin 3 */
#define LEDs_LED2_PORT                                                   (GPIOB)
#define LEDs_LED2_PIN                                           (DL_GPIO_PIN_15)
#define LEDs_LED2_IOMUX                                          (IOMUX_PINCM32)
/* Defines for LED3: GPIOB.16 with pinCMx 33 on package pin 4 */
#define LEDs_LED3_PORT                                                   (GPIOB)
#define LEDs_LED3_PIN                                           (DL_GPIO_PIN_16)
#define LEDs_LED3_IOMUX                                          (IOMUX_PINCM33)
/* Defines for LED4: GPIOA.12 with pinCMx 34 on package pin 5 */
#define LEDs_LED4_PORT                                                   (GPIOA)
#define LEDs_LED4_PIN                                           (DL_GPIO_PIN_12)
#define LEDs_LED4_IOMUX                                          (IOMUX_PINCM34)
/* Defines for LED5: GPIOA.13 with pinCMx 35 on package pin 6 */
#define LEDs_LED5_PORT                                                   (GPIOA)
#define LEDs_LED5_PIN                                           (DL_GPIO_PIN_13)
#define LEDs_LED5_IOMUX                                          (IOMUX_PINCM35)
/* Defines for LED6: GPIOB.17 with pinCMx 43 on package pin 14 */
#define LEDs_LED6_PORT                                                   (GPIOB)
#define LEDs_LED6_PIN                                           (DL_GPIO_PIN_17)
#define LEDs_LED6_IOMUX                                          (IOMUX_PINCM43)
/* Defines for LED7: GPIOB.18 with pinCMx 44 on package pin 15 */
#define LEDs_LED7_PORT                                                   (GPIOB)
#define LEDs_LED7_PIN                                           (DL_GPIO_PIN_18)
#define LEDs_LED7_IOMUX                                          (IOMUX_PINCM44)
/* Defines for KEY1: GPIOB.19 with pinCMx 45 on package pin 16 */
#define KEYs_KEY1_PORT                                                   (GPIOB)
#define KEYs_KEY1_PIN                                           (DL_GPIO_PIN_19)
#define KEYs_KEY1_IOMUX                                          (IOMUX_PINCM45)
/* Defines for KEY2: GPIOA.21 with pinCMx 46 on package pin 17 */
#define KEYs_KEY2_PORT                                                   (GPIOA)
#define KEYs_KEY2_PIN                                           (DL_GPIO_PIN_21)
#define KEYs_KEY2_IOMUX                                          (IOMUX_PINCM46)
/* Defines for KEY3: GPIOA.22 with pinCMx 47 on package pin 18 */
#define KEYs_KEY3_PORT                                                   (GPIOA)
#define KEYs_KEY3_PIN                                           (DL_GPIO_PIN_22)
#define KEYs_KEY3_IOMUX                                          (IOMUX_PINCM47)
/* Defines for KEY4: GPIOB.20 with pinCMx 48 on package pin 19 */
#define KEYs_KEY4_PORT                                                   (GPIOB)
#define KEYs_KEY4_PIN                                           (DL_GPIO_PIN_20)
#define KEYs_KEY4_IOMUX                                          (IOMUX_PINCM48)
/* Defines for KEY5: GPIOB.21 with pinCMx 49 on package pin 20 */
#define KEYs_KEY5_PORT                                                   (GPIOB)
#define KEYs_KEY5_PIN                                           (DL_GPIO_PIN_21)
#define KEYs_KEY5_IOMUX                                          (IOMUX_PINCM49)
/* Defines for KEY6: GPIOB.22 with pinCMx 50 on package pin 21 */
#define KEYs_KEY6_PORT                                                   (GPIOB)
#define KEYs_KEY6_PIN                                           (DL_GPIO_PIN_22)
#define KEYs_KEY6_IOMUX                                          (IOMUX_PINCM50)
/* Defines for KEY7: GPIOB.23 with pinCMx 51 on package pin 22 */
#define KEYs_KEY7_PORT                                                   (GPIOB)
#define KEYs_KEY7_PIN                                           (DL_GPIO_PIN_23)
#define KEYs_KEY7_IOMUX                                          (IOMUX_PINCM51)



/* Defines for DAC12 */
#define DAC12_IRQHandler                                         DAC0_IRQHandler
#define DAC12_INT_IRQN                                           (DAC0_INT_IRQn)
#define GPIO_DAC12_OUT_PORT                                                GPIOA
#define GPIO_DAC12_OUT_PIN                                        DL_GPIO_PIN_15
#define GPIO_DAC12_IOMUX_OUT                                     (IOMUX_PINCM37)
#define GPIO_DAC12_IOMUX_OUT_FUNC                   IOMUX_PINCM37_PF_UNCONNECTED

/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_KEYs_init(void);
void SYSCFG_DL_SPI_flash_init(void);
void SYSCFG_DL_SPI_screen_init(void);
void SYSCFG_DL_DMA_init(void);

void SYSCFG_DL_DAC12_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
