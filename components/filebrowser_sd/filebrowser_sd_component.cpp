#include "filebrowser_sd_component.h"
#include "esphome/core/log.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "esp_wifi.h"  // Pour gérer le Wi-Fi en mode ESP-IDF

namespace esphome {
namespace filebrowser_sd {

static const char *TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  // La carte SD est déjà montée par le composant sd_mmc_card, donc on ne fait pas de montage ici
  ESP_LOGCONFIG(TAG, "FileBrowser SD Component initialized");
  ESP_LOGCONFIG(TAG, "  Base Path: %s", this->base_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Address IP: %s", this->address_ip_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
  ESP_LOGCONFIG(TAG, "  Root Path: %s", this->root_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
  ESP_LOGCONFIG(TAG, "  Password: [hidden]");
  ESP_LOGCONFIG(TAG, "  Max Files: %d", this->max_files_);
  ESP_LOGCONFIG(TAG, "  Format If Mount Failed: %s", YESNO(this->format_if_mount_failed_));

  // Démarrer le serveur web si l'adresse IP et le port sont configurés
  if (!this->address_ip_.empty() && this->port_ != 0) {
    this->start_wifi_and_web_server();
  }
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser SD Component:");
  ESP_LOGCONFIG(TAG, "  Base Path: %s", this->base_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Address IP: %s", this->address_ip_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
  ESP_LOGCONFIG(TAG, "  Root Path: %s", this->root_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
  ESP_LOGCONFIG(TAG, "  Password: [hidden]");
  ESP_LOGCONFIG(TAG, "  Max Files: %d", this->max_files_);
  ESP_LOGCONFIG(TAG, "  Format If Mount Failed: %s", YESNO(this->format_if_mount_failed_));
}

bool FileBrowserSDComponent::authenticate(const std::string &username, const std::string &password) {
  if (this->username_.empty() && this->password_.empty()) {
    // Pas d'authentification requise
    return true;
  }
  return (username == this->username_ && password == this->password_);
}

void FileBrowserSDComponent::start_wifi_and_web_server() {
  if (this->address_ip_.empty() || this->port_ == 0) {
    ESP_LOGE(TAG, "Address IP or port not configured");
    return;
  }

  // Initialisation du Wi-Fi en mode ESP-IDF
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t wifi_config = {};
  strncpy((char *)wifi_config.sta.ssid, "your-ssid", sizeof(wifi_config.sta.ssid));  // Remplacez par votre SSID
  strncpy((char *)wifi_config.sta.password, "your-password", sizeof(wifi_config.sta.password));  // Remplacez par votre mot de passe

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "Connecting to Wi-Fi...");
  esp_wifi_connect();

  // Attendre que la connexion Wi-Fi soit établie
  while (true) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      ESP_LOGI(TAG, "Connected to Wi-Fi");
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(TAG, "Web server started at http://%s:%d", this->address_ip_.c_str(), this->port_);

  // Démarrer le serveur web
  // (Vous devrez implémenter cette partie en fonction de votre besoin)
  // Exemple : utiliser un serveur HTTP ou FTP
}

bool FileBrowserSDComponent::list_dir(const std::string &path) {
  DIR *dir = nullptr;
  struct dirent *entry = nullptr;
  struct stat stat_buf;
  std::string full_path = this->mount_point_ + this->root_path_ + path;
  
  ESP_LOGI(TAG, "Listing directory: %s", full_path.c_str());
  
  dir = opendir(full_path.c_str());
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open directory: %s", full_path.c_str());
    return false;
  }
  
  while ((entry = readdir(dir)) != nullptr) {
    std::string entry_path = full_path + "/" + entry->d_name;
    
    if (stat(entry_path.c_str(), &stat_buf) == 0) {
      if (S_ISDIR(stat_buf.st_mode)) {
        ESP_LOGI(TAG, "[DIR] %s", entry->d_name);
      } else {
        ESP_LOGI(TAG, "[FILE] %s (%ld bytes)", entry->d_name, stat_buf.st_size);
      }
    } else {
      ESP_LOGE(TAG, "Failed to get stats for %s", entry_path.c_str());
    }
  }
  
  closedir(dir);
  return true;
}

bool FileBrowserSDComponent::create_dir(const std::string &path) {
  std::string full_path = this->mount_point_ + this->root_path_ + path;
  
  ESP_LOGI(TAG, "Creating directory: %s", full_path.c_str());
  
  if (mkdir(full_path.c_str(), 0755) != 0) {
    ESP_LOGE(TAG, "Failed to create directory: %s", full_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::delete_file(const std::string &path) {
  std::string full_path = this->mount_point_ + this->root_path_ + path;
  
  ESP_LOGI(TAG, "Deleting file: %s", full_path.c_str());
  
  if (unlink(full_path.c_str()) != 0) {
    ESP_LOGE(TAG, "Failed to delete file: %s", full_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::rename_file(const std::string &old_path, const std::string &new_path) {
  std::string full_old_path = this->mount_point_ + this->root_path_ + old_path;
  std::string full_new_path = this->mount_point_ + this->root_path_ + new_path;
  
  ESP_LOGI(TAG, "Renaming file: %s -> %s", full_old_path.c_str(), full_new_path.c_str());
  
  if (rename(full_old_path.c_str(), full_new_path.c_str()) != 0) {
    ESP_LOGE(TAG, "Failed to rename file: %s -> %s", full_old_path.c_str(), full_new_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::get_file_info(const std::string &path) {
  std::string full_path = this->mount_point_ + this->root_path_ + path;
  struct stat stat_buf;
  
  ESP_LOGI(TAG, "Getting file info: %s", full_path.c_str());
  
  if (stat(full_path.c_str(), &stat_buf) != 0) {
    ESP_LOGE(TAG, "Failed to get file info: %s", full_path.c_str());
    return false;
  }
  
  if (S_ISDIR(stat_buf.st_mode)) {
    ESP_LOGI(TAG, "Type: Directory");
  } else {
    ESP_LOGI(TAG, "Type: File");
    ESP_LOGI(TAG, "Size: %ld bytes", stat_buf.st_size);
    ESP_LOGI(TAG, "Modified time: %ld", stat_buf.st_mtime);
  }
  
  return true;
}

}  // namespace filebrowser_sd
}  // namespace esphome
