#include "addition.h"

void convert_int_to_str(int number, char *str, size_t size)
{
    if (size == 0)
        return;

    char *str_start = str;
    bool is_negative = false;

    if (number < 0)
    {
        is_negative = true;
        number = -number;
    }

    do
    {
        *str++ = '0' + (number % 10);
        number /= 10;
    } while (number > 0 && str - str_start < size);

    if (is_negative && str - str_start < size)
    {
        *str++ = '-';
    }

    *str-- = '\0';

    // Reverse the string
    while (str_start < str)
    {
        char tmp = *str_start;
        *str_start = *str;
        *str = tmp;
        str_start++;
        str--;
    }
}

int get_str_length(char *str, int max_length)
{
    uint16_t length = 0;
    while (str[length] != '\0' && length < max_length)
    {
        length++;
    }
    return length;
}

void print_number(UART_Handle handle, int n)
{
    char str_num[20];
    convert_int_to_str(n, str_num, sizeof(str_num));
    UART_write(handle, str_num, get_str_length(str_num, sizeof(str_num)));
    UART_write(handle, "\r\n", sizeof("\r\n"));
}

int uart_read_number(UART_Handle handle)
{
    int number = 0;
    int sign = 1;
    uint8_t data;

    while (1)
    {
        UART_read(handle, &data, 1);
        UART_write(handle, &data, 1);

        if (data == '-')
        {
            sign = -1;
        }
        else if (data >= '0' && data <= '9')
        {
            number = number * 10 + (data - '0');
        }
        else if (data == '\n' || data == '\r')
        {
            break;
        }
    }
    UART_write(handle, "\r\n", 2);

    return number * sign;
}
