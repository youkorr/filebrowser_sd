#include "filebrowser_sd.h"
#include "esphome/core/log.h"
#include <dirent.h>
#include <sys/stat.h>
#include "esp_base64.h"

namespace esphome {
namespace filebrowser_sd {

static const char *const TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up FileBrowser SD WebDAV Client...");
}

esp_http_client_handle_t FileBrowserSDComponent::create_client(const char* url) {
  esp_http_client_config_t config = {
    .url = url,
    .timeout_ms = 20000,
  };
  
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  if (!this->username_.empty() && !this->password_.empty()) {
    std::string auth_string = this->username_ + ":" + this->password_;
    size_t out_len;
    unsigned char *base64_auth = esp_base64_encode((const unsigned char*)auth_string.c_str(), 
                                                  auth_string.length(), &out_len);
    std::string auth_header = "Basic ";
    auth_header += (char*)base64_auth;
    free(base64_auth);
    
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
  }
  
  return client;
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

void FileBrowserSDComponent::upload_file(const std::string &local_path, const std::string &remote_path) {
  ESP_LOGI(TAG, "Uploading %s to %s", local_path.c_str(), remote_path.c_str());
  
  FILE *file = fopen(local_path.c_str(), "r");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open local file for upload");
    return;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  // Create HTTP client
  esp_http_client_handle_t client = create_client(remote_path.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_PUT);
  esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
  
  // Upload in chunks
  const size_t buffer_size = 1024;
  uint8_t *buffer = new uint8_t[buffer_size];
  size_t bytes_read;
  
  esp_http_client_open(client, file_size);
  
  while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
    esp_http_client_write(client, (char*)buffer, bytes_read);
  }
  
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  
  delete[] buffer;
  fclose(file);
}

void FileBrowserSDComponent::download_file(const std::string &remote_path, const std::string &local_path) {
  ESP_LOGI(TAG, "Downloading %s to %s", remote_path.c_str(), local_path.c_str());
  
  esp_http_client_handle_t client = create_client(remote_path.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  
  FILE *file = fopen(local_path.c_str(), "w");
  if (!file) {
    ESP_LOGE(TAG, "Failed to create local file");
    esp_http_client_cleanup(client);
    return;
  }
  
  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection");
    fclose(file);
    esp_http_client_cleanup(client);
    return;
  }
  
  const size_t buffer_size = 1024;
  uint8_t *buffer = new uint8_t[buffer_size];
  int read_len;
  
  while ((read_len = esp_http_client_read(client, (char*)buffer, buffer_size)) > 0) {
    fwrite(buffer, 1, read_len, file);
  }
  
  delete[] buffer;
  fclose(file);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
}

void FileBrowserSDComponent::sync_from_filebrowser() {
  ESP_LOGI(TAG, "Starting sync from Filebrowser...");
  list_remote_files();
}

void FileBrowserSDComponent::list_remote_files() {
  ESP_LOGI(TAG, "Listing remote files...");
  
  esp_http_client_handle_t client = create_client(this->webdav_url_.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_PROPFIND);
  esp_http_client_set_header(client, "Depth", "1");
  
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    if (status_code == 207) {
      ESP_LOGI(TAG, "Successfully retrieved directory listing");
      // TODO: Parse WebDAV XML response
    } else {
      ESP_LOGE(TAG, "Failed to list files, status code: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "Failed to perform HTTP request: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser SD WebDAV Client:");
  ESP_LOGCONFIG(TAG, "  WebDAV URL: %s", this->webdav_url_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
}

}  // namespace filebrowser_sd
}  // namespace esphome
