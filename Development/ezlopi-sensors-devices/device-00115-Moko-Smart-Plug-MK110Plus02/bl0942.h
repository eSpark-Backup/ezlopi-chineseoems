#include "ezlopi_hal_spi_master.h"

#define BL0942_SPI_CS_PIN       -1
#define BL0942_SPI_SCLK_PIN     17
#define BL0942_SPI_MOSI_PIN     18
#define BL0942_SPI_MISO_PIN     19
#define BL0942_SPI_TRANSFER_SZ  64
#define BL0942_SPI_CMD_BITS     00
#define BL0942_SPI_ADDR_BITS    00
#define BL0942_SPI_CLK_MHZ      01
#define BL0942_SPI_ADDR_BITS    00
#define BL0942_SPI_QUEUE_SZ     01

#define CURRENT_ADDR            0x03
#define VOLTAGE_ADDR            0x04

ezlopi_error_t bl0942_init();
uint32_t bl0942_read_voltage();
uint32_t bl0942_read_current();
uint32_t bl0942_read_register(uint8_t reg_addr);