#include "bht003.h"
#include "esp_log.h" 

#define START           0x00
#define HEADER          0x55

#define POWER           0x01
#define TEMP_SP         0x02
#define AMBIENT_TEMP    0x03
#define MODE_SELECTION  0x04
#define ENERGY_SAVING   0x05
#define CHILD_LOCK      0x06

static const char *TAG = "BHT";
void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle);
void normal_packet();

typedef struct command{
    uint16_t command;
    void (*callback)();

}command_t;

command_t start_cmd[]= {
    {.command = START,  .callback = start_packet},
    {.command = HEADER, .callback = normal_packet},
};

// command_t data_cmd[]= {
//     {.command = POWER,          .callback = power},
//     {.command = TEMP_SP,        .callback = normal_poacket},
//     {.command = AMBIENT_TEMP,   .callback = normal_poacket},
//     {.command = MODE_SELECTION, .callback = normal_poacket},
//     {.command = ENERGY_SAVING,  .callback = normal_poacket},
//     {.command = CHILD_LOCK,     .callback = normal_poacket},
// }

void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle)
{
    uart_write_bytes(uart_object_handle->ezlopi_uart.channel, (const char *)START, sizeof(START));
}

void normal_packet() {
    // Normal packet handling code
}

// void power() {
//     // Power handling code
// }

void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length)
{
    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x00;

    switch (cmd)
    {
        case CMD_HEARTBEAT:
            packet[3] = 0x00; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0x00; packet[7] = 0xFF;
            *length = 7;
            break;
        case CMD_TURN_ON:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x0E;
            *length = 12;
            break;
        case CMD_TURN_OFF:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x0D;
            *length = 12;
            break;
        case CMD_PROGRAM_MODE:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x13;
            *length = 12;
            break;
        case CMD_MANUAL_MODE:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x14;
            *length = 12;
            break;
        case CMD_ENERGY_SAVING_ON:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x12;
            *length = 12;
            break;
        case CMD_ENERGY_SAVING_OFF:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x11;
            *length = 12;
            break;
        case CMD_LOCK_ENABLE:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x13;
            *length = 12;
            break;
        case CMD_LOCK_DISABLE:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
            packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x12;
            *length = 12;
            break;
        case CMD_TEMP:
            packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = 0x02; packet[7] = 0x02;
            packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
            packet[12] = 0x00; packet[13] = 0x0C; packet[14] = 0x21;
            *length = 15;
            break;
        case CMD_WIFI:
            packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x05; packet[7] = 0x08;
            *length = 8;
            break;
        default:
            ESP_LOGE(TAG, "Unknown command type");
            break;
    }
}

void bht003_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    for (size_t i = 0; sizeof(start_cmd) / sizeof(start_cmd[0]); i++)
    {
        if (start_cmd[i].command == buffer)
        {
            start_cmd[i].callback(uart_object_handle);
            return;
        }
    }
}

bool bht003_init(s_ezlopi_uart_t *bht003_uart_config)
{
    if(bht003_uart_config->enable)
    {
        return ezlopi_uart_init(bht003_uart_config->baudrate,bht003_uart_config->tx,bht003_uart_config->rx,bht003_callback,NULL);
    }

    return false;
}

void send_command(CommandType cmd) {
    uint8_t packet[15];
    uint8_t length;
    
    create_packet(cmd, packet, &length);
    // uart_write_bytes(UART_NUM, (const char *)packet, length);
    
    ESP_LOGI(TAG, "Command sent:");
    for (int i = 0; i < length; i++) {
        ESP_LOGI(TAG, "%02X ", packet[i]);
    }
}