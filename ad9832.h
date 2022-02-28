#ifndef AD9832_H
#define AD9832_H

/* register selection pins */
#define FSYNC   LATAbits.LATA2
#define FSELECT LATAbits.LATA3
#define PSEL0   LATAbits.LATA5
#define PSEL1   LATAbits.LATA6

/* numerically controlled oscillator (NCO) */
#define AD9832_device_select()     FSYNC = 0
#define AD9832_device_unselect()   FSYNC = 1

/* function prototypes */
void AD9832_turn_off();
void AD9832_turn_on();
void select_phase0_register();
void write_phase0_register(float phase);
void select_phase1_register();
void write_phase1_register(float phase);
void select_phase2_register();
void write_phase2_register(float phase);
void select_phase3_register();
void write_phase3_register(float phase);
void select_freq0_register();
void write_freq0_register(float frequency);
void select_freq1_register();
void write_freq1_register(float frequency);

/* control commands */
#define CMD_CONTROL_SYNC_SELRC      0x8000
#define CMD_CONTROL_SLEEP_RESET_CLR 0xC000

/* control commands bits */
#define SYNC_BIT  0x2000
#define SELRC_BIT 0x1000 // when SELSRC = 0, the pins are used, and when SELSRC = 1 the bits are used
#define SLEEP_BIT 0x2000
#define RESET_BIT 0x1000
#define CLR_BIT   0x0800

/* registers related commands */
#define CMD_WRITE_16_BITS_PHASE_REG      0x0000
#define CMD_WRITE_8_PHASE_BITS_DEFER_REG 0x1000
#define CMD_WRITE_16_BITS_FREQ_REG       0x2000
#define CMD_WRITE_8_FREQ_BITS_DEFER_REG  0x3000
#define CMD_SELECT_PHASE_REG             0x4000
#define CMD_SELECT_FREQ_REG              0x5000
#define CMD_SELECT_PHASE_FREQ_REG        0x6000

/* register selection bit masks */
#define FREQ0_SELECT  0x0000
#define FREQ1_SELECT  0x0800
#define PHASE0_SELECT 0x0000
#define PHASE1_SELECT 0x0200
#define PHASE2_SELECT 0x0400
#define PHASE3_SELECT 0x0600

/* registers */
#define FREQ0_L_LSB      0x0000
#define FREQ0_H_LSB      0x0100
#define FREQ0_L_MSB      0x0200
#define FREQ0_H_MSB      0x0300
#define FREQ1_L_LSB      0x0400
#define FREQ1_H_LSB      0x0500
#define FREQ1_L_MSB      0x0600
#define FREQ1_H_MSB      0x0700
#define PHASE0_LSB       0x0800
#define PHASE0_MSB       0x0900
#define PHASE1_LSB       0x0A00
#define PHASE1_MSB       0x0B00
#define PHASE2_LSB       0x0C00
#define PHASE2_MSB       0x0D00
#define PHASE3_LSB       0x0E00
#define PHASE3_MSB       0x0F00

#endif
