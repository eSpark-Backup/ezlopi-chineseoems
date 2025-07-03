#include "bht003.h"
#include "driver/uart.h"
#include "ezlopi_service_webprov.h"
#include "ezlopi_core_device_value_updated.h"

#define COPY_AND_SWAP32(dest, src) \
    (dest) = (((uint32_t)(src)[0] << 24) | \
              ((uint32_t)(src)[1] << 16) | \
              ((uint32_t)(src)[2] << 8)  | \
              ((uint32_t)(src)[3]))

#define START           0x00
#define HEADER          0x55

#define POWER           0x01
#define TEMP_SP         0x02
#define AMBIENT_TEMP    0x03
#define MODE_SELECTION  0x04
#define ENERGY_SAVING   0x05
#define CHILD_LOCK      0x06
#define FLOOR_TEMP      0x66

#define OTA        (2*1000*1000)

static uint64_t start_time = 0;

typedef struct command {
    uint16_t command;
    void (*callback)();
}command_t;

typedef struct values
{
    uint8_t command_sent;
    bool power_value;
    uint32_t temp_sp_value;
    uint32_t amb_temp_value;
    uint8_t mode_selection_value;
     bool energy_saving_value;
    bool child_lock_value;
    uint32_t floor_temp_value;
}values_t;

values_t Values;
s_ezlopi_uart_object_handle_t bht003_uart_object_handle;

void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle);
void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void temp_sp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void ambient_temp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void mode_selection(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void energy_saving(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void child_lock(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);

command_t start_cmd[] = {
    {.command = START,  .callback = start_packet},
    {.command = HEADER, .callback = normal_packet},
};

command_t dpid_cmd[] = {
    {.command = POWER,          .callback = power},
    {.command = TEMP_SP,        .callback = temp_sp},
    {.command = AMBIENT_TEMP,   .callback = ambient_temp},
    {.command = MODE_SELECTION, .callback = mode_selection},
    {.command = ENERGY_SAVING,  .callback = energy_saving},
    {.command = CHILD_LOCK,     .callback = child_lock},
};

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle)
{
    uart_write_bytes(uart_object_handle->ezlopi_uart.channel, (const char *)START, sizeof(START));
}

void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    for (size_t i = 0; i < sizeof(dpid_cmd) / sizeof(dpid_cmd[0]); i++)
    {
        // TRACE_I("----------DPID----------%X",buffer[6]);
        if (dpid_cmd[i].command == (buffer[6]))
        {
            // TRACE_I("----------DPID----------%X",dpid_cmd[i].command);
            dpid_cmd[i].callback(buffer, output_len, uart_object_handle);
        }
    }
}

void power(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        Values.power_value = buffer[10];
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            l_ezlopi_item_t *power_item = device_head->items->next->next;
            if(power_item)
            {
                EZPI_core_device_value_updated_from_device_broadcast(power_item);
            }
        }
        if(esp_timer_get_time()-start_time < (OTA))
        {
            cJSON *cj_data = cJSON_CreateObject(__FUNCTION__);
            if (cj_data)
            {
                cJSON_AddStringToObject(__FUNCTION__, cj_data, "method", "hub.firmware.update.start");
                cJSON_AddStringToObject(__FUNCTION__, cj_data, "id", "_ID_");
                cJSON *params = cJSON_CreateObject(__FUNCTION__);
                if (params)
                {
                    cJSON_AddItemToObject(__FUNCTION__, cj_data, "params", params);
                    if(__send_cjson_data_to_nma_websocket(cj_data) == EZPI_SUCCESS)
                    {
                        TRACE_E("OTA METHOD SENT");
                    }
                    else{
                        TRACE_E("OTA METHOD NOT SENT");
                    }
                }
                cJSON_Delete(__FUNCTION__, cj_data);
            }
        }
        else{
            start_time = esp_timer_get_time();
        }
        // TRACE_I("----------POWER Value Received----------: %d",Values.power_value);
    }
}

void temp_sp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        Values.temp_sp_value = map(buffer[13], 10, 70, 0, 50);
        // TRACE_I("----------TEMP SP Received----------: %d",Values.temp_sp_value);
        if(device_head)
        {
            l_ezlopi_item_t *temp_sp_item = device_head->items->next;
            if(temp_sp_item)
            {
                EZPI_core_device_value_updated_from_device_broadcast(temp_sp_item);
            }
        }
        if(buffer[21] == ENERGY_SAVING)
        {
            Values.energy_saving_value = buffer[25];
            if(device_head)
            {
                l_ezlopi_item_t *energy_saving_item = device_head->items->next->next->next;
                if(energy_saving_item)
                {
                    EZPI_core_device_value_updated_from_device_broadcast(energy_saving_item);
                }
            }
            // TRACE_I("----------Energy Saving Received----------: %d",Values.energy_saving_value);
        }
    }
}

void ambient_temp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        COPY_AND_SWAP32(Values.amb_temp_value, &buffer[10]);
        // TRACE_I("----------AMB TEMP----------: %.2f",(Values.amb_temp_value/2.00));
        if(buffer[21] == FLOOR_TEMP)
        {
            COPY_AND_SWAP32(Values.floor_temp_value, &buffer[25]);
            // TRACE_I("----------FLOOR TEMP----------: %.2f",(Values.floor_temp_value/2.00));
        }
    }
}

void energy_saving(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        Values.energy_saving_value = buffer[13];
        // TRACE_I("----------ENERGY SAVING----------: %d",Values.energy_saving_value);
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            l_ezlopi_item_t *energy_saving_item = device_head->items->next->next->next;
            if(energy_saving_item)
            {
                EZPI_core_device_value_updated_from_device_broadcast(energy_saving_item);
            }
        }
    }
}

void child_lock(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        Values.child_lock_value = buffer[10];
        // TRACE_I("----------Child Lock----------: %d",Values.child_lock_value);
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            l_ezlopi_item_t *child_lock_item = device_head->next->items;
            if(child_lock_item)
            {
                EZPI_core_device_value_updated_from_device_broadcast(child_lock_item);
            }
        }
    }
}

void mode_selection(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if (buffer)
    {
        Values.mode_selection_value = buffer[10];
        // TRACE_I("----------Mode Selection----------: %d",Values.mode_selection_value);l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        l_ezlopi_device_t* device_head = EZPI_core_device_get_head();
        if(device_head)
        {
            l_ezlopi_item_t *mode_selection_item = device_head->next->next->items;
            if(mode_selection_item)
            {
                EZPI_core_device_value_updated_from_device_broadcast(mode_selection_item);
            }
        }
    }
}

bool bht003_power_value()
{
    // TRACE_I("----------POWER----------: %d",Values.power_value);
    return Values.power_value;
}

uint32_t bht003_temp_sp_value()
{
    // TRACE_I("----------TEMP SP----------: %d",Values.temp_sp_value);
    return Values.temp_sp_value;
}

float bht003_amb_temp_value()
{
    // TRACE_I("----------AMB TEMP----------: %.2f",(Values.amb_temp_value/2.00));
    return (Values.amb_temp_value/2.00);
}

bool bht003_energy_saving_value()
{
    // TRACE_I("----------ENERGY SAVING----------: %d",Values.energy_saving_value);
    return Values.energy_saving_value;
}

bool bht003_child_lock_value()
{
    // TRACE_I("----------CHILD LOCK----------: %d",Values.child_lock_value);
    return Values.child_lock_value;
}

uint8_t bht003_mode_selection_value()
{
    // TRACE_I("----------MODE SELECTION----------: %d",Values.mode_selection_value);
    return Values.mode_selection_value;
}

float bht003_floor_temp_value()
{
    // TRACE_I("----------Floor TEMP----------: %.2f",(Values.floor_temp_value/2.00));
    return (Values.floor_temp_value/2.00);
}

void create_packet(CommandType cmd, uint8_t *packet, uint8_t *length, int data)
{
    packet[0] = 0x55;
    packet[1] = 0xAA;
    packet[2] = 0x00;

    switch (cmd)
    {
    case CMD_HEARTBEAT:
        Values.command_sent = 0;
        packet[3] = 0x00; packet[4] = 0x00; packet[5] = 0x00; packet[6] = 0x00; packet[7] = 0xFF;
        *length = 7;
        break;
    case CMD_TURN_ON:
    Values.command_sent = 1;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x0E;
        *length = 12;
        break;
    case CMD_TURN_OFF:
        Values.command_sent = 1;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x01; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x0D;
        *length = 12;
        break;
    case CMD_PROGRAM_MODE:
        Values.command_sent = 2;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x13;
        *length = 12;
        break;
    case CMD_MANUAL_MODE:
        Values.command_sent = 2;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x04; packet[7] = 0x04;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x14;
        *length = 12;
        break;
    case CMD_ENERGY_SAVING_ON:
        Values.command_sent = 3;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x12;
        *length = 12;
        break;
    case CMD_ENERGY_SAVING_OFF:
        Values.command_sent = 3;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x05; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x11;
        *length = 12;
        break;
    case CMD_LOCK_ENABLE:
        Values.command_sent = 4;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x01; packet[11] = 0x13;
        *length = 12;
        break;
    case CMD_LOCK_DISABLE:
        Values.command_sent = 4;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x05; packet[6] = 0x06; packet[7] = 0x01;
        packet[8] = 0x00; packet[9] = 0x01; packet[10] = 0x00; packet[11] = 0x12;
        *length = 12;
        break;
    case CMD_TEMP:
        Values.command_sent = 5;
        *length = 15;
        packet[3] = 0x06; packet[4] = 0x00; packet[5] = 0x08; packet[6] = 0x02; packet[7] = 0x02;
        packet[8] = 0x00; packet[9] = 0x04; packet[10] = 0x00; packet[11] = 0x00;
        packet[12] = 0x00; packet[13] = map(data, 0, 50, 10, 70); packet[14] = 0x00;
        for (int i = 1; i < *length; i++) 
        {
            packet[14] += packet[i-1];
        }
        break;
    // case CMD_WIFI:
    //     packet[3] = 0x03; packet[4] = 0x00; packet[5] = 0x01; packet[6] = 0x05; packet[7] = 0x08;
    //     *length = 8;
    //     break;
    default:
        TRACE_E("Unknown command type");
        break;
    }
}

void bht003_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    if ((buffer[0]) == (start_cmd[0].command))
    {
        TRACE_I("----------Start----------");
        start_cmd[0].callback(uart_object_handle);
    }
    else if((buffer[0]) == (start_cmd[1].command))
    {
        TRACE_I("----------HEADER----------%X",buffer[0]);
        start_cmd[1].callback(buffer, output_len, uart_object_handle);
    }
    else{
    }
}

bool bht003_init(s_ezlopi_uart_t *bht003_uart_config)
{
    if (bht003_uart_config->enable)
    {
        bht003_uart_object_handle = EZPI_hal_uart_init(bht003_uart_config->baudrate, bht003_uart_config->tx, bht003_uart_config->rx, bht003_callback, NULL);
        return bht003_uart_object_handle;
    }

    return false;
}

void send_command(CommandType cmd)
{
    uint8_t packet[15];
    uint8_t length;
    uint8_t value=0;
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(bht003_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

    // TRACE_I("Command sent:");
    // for (int i = 0; i < length; i++) {
    //     TRACE_I("%02X ", packet[i]);
    // }
}

void send_temp_sp(int value,CommandType cmd)
{
    uint8_t packet[15];
    uint8_t length;
    create_packet(cmd, packet, &length, value);
    uart_write_bytes(bht003_uart_object_handle->ezlopi_uart.channel, (const char *)packet, length);

    // TRACE_I("Command sent:");
    // for (int i = 0; i < length; i++) {
    //     TRACE_I("%02X ", packet[i]);
    // }
}