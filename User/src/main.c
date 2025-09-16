#include "inc/ti_msp_dl_config.h"
#include "SPIflash.h"

#define SPIF_DEBUG SPIF_DEBUG_DISABLE

int main(void) {
    SYSCFG_DL_init();
    DL_GPIO_setPins(GPIO_test_PORT, GPIO_test_PIN_0_PIN);
    while (1) {
        DL_GPIO_togglePins(GPIO_test_PORT, GPIO_test_PIN_0_PIN);
        delay_cycles(32000000);
    }
}