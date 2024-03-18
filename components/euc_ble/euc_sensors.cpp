#include "euc_sensors.h"

#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"


#ifdef USE_ESP32

namespace esphome {
namespace euc_ble {

static const char *const TAG = "euc_sensors";

void EUCSensors::loop() {}

void EUCSensors::dump_config() {
  ESP_LOGCONFIG(TAG, "  MAC address        : %s", this->parent()->address_str().c_str());
  ESP_LOGCONFIG(TAG, "  Service UUID       : %s", this->decoder_->get_service_uuid().to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Characteristic UUID: %s", this->decoder_->get_char_uuid().to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Notifications      : %s", YESNO(this->notify_));
  LOG_UPDATE_INTERVAL(this);

  for (StatType t = StatType::_START; t != StatType::_END; ++t) {
    const char* name = STAT_NAMES[static_cast<uint8_t>(t)];
    name = name != nullptr ? name : "";
    if (this->sensors_[t] != nullptr) {
        LOG_SENSOR("", name, this->sensors_[t]);
    }
  }
}


void EUCSensors::set_type(const std::string& type_) {
    if (type_ == "veteran") {
        this->decoder_ = new VeteranDecoder();
    }
}

void EUCSensors::clear_sensors() {
  if (this->retain_on_disconnect_) {
    return;
  }
  for (StatType t = StatType::_START; t != StatType::_END; ++t) {
    if (this->sensors_[t] != nullptr) {
        this->sensors_[t]->publish_state(NAN);
    }
  }
}

void EUCSensors::update_sensors(const uint8_t *data, uint16_t length)
{
    if (this->decoder_ == nullptr) {
        return;
    }
    if (!this->decoder_->decode(data, length)) {
        return;
    }
    for (StatType t = StatType::_START; t != StatType::_END; ++t) {
        auto stat = this->decoder_->get_stat(t);
        if (this->sensors_[t] != nullptr && stat) {
            this->sensors_[t]->publish_state(*stat);
        }
    }
}

void EUCSensors::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                    esp_ble_gattc_cb_param_t *param) {

    if (this->decoder_ == nullptr) {
        return;
    }
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "[%s] Connected successfully!", this->decoder_->get_name().c_str());
        break;
      }
      break;
    }
    case ESP_GATTC_CLOSE_EVT: {
      ESP_LOGW(TAG, "[%s] Disconnected!", this->decoder_->get_name().c_str());
      this->status_set_warning();
      this->clear_sensors();
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      this->handle = 0;
      auto *chr = this->parent()->get_characteristic(this->decoder_->get_service_uuid(), this->decoder_->get_char_uuid());
      if (chr == nullptr) {
        this->status_set_warning();
        this->clear_sensors();
        ESP_LOGW(TAG, "No sensor characteristic found at service %s char %s", this->decoder_->get_service_uuid().to_string().c_str(),
                 this->decoder_->get_char_uuid().to_string().c_str());
        break;
      }
      this->handle = chr->handle;
      if (this->notify_) {
        auto status = esp_ble_gattc_register_for_notify(this->parent()->get_gattc_if(),
                                                        this->parent()->get_remote_bda(), chr->handle);
        if (status) {
          ESP_LOGW(TAG, "esp_ble_gattc_register_for_notify failed, status=%d", status);
        }
      } else {
        this->node_state = espbt::ClientState::ESTABLISHED;
      }
      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      if (param->read.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Error reading char at handle %d, status=%d", param->read.handle, param->read.status);
        break;
      }
      if (param->read.handle == this->handle) {
        this->status_clear_warning();
        this->update_sensors(param->read.value, param->read.value_len);
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGD(TAG, "[%s] ESP_GATTC_NOTIFY_EVT: handle=0x%x, value=0x%x", this->decoder_->get_name().c_str(),
               param->notify.handle, param->notify.value[0]);
      if (param->notify.handle != this->handle) {
        break;
      }
      this->update_sensors(param->notify.value, param->notify.value_len);
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (param->reg_for_notify.handle == this->handle) {
        if (param->reg_for_notify.status != ESP_GATT_OK) {
          ESP_LOGW(TAG, "Error registering for notifications at handle %d, status=%d", param->reg_for_notify.handle,
                   param->reg_for_notify.status);
          break;
        }
        this->node_state = espbt::ClientState::ESTABLISHED;
        ESP_LOGD(TAG, "Register for notify on %s complete", this->decoder_->get_char_uuid().to_string().c_str());
      }
      break;
    }
    default:
      break;
  }
}

void EUCSensors::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] Cannot poll, not connected", this->decoder_->get_name().c_str());
    return;
  }
  if (this->handle == 0) {
    ESP_LOGW(TAG, "[%s] Cannot poll, no service or characteristic found", this->decoder_->get_name().c_str());
    return;
  }

  auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->handle,
                                        ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    this->status_set_warning();
    this->clear_sensors();
    ESP_LOGW(TAG, "[%s] Error sending read request for sensor, status=%d", this->decoder_->get_name().c_str(), status);
  }
}


}  // namespace euc_ble
}  // namespace esphome

#endif