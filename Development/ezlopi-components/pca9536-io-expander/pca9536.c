/*
  This is a library written for the PCA9536 4-bit I2C I/O Expander
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/14733
  Written by Jim Lindblom @ SparkFun Electronics, May 4th, 2018
  The PCA9536 is a 4-bit I/O digital expander, which communicates via an I2C bus.
  The expander can read or write four separate I/O.
  
  This library handles the initialization of the PCA9536 and supports standard
  pinMode, digitalRead, and digitalWrite functions.
  https://github.com/sparkfunX/SparkFun_PCA9536_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.5
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pca9536.h>

static s_ezlopi_i2c_master_t *g_i2c_master_conf = NULL;

bool pca9536_init(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    g_i2c_master_conf = i2c_master_conf;
    return !ezlopi_i2c_master_init(i2c_master_conf); // ESP_OK to True
}

bool pca9536_begin()
{
    return pca9536_is_connected();
}

//Returns true if a device ack's at given address
bool pca9536_is_connected(void)
{
    uint8_t dummy_data;
    esp_err_t result = ezlopi_i2c_master_read_from_device(g_i2c_master_conf, &dummy_data, 1);
    
    return !result; // ESP_OK to True
}

PCA9536_error_t pca9536_pin_mode(uint8_t pin, uint8_t mode)
{
    PCA9536_error_t err;
    uint8_t cfgRegister = 0;

    if (pin > PCA9536_MAX_GPIO)
        return PCA9536_ERROR_UNDEFINED;

    err = pca9536_read_i2c_register(&cfgRegister, PCA9536_REGISTER_CONFIGURATION);
    if (err != PCA9536_ERROR_SUCCESS)
    {
        return err;
    }
    cfgRegister &= ~(1 << pin); // Clear pin bit
    if (mode == INPUT)          // Set the bit if it's being set to INPUT (opposite of Arduino)
    {
        cfgRegister |= (1 << pin);
    }
    return pca9536_write_i2c_register(cfgRegister, PCA9536_REGISTER_CONFIGURATION);
}

PCA9536_error_t pca9536_write(uint8_t pin, uint8_t value)
{
    PCA9536_error_t err;
    uint8_t outputRegister = 0;

    if (pin > PCA9536_MAX_GPIO)
        return PCA9536_ERROR_UNDEFINED;

    err = pca9536_read_i2c_register(&outputRegister, PCA9536_REGISTER_OUTPUT_PORT);
    if (err != PCA9536_ERROR_SUCCESS)
    {
        return err;
    }
    // TODO: Break out of here if it's already set correctly
    outputRegister &= ~(1 << pin); // Clear pin bit
    if (value == HIGH)             // Set the bit if it's being set to HIGH (opposite of Arduino)
    {
        outputRegister |= (1 << pin);
    }
    return pca9536_write_i2c_register(outputRegister, PCA9536_REGISTER_OUTPUT_PORT);
}

PCA9536_error_t pca9536_digital_write(uint8_t pin, uint8_t value)
{
    return pca9536_write(pin, value);
}

uint8_t pca9536_read_reg()
{
    PCA9536_error_t err;
    uint8_t inputRegister = 0;

    err = pca9536_read_i2c_register(&inputRegister, PCA9536_REGISTER_INPUT_PORT);
    if (err != PCA9536_ERROR_SUCCESS)
    {
        return err;
    }
    return (inputRegister & (0x0f));
}

uint8_t pca9536_read(uint8_t pin)
{
    PCA9536_error_t err;
    uint8_t inputRegister = 0;

    if (pin > PCA9536_MAX_GPIO)
        return PCA9536_ERROR_UNDEFINED;

    err = pca9536_read_i2c_register(&inputRegister, PCA9536_REGISTER_INPUT_PORT);
    if (err != PCA9536_ERROR_SUCCESS)
    {
        return err;
    }
    return (inputRegister & (1 << pin)) >> pin;
}

uint8_t pca9536_digital_read(uint8_t pin)
{
    return pca9536_read(pin);
}

PCA9536_error_t pca9536_invert(uint8_t pin, PCA9536_invert_t inversion)
{
    PCA9536_error_t err;
    uint8_t invertRegister = 0;

    if (pin > PCA9536_MAX_GPIO)
        return PCA9536_ERROR_UNDEFINED;

    err = pca9536_read_i2c_register(&invertRegister, PCA9536_REGISTER_POLARITY_INVERSION);
    if (err != PCA9536_ERROR_SUCCESS)
    {
        return err;
    }
    // TODO: Break out of here if it's already set correctly
    invertRegister &= ~(1 << pin);   // Clear pin bit
    if (inversion == PCA9536_INVERT) // Set the bit if it's being set to inverted
    {
        invertRegister |= (1 << pin);
    }
    return pca9536_write_i2c_register(invertRegister, PCA9536_REGISTER_POLARITY_INVERSION);
}

PCA9536_error_t pca9536_revert(uint8_t pin)
{
    return pca9536_invert(pin, PCA9536_RETAIN);
}

PCA9536_error_t pca9536_read_i2c_buffer(uint8_t *dest, PCA9536_REGISTER_t startRegister, uint16_t len)
{
    // PCA9536_DEBUGLN((STORAGE("(readI2CBuffer): read ") + String(len) +
    //                  STORAGE(" @ 0x") + String(startRegister, HEX)));
    if (g_i2c_master_conf->address == PCA9536_ADDRESS_INVALID)
    {
        // PCA9536_DEBUGLN(STORAGE("    ERR (readI2CBuffer): Invalid address"));
        return PCA9536_ERROR_INVALID_ADDRESS;
    }

    esp_err_t ret =  ezlopi_i2c_master_write_to_device(g_i2c_master_conf, (uint8_t*) &startRegister, 1);
    // _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    // _i2cPort->write(startRegister);
    if (ret != ESP_OK)
    {
        // PCA9536_DEBUGLN(STORAGE("    ERR (readI2CBuffer): End transmission"));
        return PCA9536_ERROR_READ;
    }

    ret = ezlopi_i2c_master_read_from_device(g_i2c_master_conf, dest, len);
    // _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)len);
    // for (int i = 0; i < len; i++)
    // {
    //     dest[i] = _i2cPort->read();
    //     // PCA9536_DEBUGLN((STORAGE("    ") + String(i) + STORAGE(": 0x") + String(dest[i], HEX)));
    // }

    return PCA9536_ERROR_SUCCESS;
}

PCA9536_error_t pca9536_write_i2c_buffer(uint8_t *src, PCA9536_REGISTER_t startRegister, uint16_t len)
{
    if (g_i2c_master_conf->address == PCA9536_ADDRESS_INVALID)
    {
        // PCA9536_DEBUGLN(STORAGE("ERR (readI2CBuffer): Invalid address"));
        return PCA9536_ERROR_INVALID_ADDRESS;
    }
    uint8_t* buffer = malloc(len + 1);
    buffer[0] = startRegister;
    for (size_t i = 0; i < len; i++)
    {
        buffer[i+1] = src[i];
    }
    
    esp_err_t ret =  ezlopi_i2c_master_write_to_device(g_i2c_master_conf, buffer, len + 1);
    if (ret != ESP_OK)
    {
        return PCA9536_ERROR_WRITE;
    }

    // _i2cPort->beginTransmission((uint8_t)_deviceAddress);
    // _i2cPort->write(startRegister);
    // for (int i = 0; i < len; i++)
    // {
    //     _i2cPort->write(src[i]);
    // }
    // if (_i2cPort->endTransmission(true) != 0)
    // {
    //     return PCA9536_ERROR_WRITE;
    // }
    return PCA9536_ERROR_SUCCESS;
}

PCA9536_error_t pca9536_read_i2c_register(uint8_t *dest, PCA9536_REGISTER_t registerAddress)
{
    return pca9536_read_i2c_buffer(dest, registerAddress, 1);
}

PCA9536_error_t pca9536_write_i2c_register(uint8_t data, PCA9536_REGISTER_t registerAddress)
{
    return pca9536_write_i2c_buffer(&data, registerAddress, 1);
}
