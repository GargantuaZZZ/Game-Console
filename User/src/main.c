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
#define DIFF_ADDR0      0x10
#define DIFF_LEN        32
#define START_ADDR      0x20
#define START_LEN       64
#define LEVEL_ADDR0     0x30
#define LEVEL_LEN       64
#define BGM_ADDR        0x40
#define BGM_LEN         3200
#define ALERT_LEN       16

enum{
    WELCOME,
    DIFF_SEL,
    LEVEL_SEL,
    LEVEL_SEL_BUSY,
    PLAYING,
    GAME_OVER
}game_state;

enum{
    IDLE_BUF1,
    FILL_BUF1,
    IDLE_BUF2,
    FILL_BUF2
}buffer_state;

enum{
    IDLE,
    BUSY,
    SELECTED,
    CONFIRMED
}diff_sel_state;

const GPIO_Regs* LEDs_PORT[7] = {LEDs_LED1_PORT, LEDs_LED2_PORT, LEDs_LED3_PORT, LEDs_LED4_PORT, LEDs_LED5_PORT, LEDs_LED6_PORT, LEDs_LED7_PORT};
const uint32_t LEDs_PIN[7] = {LEDs_LED1_PIN, LEDs_LED2_PIN, LEDs_LED3_PIN, LEDs_LED4_PIN, LEDs_LED5_PIN, LEDs_LED6_PIN, LEDs_LED7_PIN};
const GPIO_Regs* KEYs_PORT[7] = {KEYs_KEY1_PORT, KEYs_KEY2_PORT, KEYs_KEY3_PORT, KEYs_KEY4_PORT, KEYs_KEY5_PORT, KEYs_KEY6_PORT, KEYs_KEY7_PORT};
const uint32_t KEYs_PIN[7] = {KEYs_KEY1_PIN, KEYs_KEY2_PIN, KEYs_KEY3_PIN, KEYs_KEY4_PIN, KEYs_KEY5_PIN, KEYs_KEY6_PIN, KEYs_KEY7_PIN};

const uint16_t speed[7] = {65535, 57343, 49151, 40959, 32767, 24575, 16383};

bool        key_state[7],is_key_triggered[7];
uint8_t     key_stable_cnt[7];
uint8_t     difficulty,level;
uint16_t    buffer1[512],buffer2[512];
uint16_t    play_prog1,play_prog2;

SPIF_HandleTypeDef *Handle;

void FillBuffer(){
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
}

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
    diff_sel_state = IDLE;
    difficulty = 0;
    level = 0;
    play_prog1 = WELCOME_ADDR;
    play_prog2 = 0;
    SPIF_ReadPage(Handle, play_prog1++, buffer1, 1024, 0);
    SPIF_ReadPage(Handle, play_prog1++, buffer2, 1024, 0);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    while (1) {
        switch (game_state){
            //欢迎界面
            case WELCOME:
                while (play_prog1 < WELCOME_ADDR + WELCOME_LEN)
                    FillBuffer();
                game_state = DIFF_SEL;
            break;

            //难度选择
            case DIFF_SEL:
                if (diff_sel_state == BUSY){
                    while (play_prog1 < DIFF_ADDR0 + (difficulty + 1) * DIFF_LEN)
                        FillBuffer();
                    diff_sel_state = SELECTED;
                }
                else if (diff_sel_state = CONFIRMED){
                    while (play_prog1 < START_ADDR + START_LEN)
                        FillBuffer();
                    DL_Timer_setLoadValue(TIMER_LEDs_INST, speed[difficulty]);
                    diff_sel_state = IDLE;
                    game_state = LEVEL_SEL;
                }
                else {
                    for (uint8_t i = 0; i < 7; i++){
                        if (is_key_triggered[i]){
                            if (diff_sel_state == IDLE){
                                play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
                                difficulty = i;
                                diff_sel_state = BUSY;
                            }
                            else if (diff_sel_state == SELECTED){
                                if (i == difficulty){
                                    play_prog1 = START_ADDR;
                                    diff_sel_state = CONFIRMED;
                                }
                                else{
                                    play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
                                    difficulty = i;
                                    diff_sel_state = BUSY;
                                }
                            }
                        }
                    }
                }
            break;

            //关卡选择
            case LEVEL_SEL:
                play_prog1 = LEVEL_ADDR0 + level * LEVEL_LEN;
                game_state = LEVEL_SEL_BUSY;
            break;

            //开始音乐播放
            case LEVEL_SEL_BUSY:
                while (play_prog1 < LEVEL_ADDR0 + (level + 1) * LEVEL_LEN)
                    FillBuffer();
                game_state = PLAYING;
                play_prog1 = BGM_ADDR;
            break;

            //开始游戏
            case PLAYING:
                if (play_prog1 >= BGM_ADDR + BGM_LEN)
                    play_prog1 = BGM_ADDR;
                FillBuffer();

            break;

            //游戏结束
            case GAME_OVER:

            break;
            default:break;
        }
    }
}

void DAC12_IRQHandler(){
    switch (DAC12_INT_IRQN){
        case DL_DAC12_IIDX_DMA_DONE:
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
        case DL_TIMER_IIDX_ZERO:
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

void TIMER_LEDs_INST_IRQHandler(){
    switch (TIMER_LEDs_INST_INT_IRQN){
        case DL_TIMER_IIDX_ZERO:

        break;
        default:break;
    }
}