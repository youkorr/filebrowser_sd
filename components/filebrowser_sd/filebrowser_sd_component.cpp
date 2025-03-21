#include "filebrowser_sd.h"
#include "esphome/core/log.h"
#include <dirent.h>
#include <sys/stat.h>

namespace esphome {
namespace filebrowser_sd {

static const char *const TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up FileBrowser SD WebDAV Client...");
  
  this->client_ = new http_client::HTTPClient();
  this->client_->set_timeout(20000); // 20 seconds timeout
  
  // Configuration de base auth pour WebDAV
  if (!this->username_.empty() && !this->password_.empty()) {
    String auth = this->username_.c_str();
    auth += ":";
    auth += this->password_.c_str();
    this->client_->set_auth_header("Basic " + base64::encode(auth));
  }
  
  ESP_LOGCONFIG(TAG, "WebDAV client initialized");
}

void FileBrowserSDComponent::sync_to_filebrowser() {
  ESP_LOGI(TAG, "Starting sync to Filebrowser...");
  
  DIR *dir = opendir(this->mount_point_.c_str());
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open local directory");
    return;
  }
  
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_REG) { // Regular file
      std::string local_path = this->mount_point_ + "/" + entry->d_name;
      std::string remote_path = this->webdav_url_ + "/" + entry->d_name;
      upload_file(local_path, remote_path);
    }
  }
  
  closedir(dir);
  ESP_LOGI(TAG, "Sync to Filebrowser completed");
}

void FileBrowserSDComponent::sync_from_filebrowser() {
  ESP_LOGI(TAG, "Starting sync from Filebrowser...");
  list_remote_files();
}

void FileBrowserSDComponent::upload_file(const std::string &local_path, const std::string &remote_path) {
  ESP_LOGI(TAG, "Uploading %s to %s", local_path.c_str(), remote_path.c_str());
  
  FILE *file = fopen(local_path.c_str(), "r");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open local file for upload");
    return;
  }
  
  // Lecture du fichier
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  uint8_t *buffer = new uint8_t[size];
  fread(buffer, 1, size, file);
  fclose(file);
  
  // Envoi via WebDAV PUT
  this->client_->set_method(http_client::HTTP_METHOD_PUT);
  this->client_->set_url(remote_path);
  this->client_->set_body(buffer, size);
  this->client_->send();
  
  delete[] buffer;
}

void FileBrowserSDComponent::download_file(const std::string &remote_path, const std::string &local_path) {
  ESP_LOGI(TAG, "Downloading %s to %s", remote_path.c_str(), local_path.c_str());
  
  this->client_->set_method(http_client::HTTP_METHOD_GET);
  this->client_->set_url(remote_path);
  
  this->client_->on_response([this, local_path](http_client::HTTPClient &client, 
                                              http_client::HTTPClientResponse response) {
    if (response.code == 200) {
      FILE *file = fopen(local_path.c_str(), "w");
      if (file) {
        fwrite(response.body.data(), 1, response.body.size(), file);
        fclose(file);
        ESP_LOGI(TAG, "File downloaded successfully");
      }
    }
  });
  
  this->client_->send();
}

void FileBrowserSDComponent::list_remote_files() {
  ESP_LOGI(TAG, "Listing remote files...");
  
  this->client_->set_method(http_client::HTTP_METHOD_PROPFIND);
  this->client_->set_url(this->webdav_url_);
  this->client_->set_header("Depth", "1");
  
  this->client_->on_response([this](http_client::HTTPClient &client, 
                                  http_client::HTTPClientResponse response) {
    if (response.code == 207) { // Multi-Status
      // Parse WebDAV XML response
      ESP_LOGI(TAG, "Received WebDAV directory listing");
      // TODO: Implement XML parsing for WebDAV response
    }
  });
  
  this->client_->send();
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser SD WebDAV Client:");
  ESP_LOGCONFIG(TAG, "  WebDAV URL: %s", this->webdav_url_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
}

}  // namespace filebrowser_sd
}  // namespace esphome
