#include "scent_diffuser.h"
#include "ezlopi_core_ota.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_core_factory_info.h"
#include "ezlopi_core_device_value_updated.h"

#define START           0x00    //{0x55, 0xAA, 0x06, 0x00, 0x22, 0x02, 0x01, 0x00, 0x00, 0x00, 0xD6, 0x5A}
#define HEADER_1        0x55
#define HEADER_2        0xAA
#define CLOSE           0x5A

#define ON              0x01
#define OFF             0x00
#define POWER           0x12
#define FAN             0x10



// #define POWER           0x01
#define TEMP_SP         0x02
#define AMBIENT_TEMP    0x03
#define MODE_SELECTION  0x04
#define ENERGY_SAVING   0x05
#define CHILD_LOCK      0x06

#define RAW             0x00
#define BOOLEAN         0x01
#define VALUE           0x02
#define STRING          0x03
#define ENUM            0x04
#define BITMAP          0x05


s_ezlopi_uart_object_handle_t scent_diffuser_uart_object_handle;

// typedef struct command {
//     uint16_t command;
//     void (*callback)();
// }command_t;

// command_t start_cmd[] = {
//     {.command = HEADER_1, .callback = normal_packet},
// };

void scent_diffuser_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    
}

bool scent_diffuser_init(s_ezlopi_uart_t *scent_diffuser_uart_config)
{
    if (scent_diffuser_uart_config->enable)
    {
        TRACE_I("  Baud Rate: %d",scent_diffuser_uart_config->baudrate);
        TRACE_I("  Tx: %d",scent_diffuser_uart_config->tx);
        TRACE_I("  Rx: %d",scent_diffuser_uart_config->rx);
        // TRACE_I("  channel: %d",scent_diffuser_uart_object_handle->ezlopi_uart.channel);
        scent_diffuser_uart_object_handle = EZPI_hal_uart_init(scent_diffuser_uart_config->baudrate, scent_diffuser_uart_config->tx, scent_diffuser_uart_config->rx, scent_diffuser_callback, NULL);
        
        
        return scent_diffuser_uart_object_handle;
    }

    return false;
}

void start_packet()
{
    char start_com[] = {0x55, 0xAA, 0x06, 0x00, 0x22, 0x02, 0x01, 0x00, 0x00, 0x00, 0xD6, 0x5A};
    uart_write_bytes(scent_diffuser_uart_object_handle->ezlopi_uart.channel, (const char *)start_com, sizeof(start_com));
}

void heart_beat_packet() {
    char heart_beat[] = {
        HEADER_1, HEADER_2, 0x1C, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x41, 0x30, 0x37, 0x36, 0x34, 0x45, 0x41, 0x43, 0x42, 0x32, 0x33, 
        0x34, 0x55, 0x08, 0x00, 0x33, 0x33, 0x32, 0x31, 0x35, 0x35, 0x32, 0x30, 0x1C, CLOSE
        };
    uart_write_bytes(scent_diffuser_uart_object_handle->ezlopi_uart.channel, (const char *)heart_beat, sizeof(heart_beat));
}


void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length)
{
    packet[0] = HEADER_1;
    packet[1] = HEADER_2;

    switch (cmd)
    {
    case CMD_CONNECTION_START:
        *length = 12;
        packet[2] = 0x06; packet[3] = 0x00; packet[4] = 0x22; packet[5] = 0x02; packet[6] = 0x01; packet[7] = 0x00;
        packet[8] = 0x00; packet[9] = 0x00; packet[10] = 0xD6; packet[11] = CLOSE;
        
        break;
    case CMD_HEARTBEAT:
        *length = 34;
        packet = ezlopi_malloc(__FUNCTION__,*length * sizeof(uint8_t));
        packet[0] = HEADER_1;
        packet[1] = HEADER_2;
        packet[2] = 0x1C; packet[3] = 0x00; packet[4] = 0x21; packet[5] = 0x00; packet[6] = 0x00; packet[7] = 0x00; 
        packet[8] = 0x00; packet[9] = 0x41; packet[10] = 0x30; packet[11] = 0x37; packet[12] = 0x36; packet[13] = 0x34; 
        packet[14] = 0x45; packet[15] = 0x41; packet[16] = 0x43; packet[17] = 0x42; packet[18] = 0x32; packet[19] = 0x33; 
        packet[20] = 0x34; packet[21] = 0x55; packet[22] = 0x08; packet[23] = 0x00; packet[24] = 0x33; packet[25] = 0x33; 
        packet[26] = 0x32; packet[27] = 0x31; packet[28] = 0x35; packet[29] = 0x35; packet[30] = 0x32; packet[31] = 0x30; 
        packet[32] = 0x1C; packet[33] = CLOSE;
        
        break;
    case CMD_TURN_ON:
        *length = 11;
        packet[2] = 0x05; packet[3] = 0x00; packet[4] = 0x02; packet[5] = 0x07; packet[6] = POWER; packet[7] = ON;
        packet[8] = 0x00; packet[9] = 0xE0; packet[10] = CLOSE;
        
        break;
    case CMD_TURN_OFF:
        *length = 11;
        packet[2] = 0x05; packet[3] = 0x00; packet[4] = 0x02; packet[5] = 0x07; packet[6] = POWER; packet[7] = OFF;
        packet[8] = 0x00; packet[9] = 0xE1; packet[10] = CLOSE;
        TRACE_I("------------------------------Packet created");

        break;
    case CMD_FAN_ON:
        *length = 11;
        packet[2] = 0x05; packet[3] = 0x00; packet[4] = 0x02; packet[5] = 0x07; packet[6] = FAN; packet[7] = ON;
        packet[8] = 0x00; packet[9] = 0xE2; packet[10] = CLOSE;

        break;
    case CMD_FAN_OFF:
        *length = 11;
        packet[2] = 0x05; packet[3] = 0x00; packet[4] = 0x02; packet[5] = 0x07; packet[6] = FAN; packet[7] = OFF;
        packet[8] = 0x00; packet[9] = 0xE3; packet[10] = CLOSE;

        break;
    default:
        TRACE_E("Unknown command type");
        break;
    }
}

void send_command(CommandType cmd) {
    uint8_t packet[12];
    uint8_t length = 0;

    if (cmd == CMD_HEARTBEAT)
    {
        char heart_beat[] = {
        HEADER_1, HEADER_2, 0x1C, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x41, 0x30, 0x37, 0x36, 0x34, 0x45, 0x41, 0x43, 0x42, 0x32, 0x33, 
        0x34, 0x55, 0x08, 0x00, 0x33, 0x33, 0x32, 0x31, 0x35, 0x35, 0x32, 0x30, 0x1C, CLOSE
        };
        uart_write_bytes(scent_diffuser_uart_object_handle->ezlopi_uart.channel, (const char *)heart_beat, sizeof(heart_beat));
    }
    else
    {
        memset(packet, 0, sizeof(packet));
        create_packet(cmd, packet, &length);
        uart_write_bytes(scent_diffuser_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);
    }
    
}
