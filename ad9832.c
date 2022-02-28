#include "mcu_shell.h"
#include "ad9832.h"

void AD9832_spi_send_16(uint16_t word)
{
    uint8_t temp;

    AD9832_device_select();
    SSPBUF = (uint8_t) (word >> 8); // most significant byte
    while (!BF); // wait for the transfer to finish
    temp = SSPBUF;

    SSPBUF = (uint8_t) (word); // least significant byte
    while (!BF); // wait for the transfer to finish
    temp = SSPBUF;
    AD9832_device_unselect();
}

void AD9832_turn_off()
{
    AD9832_spi_send_16(CMD_CONTROL_SLEEP_RESET_CLR | SLEEP_BIT | RESET_BIT | CLR_BIT);
}

void AD9832_turn_on()
{
    AD9832_spi_send_16(CMD_CONTROL_SLEEP_RESET_CLR);
    AD9832_spi_send_16(CMD_CONTROL_SYNC_SELRC | SYNC_BIT | SELRC_BIT);
}

void select_phase0_register()
{
    AD9832_spi_send_16(CMD_SELECT_PHASE_REG | PHASE0_SELECT);
}

void write_phase0_register(float phase)
{
    uint16_t dds_phase = (uint16_t) ((phase / (M_PI * 2.0f))*4096.0f); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    AD9832_spi_send_16(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE0_MSB | msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_PHASE_REG | PHASE0_LSB | lsb);
}

void select_phase1_register()
{
    AD9832_spi_send_16(CMD_SELECT_PHASE_REG | PHASE1_SELECT);
}

void write_phase1_register(float phase)
{
    uint16_t dds_phase = (uint16_t) ((phase / (M_PI * 2.0f))*4096.0f); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    AD9832_spi_send_16(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE1_MSB | msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_PHASE_REG | PHASE1_LSB | lsb);
}

void select_phase2_register()
{
    AD9832_spi_send_16(CMD_SELECT_PHASE_REG | PHASE2_SELECT);
}

void write_phase2_register(float phase)
{
    uint16_t dds_phase = (uint16_t) ((phase / (M_PI * 2.0f))*4096.0f);  // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    AD9832_spi_send_16(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE2_MSB | msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_PHASE_REG | PHASE2_LSB | lsb);
}

void select_phase3_register()
{
    AD9832_spi_send_16(CMD_SELECT_PHASE_REG | PHASE3_SELECT);
}

void write_phase3_register(float phase)
{
    uint16_t dds_phase = (uint16_t) ((phase / (M_PI * 2.0f))*4096.0f); // phase is 2^12

    uint8_t msb = (uint8_t) (dds_phase >> 8);
    uint8_t lsb = (uint8_t) (dds_phase);

    AD9832_spi_send_16(CMD_WRITE_8_PHASE_BITS_DEFER_REG | PHASE3_MSB | msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_PHASE_REG | PHASE3_LSB | lsb);
}

void select_freq0_register()
{
    AD9832_spi_send_16(CMD_SELECT_FREQ_REG | FREQ0_SELECT);
}

void write_freq0_register(float frequency)
{
    uint32_t dds_freq = (uint32_t) ((frequency / 25.0f)*0xFFFFFFFF); // frequency in MHz

    uint8_t h_msb = (uint8_t) (dds_freq >> 24);
    uint8_t l_msb = (uint8_t) (dds_freq >> 16);
    uint8_t h_lsb = (uint8_t) (dds_freq >> 8);
    uint8_t l_lsb = (uint8_t) (dds_freq);

    AD9832_spi_send_16(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ0_H_MSB | h_msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_FREQ_REG | FREQ0_L_MSB | l_msb);
    AD9832_spi_send_16(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ0_H_LSB | h_lsb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_FREQ_REG | FREQ0_L_LSB | l_lsb);
}

void select_freq1_register()
{
    AD9832_spi_send_16(CMD_SELECT_FREQ_REG | FREQ1_SELECT);
}

void write_freq1_register(float frequency)
{
    uint32_t dds_freq = (uint32_t) ((frequency / 25.0f)*0xFFFFFFFF); // frequency in MHz

    uint8_t h_msb = (uint8_t) (dds_freq >> 24);
    uint8_t l_msb = (uint8_t) (dds_freq >> 16);
    uint8_t h_lsb = (uint8_t) (dds_freq >> 8);
    uint8_t l_lsb = (uint8_t) (dds_freq);

    AD9832_spi_send_16(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ1_H_MSB | h_msb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_FREQ_REG | FREQ1_L_MSB | l_msb);
    AD9832_spi_send_16(CMD_WRITE_8_FREQ_BITS_DEFER_REG | FREQ1_H_LSB | h_lsb);
    AD9832_spi_send_16(CMD_WRITE_16_BITS_FREQ_REG | FREQ1_L_LSB | l_lsb);
}
