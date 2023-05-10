#ifndef _ADDITION_H_
#define _ADDITION_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <ti/drivers/UART.h>

void print_number(UART_Handle handle, int n);

int uart_read_number(UART_Handle handle);

#endif
