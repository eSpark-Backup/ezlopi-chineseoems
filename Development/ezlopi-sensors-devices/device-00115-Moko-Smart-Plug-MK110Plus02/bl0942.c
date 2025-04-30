#include "bl0942.h"
#include "ezlopi_util_trace.h"
#include "ezlopi_core_errors.h"

spi_device_handle_t bl0942_handle = NULL;

ezlopi_error_t bl0942_init()
{
    static s_ezlopi_spi_master_t bl0942_spi_config = {
        .enable = true,
        .channel = EZLOPI_SPI_1,
        .miso = BL0942_SPI_MISO_PIN,
        .mosi = BL0942_SPI_MOSI_PIN,
        .sck = BL0942_SPI_SCLK_PIN,
        .cs = BL0942_SPI_CS_PIN,
        .transfer_sz = BL0942_SPI_TRANSFER_SZ,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .command_bits = BL0942_SPI_CMD_BITS,
        .addr_bits = BL0942_SPI_ADDR_BITS,
        .mode = EZLOPI_SPI_CPOL_LOW_CPHA_HIGH,
        .clock_speed_mhz = BL0942_SPI_ADDR_BITS,
        .queue_size = BL0942_SPI_QUEUE_SZ,
    };
    
    return EZPI_hal_spi_master_init(&bl0942_spi_config);
}

uint32_t bl0942_read_register(uint8_t reg_addr)
{
    if (!bl0942_handle) {
        TRACE_E("BL0942 handle not initialized!");
        return 0;
    }

    spi_transaction_t trans = {
        .cmd = 0,
        .addr = 0,
        .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
        .length = 8 * 4,
        .tx_data = { reg_addr | 0x80, 0x00, 0x00, 0x00 },
    };

    esp_err_t err = spi_device_transmit(bl0942_handle, &trans);
    if (err != ESP_OK) {
        TRACE_E("SPI transmit failed: %d", err);
        return 0;
    }

    // Combine 3 received bytes (starting from index 1)
    uint32_t value = ((uint32_t)trans.rx_data[1] << 16) |
                     ((uint32_t)trans.rx_data[2] << 8)  |
                     ((uint32_t)trans.rx_data[3]);

    return value;
}

uint32_t bl0942_read_voltage()
{
    return bl0942_read_register(VOLTAGE_ADDR);
}

uint32_t bl0942_read_current()
{
    return bl0942_read_register(CURRENT_ADDR);
}