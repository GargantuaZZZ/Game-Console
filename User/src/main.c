#include "inc/ti_msp_dl_config.h"
#include "inc/SPIflash.h"
#include "inc/OLED.h"
#include "inc/Alerts.h"

#define SPIF_DEBUG SPIF_DEBUG_FULL

#define KEY_PRESSED     false
#define KEY_RELEASED    true
#define STABLE_CNT      20

#define WELCOME_ADDR    0x00
#define WELCOME_LEN     320
#define BGM_ADDR        0x10
#define BGM_LEN         3200
#define ALERT_LEN       16

enum{
    WELCOME,
    DIFF_SEL,
    LEVEL_SEL,
    PLAYING,
    GAME_OVER
}game_state;

enum{
    IDLE_BUF1,
    FILL_BUF1,
    IDLE_BUF2,
    FILL_BUF2
}buffer_state;

const GPIO_Regs* LEDs_PORT[7] = {LEDs_LED1_PORT, LEDs_LED2_PORT, LEDs_LED3_PORT, LEDs_LED4_PORT, LEDs_LED5_PORT, LEDs_LED6_PORT, LEDs_LED7_PORT};
const uint32_t LEDs_PIN[7] = {LEDs_LED1_PIN, LEDs_LED2_PIN, LEDs_LED3_PIN, LEDs_LED4_PIN, LEDs_LED5_PIN, LEDs_LED6_PIN, LEDs_LED7_PIN};
const GPIO_Regs* KEYs_PORT[7] = {KEYs_KEY1_PORT, KEYs_KEY2_PORT, KEYs_KEY3_PORT, KEYs_KEY4_PORT, KEYs_KEY5_PORT, KEYs_KEY6_PORT, KEYs_KEY7_PORT};
const uint32_t KEYs_PIN[7] = {KEYs_KEY1_PIN, KEYs_KEY2_PIN, KEYs_KEY3_PIN, KEYs_KEY4_PIN, KEYs_KEY5_PIN, KEYs_KEY6_PIN, KEYs_KEY7_PIN};

const uint16_t speed[7] = {65535, 57343, 49151, 40959, 32767, 24575, 16383};

bool        key_state[7],is_key_triggered[7];
uint8_t     key_stable_cnt[7];
uint8_t     difficulty;
uint16_t    buffer1[512],buffer2[512];
uint16_t    play_prog1,play_prog2;

SPIF_HandleTypeDef *Handle;

int main(void) {
    SYSCFG_DL_init();
    __NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    __NVIC_EnableIRQ(DAC12_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_LEDs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_LEDs_INST_INT_IRQN);
    SPIF_Init(Handle, SPI_Flash_INST, COMs_PORT, COMs_CS_Flash_PIN);

    for (uint8_t i = 0; i < 7; i++){
        key_state[i] = KEY_RELEASED;
        is_key_triggered[i] = false;
        key_stable_cnt[i] = 0;
    }
    game_state = WELCOME;
    buffer_state = IDLE_BUF2;
    play_prog1 = WELCOME_ADDR;
    play_prog2 = 0;
    SPIF_ReadPage(Handle, play_prog1++, buffer1, 1024, 0);
    SPIF_ReadPage(Handle, play_prog1++, buffer2, 1024, 0);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    while (1) {
        //音频播放缓冲区更新
        if (buffer_state == FILL_BUF1){
            DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer2[0]);
            DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
            DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
            DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
            SPIF_ReadPage(Handle, play_prog1++, buffer1, 1024, 0);
            buffer_state = IDLE_BUF1;
        }
        else if (buffer_state == FILL_BUF2){
            DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
            DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
            DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
            DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
            SPIF_ReadPage(Handle, play_prog1++, buffer2, 1024, 0);
            buffer_state = IDLE_BUF2;
        }

        switch (game_state){
            case WELCOME:

            break;
            case DIFF_SEL:

            break;
            case LEVEL_SEL:

            break;
            case PLAYING:

            break;
            case GAME_OVER:

            break;
            default:break;
        }
    }
}

void DAC12_IRQHandler(){
    switch (DAC12_INT_IRQN){
        case DAC12_INST_DMA_TRIGGER:
            if (buffer_state == IDLE_BUF1)
                buffer_state == FILL_BUF2;
            else if (buffer_state == IDLE_BUF2)
                buffer_state == FILL_BUF1;
        break;
        default:break;
    }
}

void TIMER_KEYs_INST_IRQHandler(){
    switch (TIMER_KEYs_INST_INT_IRQN){
        case TIMER_KEYs_INST_LOAD_VALUE:
            bool current_state;
            for (uint8_t i = 0; i < 7; i++){
                current_state = DL_GPIO_readPins(KEYs_PORT[i], KEYs_PIN[i]);
                if (current_state == key_state[i])
                    key_stable_cnt[i] = 0;
                else{
                    key_stable_cnt[i]++;
                    if (key_stable_cnt[i] >= STABLE_CNT){
                        key_state[i] = current_state;
                        key_stable_cnt[i] = 0;
                        if (current_state == KEY_PRESSED)
                            is_key_triggered[i] = true;
                    }
                }
            }

        break;
        default:break;
    }
}