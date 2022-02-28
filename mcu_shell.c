#include "mcu_shell.h"
#include "analog.h"
#include "nrf24l01.h"
#include "ili9341.h"

uint32_t timer0_time = 0;
uint32_t timer1_time = 0;
uint32_t timer2_time = 0;
uint32_t timer3_time = 0;
uint32_t timer4_time = 0;

char *msg_incorrect_parameters = "incorrect parameters\r";
char *msg_unacceptable_symbol = "unacceptable symbol\r";
char *msg_large_expression = "large expression\r";
char *msg_command_not_found = "command not found\r";

struct { // UART transmit buffer
    char *offset_p, *offset_s;
    uint8_t counter;
    char memory[UART_BUFFER_SIZE];
} u_tx = {u_tx.memory, u_tx.memory};

struct { // UART receive buffer
    char *offset_p, *offset_s;
    uint8_t counter;
    char memory[UART_BUFFER_SIZE];
} u_rc = {u_rc.memory, u_rc.memory};

struct {
    char *function_name;
    char *function_desc;
    int8_t min_args, max_args;
    void (*shell_function)(int8_t argc, char *argv[]);
} shell_table[] = {
    {"help", "", 1, 1, mcu_shell_help},
    {"echo", "", 1, 5, mcu_shell_echo},
    {"glcd_brightness", "", 1, 5, mcu_shell_glcd_brightness},
    {"", "", 0, 0, ((void *) 0)},
};

void mcu_shell_echo(int8_t argc, char *argv[])
{
    for (uint8_t i = 0; i < argc; i++) {
        uart_transmit(argv[i], strlen(argv[i]));
        uart_transmit("\n\r", strlen("\n\r"));
    }
}

void mcu_shell_glcd_brightness(int8_t argc, char *argv[])
{
//    glcd_font font;
//    
//    glcd_select_font(GLCD_FONT_8X12, &font);
//    
//    for (uint8_t i = 0; i < argc; i++) {
//        glcd_draw_framed_string(argv[i], 10, i*15, font);
//    }
    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_WRITE_DISPLAY_BRIGHTNESS); // 0x51
    
    glcd_write_data();
    glcd_spi_send_8(argv[1]);   
}

void mcu_shell_help(int8_t argc, char *argv[])
{
    uint8_t f_index = 0;

    while (strlen(shell_table[f_index].function_name)) {
        uart_transmit(shell_table[f_index].function_name, strlen(shell_table[f_index++].function_name));
        uart_transmit("\n\r", strlen("\n\r"));
    };
}

uint8_t uart_transmit(char bytes[], uint8_t length)
{
    uint8_t bytes_transmit = 0;

    do {
        if (u_tx.counter < sizeof (u_tx.memory)) { // there is free space in buffer
            *u_tx.offset_p++ = *bytes++;
            if (u_tx.offset_p >= (u_tx.memory + sizeof (u_tx.memory))) { // wrap if beyond buffer border
                u_tx.offset_p = u_tx.memory;
            }
            u_tx.counter++, length--, bytes_transmit++;
        } else { // no free space in buffer
            TX1IE = 1;
        }
    } while (length);

    TX1IE = 1;
    return bytes_transmit;
}

uint8_t uart_receive(char bytes[], uint8_t length)
{
    uint8_t bytes_receive = 0;

    do {
        if (u_rc.counter) { // there is something to receive in buffer 
            *bytes++ = *u_rc.offset_s++;
            if (u_rc.offset_s >= (u_rc.memory + sizeof (u_rc.memory))) { // wrap if beyond buffer border
                u_rc.offset_s = u_rc.memory;
            }
            u_rc.counter--, length--, bytes_receive++;
        } else { // nothing to receive in buffer
            break;
        }
    } while (length);

    return bytes_receive;
}

void __interrupt(high_priority) mcu_interrupt_high(void)
{
    if (HLVDIE && HLVDIF) {
        HLVDIF = 0;
    } else if (TMR0IE && TMR0IF) {
        ++timer0_time;
        //TMR0L = TIMER_PRELOAD_1000; // 0x9C (156) - 100000.00Hz,  0.00001000 sec, 0xF6 (246) - 1000000.00Hz, 0.00000100 sec
        //TMR0L = TIMER_PRELOAD_2500; // 0x06 (  6) - 40000.00Hz,   0.00002500 sec
        TMR0IF = 0;
    } else if (TMR1IE && TMR1IF) {
        ++timer1_time;
        TMR1IF = 0;
    } else if (TMR2IE && TMR2IF) {
        ++timer2_time;
        TMR2IF = 0;
    } else if (TMR3IE && TMR3IF) {
        ++timer3_time;
        TMR3IF = 0;
    } else if (TMR4IE && TMR4IF) {
        ++timer4_time;
        TMR4IF = 0;
    }
}

void __interrupt(low_priority) mcu_interrupt_low(void)
{
    if (ADIE && ADIF) {
        ADIF = 0;
    } else if (INT1IE && INT1IF) {
        INT1IF = 0;
    } else if (INT2IE && INT2IF) {
        INT2IF = 0;
    } else if (INT3IE && INT3IF) {
        INT3IF = 0;
    } else if (TX1IE && TX1IF) {
        while (TX1IE) { // transmit for the lost
            if (TRMT1) {
                if (u_tx.counter) {
                    TXREG1 = *u_tx.offset_s++;
                    if (u_tx.offset_s >= (u_tx.memory + sizeof (u_tx.memory))) { // wrap if beyond buffer border
                        u_tx.offset_s = u_tx.memory;
                    }
                    u_tx.counter--;
                } else {
                    TX1IE = 0;
                    while (TRMT1 == 0) // wait for last byte transmits
                        continue;
                }
            } else {
                break;
            }
        }
    } else if (RC1IE && RC1IF) {
        while (RC1IF) { // receive for the lost
            if (FERR1) {
                asm("movf RCREG1,W"); // discard received data that has error
            }
            if (OERR1) {
                asm("bcf RCSTA1,4"); // reset the receiver (CREN - RCSTA bit 4)
                asm("bsf RCSTA1,4"); // enable reception again
            }
            if (u_rc.counter < sizeof (u_rc.memory)) { // there is free space in buffer
                *u_rc.offset_p++ = RCREG1;
                if (u_rc.offset_p >= (u_rc.memory + sizeof (u_rc.memory))) { // wrap if beyond buffer border
                    u_rc.offset_p = u_rc.memory;
                }
                u_rc.counter++;
            } else { // no free space in buffer, overwrite last byte
                *u_rc.offset_p = RCREG1;
            }
        }
    } else if (CTMUIE && CTMUIF) {
        CTMUIF = 0;
    }
}

void mcu_setup_uart_advanced(uint16_t baud_rate) // UART1: TRISC6 - 0, TRISC7 - 1, UART2: TRISB6 - 0, TRISB7 - 1
{
    uint16_t SPBRG_value;

    SPBRG_value = (uint16_t) (_XTAL_FREQ / (4UL * baud_rate) - 1); // baud rate formula

    SPBRGH1 = (uint8_t) (SPBRG_value >> 8), SPBRG1 = (uint8_t) SPBRG_value;
    SPBRGH2 = (uint8_t) (SPBRG_value >> 8), SPBRG2 = (uint8_t) SPBRG_value;

    SET_BITMASK(BAUDCON1, UART_BAUDCON_BRG16); // Baud Rate Generator (BRG) in 16-bit mode with high speed
    //SET_BITMASK(BAUDCON2, UART_BAUDCON_BRG16); // allows wide range of baud rates and low error

    SET_BITMASK(TXSTA1, UART_TXSTA_BRGH | UART_TXSTA_TXEN);
    //SET_BITMASK(TXSTA2, UART_TXSTA_BRGH | UART_TXSTA_TXEN);

    SET_BITMASK(RCSTA1, UART_RCSTA_CREN | UART_RCSTA_SPEN);
    //SET_BITMASK(RCSTA2, UART_RCSTA_CREN | UART_RCSTA_SPEN);
}

void mcu_setup_uart_compatible(uint16_t baud_rate) // UART1: TRISC6 - 0, TRISC7 - 1, UART2: TRISB6 - 0, TRISB7 - 1
{
    if (_XTAL_FREQ >= 10000000) {
        SPBRG1 = (uint8_t) (_XTAL_FREQ / (64UL * baud_rate) - 1); // baud rate formula for low speed
        SPBRG2 = (uint8_t) (_XTAL_FREQ / (64UL * baud_rate) - 1);

        SET_BITMASK(TXSTA1, UART_TXSTA_TXEN); // Baud Rate Generator (BRG) with low speed, if processor frequency is high
        //SET_BITMASK(TXSTA2, UART_TXSTA_TXEN);

        SET_BITMASK(RCSTA1, UART_RCSTA_CREN | UART_RCSTA_SPEN);
        //SET_BITMASK(RCSTA2, UART_RCSTA_CREN | UART_RCSTA_SPEN);
    } else {
        SPBRG1 = (uint8_t) (_XTAL_FREQ / (16UL * baud_rate) - 1); // baud rate formula for high speed
        SPBRG2 = (uint8_t) (_XTAL_FREQ / (16UL * baud_rate) - 1);

        SET_BITMASK(TXSTA1, UART_TXSTA_BRGH | UART_TXSTA_TXEN); // Baud Rate Generator (BRG) with high speed, if processor frequency is low
        //SET_BITMASK(TXSTA2, UART_TXSTA_BRGH | UART_TXSTA_TXEN);

        SET_BITMASK(RCSTA1, UART_RCSTA_CREN | UART_RCSTA_SPEN);
        //SET_BITMASK(RCSTA2, UART_RCSTA_CREN | UART_RCSTA_SPEN);
    }
}

void mcu_setup_spi(void) // TRISC5 - 0 (SDO), TRISC4 - 1 (SDI), TRISC3 - 0 (SCK))
{
    ODCONbits.SSPOD = 0; // disable open drain output
    SSPCON1bits.SSPM = 0b0000; // 0b0000 - clock = FOSC/4, 0b1010- clock = FOSC/8, 0b0001 - clock = FOSC/16, 0b0010 - clock = FOSC/64, 
    SSPCON1bits.CKP = 0; // clock polarity select bit
    SSPSTATbits.CKE = 1; // clock edge select
    SSPSTATbits.SMP = 1; // sample bit
    SSPCON1bits.SSPEN = 1;
    
    // NRF24L01, CKE=1 SMP=1
}

void mcu_setup_timer0(void)
{
    T0CONbits.TMR0ON = 0; // 0 - Stops Timer0, 1 - Enables Timer0
    T0CONbits.T08BIT = 1; // 0 - Timer0 is configured as a 16-bit timer/counter, 1 - Timer0 is configured as an 8-bit timer/counter
    T0CONbits.T0CS = 0; // 0 - Internal instruction cycle clock (CLKO), 1 - Transitions on T0CKI pin
    T0CONbits.T0SE = 0; // 0 - Increments on low-to-high transition on T0CKI pin, 1 - Increments on high-to-low transition on T0CKI pin
    T0CONbits.PSA = 1; // 1 - Timer0 prescaler is not assigned, 0 - Timer0 prescaler is assigned
    T0CONbits.T0PS = 0b000; // 0b000 - 1:2 prescale value, 0b001 - 1:4 prescale value, 0b010 - 1:8 prescale value
    TMR0H = 0, TMR0L = 0; // timer_time = (uint16_t)((TMR0H << 8) | TMR0L);
    //TMR0H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR0L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //    
    TMR0ON = 1; // 1 - Enables Timer0, 0 - Stops Timer0
}

void mcu_setup_timer1(void)
{
    T1GCONbits.TMR1GE = 0; // 0 - Timer1 counts regardless of Timer1 gate function, 1 - Timer1 counting is controlled by the Timer1 gate function
    T1GCONbits.T1GPOL = 0; // 1 - Timer1 gate is active-high (Timer1 counts when gate is high), 0 - Timer1 gate is active-low (Timer1 counts when gate is low)
    T1GCONbits.T1GTM = 0; // 1 - Timer1 Gate Toggle mode is enabled, 0 - Timer1 Gate Toggle mode is disabled and toggle flip-flop is cleared
    T1GCONbits.T1GSPM = 0; // 1 - Timer1 Gate Single Pulse mode is enabled and is controlling Timer1 gate, 0 - Timer1 Gate Single Pulse mode is disabled
    T1GCONbits.T1GVAL = 0; // Indicates the current state of the Timer1 gate that could be provided to TMR1H:TMR1L; unaffected by Timer1 Gate Enable (TMR1GE) bit
    T1GCONbits.T1GSS = 0; // Timer1 Gate Source Select bits, 0b11 - Comparator 2 output, 0b10 - Comparator 1 output, 0b01 - TMR2 to match PR2 output, 0b00 - Timer1 gate pin    

    T1CONbits.TMR1ON = 0; // 0 - Stops Timer1, 1 - Enables Timer1
    T1CONbits.TMR1CS = 0; // 1 - Timer1 clock source is the system clock (FOSC), 0 - Timer1 clock source is the instruction clock (FOSC/4)
    T1CONbits.T1CKPS = 0; // 0 - 1:1 Prescale value
    T1CONbits.SOSCEN = 0; // 0 - SOSC is disabled for Timer1, 1 - SOSC is enabled and available for Timer1
    T1CONbits.NOT_T1SYNC = 0; // 1 - Do not synchronize external clock input, 0 -  Synchronizes external clock input
    T1CONbits.RD16 = 1; // 1 - Enables register read/write of Timer1 in one 16-bit operation, 0 - Enables register read/write of Timer1 in two 8-bit operations
    TMR1H = 0, TMR1L = 0; // timer_time = (uint16_t)((TMR1H << 8) | TMR1L);
    //TMR1H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR1L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //
    TMR1ON = 0; // 1 - Enables Timer1, 0 - Stops Timer1
}

void mcu_setup_timer2(void)
{
    T2CONbits.TMR2ON = 0; // 0 - Stops Timer2, 1 - Enables Timer2
    T2CONbits.T2OUTPS = 0; // 0b0000 - 1:1 Postscale, 0b0001 - 1:2 Postscale, ... 0b1111 - 1:16 Postscale
    T2CONbits.T2CKPS = 0; // 0b00 - Prescaler is 1, 0b01 - Prescaler is 4, 0b1x - Prescaler is 16
    TMR2 = 0, PR2 = 0xFF; // Eight-bit Timer and Period registers (TMR2 and PR2, respectively)
    T2CONbits.TMR2ON = 0; // 1 - Enables Timer2, 0 - Stops Timer2
}

void mcu_setup_timer3(void)
{
    T3GCONbits.TMR3GE = 0; // 0 - Timer1 counts regardless of Timer1 gate function, 1 - Timer1 counting is controlled by the Timer1 gate function
    T3GCONbits.T3GPOL = 0; // 1 - Timer1 gate is active-high (Timer1 counts when gate is high), 0 - Timer1 gate is active-low (Timer1 counts when gate is low)
    T3GCONbits.T3GTM = 0; // 1 - Timer1 Gate Toggle mode is enabled, 0 - Timer1 Gate Toggle mode is disabled and toggle flip-flop is cleared
    T3GCONbits.T3GSPM = 0; // 1 - Timer1 Gate Single Pulse mode is enabled and is controlling Timer1 gate, 0 - Timer1 Gate Single Pulse mode is disabled
    T3GCONbits.T3GVAL = 0; // Indicates the current state of the Timer1 gate that could be provided to TMR1H:TMR1L; unaffected by Timer1 Gate Enable (TMR1GE) bit
    T3GCONbits.T3GSS = 0; // Timer1 Gate Source Select bits, 0b11 - Comparator 2 output, 0b10 - Comparator 1 output, 0b01 - TMR2 to match PR2 output, 0b00 - Timer1 gate pin    

    T3CONbits.TMR3ON = 0; // 0 - Stops Timer3, 1 - Enables Timer3
    T3CONbits.TMR3CS = 0; // 1 - Timer1 clock source is the system clock (FOSC), 0 - Timer1 clock source is the instruction clock (FOSC/4)
    T3CONbits.T3CKPS = 0; // 0 - 1:1 Prescale value
    T3CONbits.SOSCEN = 0; // 0 - SOSC is disabled for Timer1, 1 - SOSC is enabled and available for Timer1
    T3CONbits.NOT_T3SYNC = 0; // 1 - Do not synchronize external clock input, 0 -  Synchronizes external clock input
    T3CONbits.RD16 = 1; // 1 - Enables register read/write of Timer1 in one 16-bit operation, 0 - Enables register read/write of Timer1 in two 8-bit operations
    TMR3H = 0, TMR3L = 0; // timer_time = (uint16_t)((TMR3H << 8) | TMR3L);
    //TMR3H = (uint8_t)((0xFFFF - timeval) >> 8);     // 0xFFFF - maximum time in 16-bit timer registers
    //TMR3L = (uint8_t)((0xFFFF - timeval) & 0x00FF); //
    TMR3ON = 0; // 1 - Enables Timer3, 0 - Stops Timer3
}

void mcu_setup_timer4(void)
{
    T4CONbits.TMR4ON = 0; // 0 - Stops Timer4, 1 - Enables Timer4
    T4CONbits.T4OUTPS = 0; // 0b0000 - 1:1 Postscale, 0b0001 - 1:2 Postscale, ... 0b1111 - 1:16 Postscale
    T4CONbits.T4CKPS = 0; // 0b00 - Prescaler is 1, 0b01 - Prescaler is 4, 0b1x - Prescaler is 16
    TMR4 = 0, PR4 = 0xFF; // Eight-bit Timer and Period registers (TMR4 and PR4, respectively)
    T4CONbits.TMR4ON = 0; // 1 - Enables Timer4, 0 - Stops Timer4
}

void mcu_setup_hlvd()
{
    HLVDCONbits.HLVDEN = 1;
    HLVDCONbits.VDIRMAG = 0; // 1 - voltage equals or exceeds trip point, 0 - voltage equals or falls below trip point
    HLVDCONbits.HLVDL = 0b1000; // 0b1000 - voltage in range ~(3.24V > 3.32V < 3.41V), 0b1110 - voltage in range ~(4.64V > 4.75V < 4.87V)
    while (!HLVDCONbits.BGVST)
        NOP();

    while (!HLVDCONbits.IRVST)
        NOP();
}

void mcu_setup_cpp_modules()
{
    CCP1CONbits.CCP1M = 0b0000; // EECCP

    CCP2CONbits.CCP2M = 0b0000; // CCP
    CCP3CONbits.CCP3M = 0b0000;
    CCP4CONbits.CCP4M = 0b0000;
    CCP5CONbits.CCP5M = 0b0000;

    CCPTMRSbits.C1TSEL = 0;
    CCPTMRSbits.C2TSEL = 0;
    CCPTMRSbits.C3TSEL = 0;
    CCPTMRSbits.C4TSEL = 0;
    CCPTMRSbits.C5TSEL = 0;
}

uint8_t build_argv(char cmdline[], uint8_t *argc, char *argv[])
{
    char h_stack[10]; // history stack
    char *h_memlc[10]; // locations in memory
    uint8_t id_stack = 0, id_memlc = 0;

    memset(h_stack, 0, sizeof (h_stack));
    memset(h_memlc, 0, sizeof (h_memlc));
    do {
        if (isspace(*cmdline)) { // is space symbol or tab
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = 's'; // overwrite
                h_memlc[id_stack] = cmdline;
                break;
            case 's':
                break; // do nothing
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1) // stack break
                    return STACK_BREAK;
                h_stack[id_stack] = 's';
                h_memlc[id_stack] = cmdline;
                break;
            }
        } else if (isalnum(*cmdline)) { // is letter or digit
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = 'l'; // overwrite
                h_memlc[id_stack] = cmdline;
                break;
            case 'l':
                break; // do nothing
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1) // stack break
                    return STACK_BREAK;
                h_stack[id_stack] = 'l';
                h_memlc[id_stack] = cmdline;
                break;
            }
        } else if (ispunct(*cmdline)) { // is punctuation character
            switch (h_stack[id_stack]) {
            default:
                break; // do nothing
            }
        } else { // not space or not alphanumeric, or end of string
            switch (h_stack[id_stack]) {
            case '\0':
                h_stack[id_stack] = *cmdline; // overwrite
                h_memlc[id_stack] = cmdline;
                break;
            default:
                id_stack++;
                if (id_stack >= sizeof (h_stack) - 1) // stack break
                    return STACK_BREAK;
                h_stack[id_stack] = *cmdline;
                h_memlc[id_stack] = cmdline;
                break;
            }
        }

    } while (*cmdline++);

    while (id_stack--) { // place the pointers
        switch (h_stack[id_memlc]) {
        case 'l':
            argv[(*argc)++] = h_memlc[id_memlc++];
            break;
        case 's':
            *h_memlc[id_memlc++] = '\0';
            break;
        default:
            return UNACCEPTABLE_SYMBOL;
        }
    }

    return RETURN_OK;
}

uint8_t mcu_shell_exec(char cmdline[])
{
    uint8_t argc = 0;
    char *argv[SHELL_ARGC_MAX];
    uint8_t function_index = 0;
    uint8_t error_code = 0;

    error_code = build_argv(cmdline, &argc, argv);
    if (error_code == RETURN_OK) {
        error_code = COMMAND_NOT_FOUND;
        while (strlen(shell_table[function_index].function_name)) { // search command
            if (strcmp(argv[0], shell_table[function_index].function_name)) {
                function_index++;
            } else { // command found
                if (argc < shell_table[function_index].min_args || argc > shell_table[function_index].max_args) {
                    error_code = INCORRECT_PARAMETERS;
                    break;
                } else {
                    (*shell_table[function_index].shell_function)(argc, argv); // exec function pointer
                    error_code = RETURN_OK;
                    break;
                }
            }
        }
    }

    switch (error_code) {
    case STACK_BREAK:
        uart_transmit(msg_large_expression, strlen(msg_large_expression));
        break;
    case UNACCEPTABLE_SYMBOL:
        uart_transmit(msg_unacceptable_symbol, strlen(msg_unacceptable_symbol));
        break;
    case INTERNAL_LIMIT:
        uart_transmit(msg_large_expression, strlen(msg_large_expression));
        break;
    case INCORRECT_PARAMETERS:
        uart_transmit(msg_incorrect_parameters, strlen(msg_incorrect_parameters));
        break;
    case COMMAND_NOT_FOUND:
        uart_transmit(msg_command_not_found, strlen(msg_command_not_found));
        break;
    }
}

void mcu_shell_machine_command(void)
{
    const uint8_t bell_character = '\a';
    const uint8_t *shell_prompt = "mcu_shell# ";
    bool echo_rx = true, show_prompt = true;
    static uint8_t command_shell_state = SHELL_SHOW_PROMPT;
    static uint8_t cmd_offset;
    static uint8_t cmd_memory[SHELL_COMAND_SIZE];

    switch (command_shell_state) {
    case SHELL_GET_INPUT:
        while (uart_receive(&cmd_memory[cmd_offset], 1)) { // receive one character
            switch (cmd_memory[cmd_offset]) {
            case '\r':
                if (echo_rx)
                    uart_transmit(&cmd_memory[cmd_offset], 1);
                cmd_memory[cmd_offset] = '\0';
                if (cmd_offset > 0) { // no command probability
                    command_shell_state = SHELL_PROCEED_COMMAND;
                } else {
                    command_shell_state = SHELL_SHOW_PROMPT;
                }
                break;
            case '\b':
                if (cmd_offset > 0) { // cmd_memory has something to delete
                    if (echo_rx)
                        uart_transmit(&cmd_memory[cmd_offset], 1);
                    cmd_offset--;
                } else {
                    if (echo_rx)
                        uart_transmit(&bell_character, 1);
                }
                break;
            default:
                if (cmd_offset >= sizeof (cmd_memory)) { // cmd_memory hits its limit
                    if (echo_rx)
                        uart_transmit(&bell_character, 1);
                } else {
                    if (echo_rx)
                        uart_transmit(&cmd_memory[cmd_offset], 1);
                    cmd_offset++;
                }
            }
        }
        break;
    case SHELL_PROCEED_COMMAND:
        mcu_shell_exec(cmd_memory);
        cmd_offset = 0;
        command_shell_state = SHELL_SHOW_PROMPT;
        break;
    case SHELL_SHOW_PROMPT:
        if (show_prompt)
            uart_transmit(shell_prompt, strlen(shell_prompt));
        command_shell_state = SHELL_GET_INPUT;
        break;
    }
}

void mcu_shell_machine_silent(void)
{
    static uint8_t silent_shell_state = SHELL_PROCEED_COMMAND;
    static uint8_t cmd_offset;
    static uint8_t cmd_memory[SHELL_COMAND_SIZE];

    switch (silent_shell_state) {
    case SHELL_PROCEED_COMMAND:
        while (uart_receive(&cmd_memory[cmd_offset], 1)) { // receive one character
            switch (cmd_memory[cmd_offset]) {
            case 'w':
                //
                offset++;
                break;
            case 's':
                //
                offset--;
                break;
            case 'a':
                //
                gain_error += 0.001;
                break;
            case 'd':
                //
                gain_error -= 0.001;
                break;
            }

            if (cmd_offset >= sizeof (cmd_memory)) { // cmd_memory hits its limit
                cmd_offset = 0;
            } else {
                cmd_offset++;
            }
            silent_shell_state = SHELL_PROCEED_COMMAND;
        }
    }
}

void mcu_setup_interrupts(void)
{
    IPEN = 1, PEIE = 0; // enable interrupt priority (1 - high priority, 0 - low priority)

    HLVDIP = 1, HLVDIF = 0, HLVDIE = 0; // High/Low-Voltage Detect module (HLVD)
    TMR0IP = 1, TMR0IF = 0, TMR0IE = 1; // timers
    TMR1IP = 0, TMR1IF = 0, TMR1IE = 0;
    TMR2IP = 0, TMR2IF = 0, TMR2IE = 0;
    TMR3IP = 0, TMR3IF = 0, TMR3IE = 0;
    TMR4IP = 0, TMR4IF = 0, TMR4IE = 0;
    TX1IP = 0, TX1IF = 0, TX1IE = 0; // UART
    RC1IP = 0, RC1IF = 0, RC1IE = 1;
    TX2IP = 0, TX2IF = 0, TX2IE = 0;
    RC2IP = 0, RC2IF = 0, RC2IE = 1;
    INT1IP = 0, INT1IF = 0, INTEDG1 = 0, INT1IE = 0; // external interrupts
    INT2IP = 0, INT2IF = 0, INTEDG2 = 0, INT2IE = 0;
    INT3IP = 0, INT3IF = 0, INTEDG3 = 0, INT3IE = 0;
    RBIP = 0, RBIE = 0, RBIF = 0; // interrupt-on-change
    ADIP = 0, ADIF = 0, ADIE = 0; // A/D converter
    SSPIP = 0, SSPIF = 0, SSPIE = 0; // MSSP    
    CTMUIP = 0, CTMUIF = 0, CTMUIE = 0; // CTMU

    GIEH = 1, GIEL = 1; // enable high and low priority interrupts
    GIE = 1; // global interrupts enable (every interrupt)
}

void main(void)
{
    TRISA = 0b00000110, LATA = 0b00000000; // TRIS (data direction registers), LAT (output level)
    TRISB = 0b00000000, LATB = 0b00000000; // read from right to left
    TRISC = 0b10010000, LATC = 0b10000000; // TRIS 0 - output, 1 - input, LAT 0 - low, 1 - high

    SLRA = 0b00000000; // 1 - output pins slew at 0.1 the standard rate, 0 - output pins slew at standard rate
    SLRB = 0b00000000;
    SLRC = 0b00000000;

    ANCON0 = 0b00000000, ANCON1 = 0b00000000; // ports analog/digital functionality (0 - digital, 1 - analog)

    //mcu_setup_hlvd();
    mcu_setup_spi();
    //mcu_setup_uart_advanced(9600);
    mcu_setup_uart_compatible(9600);
    //mcu_setup_analog_systems();
    mcu_setup_interrupts();
    //mcu_setup_timer0();

    glcd_initialize();
    glcd_demo();

    for (;;) { // main loop
        mcu_shell_machine_command();
        //mcu_shell_machine_silent();
        // -------------------------------------------------

        // -------------------------------------------------
        //nrf_module_as_receiver();
        //nrf_module_as_transmitter();
        //__delay_us(100);
    }
}
