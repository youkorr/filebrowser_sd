import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ['sd_mmc_card']
AUTO_LOAD = ['sd_mmc_card']

filebrowser_sd_ns = cg.esphome_ns.namespace('filebrowser_sd')
FileBrowserSDComponent = filebrowser_sd_ns.class_('FileBrowserSDComponent', cg.Component)

# Constantes pour la configuration
CONF_BASE_PATH = 'base_path'
CONF_MOUNT_POINT = 'mount_point'
CONF_MAX_FILES = 'max_files'
CONF_FORMAT_IF_MOUNT_FAILED = 'format_if_mount_failed'

# Configuration du composant
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FileBrowserSDComponent),
    cv.Optional(CONF_BASE_PATH, default='/filebrowser'): cv.string,
    cv.Optional(CONF_MOUNT_POINT, default='/sdcard'): cv.string,
    cv.Optional(CONF_MAX_FILES, default=5): cv.int_range(min=1, max=20),
    cv.Optional(CONF_FORMAT_IF_MOUNT_FAILED, default=False): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Configurer les options
    cg.add(var.set_base_path(config[CONF_BASE_PATH]))
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
    cg.add(var.set_max_files(config[CONF_MAX_FILES]))
    cg.add(var.set_format_if_mount_failed(config[CONF_FORMAT_IF_MOUNT_FAILED]))
