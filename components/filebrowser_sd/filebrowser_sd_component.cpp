#include "filebrowser_sd_component.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <dirent.h>
#include <sys/stat.h>

namespace esphome {
namespace filebrowser_sd {

static const char *const TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up FileBrowser SD Client...");
  // Test connection on setup
  test_connection();
}

void FileBrowserSDComponent::test_connection() {
  ESP_LOGI(TAG, "Testing connection to Filebrowser...");
  
  std::string test_url = this->webdav_url_ + "/api/resources";
  esp_http_client_handle_t client = create_client(test_url.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Connection test status code: %d", status_code);
    
    if (status_code == 200) {
      ESP_LOGI(TAG, "Successfully connected to Filebrowser!");
    } else if (status_code == 401) {
      ESP_LOGE(TAG, "Authentication failed. Check username and password.");
    } else {
      ESP_LOGE(TAG, "Connection failed with status code: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "Connection test failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
}

esp_http_client_handle_t FileBrowserSDComponent::create_client(const char* url) {
  ESP_LOGI(TAG, "Creating client for URL: %s", url);

  esp_http_client_config_t config = {};
  config.url = url;
  config.timeout_ms = 20000;
  config.disable_auto_redirect = false;
  
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  if (!this->username_.empty() && !this->password_.empty()) {
    std::string auth_string = this->username_ + ":" + this->password_;
    std::string base64_auth = esphome::base64_encode(
      (const unsigned char*)auth_string.c_str(), 
      auth_string.length()
    );
    std::string auth_header = "Basic " + base64_auth;
    
    ESP_LOGI(TAG, "Setting auth header for user: %s", this->username_.c_str());
    esp_http_client_set_header(client, "Authorization", auth_header.c_str());
  }
  
  esp_http_client_set_header(client, "Accept", "application/json");
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
      std::string remote_path = this->webdav_url_ + "/api/resources/" + entry->d_name;
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
  
  esp_err_t err = esp_http_client_open(client, file_size);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection for upload");
    delete[] buffer;
    fclose(file);
    esp_http_client_cleanup(client);
    return;
  }
  
  while ((bytes_read = fread(buffer, 1, buffer_size, file)) > 0) {
    if (esp_http_client_write(client, (char*)buffer, bytes_read) <= 0) {
      ESP_LOGE(TAG, "Failed to write data to HTTP connection");
      break;
    }
  }
  
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  
  delete[] buffer;
  fclose(file);
  
  ESP_LOGI(TAG, "File upload completed");
}

void FileBrowserSDComponent::download_file(const std::string &remote_path, const std::string &local_path) {
  ESP_LOGI(TAG, "Downloading %s to %s", remote_path.c_str(), local_path.c_str());
  
  std::string download_url = this->webdav_url_ + "/api/resources/raw" + remote_path;
  esp_http_client_handle_t client = create_client(download_url.c_str());
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
    if (fwrite(buffer, 1, read_len, file) != (size_t)read_len) {
      ESP_LOGE(TAG, "Failed to write data to file");
      break;
    }
  }
  
  delete[] buffer;
  fclose(file);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  
  if (read_len == 0) {
    ESP_LOGI(TAG, "File download completed");
  } else {
    ESP_LOGE(TAG, "File download failed");
  }
}

void FileBrowserSDComponent::sync_from_filebrowser() {
  ESP_LOGI(TAG, "Starting sync from Filebrowser...");
  list_remote_files();
}

void FileBrowserSDComponent::list_remote_files() {
  ESP_LOGI(TAG, "Listing remote files...");
  
  std::string list_url = this->webdav_url_ + "/api/resources";
  esp_http_client_handle_t client = create_client(list_url.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "List files status code: %d", status_code);
    
    if (status_code == 200) {
      // Read response
      char buffer[512] = {0};
      int read_len = esp_http_client_read(client, buffer, sizeof(buffer)-1);
      if (read_len > 0) {
        buffer[read_len] = 0;
        ESP_LOGI(TAG, "Response: %s", buffer);
      }
    } else {
      ESP_LOGE(TAG, "Failed to list files: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser SD Client:");
  ESP_LOGCONFIG(TAG, "  URL: %s", this->webdav_url_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
}

}  // namespace filebrowser_sd
}  // namespace esphome
