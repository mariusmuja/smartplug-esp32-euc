#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"


#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome {
namespace euc_ble {

namespace espbt = esphome::esp32_ble_tracker;


enum class StatType: uint8_t {
    _START = 0,
    VOLTAGE = _START,
    ODOMETER,
    TRIP,
    TEMPERATURE,
    CURRENT,
    _END,
}; // Don't forget to update STAT_NAMES below if adding additional stat types

static const uint8_t STATS_NUM = static_cast<std::underlying_type<StatType>::type>(StatType::_END);
static const char* STAT_NAMES[STATS_NUM] = { "Voltage", "Odometer", "Trip", "Temperature", "Current" };

inline StatType& operator++ (StatType& e)
{
    if (e != StatType::_END) {
        e = StatType(static_cast<std::underlying_type<StatType>::type>(e) + 1);
    }
    return e;
}

class Stats {
public:
    optional<float> operator[](StatType t) const { return std::move(this->stats_[static_cast<uint8_t>(t)]); }
    optional<float>& operator[](StatType t) { return this->stats_[static_cast<uint8_t>(t)]; }
protected:   
    optional<float> stats_[STATS_NUM] { nullopt };
};


class Sensors {
public:
    const sensor::Sensor* operator[](StatType t) const { return this->sensors_[static_cast<uint8_t>(t)]; }
    sensor::Sensor*& operator[](StatType t) { return this->sensors_[static_cast<uint8_t>(t)]; }
protected:   
    sensor::Sensor* sensors_[STATS_NUM] { nullptr };
};

class EUCDecoder {
public:
    virtual bool decode(const uint8_t *data, uint16_t length) = 0;
    virtual espbt::ESPBTUUID get_service_uuid() = 0;
    virtual espbt::ESPBTUUID get_char_uuid() = 0;
    virtual std::string get_name() = 0;

    optional<float> get_stat(StatType t) { return this->stats_[t]; }

protected:
    Stats stats_;
};


static const uint16_t VETERAN_SERVICE_UUID = 0xFFE0;
static const uint16_t VETERAN_CHAR_UUID = 0xFFE1;

class VeteranDecoder : public EUCDecoder {
public:
    bool decode(const uint8_t *data, uint16_t length) {
        
        if (length==20) {
            ESP_LOGD("euc_decoder", 
                "packet length: %d [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]", 
                length, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
                data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19]
            );
        }
        else if (length==11) {
            ESP_LOGD("euc_decoder", 
                "packet length: %d [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]", 
                length, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
                data[10]
            );
        }
        else if (length==14) {
            ESP_LOGD("euc_decoder", 
                "packet length: %d [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]", 
                length, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
                 data[10], data[11], data[12], data[13]
            );
        }
        else if (length==15) {
            ESP_LOGD("euc_decoder", 
                "packet length: %d [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]", 
                length, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9],
                 data[10], data[11], data[12], data[13], data[14]
            );
        }
        else {
            ESP_LOGD("euc_decoder", "packet length: %d [%02X %02X %02X ... ]", length, data[0], data[1], data[2]);
        }

        if (length<20) return false;
        if (!(data[0]==0xDC && data[1]==0x5A && data[2] == 0x5C)) return false;

        this->stats_[StatType::VOLTAGE] = (data[4] << 8 | data[5]) / 100.0;
        this->stats_[StatType::ODOMETER] = (data[14] << 24 | data[15] << 16 | data[12] << 8 | data[13]) / 1000.0;
        this->stats_[StatType::TRIP] = (data[10] << 24 | data[11] << 16 | data[8] << 8 | data[9]) / 1000.0;
        this->stats_[StatType::TEMPERATURE] = (data[18] << 8 | data[19]) / 100.0;
        this->stats_[StatType::CURRENT] = static_cast<int16_t>(data[16] << 8 | data[17]) / 10.0;

        return true;
    }

    espbt::ESPBTUUID get_service_uuid() { return this-> service_uuid_; }
    espbt::ESPBTUUID get_char_uuid() { return this ->char_uuid_; }
    std::string get_name() { return "veteran"; }

protected:
    const espbt::ESPBTUUID service_uuid_ { espbt::ESPBTUUID::from_uint16(VETERAN_SERVICE_UUID) };
    const espbt::ESPBTUUID char_uuid_ { espbt::ESPBTUUID::from_uint16(VETERAN_CHAR_UUID) };
};


class EUCSensors : public esphome::ble_client::BLEClientNode, public PollingComponent {
public:    
    void loop() override;
    void update() override;

    void dump_config() override;

    void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

    void set_sensor(StatType t, sensor::Sensor *sensor) { this->sensors_[t] = sensor; }
    void set_enable_notify(bool notify) { this->notify_ = notify; }
    void set_type(const std::string& type_);
    void set_retain_on_disconnect(bool retain_on_disconnect) { this->retain_on_disconnect_ = retain_on_disconnect; }

    void clear_sensors();
    void update_sensors(const uint8_t *data, uint16_t length);

protected:
    uint16_t handle;
    bool notify_ { true };
    bool retain_on_disconnect_ { true };
    EUCDecoder* decoder_ {nullptr};
    Sensors sensors_;
};


}  // namespace euc_ble
}  // namespace esphome

#endif