#include "inc/ti_msp_dl_config.h"
#include "inc/SPIflash.h"
#include "inc/OLED.h"

#define SPIF_DEBUG SPIF_DEBUG_DISABLE

#define KEY_PRESSED     false
#define KEY_RELEASED    true
#define STABLE_CNT      20

#define READ_KEY(n)     DL_GPIO_readPins(KEYs_KEY##n##_PORT, KEYs_KEY##n##_PIN)
#define SET_LED(n)      DL_GPIO_setPins(LEDs_LED##n##_PORT, LEDs_LED##n##_PIN)
#define CLEAR_LED(n)    DL_GPIO_clearPins(LEDs_LED##n##_PORT, LEDs_LED##n##_PIN)

bool key_state[7],is_key_triggered[7];
uint8_t key_stable_cnt[7];

int main(void) {
    SYSCFG_DL_init();

    for (uint8_t i = 0; i < 7; i++){
        key_state[i] = KEY_RELEASED;
        is_key_triggered[i] = false;
        key_stable_cnt[i] = 0;
    }

    while (1) {

    }
}

void TIMER_KEYs_INST_IRQHandler(){
    switch (TIMER_KEYs_INST_INT_IRQN){
        case TIMER_KEYs_INST_LOAD_VALUE:
            bool current_state;
            for (uint8_t i = 0; i < 7; i++){
                current_state = READ_KEY(i);
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