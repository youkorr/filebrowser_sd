import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_URL, CONF_USERNAME, CONF_PASSWORD

DEPENDENCIES = ['sd_mmc_card']
AUTO_LOAD = ['sd_mmc_card']

CONF_MOUNT_POINT = 'mount_point'

filebrowser_sd_ns = cg.esphome_ns.namespace('filebrowser_sd')
FileBrowserSDComponent = filebrowser_sd_ns.class_('FileBrowserSDComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FileBrowserSDComponent),
    cv.Required(CONF_URL): cv.string,
    cv.Required(CONF_USERNAME): cv.string,
    cv.Required(CONF_PASSWORD): cv.string,
    cv.Optional(CONF_MOUNT_POINT, default='/sdcard'): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_base_url(config[CONF_URL]))
    cg.add(var.set_username(config[CONF_USERNAME]))
    cg.add(var.set_password(config[CONF_PASSWORD]))
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
