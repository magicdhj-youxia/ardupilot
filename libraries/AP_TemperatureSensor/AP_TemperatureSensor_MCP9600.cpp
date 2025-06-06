/*
 * Copyright (C) 2022  Kraus Hamdani Aerospace Inc. All rights reserved.
 *
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Code by Tom Pittenger
 */
/*
    Implements I2C driver for Microchip MCP9600 and MCP9601 digital thermocouple EMF to Temperature converter
*/

#include "AP_TemperatureSensor_MCP9600.h"

#if AP_TEMPERATURE_SENSOR_MCP9600_ENABLED

#include <stdio.h>
#include <AP_HAL/I2CDevice.h>
#include <AP_Math/AP_Math.h>
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL &hal;

static const uint8_t MCP9600_CMD_HOT_JUNCT_TEMP     = 0x00;     // thermocoupler temp
//static const uint8_t MCP9600_CMD_JUNCT_TEMP_DELTA   = 0x01;
//static const uint8_t MCP9600_CMD_COLD_JUNCT_TEMP    = 0x02;     // ambient temp
//static const uint8_t MCP9600_CMD_RAW_DATA_ADC       = 0x03;
static const uint8_t MCP9600_CMD_STATUS             = 0x04;
static const uint8_t MCP9600_CMD_SENSOR_CFG         = 0x05;
//static const uint8_t MCP9600_CMD_DEVICE_CFG         = 0x06;
static const uint8_t MCP9600_CMD_DEVICE_ID_REV      = 0x20;     // to fetch WHOAMI

static const uint8_t MCP9600_CMD_STATUS_UPDATE_READY = 0x40;

static const uint8_t MCP9600_WHOAMI                 = 0x40;
static const uint8_t MCP9601_WHOAMI                 = 0x41;


#define MCP9600_ADDR_LOW        0x60    // ADDR pin pulled low
#define MCP9600_ADDR_HIGH       0x67    // ADDR pin pulled high

#define AP_TemperatureSensor_MCP9600_UPDATE_INTERVAL_MS     500
#define AP_TemperatureSensor_MCP9600_SCALE_FACTOR           (0.0625f)

#ifndef AP_TemperatureSensor_MCP9600_ADDR_DEFAULT
    #define AP_TemperatureSensor_MCP9600_ADDR_DEFAULT    MCP9600_ADDR_LOW
#endif

#ifndef AP_TemperatureSensor_MCP9600_Filter
    #define AP_TemperatureSensor_MCP9600_Filter         2   // can be values 0 through 7 where 0 is no filtering (fast) and 7 is lots of smoothing (very very slow)
#endif

void AP_TemperatureSensor_MCP9600::init()
{
    constexpr char name[] = "MCP9600";

    _dev = hal.i2c_mgr->get_device_ptr(_params.bus, _params.bus_address);
    if (!_dev) {
        // device not found
        return;
    }

    WITH_SEMAPHORE(_dev->get_semaphore());

    _dev->set_retries(10);

    uint8_t buf;
    if (!_dev->read_registers(MCP9600_CMD_DEVICE_ID_REV, &buf, 1)) {
        printf("%s failed to get WHOAMI", name);
        return;
    }
    if (buf != MCP9600_WHOAMI && buf != MCP9601_WHOAMI) {
        printf("%s Got wrong WHOAMI: detected 0x%2X", name, (unsigned)buf);
        return;
    }
    if (!set_config(ThermocoupleType::K, AP_TemperatureSensor_MCP9600_Filter)) {
        printf("%s unable to configure", name);
        return;
    }

    // lower retries for run
    _dev->set_retries(3);

    /* Request 5Hz update */
    _dev->register_periodic_callback(200 * AP_USEC_PER_MSEC,
                                     FUNCTOR_BIND_MEMBER(&AP_TemperatureSensor_MCP9600::_timer, void));
}

void AP_TemperatureSensor_MCP9600::_timer()
{
    float temperature;
    if (read_temperature(temperature)) {
        set_temperature(temperature);
    }
}

bool AP_TemperatureSensor_MCP9600::set_config(const ThermocoupleType thermoType, const uint8_t filter)
{
    const uint8_t data = (uint8_t)thermoType << 4 | (filter & 0x7);
    if (!_dev->write_register(MCP9600_CMD_SENSOR_CFG, data)) {
        return false;
    }
    return true;
}

bool AP_TemperatureSensor_MCP9600::read_temperature(float &temperature)
{
    // confirm new temperature is available and then read it

    // First, read STATUS register:
    uint8_t data[2];
    if (!_dev->read_registers(MCP9600_CMD_STATUS, data, 1)) {
        return false;
    }
    // check UPDATE bit for new temperature data ready:
    if ((data[0] & MCP9600_CMD_STATUS_UPDATE_READY) == 0) {
        return false;
    }
    // read temperature:
    if (!_dev->read_registers(MCP9600_CMD_HOT_JUNCT_TEMP, data, 2)) {
        return false;
    }
    // clear update bit:
    if (!_dev->write_register(0x04, data[0] & ~MCP9600_CMD_STATUS_UPDATE_READY)) {
        return false;
    }

    // scale temperature:
    temperature = int16_t(UINT16_VALUE(data[0], data[1])) * AP_TemperatureSensor_MCP9600_SCALE_FACTOR;

    return true;
}
#endif // AP_TEMPERATURE_SENSOR_MCP9600_ENABLED

