#include <string.h>
#include <ctype.h>

#include "ezlopi_util_trace.h"
#include "ezlopi_cloud_keywords.h"
#include "ezlopi_core_nvs.h"
#include "ezlopi_core_factory_info.h"
#include "EZLOPI_USER_CONFIG.h"

#define UPDATE_STRING_VALUE(buffer, data, offset, length)  \
    {                                                      \
        if (data)                                          \
        {                                                  \
            snprintf(buffer + offset, length, "%s", data); \
        }                                                  \
    }

static uint32_t g_provisioning_status = 0;
static const esp_partition_t* partition_ctx_v3 = NULL;

/* Should not free, Since these informations are used by web-socket-client and are used to reconnect. */
/* will help decrease heap-fragmentation */
static char * __ca_cert = NULL;
static char * __pvt_key = NULL;
static char * __pub_key = NULL;

static int ezlopi_factory_info_v3_set_4kb(const char* data, uint32_t offset, uint32_t len)
{
    int ret = 0;
    char * tmp_variable = ezlopi_malloc(__FUNCTION__, len + 1);

    if (tmp_variable)
    {
        snprintf(tmp_variable, len + 1, "%.*s", len, data);

        if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, offset, EZLOPI_FINFO_READ_LEN_4KB))
        {
            if (ESP_OK == esp_partition_write(partition_ctx_v3, offset, tmp_variable, len + 1))
            {
                TRACE_S("Flash write successful");
                ret = 1;
            }
            else
            {
                TRACE_E("esp-partition write failed!");
            }
        }
        else
        {
            TRACE_E("esp-partition erase failed!");
        }

        ezlopi_free(__FUNCTION__, tmp_variable);
    }

    return ret;
}

uint32_t ezlopi_factory_info_v3_get_abs_address(uint32_t relative_offset, e_factory_info_v3_partition_type_t partition_type)
{
    if (partition_type == E_EZLOPI_FACTORY_INFO_CONN_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_CONN_DATA);
    }
    else if (partition_type == E_EZLOPI_FACTORY_INFO_HUB_DATA)
    {
        return (relative_offset + EZLOPI_FINFO_OFFSET_HUB_DATA);
    }
    else
    {
        return 0;
    }
}

static char *ezlopi_factory_info_v3_read_string(const char * name, e_ezlopi_factory_info_v3_offset_t offset, e_ezlopi_factory_info_v3_length_t length)
{
    char* read_string = NULL;

    if (ezlopi_factory_info_v3_init())
    {
        // TRACE_D("%s", name);
        char *tmp_buffer = (char *)ezlopi_malloc(name, length);
        if (tmp_buffer)
        {
            if (ESP_OK == esp_partition_read(partition_ctx_v3, offset, tmp_buffer, length))
            {
                int s_length = (strlen(tmp_buffer) < length) ? strlen(tmp_buffer) : length;
                read_string = (char*)ezlopi_malloc(name, s_length + 1);

                if (NULL != read_string)
                {
                    if (1) // (isprint(tmp_buffer[0]))
                    {
                        snprintf(read_string, s_length + 1, "%s", tmp_buffer);
                    }
                    else
                    {
                        ezlopi_free(__FUNCTION__, read_string);
                        read_string = NULL;
                    }
                }
                else
                {
                    TRACE_E("'read_string' malloc failed!");
                }
            }
            else
            {
                TRACE_E("Couldn't fetch 'string' from id-flash-region!");
            }

            // dump("tmp_buffer", tmp_buffer, 0, length);
            ezlopi_free(__FUNCTION__, tmp_buffer);
        }
        else
        {
            TRACE_E("'tmp_buffer' malloc failed!");
        }
    }

    return read_string;
}

const esp_partition_t* ezlopi_factory_info_v3_init(void)
{
    if (NULL == partition_ctx_v3)
    {
        partition_ctx_v3 = esp_partition_find_first(EZLOPI_FACTORY_INFO_V3_PARTITION_TYPE, EZLOPI_FACTORY_INFO_V3_SUBTYPE, EZLOPI_FACTORY_INFO_V3_PARTITION_NAME);
    }
    return partition_ctx_v3;
}

void ezlopi_factory_info_v3_free(void* arg)
{
    if (arg)
    {
        ezlopi_free(__FUNCTION__, arg);
        arg = NULL;
    }
}

void print_factory_info_v3(void)
{
#if (1 == ENABLE_TRACE)
    // uint16_t version = ezlopi_factory_info_v3_get_version();
    unsigned long long id = ezlopi_factory_info_v3_get_id();

    char* mac = ezlopi_factory_info_v3_get_ezlopi_mac();
    char* name = ezlopi_factory_info_v3_get_name();
    char* manufacturer = ezlopi_factory_info_v3_get_manufacturer();
    char* brand = ezlopi_factory_info_v3_get_brand();
    char* model = ezlopi_factory_info_v3_get_model();
    char* device_uuid = ezlopi_factory_info_v3_get_device_uuid();
    char* provisioning_uuid = ezlopi_factory_info_v3_get_provisioning_uuid();
    char* wifi_ssid = ezlopi_factory_info_v3_get_ssid(NULL);
    char* wifi_password = ezlopi_factory_info_v3_get_password(NULL);
    char* cloud_server = ezlopi_factory_info_v3_get_cloud_server();
    char * provisioning_token = ezlopi_factory_info_get_v3_provision_token();
    // const char* provision_server = ezlopi_factory_info_v3_get_provisioning_server();
    const char* device_type = ezlopi_factory_info_v3_get_device_type();
    char* ca_certificate = ezlopi_factory_info_v3_get_ca_certificate();
    char* ssl_private_key = ezlopi_factory_info_v3_get_ssl_private_key();
    char* ssl_shared_key = ezlopi_factory_info_v3_get_ssl_shared_key();
    char* ezlopi_config = ezlopi_factory_info_v3_get_ezlopi_config();

    TRACE_W("----------------- Factory Info -----------------");
    // TRACE_W("VERSION[off: 0x%04X, size: 0x%04X]:                %d", VERSION_OFFSET, VERSION_LENGTH, version);
    TRACE_W("SERIAL-ID [off: 0x%04X, size: 0x%04X]:             %llu", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_ID, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_DEVICE_ID, id);
    TRACE_W("MAC [off: 0x%04X, size: 0x%04X]:                   %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_MAC, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_DEVICE_MAC, mac ? mac : "null");
    TRACE_W("NAME [off: 0x%04X, size: 0x%04X]:                  %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_DEVICE_NAME, name ? name : "null");
    TRACE_W("MANUFACTURER [off: 0x%04X, size: 0x%04X]:          %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_MANUF_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_MANUF_NAME, manufacturer ? manufacturer : "null");
    TRACE_W("BRAND [off: 0x%04X, size: 0x%04X]:                 %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_BRAND_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_BRAND_NAME, brand ? brand : "null");
    TRACE_W("MODEL [off: 0x%04X, size: 0x%04X]:                 %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_MODEL_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_MODEL_NAME, model ? model : "null");
    TRACE_W("DEVICE_UUID [off: 0x%04X, size: 0x%04X]:           %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_UUID, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_DEVICE_UUID, device_uuid ? device_uuid : "null");
    TRACE_W("PROVISIONING_UUID [off: 0x%04X, size: 0x%04X]:     %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_PROVISIONING_UUID, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_PROV_UUID, provisioning_uuid ? provisioning_uuid : "null");
    TRACE_W("WIFI-SSID [off: 0x%04X, size: 0x%04X]:             %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_SSID, wifi_ssid ? wifi_ssid : "null");
    // TRACE_W("WIFI-PASSWORD [off: 0x%04X, size: 0x%04X]:         %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_PASS, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_PASS, wifi_password ? wifi_password : "null");
    TRACE_W("PROVISIONING-TOKEN [off: 0x%04X, size: 0x%04X]:    %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_PROVISIONING_TOKEN, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_PROVISIONING_TOKEN, provisioning_token ? provisioning_token : "null");
    TRACE_W("CLOUD_SERVER [off: 0x%04X, size: 0x%04X]:          %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_CLOUD_SERVER_URL, cloud_server ? cloud_server : "null");
    TRACE_W("DEVICE_TYPE [off: 0x%04X, size: 0x%04X]:           %s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_DEVICE_TYPE, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_EZLOPI_DEVICE_TYPE, device_type ? device_type : "null");
    TRACE_W("CA_CERTIFICATE [off: 0x%04X, size: 0x%04X]:        \r\n\r\n%s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_CA_CERTIFICATE, ca_certificate ? ca_certificate : "null");
    TRACE_W("SSL_PRIVATE_KEY [off: 0x%04X, size: 0x%04X]:       \r\n\r\n%s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_PRIVATE_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_SSL_PRIVATE_KEY, ssl_private_key ? ssl_private_key : "null");
    TRACE_W("SSL_SHARED_KEY [off: 0x%04X, size: 0x%04X]:        \r\n\r\n%s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_SSL_SHARED_KEY, ssl_shared_key ? ssl_shared_key : "null");
    TRACE_W("EZLOPI_CONFIG [off: 0x%04X, size: 0x%04X]:         \r\n\r\n%s", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_EZLOPI_CONFIG_JSON, ezlopi_config ? ezlopi_config : "null");
    TRACE_W("-------------------------------------------------");

    // ezlopi_factory_info_v3_free(mac);
    // ezlopi_factory_info_v3_free(name);
    ezlopi_factory_info_v3_free(manufacturer);
    ezlopi_factory_info_v3_free(brand);
    ezlopi_factory_info_v3_free(model);
    // ezlopi_factory_info_v3_free(device_uuid);
    // ezlopi_factory_info_v3_free(provisioning_uuid);
    // ezlopi_factory_info_v3_free(wifi_ssid);
    // ezlopi_factory_info_v3_free(wifi_password);
    // ezlopi_factory_info_v3_free(cloud_server);
    // ezlopi_factory_info_v3_free(provisioning_token);
    // ezlopi_factory_info_v3_free(ca_certificate); // allocated once for all, do not free
    // ezlopi_factory_info_v3_free(ssl_private_key); // allocated once for all, do not free
    // ezlopi_factory_info_v3_free(ssl_shared_key); // allocated once for all, do not free
    // ezlopi_factory_info_v3_free(ezlopi_config);
#endif
}

/** Getter */
uint32_t ezlopi_factory_info_v3_get_provisioning_status(void)
{
    char* cloud_server = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_CLOUD_SERVER_URL);
    if (cloud_server && strstr(cloud_server, "https://"))
    {
        g_provisioning_status = 1;
        ezlopi_free(__FUNCTION__, cloud_server);
    }

    return g_provisioning_status;
}

uint16_t ezlopi_factory_info_v3_get_version(void)
{
    uint16_t _version = 0ULL;

    if (ezlopi_factory_info_v3_init())
    {
        uint8_t tmp_version_arr[EZLOPI_FINFO_LEN_VERSION];
        memset(tmp_version_arr, 0, EZLOPI_FINFO_LEN_VERSION);
        esp_partition_read(partition_ctx_v3, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_FMW_VERSION, E_EZLOPI_FACTORY_INFO_HUB_DATA), &tmp_version_arr, EZLOPI_FINFO_LEN_VERSION);

        for (int i = 0; i < 4; i++)
        {
            _version |= tmp_version_arr[i] << (i * 8);
        }
    }
    else
    {
        TRACE_E("'ezlopi_factory_info_v3' failed");
    }

    return _version;
}

char* ezlopi_factory_info_v3_get_name(void)
{
    char *read_data = "EzloPi";
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_DEVICE_NAME);
    if (read_data)
    {
        if (!isprint(read_data[0]))
        {
            ezlopi_free(__FUNCTION__, read_data);
                read_data = NULL;
        }
    }
    return read_data;
}

char* ezlopi_factory_info_v3_get_manufacturer(void)
{
    char *read_data = NULL;
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_MANUF_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_MANUF_NAME);

    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_v3_get_brand(void)
{
    char *read_data = NULL;
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_BRAND_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_BRAND_NAME);

    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_v3_get_model(void)
{
    char *read_data = NULL;
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_MODEL_NAME, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_MODEL_NAME);
    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_get_v3_provision_token(void)
{
    char *read_data = "3040c38fa3d8d5538b124a77d497d5f2a5b67f087ecf1e8f5ab3ecc7b56c0b6fdb6d201d7010f53db09bc8ac11a3a3c263f0e4db928c550456fadcd37725cc74bd58e0e698777c7d2960330c96c6a313165512525795476b8c9312c7afef57eeb89e53bfe131324b5c526ec19a89287232e8628589a4192af960dcd41ef8639a";
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_PROVISIONING_TOKEN, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_PROVISIONING_TOKEN);
    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

uint16_t ezlopi_factory_info_v3_get_config_version(void)
{
    uint16_t config_version = 0;

    if (ezlopi_factory_info_v3_init())
    {
        uint8_t tmp_config_version_arr[2];
        memset(tmp_config_version_arr, 0, 2);
        esp_partition_read(partition_ctx_v3, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CONFIG_VERSION, E_EZLOPI_FACTORY_INFO_CONN_DATA), tmp_config_version_arr, EZLOPI_FINFO_LEN_CONFIG_VERSION);
        config_version = (tmp_config_version_arr[0] << 8) | tmp_config_version_arr[1];
    }

    return config_version;
}

unsigned long long ezlopi_factory_info_v3_get_id(void)
{
    unsigned long long _id = 102003076;//0ULL
    //rafay
    // if (ezlopi_factory_info_v3_get_provisioning_status() != 1)
    // {
    //     return 0;
    // }

    // if (ezlopi_factory_info_v3_init())
    // {
    //     uint8_t tmp_id_arr[8];
    //     memset(tmp_id_arr, 0, 8);
    //     esp_partition_read(partition_ctx_v3, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_ID, E_EZLOPI_FACTORY_INFO_HUB_DATA), tmp_id_arr, EZLOPI_FINFO_LEN_DEVICE_ID);

    //     for (int i = 0; i < 8; i++)
    //     {
    //         _id |= tmp_id_arr[i] << (i * 8);
    //     }

    //     dump("serial-id", tmp_id_arr, 0, 8);
    // }
    // else
    // {
    //     TRACE_E("Factory info un-initialized!");
    // }

    return _id;
}

char* ezlopi_factory_info_v3_get_device_uuid(void)
{
    char *read_data = "a7ee12e0-8394-11ee-8889-0f56091e7639";
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_UUID, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_DEVICE_UUID);
    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_v3_get_provisioning_uuid(void)
{
    char *read_data = "91acc056-07f0-43f8-89d8-d1492022d8c8";
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_PROVISIONING_UUID, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_PROV_UUID);
    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_v3_get_ssid(bool state)
{
    // char *read_data = "eSpark Wifi";
    char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_SSID);
    TRACE_I("------------VALUE-----------%d",state);
    if(state)
    {
        read_data = ezlopi_factory_info_v3_write_null(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_SSID);
    }
    else
    {
        if (read_data)
        {
            if (!isprint(read_data[0]))
            {
                ezlopi_free(__FUNCTION__, read_data);
                read_data = NULL;
            }
        }
    }

    return read_data;
}

char* ezlopi_factory_info_v3_get_password(bool state)
{
    // char *read_data = "47356880";
    char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_PASS, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_PASS);
    if(state)
    {
        read_data = ezlopi_factory_info_v3_write_null(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_WIFI_SSID);
    }
    else
    {
        if (read_data)
        {
            if (!isprint(read_data[0]))
            {
                ezlopi_free(__FUNCTION__, read_data);
                read_data = NULL;
            }
        }
    }

    return read_data;
}

char* ezlopi_factory_info_v3_get_ezlopi_mac(void)
{

    char *read_data = "a2:e6:b3:e8:b2:49";
    // char *read_data = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_DEVICE_MAC, E_EZLOPI_FACTORY_INFO_HUB_DATA), EZLOPI_FINFO_LEN_DEVICE_MAC);
    // if (read_data)
    // {
    //     if (!isprint(read_data[0]))
    //     {
    //         ezlopi_free(__FUNCTION__, read_data);
    //         read_data = NULL;
    //     }
    // }
    return read_data;
}

char* ezlopi_factory_info_v3_get_cloud_server(void)
{
    char *cloud_server = "https://cloud.ezlo.com:7000/getserver";
    // char *cloud_server = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_CLOUD_SERVER_URL);
    if (cloud_server && strstr(cloud_server, "https://"))
    {
        g_provisioning_status = 1;
    }
    return cloud_server;
}

char* ezlopi_factory_info_v3_get_provisioning_server(void)
{
    char *provisioning_server = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_PROVISIONING_SERVER_URL, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_PROVISIONING_SERVER_URL);
    if (provisioning_server && strstr(provisioning_server, "https://"))
    {
        return provisioning_server;
    }
    else
    {
        return NULL;
    }
}

const char* ezlopi_factory_info_v3_get_device_type(void)
{
    // return "ezlopi_generic";
    return ezlopi_d01_type_str;
}

void ezlopi_factory_info_v3_free_ca_certificate(void)
{
    if (__ca_cert)
    {
        ezlopi_free(__FUNCTION__, __ca_cert);
        __ca_cert = NULL;
    }
}
char* ezlopi_factory_info_v3_get_ca_certificate(void)
{
    if (NULL == __ca_cert)
    {
        __ca_cert = "-----BEGIN CERTIFICATE-----\n\
MIICbDCCAhGgAwIBAgIJAOByzaI7aHY9MAoGCCqGSM49BAMDMIGQMQswCQYDVQQG\n\
EwJVUzEUMBIGA1UECAwLIE5ldyBKZXJzZXkxEDAOBgNVBAcMB0NsaWZ0b24xDzAN\n\
BgNVBAoMBklUIE9wczEPMA0GA1UECwwGSVQgT3BzMRQwEgYDVQQDDAtlWkxPIExU\n\
RCBDQTEhMB8GCSqGSIb3DQEJARYSc3lzYWRtaW5zQGV6bG8uY29tMCAXDTE5MDUz\n\
MTE3MDE0N1oYDzIxMTkwNTA3MTcwMTQ3WjCBkDELMAkGA1UEBhMCVVMxFDASBgNV\n\
BAgMCyBOZXcgSmVyc2V5MRAwDgYDVQQHDAdDbGlmdG9uMQ8wDQYDVQQKDAZJVCBP\n\
cHMxDzANBgNVBAsMBklUIE9wczEUMBIGA1UEAwwLZVpMTyBMVEQgQ0ExITAfBgkq\n\
hkiG9w0BCQEWEnN5c2FkbWluc0BlemxvLmNvbTBWMBAGByqGSM49AgEGBSuBBAAK\n\
A0IABHLQdhLDYsafIFY8pZh96aDGqVm6E4r8nW9s4CfdpXaa/R4CnjaVpDQI7UmQ\n\
9vVDGZn8mcmm7VjKx+TSCS0MIKOjUzBRMB0GA1UdDgQWBBRiTl8Ez1l94jaqcxbi\n\
yxkVC0FkBTAfBgNVHSMEGDAWgBRiTl8Ez1l94jaqcxbiyxkVC0FkBTAPBgNVHRMB\n\
Af8EBTADAQH/MAoGCCqGSM49BAMDA0kAMEYCIQD7EUs8j50jKFd/46Zo95NbrPYQ\n\
PtLTHH9YjUkMEkYD5gIhAMP4y7E1aB78nQrmd3IX8MM32k9dM8xT0MztR16OtsuV\n\
-----END CERTIFICATE-----";
        // __ca_cert = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_CA_CERTIFICATE);
    }
    return __ca_cert;
}

void ezlopi_factory_info_v3_free_ssl_private_key(void)
{
    if (__pvt_key)
    {
        ezlopi_free(__FUNCTION__, __pvt_key);
        __pvt_key = NULL;
    }
}
char* ezlopi_factory_info_v3_get_ssl_private_key(void)
{
    if (NULL == __pvt_key)
    {
        __pvt_key = "-----BEGIN PRIVATE KEY-----\n\
MIGEAgEAMBAGByqGSM49AgEGBSuBBAAKBG0wawIBAQQgPd6NUOnOOfOPJcvuACMm\n\
CeCfAfUR2C1uT/oU2kGw2kyhRANCAATq50WVy/7cgyQv0oGL2Mx3qXd/29H2kEGc\n\
4gF6V3MaCKicOYCQsNnxrHwFdvBUgwxBdtRP+aTRAXa5UWCzGu40\n\
-----END PRIVATE KEY-----";
        // __pvt_key = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_PRIVATE_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_SSL_PRIVATE_KEY);
    }
    return __pvt_key;
}

void  ezlopi_factory_info_v3_free_ssl_shared_key(void)
{
    if (__pub_key)
    {
        ezlopi_free(__FUNCTION__, __pub_key);
        __pub_key = NULL;
    }
}
char* ezlopi_factory_info_v3_get_ssl_shared_key(void)
{
    if (NULL == __pub_key)
    {
        __pub_key = "-----BEGIN CERTIFICATE-----\n\
MIICDTCCAbKgAwIBAgIDBImxMAoGCCqGSM49BAMCMIGQMQswCQYDVQQGEwJVUzEU\n\
MBIGA1UECAwLIE5ldyBKZXJzZXkxEDAOBgNVBAcMB0NsaWZ0b24xDzANBgNVBAoM\n\
BklUIE9wczEPMA0GA1UECwwGSVQgT3BzMRQwEgYDVQQDDAtlWkxPIExURCBDQTEh\n\
MB8GCSqGSIb3DQEJARYSc3lzYWRtaW5zQGV6bG8uY29tMCAXDTIzMTExNTA4NTUw\n\
MFoYDzIyOTcwODI5MDg1NTAwWjCBjDELMAkGA1UEBhMCVVMxEzARBgNVBAgMCk5l\n\
dyBKZXJzZXkxEDAOBgNVBAcMB0NsaWZ0b24xEzARBgNVBAoMCmNvbnRyb2xsZXIx\n\
LTArBgNVBAsMJGE4YWI2ZjcwLTgzOTQtMTFlZS04ODg5LTBmNTYwOTFlNzYzOTES\n\
MBAGA1UEAwwJMTAyMDAzMDc2MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAE6udFlcv+\n\
3IMkL9KBi9jMd6l3f9vR9pBBnOIBeldzGgionDmAkLDZ8ax8BXbwVIMMQXbUT/mk\n\
0QF2uVFgsxruNDAKBggqhkjOPQQDAgNJADBGAiEAyNCCJq7wmFS9sQez7MUAMOvu\n\
RFv6CUjgByV14FZ6lv0CIQCnSpPeGBvf1Cf7CEo1o25Gnewf2ccSMAUSt+d8BPuH\n\
CA==\n\
-----END CERTIFICATE-----";
        // __pub_key = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_SSL_SHARED_KEY);
    }
    return __pub_key;
}

char* ezlopi_factory_info_v3_get_ezlopi_config(void)
{
    // TRACE_E("ezlopi-config address: %d", ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA));
    char *ret = "{\"config_id\":904483883,\"config_time\":1720448712,\"cmd\":4,\"status\":1,\"dev_total\":0,\"dev_detail\":[],\"chipset\":\"ESP32\"}";
    // char *ret = ezlopi_factory_info_v3_read_string(__FUNCTION__, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA), EZLOPI_FINFO_LEN_EZLOPI_CONFIG_JSON);
    // if (false == isprint(ret[0]))
    // {
    //     if (ret)
    //     {
    //         ezlopi_free(__FUNCTION__, ret);
    //         ret = NULL;
    //     }
    // }
    return ret;
}

int ezlopi_factory_info_v3_set_basic(s_basic_factory_info_t* ezlopi_config_basic)
{
    int ret = 0;

    if (ezlopi_config_basic)
    {
        if (ezlopi_factory_info_v3_init())
        {
            char* tmp_buffer = (char*)ezlopi_malloc(__FUNCTION__, EZLOPI_FINFO_READ_LEN_4KB);
            if (tmp_buffer)
            {
                if (ESP_OK == esp_partition_read(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if (NULL != ezlopi_config_basic->device_name)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_NAME, 0, EZLOPI_FINFO_LEN_DEVICE_NAME);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_NAME, ezlopi_config_basic->device_name, EZLOPI_FINFO_LEN_DEVICE_NAME);
                        TRACE_S("device_name: %s", ezlopi_config_basic->device_name);
                    }

                    if (NULL != ezlopi_config_basic->manufacturer)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_MANUF_NAME, 0, EZLOPI_FINFO_LEN_MANUF_NAME);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_MANUF_NAME, ezlopi_config_basic->manufacturer, EZLOPI_FINFO_LEN_MANUF_NAME);
                        TRACE_S("manufacturer: %s", ezlopi_config_basic->manufacturer);
                    }

                    if (NULL != ezlopi_config_basic->brand)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_BRAND_NAME, 0, EZLOPI_FINFO_LEN_BRAND_NAME);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_BRAND_NAME, ezlopi_config_basic->brand, EZLOPI_FINFO_LEN_BRAND_NAME);
                        TRACE_S("brand: %s", ezlopi_config_basic->brand);
                    }
                    if (NULL != ezlopi_config_basic->model_number)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_MODEL_NAME, 0, EZLOPI_FINFO_LEN_MODEL_NAME);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_MODEL_NAME, ezlopi_config_basic->model_number, EZLOPI_FINFO_LEN_MODEL_NAME);
                        TRACE_S("model_number: %s", ezlopi_config_basic->model_number);
                    }
                    if (NULL != ezlopi_config_basic->device_type)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_EZLOPI_DEVICE_TYPE, 0, EZLOPI_FINFO_LEN_EZLOPI_DEVICE_TYPE);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_EZLOPI_DEVICE_TYPE, ezlopi_config_basic->device_type, EZLOPI_FINFO_LEN_EZLOPI_DEVICE_TYPE);
                        TRACE_S("device_type: %s", ezlopi_config_basic->device_type);
                    }

                    if (NULL != ezlopi_config_basic->device_mac)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_MAC, 0, EZLOPI_FINFO_LEN_DEVICE_MAC);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_MAC, ezlopi_config_basic->device_mac, EZLOPI_FINFO_LEN_DEVICE_MAC);
                        TRACE_S("EzloPi Device MAC: %s", ezlopi_config_basic->device_mac);
                    }

                    if (0 != ezlopi_config_basic->id)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_ID, 0, EZLOPI_FINFO_LEN_DEVICE_ID);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_ID, &ezlopi_config_basic->id, EZLOPI_FINFO_LEN_DEVICE_ID);
                        TRACE_S("EzloPi Device ID: %lld", ezlopi_config_basic->id);
                    }

                    if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                    {
                        if (ESP_OK == esp_partition_write(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                        {
                            ret = 1;
                        }
                    }
                }
                else
                {
                    TRACE_E("Couldn't fetch 'data' from id-flash-region!");
                }

                if (ESP_OK == esp_partition_read(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    if (0 != ezlopi_config_basic->config_version)
                    {
                        uint8_t config_version_arry[2] = { 0 };
                        config_version_arry[0] = (uint8_t)((ezlopi_config_basic->config_version & 0xFF00) >> 8);
                        config_version_arry[1] = (uint8_t)(ezlopi_config_basic->config_version & 0x00FF);
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_CONFIG_VERSION, 0, EZLOPI_FINFO_LEN_CONFIG_VERSION);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_CONFIG_VERSION, config_version_arry, EZLOPI_FINFO_LEN_CONFIG_VERSION);
                        TRACE_S("Config Version: %d", ezlopi_config_basic->config_version);
                    }

                    if (NULL != ezlopi_config_basic->cloud_server)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, 0, EZLOPI_FINFO_LEN_CLOUD_SERVER_URL);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_CLOUD_SERVER_URL, ezlopi_config_basic->cloud_server, EZLOPI_FINFO_LEN_CLOUD_SERVER_URL);
                        TRACE_S("cloud_server: %s", ezlopi_config_basic->cloud_server);
                    }
                    if (NULL != ezlopi_config_basic->device_uuid)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_UUID, 0, EZLOPI_FINFO_LEN_DEVICE_UUID);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_DEVICE_UUID, ezlopi_config_basic->device_uuid, EZLOPI_FINFO_LEN_DEVICE_UUID);
                        TRACE_S("device_uuid: %s", ezlopi_config_basic->device_uuid);
                    }
                    if (NULL != ezlopi_config_basic->prov_uuid)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_UUID, 0, EZLOPI_FINFO_LEN_PROV_UUID);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_UUID, ezlopi_config_basic->prov_uuid, EZLOPI_FINFO_LEN_PROV_UUID);
                        TRACE_S("prov_uuid: %s", ezlopi_config_basic->prov_uuid);
                    }
                    if (NULL != ezlopi_config_basic->provision_server)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_SERVER_URL, 0, EZLOPI_FINFO_LEN_PROVISIONING_SERVER_URL);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_SERVER_URL, ezlopi_config_basic->provision_server, EZLOPI_FINFO_LEN_PROVISIONING_SERVER_URL);
                        TRACE_S("provision_server: %s", ezlopi_config_basic->provision_server);
                    }

                    if (NULL != ezlopi_config_basic->provision_token)
                    {
                        memset(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_TOKEN, 0, EZLOPI_FINFO_LEN_PROVISIONING_TOKEN);
                        memcpy(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_PROVISIONING_TOKEN, ezlopi_config_basic->provision_token, EZLOPI_FINFO_LEN_PROVISIONING_TOKEN);
                        TRACE_S("provision_token: %s", ezlopi_config_basic->provision_token);
                    }

                    if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                    {
                        if (ESP_OK == esp_partition_write(partition_ctx_v3, EZLOPI_FINFO_OFFSET_CONN_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                        {
                            ret = (ret == 1) ? 1 : 0;
                        }
                        else
                        {
                            ret = 0;
                        }
                    }
                    else
                    {
                        ret = 0;
                    }
                }
                else
                {
                    TRACE_E("Couldn't fetch 'string' from id-flash-region!");
                }
                // ESP_LOG_BUFFER_HEXDUMP("PROV", tmp_buffer, length, ESP_LOG_INFO);
                ezlopi_free(__FUNCTION__, tmp_buffer);
            }
        }
    }
    return ret;
}

int ezlopi_factory_info_v3_set_wifi(char* ssid, char* password)
{
    int ret = 0;

    if (ssid && password)
    {
        if (ezlopi_factory_info_v3_init())
        {

            char* tmp_buffer = (char*)ezlopi_malloc(__FUNCTION__, EZLOPI_FINFO_READ_LEN_4KB);
            if (tmp_buffer)
            {
                if (ESP_OK == esp_partition_read(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                {
                    snprintf(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_WIFI_SSID, EZLOPI_FINFO_LEN_WIFI_SSID, "%s", ssid);
                    snprintf(tmp_buffer + EZLOPI_FINFO_REL_OFFSET_WIFI_PASS, EZLOPI_FINFO_LEN_WIFI_PASS, "%s", password);

                    if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, EZLOPI_FINFO_READ_LEN_4KB))
                    {
                        if (ESP_OK == esp_partition_write(partition_ctx_v3, EZLOPI_FINFO_OFFSET_HUB_DATA, tmp_buffer, EZLOPI_FINFO_READ_LEN_4KB))
                        {
                            ret = 1;
                        }
                    }
                    else
                    {
                        TRACE_E("failed to save wifi");
                    }
                }
                else
                {
                    TRACE_E("Couldn't fetch 'string' from id-flash-region!");
                }

                ezlopi_free(__FUNCTION__, tmp_buffer);
            }
            else
            {
                TRACE_E("'tmp_buffer' malloc failed!");
            }
        }
    }

    return ret;
}

int ezlopi_factory_info_v3_set_ssl_private_key(cJSON * cj_data)
{
    int ret = 0;
    if (cj_data && cj_data->valuestring && cj_data->str_value_len)
    {
        ret = ezlopi_factory_info_v3_set_4kb(cj_data->valuestring, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_PRIVATE_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), cj_data->str_value_len);
    }
    return ret;
}

int ezlopi_factory_info_v3_set_ssl_public_key(cJSON * cj_data)
{
    int ret = 0;
    if (cj_data && cj_data->valuestring && cj_data->str_value_len)
    {
        ret = ezlopi_factory_info_v3_set_4kb(cj_data->valuestring, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), cj_data->str_value_len);
    }
    return ret;
}

int ezlopi_factory_info_v3_set_ssl_shared_key(cJSON * cj_data)
{
    int ret = 0;
    if (cj_data && cj_data->valuestring && cj_data->str_value_len)
    {
        ret = ezlopi_factory_info_v3_set_4kb(cj_data->valuestring, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_SSL_SHARED_KEY, E_EZLOPI_FACTORY_INFO_CONN_DATA), cj_data->str_value_len);
    }
    return ret;
}

int ezlopi_factory_info_v3_set_ca_cert(cJSON * cj_data)
{
    int ret = 0;
    if (cj_data && cj_data->valuestring && cj_data->str_value_len)
    {
        ret = ezlopi_factory_info_v3_set_4kb(cj_data->valuestring, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_CA_CERTIFICATE, E_EZLOPI_FACTORY_INFO_CONN_DATA), cj_data->str_value_len);
    }
    return ret;
}

int ezlopi_factory_info_v3_set_ezlopi_config(cJSON * cj_data)
{
    int ret = 0;
    if (cj_data)
    {
        char * tmp_data = ezlopi_malloc(__FUNCTION__, 4 * 1024);
        if (tmp_data)
        {
            if (cJSON_PrintPreallocated(__FUNCTION__, cj_data, tmp_data, 4 * 1024, false))
            {
                // TRACE_D("added-chipset: %.*s", tmp_data);
                ret = ezlopi_factory_info_v3_set_4kb(tmp_data, ezlopi_factory_info_v3_get_abs_address(EZLOPI_FINFO_REL_OFFSET_EZLOPI_CONFIG_JSON, E_EZLOPI_FACTORY_INFO_CONN_DATA), strlen(tmp_data) + 1);
            }

            ezlopi_free(__FUNCTION__, tmp_data);
        }
    }

    return ret;
}

int ezlopi_factory_info_v3_factory_reset(void)
{
    int ret = 0;
    if (ezlopi_factory_info_v3_init())
    {

        if (ESP_OK == esp_partition_erase_range(partition_ctx_v3, 0, EZLOPI_FACTORY_INFO_V3_PARTITION_SIZE))
        {
            ret = 1;
        }
    }

    return ret;
}

int ezlopi_factory_info_v3_scenes_factory_soft_reset(void)
{
    int ret = 0;
    if (ezlopi_factory_info_v3_set_wifi("ezlopitest", "ezlopitest"))
    {
        ret = 1;
    }
    else
    {
        TRACE_E("Could not activate factory/soft reset");
    }
    return ret;
}

char *ezlopi_factory_info_v3_write_null(const char * name, e_ezlopi_factory_info_v3_offset_t offset, e_ezlopi_factory_info_v3_length_t length)
{
    char *write_string=NULL;
    if (ezlopi_factory_info_v3_init())
    {
        char *tmp_buffer = (char *)ezlopi_malloc(name, length);
        if (tmp_buffer)
        {
            if (ESP_OK == esp_partition_write(partition_ctx_v3, offset, tmp_buffer, length))
            {
                int s_length = (strlen(tmp_buffer) < length) ? strlen(tmp_buffer) : length;
                write_string = (char*)ezlopi_malloc(name, s_length + 1);
                write_string = tmp_buffer;
                if (NULL != write_string)
                {
                    if (1) // (isprint(tmp_buffer[0]))
                    {
                        snprintf(write_string, s_length + 1, "%s", tmp_buffer);
                    }
                    else
                    {
                        ezlopi_free(__FUNCTION__, write_string);
                        write_string = NULL;
                    }
                }
                else
                {
                    TRACE_E("'write_string' malloc failed!");
                }
            }
            else
            {
                TRACE_E("Couldn't write 'NULL' to id-flash-region!");
            }
        }
    }

    return write_string;
}