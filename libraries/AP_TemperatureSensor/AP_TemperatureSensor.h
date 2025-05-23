/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "AP_TemperatureSensor_config.h"

#if AP_TEMPERATURE_SENSOR_ENABLED

#include "AP_TemperatureSensor_Params.h"

// declare backend class
class AP_TemperatureSensor_Backend;
class AP_TemperatureSensor_TSYS01;
class AP_TemperatureSensor_MCP9600;
class AP_TemperatureSensor_MAX31865;
class AP_TemperatureSensor_TSYS03;
class AP_TemperatureSensor_Analog;
class AP_TemperatureSensor_MLX90614;

class AP_TemperatureSensor
{
    friend class AP_TemperatureSensor_Backend;
    friend class AP_TemperatureSensor_TSYS01;
    friend class AP_TemperatureSensor_MCP9600;
    friend class AP_TemperatureSensor_MAX31865;
    friend class AP_TemperatureSensor_TSYS03;
    friend class AP_TemperatureSensor_Analog;
    friend class AP_TemperatureSensor_DroneCAN;
    friend class AP_TemperatureSensor_MLX90614;

public:

    // Constructor
    AP_TemperatureSensor();

    CLASS_NO_COPY(AP_TemperatureSensor);

    static AP_TemperatureSensor *get_singleton() { return _singleton; }

    // Return the number of temperature sensors instances
    uint8_t num_instances(void) const { return _num_instances; }

    // detect and initialise any available temperature sensors
    __INITFUNC__ void init();

    // Update the temperature for all temperature sensors
    void update();

    // return temperature from sensor - in degrees Celsius
    bool get_temperature(float &temp, const uint8_t instance = AP_TEMPERATURE_SENSOR_PRIMARY_INSTANCE) const;

    bool healthy(const uint8_t instance = AP_TEMPERATURE_SENSOR_PRIMARY_INSTANCE) const;

    // accessors to params
    AP_TemperatureSensor_Params::Type get_type(const uint8_t instance = AP_TEMPERATURE_SENSOR_PRIMARY_INSTANCE) const;
    AP_TemperatureSensor_Params::Source get_source(const uint8_t instance = AP_TEMPERATURE_SENSOR_PRIMARY_INSTANCE) const;
    int32_t get_source_id(const uint8_t instance = AP_TEMPERATURE_SENSOR_PRIMARY_INSTANCE) const;

    static const struct AP_Param::GroupInfo var_info[];
    static const struct AP_Param::GroupInfo *backend_var_info[AP_TEMPERATURE_SENSOR_MAX_INSTANCES];

protected:
    // parameters
    AP_TemperatureSensor_Params _params[AP_TEMPERATURE_SENSOR_MAX_INSTANCES];

private:
    static AP_TemperatureSensor *_singleton;

    // The TemperatureSensor_State structure is filled in by the backend driver
    struct TemperatureSensor_State {
        uint32_t    last_time_ms;              // time when the sensor was last read in milliseconds
        float       temperature;               // temperature (deg C)
        uint8_t     instance;                  // instance number
        const struct AP_Param::GroupInfo *var_info; 
    };

    TemperatureSensor_State _state[AP_TEMPERATURE_SENSOR_MAX_INSTANCES];
    AP_TemperatureSensor_Backend *drivers[AP_TEMPERATURE_SENSOR_MAX_INSTANCES];

    uint8_t     _num_instances;         // number of temperature sensors

#if HAL_LOGGING_ENABLED
    enum class LoggingType : uint8_t {
        All = 1,
        SourceNone = 2,
    };
    AP_Enum<LoggingType> _logging_type;
#endif

};

namespace AP {
    AP_TemperatureSensor &temperature_sensor();
};

#endif // AP_TEMPERATURE_SENSOR_ENABLED
