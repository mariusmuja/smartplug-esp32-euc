import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, ble_client
from esphome.const import (
    CONF_ID,
    CONF_VOLTAGE,
    CONF_TEMPERATURE,
    CONF_CURRENT,
    CONF_TYPE,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_TEMPERATURE,
    ICON_BATTERY,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_KILOMETER,
    UNIT_CELSIUS,
)


# AUTO_LOAD = ["esp32_ble_client"]
DEPENDENCIES = ["ble_client"]

euc_ble_ns = cg.esphome_ns.namespace("euc_ble")
EUCSensors = euc_ble_ns.class_(
    "EUCSensors", ble_client.BLEClientNode, cg.PollingComponent
)
StatType = euc_ble_ns.enum("StatType", is_class=True)

CONF_ODOMETER = "odometer"
CONF_TRIP = "trip"
CONF_NOTIFY = "notify"
CONF_RETAIN = "retain_on_disconnect"

ICON_TEMPERATURE_CELSIUS = "mdi:temperature-celsius"
ICON_CURRENT_DC="mdi:current-dc"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EUCSensors),
            cv.Optional(CONF_NOTIFY, default=True): cv.boolean,
            cv.Optional(CONF_TYPE, default="veteran"): cv.one_of("veteran", lower=True),
            cv.Optional(CONF_RETAIN, default=True): cv.boolean,
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_BATTERY,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ODOMETER): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOMETER,
                icon=ICON_COUNTER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_DISTANCE,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_TRIP): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOMETER,
                icon=ICON_COUNTER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_DISTANCE,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_TEMPERATURE_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_CURRENT_DC,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    cg.add(var.set_enable_notify(config[CONF_NOTIFY]))
    cg.add(var.set_retain_on_disconnect(config[CONF_RETAIN]))
    cg.add(var.set_type(config[CONF_TYPE]))
    
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_sensor(StatType.VOLTAGE, sens))
    if CONF_ODOMETER in config:
        sens = await sensor.new_sensor(config[CONF_ODOMETER])
        cg.add(var.set_sensor(StatType.ODOMETER, sens))
    if CONF_TRIP in config:
        sens = await sensor.new_sensor(config[CONF_TRIP])
        cg.add(var.set_sensor(StatType.TRIP, sens))
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_sensor(StatType.TEMPERATURE, sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_sensor(StatType.CURRENT, sens))

