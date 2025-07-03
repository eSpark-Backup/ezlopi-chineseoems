#include "beca10003A.h"
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
    bool child_lock_value;
    uint32_t floor_temp_value;
}values_t;

values_t Values;

s_ezlopi_uart_object_handle_t beca10003A_uart_object_handle;

void start_packet(s_ezlopi_uart_object_handle_t uart_object_handle);
void normal_packet(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void temp_sp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);
void ambient_temp(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle);


command_t start_cmd[] = {
    {.command = START,  .callback = start_packet},
    {.command = HEADER, .callback = normal_packet},
};

command_t dpid_cmd[] = {
    // {.command = POWER,          .callback = power},
    {.command = TEMP_SP,        .callback = temp_sp},
    {.command = AMBIENT_TEMP,   .callback = ambient_temp}
    // {.command = MODE_SELECTION, .callback = mode_selection},
    // {.command = CHILD_LOCK,     .callback = child_lock},
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
        // if(buffer[21] == ENERGY_SAVING)
        // {
        //     Values.energy_saving_value = buffer[25];
        //     if(device_head)
        //     {
        //         l_ezlopi_item_t *energy_saving_item = device_head->items->next->next->next;
        //         if(energy_saving_item)
        //         {
        //             EZPI_core_device_value_updated_from_device_broadcast(energy_saving_item);
        //         }
        //     }
            // TRACE_I("----------Energy Saving Received----------: %d",Values.energy_saving_value);
        // }
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

uint32_t beca10003A_temp_sp_value()
{
    // TRACE_I("----------TEMP SP----------: %d",Values.temp_sp_value);
    return Values.temp_sp_value;
}
void beca10003A_callback(uint8_t* buffer, uint32_t output_len, s_ezlopi_uart_object_handle_t uart_object_handle)
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

bool beca10003A_init(s_ezlopi_uart_t *beca10003A_uart_config)
{
    if (beca10003A_uart_config->enable)
    {
        beca10003A_uart_object_handle = EZPI_hal_uart_init(beca10003A_uart_config->baudrate, beca10003A_uart_config->tx, beca10003A_uart_config->rx, beca10003A_callback, NULL);
        return beca10003A_uart_object_handle;
    }

    return false;
}
