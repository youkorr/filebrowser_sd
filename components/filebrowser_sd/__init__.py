import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ['sd_mmc_card']
AUTO_LOAD = ['sd_mmc_card']

filebrowser_sd_ns = cg.esphome_ns.namespace('filebrowser_sd')
FileBrowserSDComponent = filebrowser_sd_ns.class_('FileBrowserSDComponent', cg.Component)

# Constantes pour la configuration
CONF_MOUNT_POINT = 'mount_point'
CONF_ADDRESS_IP = 'adress_ip'
CONF_PORT = 'port'
CONF_ROOT_PATH = 'root_path'
CONF_USERNAME = 'username'
CONF_PASSWORD = 'password'

# Configuration du composant
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FileBrowserSDComponent),
    cv.Required(CONF_MOUNT_POINT): cv.string,
    cv.Optional(CONF_ADDRESS_IP): cv.string,
    cv.Optional(CONF_PORT): cv.port,
    cv.Optional(CONF_ROOT_PATH, default="/"): cv.string,
    cv.Optional(CONF_USERNAME, default="admin"): cv.string,
    cv.Optional(CONF_PASSWORD): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Configurer les options
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
    if CONF_ADDRESS_IP in config:
        cg.add(var.set_address_ip(config[CONF_ADDRESS_IP]))
    if CONF_PORT in config:
        cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_root_path(config[CONF_ROOT_PATH]))
    cg.add(var.set_username(config[CONF_USERNAME]))
    if CONF_PASSWORD in config:
        cg.add(var.set_password(config[CONF_PASSWORD]))
