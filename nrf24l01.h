#ifndef NRF24L01_H
#define NRF24L01_H

#define SET_BITMASK(a,b) ((a) |= (b))
#define CLEAR_BITMASK(a,b) ((a) &= (~(b)))
#define SET_BIT(a,n) ((a) |= (1ULL<<(n)))
#define CLEAR_BIT(a,n) ((a) &= ~(1ULL<<(n)))
#define BIT_IS_SET(a,n) (!!((a) & (1ULL<<(n))))

#define NRF_CSN        LATAbits.LATA2 // chip select/not (SPI select device)
#define NRF_CE         LATAbits.LATA3 // chip enable (trigger transmit, watch the radio waves)
#define NRF_IRQ        LATBbits.LATB1 // interrupt pin (IRQ)
#define NRF_SYNC_LED   LATBbits.LATB0
#define NRF_SYNC_LED   LATBbits.LATB2

#define nrf_select()   NRF_CSN = 0
#define nrf_ce_high()  NRF_CE = 1
#define nrf_ce_low()   NRF_CE = 0
#define nrf_unselect() NRF_CSN = 1

/* verify bits in CONFIG register */
#define IS_SET_POWER_UP(a)  BIT_IS_SET(a,1)
#define SET_POWER_UP(a)     SET_BIT(a,1)
#define CLEAR_POWER_UP(a)   CLEAR_BIT(a,1)
#define IS_SET_PRIM_RX(a)   BIT_IS_SET(a,0)
#define SET_PRIM_RX(a)      SET_BIT(a,0)
#define CLEAR_PRIM_RX(a)    CLEAR_BIT(a,0)

/* verify bits in STATUS register */
#define RX_DR_IS_SET(a)      BIT_IS_SET(a,6)
#define RX_DR_IS_NOT_SET(a)  (!(BIT_IS_SET(a,6)))
#define CLEAR_RX_DR(a)       SET_BIT(a,6)
#define TX_DS_IS_SET(a)      BIT_IS_SET(a,5)
#define TX_DS_IS_NOT_SET(a)  (!(BIT_IS_SET(a,5)))
#define CLEAR_TX_DS(a)       SET_BIT(a,5)
#define MAX_RT_IS_SET(a)     BIT_IS_SET(a,4)
#define MAX_RT_IS_NOT_SET(a) (!(BIT_IS_SET(a,4)))
#define CLEAR_MAX_RT(a)      SET_BIT(a,4)

/* verify bits in FIFO_STATUS register */
#define TX_FIFO_FULL(a)      BIT_IS_SET(a,5) // TX FIFO full
#define TX_FIFO_NOT_FULL(a)  (!(BIT_IS_SET(a,5))) // TX FIFO locations are available
#define TX_FIFO_EMPTY(a)     BIT_IS_SET(a,4) // TX FIFO empty
#define TX_FIFO_NOT_EMPTY(a) (!(BIT_IS_SET(a,4))) // data in TX FIFO
#define RX_FIFO_FULL(a)      BIT_IS_SET(a,1) // RX FIFO full
#define RX_FIFO_NOT_FULL(a)  (!(BIT_IS_SET(a,1))) // RX FIFO locations are available
#define RX_FIFO_EMPTY(a)     BIT_IS_SET(a,0) // RX FIFO empty
#define RX_FIFO_NOT_EMPTY(a) (!(BIT_IS_SET(a,0))) // there is data in RX FIFO

enum {
    NRF_PAYLOAD_SYNC,
    NRF_PAYLOAD_STRING,
    NRF_PAYLOAD_STREAM
};

enum {
    NRF_STATE_POWER_UP,
    NRF_STATE_SYNC,
    NRF_STATE_TRANSMITTER,
    NRF_STATE_RECEIVER,
    NRF_STATE_POWER_DOWN
};

#define NRF_PAYLOAD_WIDTH  8
#define NRF_PAYLOADS_COUNT 6

typedef struct {
    uint8_t mem[NRF_PAYLOADS_COUNT][NRF_PAYLOAD_WIDTH];
    uint8_t pri, sec, cnt;
} NRF_BUFFER;

/* function prototypes */
uint8_t nrf_spi_send(uint8_t spi_byte);
uint8_t nrf_read_register(uint8_t reg_addr, uint8_t *reg_val);
uint8_t nrf_write_register(uint8_t reg_addr, uint8_t reg_val);
void nrf_flush_tx_fifo(void);
void nrf_flush_rx_fifo(void);
void nrf_power_up(void);
void nrf_power_down(void);
void nrf_mask_interrupts(void);
void nrf_unmask_interrupts(void);
void nrf_clear_all_interrupts(void);
void nrf_set_as_transmitter(void);
void nrf_set_as_receiver(void);
void nrf_write_payload(uint8_t *payload, uint8_t width);
void nrf_read_payload(uint8_t *payload, uint8_t width);
void nrf_interrupt(void);
void nrf_sync_switch_over(void);
void print_sync(uint8_t *payload);
void nrf_transmit_sync(void);
void nrf_receive_sync(void);
void nrf_module_as_receiver(void);
void nrf_module_as_transmitter(void);
void nrf_print_registers(void);

/* NRF24L01 SPI commands */
#define NRF_R_REGISTER             0x00
#define NRF_W_REGISTER             0x20
#define NRF_R_RX_PAYLOAD           0x61
#define NRF_W_TX_PAYLOAD           0xA0
#define NRF_FLUSH_TX               0xE1
#define NRF_FLUSH_RX               0xE2
#define NRF_REUSE_TX_PL            0xE3
#define NRF_NOP                    0xFF

/* NRF24L01 registers */
#define NRF_CONFIG                 0x00
#define NRF_EN_AA                  0x01 // auto acknowledgment
#define NRF_EN_RXADDR              0x02 // enable or disable RX pipes
#define NRF_SETUP_AW               0x03
#define NRF_SETUP_RETR             0x04
#define NRF_RF_CH                  0x05
#define NRF_RF_SETUP               0x06
#define NRF_STATUS                 0x07
#define NRF_OBSERVE_TX             0x08
#define NRF_CD                     0x09
#define NRF_RX_ADDR_P0             0x0A // addresses for data pipes
#define NRF_RX_ADDR_P1             0x0B
#define NRF_RX_ADDR_P2             0x0C
#define NRF_RX_ADDR_P3             0x0D
#define NRF_RX_ADDR_P4             0x0E
#define NRF_RX_ADDR_P5             0x0F
#define NRF_TX_ADDR                0x10 // transmit addresss 
#define NRF_RX_PW_P0               0x11 // payload width registers for pipes
#define NRF_RX_PW_P1               0x12 //
#define NRF_RX_PW_P2               0x13 //
#define NRF_RX_PW_P3               0x14 //
#define NRF_RX_PW_P4               0x15 //
#define NRF_RX_PW_P5               0x16 //
#define NRF_FIFO_STATUS            0x17

/* default register values */
#define NRF_CONFIG_DEFAULT         0x08
#define NRF_EN_AA_DEFAULT          0x3F
#define NRF_EN_RXADDR_DEFAULT      0x03
#define NRF_SETUP_AW_DEFAULT       0x03
#define NRF_SETUP_RETR_DEFAULT     0x03
#define NRF_RF_CH_DEFAULT          0x02
#define NRF_RF_SETUP_DEFAULT       0x0F
#define NRF_STATUS_DEFAULT         0x0E
#define NRF_OBSERVE_TX_DEFAULT     0x00
#define NRF_CD_DEFAULT             0x00
#define NRF_RX_ADDR_P0_DEFAULT     0xE7
#define NRF_RX_ADDR_P1_DEFAULT     0xC2
#define NRF_RX_ADDR_P2_DEFAULT     0xC3
#define NRF_RX_ADDR_P3_DEFAULT     0xC4
#define NRF_RX_ADDR_P4_DEFAULT     0xC5
#define NRF_RX_ADDR_P5_DEFAULT     0xC6
#define NRF_TX_ADDR_DEFAULT        0xE7
#define NRF_RX_PW_P0_DEFAULT       0x00
#define NRF_RX_PW_P1_DEFAULT       0x00
#define NRF_RX_PW_P2_DEFAULT       0x00
#define NRF_RX_PW_P3_DEFAULT       0x00
#define NRF_RX_PW_P4_DEFAULT       0x00
#define NRF_RX_PW_P5_DEFAULT       0x00
#define NRF_FIFO_STATUS_DEFAULT    0x11

/* CONFIG register */
#define NRF_CONFIG_RESERVED        0x80
#define	NRF_CONFIG_MASK_RX_DR      0x40 // mask interrupts
#define	NRF_CONFIG_MASK_TX_DS      0x20 //
#define	NRF_CONFIG_MASK_MAX_RT     0x10 //
#define	NRF_CONFIG_EN_CRC          0x08
#define	NRF_CONFIG_CRCO            0x04 // CRC encoding scheme
#define	NRF_CONFIG_PWR_UP          0x02
#define	NRF_CONFIG_PRIM_RX         0x01

/* EN_AA register */
#define NRF_EN_AA_RESERVED         0xC0
#define NRF_EN_AA_ENAA_ALL         0x3F // enable auto-acknowledgment on all pipes
#define NRF_EN_AA_ENAA_P5          0x20
#define NRF_EN_AA_ENAA_P4          0x10
#define NRF_EN_AA_ENAA_P3          0x08
#define NRF_EN_AA_ENAA_P2          0x04
#define NRF_EN_AA_ENAA_P1          0x02
#define NRF_EN_AA_ENAA_P0          0x01
#define NRF_EN_AA_ENAA_NONE        0x00 // disable auto-acknowledgment on all pipes

/* EN_RXADDR register */
#define NRF_EN_RXADDR_RESERVED     0xC0
#define NRF_EN_RXADDR_ERX_ALL      0x3F // enable all RX pipes
#define NRF_EN_RXADDR_ERX_P5       0x20
#define NRF_EN_RXADDR_ERX_P4       0x10
#define NRF_EN_RXADDR_ERX_P3       0x08
#define NRF_EN_RXADDR_ERX_P2       0x04
#define NRF_EN_RXADDR_ERX_P1       0x02
#define NRF_EN_RXADDR_ERX_P0       0x01
#define NRF_EN_RXADDR_ERX_NONE     0x00 // disable all RX pipes

/* SETUP_AW register */
#define NRF_SETUP_AW_RESERVED      0xFC
#define NRF_SETUP_AW_5BYTES        0x03
#define NRF_SETUP_AW_4BYTES        0x02
#define NRF_SETUP_AW_3BYTES        0x01
#define NRF_SETUP_AW_ILLEGAL       0x00

/* SETUP_RETR register */
#define NRF_SETUP_RETR_ARD_4000    0xF0
#define NRF_SETUP_RETR_ARD_3750    0xE0
#define NRF_SETUP_RETR_ARD_3500    0xD0
#define NRF_SETUP_RETR_ARD_3250    0xC0
#define NRF_SETUP_RETR_ARD_3000    0xB0
#define NRF_SETUP_RETR_ARD_2750    0xA0
#define NRF_SETUP_RETR_ARD_2500    0x90
#define NRF_SETUP_RETR_ARD_2250    0x80
#define NRF_SETUP_RETR_ARD_2000    0x70
#define NRF_SETUP_RETR_ARD_1750    0x60
#define NRF_SETUP_RETR_ARD_1500    0x50
#define NRF_SETUP_RETR_ARD_1250    0x40
#define NRF_SETUP_RETR_ARD_1000    0x30
#define NRF_SETUP_RETR_ARD_750     0x20
#define NRF_SETUP_RETR_ARD_500     0x10
#define NRF_SETUP_RETR_ARD_250     0x00
#define NRF_SETUP_RETR_ARC_15      0x0F
#define NRF_SETUP_RETR_ARC_14      0x0E
#define NRF_SETUP_RETR_ARC_13      0x0D
#define NRF_SETUP_RETR_ARC_12      0x0C
#define NRF_SETUP_RETR_ARC_11      0x0B
#define NRF_SETUP_RETR_ARC_10      0x0A
#define NRF_SETUP_RETR_ARC_9       0x09
#define NRF_SETUP_RETR_ARC_8       0x08
#define NRF_SETUP_RETR_ARC_7       0x07
#define NRF_SETUP_RETR_ARC_6       0x06
#define NRF_SETUP_RETR_ARC_5       0x05
#define NRF_SETUP_RETR_ARC_4       0x04
#define NRF_SETUP_RETR_ARC_3       0x03
#define NRF_SETUP_RETR_ARC_2       0x02
#define NRF_SETUP_RETR_ARC_1       0x01
#define NRF_SETUP_RETR_ARC_0       0x00 // retransmit disabled

/* RF_CH register */
#define NRF24l01_RF_CH_RESERVED	   0x80

/* RF_SETUP register */
#define NRF_RF_SETUP_RESERVED      0xE0
#define NRF_RF_SETUP_PLL_LOCK      0x10
#define NRF_RF_SETUP_RF_DR         0x08
#define NRF_RF_SETUP_RF_PWR        0x06
#define NRF_RF_SETUP_RF_PWR_0      0x06
#define NRF_RF_SETUP_RF_PWR_6      0x04
#define NRF_RF_SETUP_RF_PWR_12     0x02
#define NRF_RF_SETUP_RF_PWR_18     0x00
#define NRF_RF_SETUP_LNA_HCURR     0x01

/* STATUS register */
#define NRF_STATUS_RESERVED        0x80
#define NRF_STATUS_RX_DR           0x40
#define NRF_STATUS_TX_DS           0x20
#define NRF_STATUS_MAX_RT          0x10
#define NRF_STATUS_RX_P_NO_5       0x0A
#define NRF_STATUS_RX_P_NO_4       0x08
#define NRF_STATUS_RX_P_NO_3       0x06
#define NRF_STATUS_RX_P_NO_2       0x04
#define NRF_STATUS_RX_P_NO_1       0x02
#define NRF_STATUS_RX_P_NO_0       0x00
#define NRF_STATUS_RX_FIFO_EMPTY   0x0E
#define NRF_STATUS_UNUSED          0x0C
#define NRF_STATUS_TX_FIFO_FULL    0x01

/* OBSERVE_TX register */
#define NRF_OBSERVE_TX_PLOS_CNT    0xF0
#define NRF_OBSERVE_TX_ARC_CNT     0x0F

/* CD register (carrier detect) */
#define NRF_CD_RESERVED            0xFE
#define NRF_CARRIER_DETECT         0x01

/* reserved bytes in payload width registers for receive pipes */
#define NRF_RX_PW_P0_RESERVED      0xC0
#define NRF_RX_PW_P1_RESERVED      0xC0
#define NRF_RX_PW_P2_RESERVED      0xC0
#define NRF_RX_PW_P3_RESERVED      0xC0 
#define NRF_RX_PW_P4_RESERVED      0xC0
#define NRF_RX_PW_P5_RESERVED      0xC0

/* FIFO_STATUS register */
#define NRF_FIFO_STATUS_RESERVED   0x8C
#define NRF_FIFO_STATUS_TX_REUSE   0x40
#define NRF_FIFO_STATUS_TX_FULL    0x20
#define NRF_FIFO_STATUS_TX_EMPTY   0x10
#define NRF_FIFO_STATUS_RX_FULL    0x02
#define NRF_FIFO_STATUS_RX_EMPTY   0x01

#endif
