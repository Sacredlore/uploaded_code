#ifndef MCU_SHELL_H
#define MCU_SHELL_H

#define _XTAL_FREQ 40000000UL

#pragma config FOSC      = HS1   // HS oscillator (Medium power, 4 MHz - 16 MHz)
#pragma config PWRTEN    = ON    // Power Up Timer
#pragma config BOREN     = OFF   // Brown Out Detect (circuit that monitors the VDD level during operation)
#pragma config MCLRE     = ON    // Master Clear Enable (MCLR Enabled)
#pragma config RETEN     = ON    // Ultra low-power regulator is Enabled (Controlled by SRETEN bit)
#pragma config INTOSCSEL = LOW   // LF-INTOSC in Low-power mode during Sleep
#pragma config SOSCSEL   = DIG   // Digital (SCLKI) mode
#pragma config FCMEN     = OFF   // Fail-Safe Clock Monitor
#pragma config IESO      = OFF   // Internal External Oscillator Switch Over Mode
#pragma config WDTEN     = OFF   // Watchdog Timer
#pragma config CANMX     = PORTB
#pragma config MSSPMSK   = MSK7  // MSSP address masking (7 Bit address masking mode)
#pragma config MCLRE     = ON    // Master Clear Enable (MCLR Enabled, RE3 Disabled)
#pragma config XINST     = OFF   // Extended Instruction Set disabled
#pragma config STVREN    = OFF   // Stack Overflow Reset (Disabled)
#pragma config PLLCFG    = ON    // PLL 4x enable bit

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strings and memory functions
#include <ctype.h> // character types
#include <stdint.h> // integer types
#include <stdbool.h>
#include <stdarg.h> // variadic functions
#include <stddef.h>
#include <math.h>

#define SET_BITMASK(a,b) ((a) |= (b))
#define CLEAR_BITMASK(a,b) ((a) &= (~(b)))
#define SET_BIT(a,n) ((a) |= (1ULL<<(n)))
#define CLEAR_BIT(a,n) ((a) &= ~(1ULL<<(n)))
#define BIT_IS_SET(a,n) (!!((a) & (1ULL<<(n))))
#define BIT_NOT_SET(a,n) (!(BIT_IS_SET(a,n)))

#define TIMER_PRELOAD_2500 0x06 // (  6) - 40000.00Hz,   0.00002500 sec     timer_frequency = (_XTAL_FREQ/4) / timer_prescaler / (256 - timer_preload);
#define TIMER_PRELOAD_2000 0x38 // ( 56) - 50000.00Hz,   0.00002000 sec     timer_period = 1 / timer_frequency;
#define TIMER_PRELOAD_1250 0x83 // (131) - 80000.00Hz,   0.00001250 sec
#define TIMER_PRELOAD_1000 0x9C // (156) - 100000.00Hz,  0.00001000 sec
#define TIMER_PRELOAD_0500 0xCE // (206) - 200000.00Hz,  0.00000500 sec
#define TIMER_PRELOAD_0400 0xD8 // (216) - 250000.00Hz,  0.00000400 sec
#define TIMER_PRELOAD_0250 0xE7 // (231) - 400000.00Hz,  0.00000250 sec
#define TIMER_PRELOAD_0200 0xEC // (236) - 500000.00Hz,  0.00000200 sec
#define TIMER_PRELOAD_0100 0xF6 // (246) - 1000000.00Hz, 0.00000100 sec
#define TIMER_PRELOAD_0050 0xFb // (251) - 2000000.00Hz, 0.00000050 sec

#define UART_TXSTA_CSRC  0x80 // clock source select bit
#define UART_TXSTA_TX9   0x40
#define UART_TXSTA_TXEN  0x20 // transmit enable bit
#define UART_TXSTA_SYNC  0x10
#define UART_TXSTA_SENDB 0x08
#define UART_TXSTA_BRGH  0x04 // high baud rate select bit
#define UART_TXSTA_TRMT  0x02 // transmit shift register status bit, 1 - TSR is empty, 0 - TSR is full
#define UART_TXSTA_TX9D  0x01

#define UART_RCSTA_SPEN  0x80 // serial port enable bit
#define UART_RCSTA_RX9   0x40
#define UART_RCSTA_SREN  0x20 // single receive enable bit
#define UART_RCSTA_CREN  0x10 // continuous receive enable bit
#define UART_RCSTA_ADDEN 0x08
#define UART_RCSTA_FERR  0x04
#define UART_RCSTA_OERR  0x02
#define UART_RCSTA_RX9D  0x01

#define UART_BAUDCON_ABDOVF   0x80 // auto-baud acquisition rollover status bit
#define UART_BAUDCON_RCIDL    0x40
#define UART_BAUDCON_RXDTP    0x20
#define UART_BAUDCON_TXCKP    0x10
#define UART_BAUDCON_BRG16    0x08 // 16-bit baud rate register enable bit
#define UART_BAUDCON_ZERO_BIT 0x04 // unimplemented, read as 0
#define UART_BAUDCON_WUE      0x02
#define UART_BAUDCON_ABDEN    0x01 // auto-baud detect enable bit

#define UART_BUFFER_SIZE  0x80
#define SHELL_COMAND_SIZE 0x40
#define SHELL_ARGC_MAX    0x05

extern char *msg_incorrect_parameters;
extern char *msg_unacceptable_symbol;
extern char *msg_large_expression;
extern char *msg_command_not_found;

enum { // shell states
    SHELL_GET_INPUT,
    SHELL_PROCEED_COMMAND,
    SHELL_SHOW_PROMPT
};

enum { // shell return codes
    STACK_BREAK,
    UNACCEPTABLE_SYMBOL,
    INTERNAL_LIMIT,
    INCORRECT_PARAMETERS,    
    COMMAND_NOT_FOUND,
    RETURN_OK
};

/* function prototypes */
void mcu_setup_uart_advanced(uint16_t baud_rate);
void mcu_setup_uart_compatible(uint16_t baud_rate);
uint8_t uart_transmit(char bytes[], uint8_t length);
uint8_t uart_receive(char bytes[], uint8_t length);
uint8_t build_argv(char cmdline[], uint8_t *argc, char *argv[]);
void mcu_shell_machine_command(void);
void mcu_shell_machine_silent(void);
void mcu_shell_echo(int8_t argc, char *argv[]);
void mcu_shell_help(int8_t argc, char *argv[]);
void mcu_shell_glcd_brightness(int8_t argc, char *argv[]);

#endif
