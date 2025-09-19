#include "inc/utilities.h"

int _write(int file, char *ptr, int len){
    for (int i = 0; i < len; i++)
        DL_UART_transmitDataBlocking(UART0, (uint8_t)ptr[i]);
    return len;
}