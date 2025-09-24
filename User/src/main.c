#include "inc/ti_msp_dl_config.h"
#include "inc/SPIflash.h"
#include "inc/OLED.h"
#include "inc/Alerts.h"
#include <stdlib.h>
#include <time.h>

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
#define WIN_ADDR        0x50
#define WIN_LEN         32
#define LOSE_ADDR       0x60
#define LOSE_LEN        32
#define WIN_ALL_ADDR    0x70
#define WIN_ALL_LEN     64

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

enum{
    HIT,
    MISS
}key_result;

const GPIO_Regs* LEDs_PORT[7] = {LEDs_LED1_PORT, LEDs_LED2_PORT, LEDs_LED3_PORT, LEDs_LED4_PORT, LEDs_LED5_PORT, LEDs_LED6_PORT, LEDs_LED7_PORT};
const uint32_t LEDs_PIN[7] = {LEDs_LED1_PIN, LEDs_LED2_PIN, LEDs_LED3_PIN, LEDs_LED4_PIN, LEDs_LED5_PIN, LEDs_LED6_PIN, LEDs_LED7_PIN};
const GPIO_Regs* KEYs_PORT[7] = {KEYs_KEY1_PORT, KEYs_KEY2_PORT, KEYs_KEY3_PORT, KEYs_KEY4_PORT, KEYs_KEY5_PORT, KEYs_KEY6_PORT, KEYs_KEY7_PORT};
const uint32_t KEYs_PIN[7] = {KEYs_KEY1_PIN, KEYs_KEY2_PIN, KEYs_KEY3_PIN, KEYs_KEY4_PIN, KEYs_KEY5_PIN, KEYs_KEY6_PIN, KEYs_KEY7_PIN};

const uint16_t SPEED[7] = {65535, 57343, 49151, 40959, 32767, 24575, 16383};
const uint16_t TARGET_SCORE[7][3] = {{20, 50, 90}, {40, 100, 160}, {50, 150, 240}, {80, 250, 320}, {100, 280, 390}, {120, 300, 460}, {150, 300, 540}};

bool        key_state[7],is_key_triggered[7];
bool        is_LED_active;
uint8_t     key_stable_cnt[7];
uint8_t     difficulty,level,target1,target2,game_prog;
uint16_t    buffer1[512],buffer2[512];
uint16_t    play_prog1,play_prog2,score;

SPIF_HandleTypeDef *Handle;



void FillBuffer(){
    if (buffer_state == FILL_BUF1){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer2[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        SPIF_ReadPage(Handle, play_prog1++, buffer1, 1024, 0);
        for (uint8_t i = 0; i < 7; i++){
            if ((i == target1 || i == target2) && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = HIT;
                is_key_triggered[i] = false;
                DL_GPIO_clearPins(LEDs_PORT[i], LEDs_PIN[i]);
                if (i == target1)
                    target1 = 8;
                else
                    target2 = 8;
                score += 3;
            }
            if (i != target1 && i != target2 && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = MISS;
                is_key_triggered[i] = false;
                score -= 1;
            }
        }
        if (play_prog2){
            for (uint8_t i = 0; i < 512; i++){
                if (key_result == HIT)
                    buffer1[i] = alert_hit[play_prog2++];
                else
                    buffer1[i] = alert_miss[play_prog2++];
                if (play_prog2 >= ALERT_LEN){
                    play_prog2 = 0;
                    break;
                }
            }
            
        }
        buffer_state = IDLE_BUF1;
    }
    else if (buffer_state == FILL_BUF2){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        SPIF_ReadPage(Handle, play_prog1++, buffer2, 1024, 0);
        for (uint8_t i = 0; i < 7; i++){
            if ((i == target1 || i == target2) && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = HIT;
                is_key_triggered[i] = false;
                DL_GPIO_clearPins(LEDs_PORT[i], LEDs_PIN[i]);
                if (i == target1)
                    target1 = 8;
                else
                    target2 = 8;
                score += 3;
            }
            if (i != target1 && i != target2 && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = MISS;
                is_key_triggered[i] = false;
                score -= 1;
            }
        }
        if (play_prog2){
            for (uint8_t i = 0; i < 512; i++){
                if (key_result == HIT)
                    buffer2[i] = alert_hit[play_prog2++];
                else
                    buffer2[i] = alert_miss[play_prog2++];
                if (play_prog2 >= ALERT_LEN){
                    play_prog2 = 0;
                    break;
                }
            }
            
        }
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
    __NVIC_ClearPendingIRQ(TIMER_PROG_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_PROG_INST_INT_IRQN);
    SPIF_Init(Handle, SPI_Flash_INST, COMs_PORT, COMs_CS_Flash_PIN);

    for (uint8_t i = 0; i < 7; i++){
        key_state[i] = KEY_RELEASED;
        is_key_triggered[i] = false;
        key_stable_cnt[i] = 0;
    }
    is_LED_active = false;
    game_state = WELCOME;
    buffer_state = IDLE_BUF2;
    diff_sel_state = IDLE;
    key_result = HIT;
    difficulty = 0;
    level = 0;
    target1 = 8;
    target2 = 8;
    game_prog = 0;
    play_prog1 = WELCOME_ADDR;
    play_prog2 = 0;
    SPIF_ReadPage(Handle, play_prog1++, buffer1, 1024, 0);
    SPIF_ReadPage(Handle, play_prog1++, buffer2, 1024, 0);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
    OLED_ShowCoverIMG();

    while (1) {
        switch (game_state){
            //欢迎界面
            case WELCOME:
                while (play_prog1 < WELCOME_ADDR + WELCOME_LEN)
                    FillBuffer();
                DL_Timer_startCounter(TIMER_KEYs_INST);
                game_state = DIFF_SEL;
            break;

            //难度选择
            case DIFF_SEL:
                if (diff_sel_state == BUSY){
                    while (play_prog1 < DIFF_ADDR0 + (difficulty + 1) * DIFF_LEN)
                        FillBuffer();
                    DL_Timer_startCounter(TIMER_KEYs_INST);
                    diff_sel_state = SELECTED;
                }
                else if (diff_sel_state = CONFIRMED){
                    while (play_prog1 < START_ADDR + START_LEN)
                        FillBuffer();
                    DL_Timer_setLoadValue(TIMER_LEDs_INST, SPEED[difficulty]);
                    level = 0;
                    diff_sel_state = IDLE;
                    game_state = LEVEL_SEL;
                }
                else {
                    for (uint8_t i = 0; i < 7; i++){
                        if (is_key_triggered[i]){
                            if (diff_sel_state == IDLE){
                                play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
                                difficulty = i;
                                DL_Timer_stopCounter(TIMER_KEYs_INST);
                                diff_sel_state = BUSY;
                            }
                            else if (diff_sel_state == SELECTED){
                                if (i == difficulty){
                                    play_prog1 = START_ADDR;
                                    DL_Timer_stopCounter(TIMER_KEYs_INST);
                                    diff_sel_state = CONFIRMED;
                                }
                                else{
                                    play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
                                    difficulty = i;
                                    DL_Timer_stopCounter(TIMER_KEYs_INST);
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
                DL_Timer_startCounter(TIMER_KEYs_INST);
                game_state = PLAYING;
                play_prog1 = BGM_ADDR;
                srand((unsigned)time(NULL));
                DL_Timer_startCounter(TIMER_LEDs_INST);
                DL_Timer_startCounter(TIMER_PROG_INST);
            break;

            //开始游戏
            case PLAYING:
                if (play_prog1 >= BGM_ADDR + BGM_LEN)
                    play_prog1 = BGM_ADDR;
                FillBuffer();
                if (game_prog >= 120){
                    DL_Timer_stopCounter(TIMER_KEYs_INST);
                    DL_Timer_stopCounter(TIMER_LEDs_INST);
                    DL_Timer_stopCounter(TIMER_PROG_INST);
                    game_prog = 0;
                    for (uint8_t i = 0; i < 7; i++)
                        DL_GPIO_clearPins(LEDs_PORT[i], LEDs_PIN[i]);
                    target1 = 8;
                    target2 = 8;
                    game_state = GAME_OVER;
                }
            break;

            //游戏结束
            case GAME_OVER:
                if (score >= TARGET_SCORE[difficulty][level]){
                    if (level < 2){
                        play_prog1 = WIN_ADDR;
                        while (play_prog1 < WIN_ADDR + WIN_LEN)
                            FillBuffer();
                        level++;
                        game_state = LEVEL_SEL;
                    }
                    else{
                        play_prog1 = WIN_ALL_ADDR;
                        while (play_prog1 < WIN_ALL_ADDR + WIN_ALL_LEN)
                            FillBuffer();
                        game_state = DIFF_SEL;
                    }
                }
                else{
                    play_prog1 = LOSE_ADDR;
                    while (play_prog1 < LOSE_ADDR + LOSE_LEN)
                        FillBuffer();
                    game_state = DIFF_SEL;
                }
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
            is_LED_active = !is_LED_active;
            if (is_LED_active){
                target1 = rand() % 7;
                DL_GPIO_setPins(LEDs_PORT[target1], LEDs_PIN[target1]);
                if (difficulty >= 4){
                    do{
                        target2 = rand() % 7;
                    }while(target2 == target1);
                    DL_GPIO_setPins(LEDs_PORT[target2], LEDs_PIN[target2]);
                }
                else
                    target2 = 8;
            }
            else{
                for (uint8_t i = 0; i < 7; i++)
                    DL_GPIO_clearPins(LEDs_PORT[i], LEDs_PIN[i]);
                target1 = 8;
                target2 = 8;
            }
        break;
        default:break;
    }
}

void TIMER_PROG_INST_IRQHandler(){
    switch (TIMER_PROG_INST_INT_IRQN){
        case DL_TIMER_IIDX_ZERO:
            game_prog++;
        break;
        default:break;
    }
}