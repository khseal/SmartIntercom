import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

espaudio_ns = cg.esphome_ns.namespace("espaudio")
ESPAudio = espaudio_ns.class_("ESPAudio", cg.Component)
CONF_RATE_MULTIPLIER = "rate_multiplier"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESPAudio),
        cv.Optional(CONF_RATE_MULTIPLIER, default=1.0): cv.float_range(min=0.1, max=4.0),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_rate_multiplier(config[CONF_RATE_MULTIPLIER]))
