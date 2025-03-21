#include "filebrowser_sd_component.h"
#include "esphome/core/log.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

namespace esphome {
namespace filebrowser_sd {

static const char *TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  // La carte SD est déjà montée par le composant sd_mmc_card, donc on ne fait pas de montage ici
  ESP_LOGCONFIG(TAG, "FileBrowser SD Component initialized");
  ESP_LOGCONFIG(TAG, "  Base Path: %s", this->base_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Max Files: %d", this->max_files_);
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser SD Component:");
  ESP_LOGCONFIG(TAG, "  Base Path: %s", this->base_path_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Max Files: %d", this->max_files_);
  ESP_LOGCONFIG(TAG, "  Format If Mount Failed: %s", YESNO(this->format_if_mount_failed_));
}

bool FileBrowserSDComponent::list_dir(const std::string &path) {
  DIR *dir = nullptr;
  struct dirent *entry = nullptr;
  struct stat stat_buf;
  std::string full_path = this->mount_point_ + path;
  
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
  std::string full_path = this->mount_point_ + path;
  
  ESP_LOGI(TAG, "Creating directory: %s", full_path.c_str());
  
  if (mkdir(full_path.c_str(), 0755) != 0) {
    ESP_LOGE(TAG, "Failed to create directory: %s", full_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::delete_file(const std::string &path) {
  std::string full_path = this->mount_point_ + path;
  
  ESP_LOGI(TAG, "Deleting file: %s", full_path.c_str());
  
  if (unlink(full_path.c_str()) != 0) {
    ESP_LOGE(TAG, "Failed to delete file: %s", full_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::rename_file(const std::string &old_path, const std::string &new_path) {
  std::string full_old_path = this->mount_point_ + old_path;
  std::string full_new_path = this->mount_point_ + new_path;
  
  ESP_LOGI(TAG, "Renaming file: %s -> %s", full_old_path.c_str(), full_new_path.c_str());
  
  if (rename(full_old_path.c_str(), full_new_path.c_str()) != 0) {
    ESP_LOGE(TAG, "Failed to rename file: %s -> %s", full_old_path.c_str(), full_new_path.c_str());
    return false;
  }
  
  return true;
}

bool FileBrowserSDComponent::get_file_info(const std::string &path) {
  std::string full_path = this->mount_point_ + path;
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
