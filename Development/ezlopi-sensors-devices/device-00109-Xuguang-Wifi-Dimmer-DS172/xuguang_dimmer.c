#include "xuguang_dimmer.h"

#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint32_t)(src)[0] << 24) | \
              ((uint32_t)(src)[1] << 16))


#define HEADER_H        0x55
#define HEADER_L        0xAA

#define POWER           0x01
#define INTENSITY       0x02

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void ambient_temp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

typedef struct values
{
    uint8_t command_sent;
    bool power_value;
    uint16_t amb_temp_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t xuguang_uart_object_handle;

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if(buffer)
    {
        Values.power_value = buffer[10];
    }
}

void ambient_temp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        COPY_AND_SWAP32(Values.amb_temp_value, &buffer[12]);
    }
}

bool xuguang_power_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.power_value;
}

float xuguang_amb_temp_value()
{
    // TRACE_I("----------AMB TEMP----------: %.2f",(Values.amb_temp_value/2.00));
    return (Values.amb_temp_value/2.00);
}

// void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length, float data)
// {
//     packet[0] = 0x55;
//     packet[1] = 0xAA;
//     packet[2] = 0x00;

//     switch (cmd)
//     {
//     case CMD_HEARTBEAT:
//         Values.command_sent = 0;
//         packet[3] = 0x00; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0x00; packet[7] = 0xFF;
//         *length = 7;
//         break;
//     case CMD_TURN_ON:
//     Values.command_sent = 1;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x0E;
//         *length = 12;
//         break;
//     case CMD_TURN_OFF:
//         Values.command_sent = 1;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x0D;
//         *length = 12;
//         break;
//     case CMD_PROGRAM_MODE:
//         Values.command_sent = 2;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x13;
//         *length = 12;
//         break;
//     case CMD_MANUAL_MODE:
//         Values.command_sent = 2;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x14;
//         *length = 12;
//         break;
//     case CMD_ENERGY_SAVING_ON:
//         Values.command_sent = 3;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x12;
//         *length = 12;
//         break;
//     case CMD_ENERGY_SAVING_OFF:
//         Values.command_sent = 3;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x11;
//         *length = 12;
//         break;
//     case CMD_LOCK_ENABLE:
//         Values.command_sent = 4;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x13;
//         *length = 12;
//         break;
//     case CMD_LOCK_DISABLE:
//         Values.command_sent = 4;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
//         packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x12;
//         *length = 12;
//         break;
//     case CMD_TEMP:
//         Values.command_sent = 5;
//         *length = 15;
//         packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = 0x02; packet[7] = 0x02;
//         packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
//         packet[12] = 0x00;
//         packet[13] = (uint8_t) data*2;
//         // packet[14] = 0x1F;
//         for (int i = 1; i < *length; i++) 
//         {
//             packet[14] += packet[i-1]; 
//         }
//         packet[14] +=1;
//         break;
//     // case CMD_WIFI:
//     //     packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x05; packet[7] = 0x08;
//     //     *length = 8;
//     //     break;
//     default:
//         TRACE_E("Unknown command type");
//         break;
//     }
// }

void xuguang_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if ((buffer[0]) == HEADER_H && (buffer[1]) == HEADER_L)
    {
        if((buffer[5]) == POWER)
        {
            power(buffer, output_len, uart_object_handle);
        }
        else if((buffer[5]) == INTENSITY)
        {
            ambient_temp(buffer, output_len, uart_object_handle);
        }
    }
    else{
    }
}

bool xuguang_init(s_ezlopi_uart_t *xuguang_uart_config)
{
    if (xuguang_uart_config->enable)
    {
        xuguang_uart_object_handle = ezlopi_uart_init(xuguang_uart_config->baudrate, xuguang_uart_config->tx, xuguang_uart_config->rx, xuguang_callback, NULL);
        return xuguang_uart_object_handle;
    }

    return false;
}

// void send_command(CommandType cmd) {
//     uint8_t packet[15];
//     uint8_t length;
//     uint8_t value=0;
//     create_packet(cmd, packet, &length, value);
//     uart_write_bytes(bht003_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

//     // ESP_LOGI(TAG, "Command sent:");
//     // for (int i = 0; i < length; i++) {
//     //     TRACE_I("%02X ", packet[i]);
//     // }
// }

// void send_temp_sp(float value,CommandType cmd) {
//     uint8_t packet[15];
//     uint8_t length;

//     create_packet(cmd, packet, &length, value);
//     uart_write_bytes(bht003_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

//     ESP_LOGI(TAG, "Command sent:");
//     for (int i = 0; i < length; i++) {
//         TRACE_I("%02X ", packet[i]);
//     }
// }