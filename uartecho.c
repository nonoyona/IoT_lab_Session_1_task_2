#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/timer/GPTimerCC26XX.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/Types.h>
/* Example/Board Header files */
#include "Board.h"
/* My Files*/
#include "addition.h"

// Predefined messages used for user communication through the UART
#define MSG_ADD_INIT_COMPLETE "Initialisation complete! (Addition)\r\n"
#define MSG_READ_FIRST "Insert first number:\r\n"
#define MSG_READ_SECOND "Insert second number:\r\n"
#define MSG_RESULT "Your result:\r\n"
#define MSG_READ "Read: "
#define MSG_ECHO_INIT_COMPLETE "Initialisation complete! (Echo)\r\n"
#define MSG_BYTES_PER_SECOND "Throughput last round was (bytes/second):"

// Timer period in milliseconds for throughput test
#define TIMER_PERIOD_MS 10000

// Enumeration of possible state machine states
typedef enum
{
    ADDITION_SM_INIT,         // Addition state machine initialization
    ADDITION_SM_READ_FIRST,   // Read the first number for addition
    ADDITION_SM_READ_SECOND,  // Read the second number for addition
    ADDITION_SM_ADD_AND_SEND, // Perform addition and send the result
    SIMPLE_ECHO,              // Simple echo state (UART)
    THROUGHPUT_TEST,          // Test throughput of UART communication
    INTERRUPT_TRIGGERED       // Handle interrupt triggered by button press
} StateMachineState;

// State machine structure holding the current state and numbers
typedef struct
{
    StateMachineState state;    // Current state of the state machine
    int_fast64_t first_number;  // First number for addition or other operations
    int_fast64_t second_number; // Second number for addition or other operations
} StateMachine;

UART_Handle uart;     // UART handle for communication with the user
StateMachine asm_obj; // State machine object to manage operations

// Timer callback function triggered periodically during throughput test
void timerCallback(GPTimerCC26XX_Handle handle,
                   GPTimerCC26XX_IntMask interruptMask)
{
    // Toggles LED and state of asm_obj.second_number
    if (asm_obj.second_number == 0)
    {
        asm_obj.second_number = 1;
    }
    else
    {
        asm_obj.second_number = 0;
        return;
    }
    GPIO_toggle(Board_GPIO_LED0);
}

GPTimerCC26XX_Handle hTimer;
void init_timer()
{
    // Set up the GPTimer
    GPTimerCC26XX_Params timerParams;
    GPTimerCC26XX_Params_init(&timerParams);
    timerParams.width = GPT_CONFIG_32BIT;
    timerParams.mode = GPT_MODE_PERIODIC_UP;
    timerParams.direction = GPTimerCC26XX_DIRECTION_UP;
    timerParams.debugStallMode = GPTimerCC26XX_DEBUG_STALL_OFF;

    hTimer = GPTimerCC26XX_open(Board_GPTIMER0A, &timerParams);
    if (hTimer == NULL)
    {
        while (1)
        {
        }
    }

    Types_FreqHz freq;
    BIOS_getCpuFreq(&freq);
    // Set Timer period and callback function
    GPTimerCC26XX_Value loadVal = (freq.lo / 1000) * TIMER_PERIOD_MS;
    GPTimerCC26XX_setLoadValue(hTimer, loadVal);
    GPTimerCC26XX_registerInterrupt(hTimer, timerCallback, GPT_INT_TIMEOUT);

    GPTimerCC26XX_start(hTimer);
}

void buttonCallback(uint_least8_t index)
{
    UART_write(uart, "Button pressed!\r\n", sizeof("Button pressed!\r\n"));
}

void addition_sm_init()
{
    asm_obj.state = ADDITION_SM_INIT;
    asm_obj.first_number = 0;
    asm_obj.second_number = 0;
}

void simple_echo_sm_init()
{
    asm_obj.state = SIMPLE_ECHO;
    asm_obj.first_number = 0;
}

void throughput_sm_init()
{
    asm_obj.state = THROUGHPUT_TEST;
    asm_obj.first_number = 0;
    init_timer();
}

void interrupt_sm_init()
{
    asm_obj.state = INTERRUPT_TRIGGERED;
    asm_obj.first_number = 0;

    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(Board_GPIO_BUTTON0, buttonCallback);
    GPIO_enableInt(Board_GPIO_BUTTON0);
}

void sm_update()
{
    int_fast64_t num;
    const char test_str[] =
        "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
    size_t bytesWritten;
    switch (asm_obj.state)
    {
    case ADDITION_SM_INIT:
        UART_write(uart, MSG_ADD_INIT_COMPLETE, sizeof(MSG_ADD_INIT_COMPLETE));
        asm_obj.state = ADDITION_SM_READ_FIRST;
        break;

    case ADDITION_SM_READ_FIRST:
        UART_write(uart, MSG_READ_FIRST, sizeof(MSG_READ_FIRST));
        num = uart_read_number(uart);
        asm_obj.first_number = num;
        asm_obj.state = ADDITION_SM_READ_SECOND;
        UART_write(uart, MSG_READ, sizeof(MSG_READ));
        print_number(uart, num);
        break;

    case ADDITION_SM_READ_SECOND:
        UART_write(uart, MSG_READ_SECOND, sizeof(MSG_READ_SECOND));
        num = uart_read_number(uart);
        asm_obj.second_number = num;
        asm_obj.state = ADDITION_SM_ADD_AND_SEND;
        UART_write(uart, MSG_READ, sizeof(MSG_READ));
        print_number(uart, num);
        break;

    case ADDITION_SM_ADD_AND_SEND:
        UART_write(uart, MSG_RESULT, sizeof(MSG_RESULT));
        int sum = asm_obj.first_number + asm_obj.second_number;
        print_number(uart, sum);
        asm_obj.state = ADDITION_SM_READ_FIRST;
        break;
    case INTERRUPT_TRIGGERED:
    case SIMPLE_ECHO:
        if (asm_obj.first_number == 0)
        {
            UART_write(uart, MSG_ECHO_INIT_COMPLETE,
                       sizeof(MSG_ECHO_INIT_COMPLETE));
            asm_obj.first_number = 1;
        }

        char read;
        UART_read(uart, &read, 1);
        if (read == '\n' || read == '\r')
        {
            UART_write(uart, "\r\n", sizeof("\r\n"));
        }
        else
        {
            UART_write(uart, &read, 1);
        }

        break;
    case THROUGHPUT_TEST:
        if (asm_obj.second_number == 0)
        {
            bytesWritten = UART_write(uart, test_str, sizeof(test_str));
            asm_obj.first_number += bytesWritten;
        }
        else if (asm_obj.second_number == 1)
        {
            uint32_t throughput = (asm_obj.first_number) / (TIMER_PERIOD_MS / 1000);

            // Print throughput
            // Reset bytes_transferred
            asm_obj.first_number = 0;
            UART_writeCancel(uart);
            UART_write(uart, "\r\n", sizeof("\r\n"));
            UART_write(uart, MSG_BYTES_PER_SECOND, sizeof(MSG_BYTES_PER_SECOND));
            print_number(uart, throughput);
            asm_obj.second_number = 2;
            // BECOMES MORE IF A HIGHER BAUD RATE IS CHOSEN OFC...
            // ALSO INCREASES SLIGHTLY IF A LARGER BUFFER IS USED
        }
        break;
    default:
        break;
    }
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{

    UART_Params uartParams;

    /* Call driver init functions */
    GPIO_init();
    UART_init();

    /* Configure the LED pin */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 500000;
    uartParams.readMode = UART_MODE_BLOCKING;

    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL)
    {
        /* UART_open() failed */
        while (1)
            ;
    }

    // addition_sm_init();
    // simple_echo_sm_init();
    // throughput_sm_init();
    interrupt_sm_init();

    while (true)
    {
        sm_update();
    }
}
