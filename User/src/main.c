#include "inc/ti_msp_dl_config.h"

int main(void) {
    SYSCFG_DL_init();
    while (1) {
        DL_GPIO_togglePins(GPIO_test_PORT, GPIO_test_PIN_0_PIN);
        delay_cycles(16000000);
    }
}