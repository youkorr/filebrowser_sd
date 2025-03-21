#include "filebrowser_sd_component.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <dirent.h>
#include <sys/stat.h>
#include <cJSON.h>

namespace esphome {
namespace filebrowser_sd {

static const char *const TAG = "filebrowser_sd";

void FileBrowserSDComponent::setup() {
  ESP_LOGCONFIG(TAG, "Initializing FileBrowser Client for ESP32-S3-Box-3...");

  if (login()) {
    ESP_LOGI(TAG, "Successfully logged in to Filebrowser");
    
    if (!this->smb_shares_.empty()) {
      if (!mount_smb_shares()) {
        ESP_LOGE(TAG, "Failed to mount SMB shares");
      }
    }
    
    list_files();
  } else {
    ESP_LOGE(TAG, "Failed to login to Filebrowser");
  }
}

bool FileBrowserSDComponent::login() {
  ESP_LOGI(TAG, "Logging in to Filebrowser...");
  
  std::string login_url = this->base_url_ + "/api/login";
  esp_http_client_handle_t client = create_client(login_url.c_str());
  
  std::string login_data = "{\"username\":\"" + this->username_ + 
                          "\",\"password\":\"" + this->password_ + "\"}";
  
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, login_data.c_str(), login_data.length());
  
  esp_err_t err = esp_http_client_perform(client);
  bool success = false;
  
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    if (status_code == 200) {
      char *token = nullptr;
      if (esp_http_client_get_header(client, "X-Auth-Token", &token) == ESP_OK && token != nullptr) {
        this->auth_token_ = std::string(token);
        success = true;
        ESP_LOGI(TAG, "Successfully obtained auth token");
      } else {
        ESP_LOGE(TAG, "Failed to get auth token from response");
      }
    } else {
      ESP_LOGE(TAG, "Login failed with status code: %d", status_code);
      if (status_code == 401) {
        ESP_LOGE(TAG, "Invalid username or password");
      }
    }
  } else {
    ESP_LOGE(TAG, "Login request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::renew_token() {
  if (this->auth_token_.empty()) {
    return login();
  }

  ESP_LOGI(TAG, "Renewing authentication token...");
  std::string renew_url = this->base_url_ + "/api/renew";
  esp_http_client_handle_t client = create_client(renew_url.c_str());
  
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  
  esp_err_t err = esp_http_client_perform(client);
  bool success = false;
  
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    if (status_code == 200) {
      success = true;
      ESP_LOGI(TAG, "Token renewed successfully");
    } else {
      ESP_LOGE(TAG, "Token renewal failed with status code: %d", status_code);
      if (status_code == 401) {
        this->auth_token_.clear();
        success = login();
      }
    }
  } else {
    ESP_LOGE(TAG, "Token renewal request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::check_and_renew_auth() {
  if (this->auth_token_.empty()) {
    return login();
  }
  return true;
}

esp_http_client_handle_t FileBrowserSDComponent::create_client(const char* url) {
  esp_http_client_config_t config = {};
  config.url = url;
  config.timeout_ms = 20000;
  config.disable_auto_redirect = false;
  
  esp_http_client_handle_t client = esp_http_client_init(&config);
  
  if (!this->auth_token_.empty()) {
    set_auth_header(client);
  }
  
  return client;
}

void FileBrowserSDComponent::set_auth_header(esp_http_client_handle_t client) {
  if (!this->auth_token_.empty()) {
    esp_http_client_set_header(client, "X-Auth-Token", this->auth_token_.c_str());
  }
}

bool FileBrowserSDComponent::list_files(const std::string &path) {
  if (!check_and_renew_auth()) {
    return false;
  }
  
  ESP_LOGI(TAG, "Listing files in path: %s", path.c_str());
  
  std::string list_url = this->base_url_ + "/api/resources" + path;
  esp_http_client_handle_t client = create_client(list_url.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  
  esp_err_t err = esp_http_client_perform(client);
  bool success = false;
  
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    if (status_code == 200) {
      char buffer[1024] = {0};
      int read_len = esp_http_client_read(client, buffer, sizeof(buffer)-1);
      if (read_len > 0) {
        buffer[read_len] = 0;
        ESP_LOGI(TAG, "Files: %s", buffer);
        success = true;
      }
    } else {
      ESP_LOGE(TAG, "List files failed with status code: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "List files request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::upload_file(const std::string &local_path, const std::string &remote_path) {
  if (!check_and_renew_auth()) {
    return false;
  }
  
  ESP_LOGI(TAG, "Uploading %s to %s", local_path.c_str(), remote_path.c_str());
  
  FILE *file = fopen(local_path.c_str(), "r");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open local file");
    return false;
  }
  
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  std::string upload_url = this->base_url_ + "/api/resources" + remote_path;
  esp_http_client_handle_t client = create_client(upload_url.c_str());
  
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
  
  esp_err_t err = esp_http_client_open(client, file_size);
  bool success = false;
  
  if (err == ESP_OK) {
    uint8_t buffer[1024];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
      if (esp_http_client_write(client, (char*)buffer, bytes_read) <= 0) {
        ESP_LOGE(TAG, "Failed to write data");
        break;
      }
    }
    
    esp_http_client_close(client);
    int status_code = esp_http_client_get_status_code(client);
    success = (status_code == 200);
    
    if (!success) {
      ESP_LOGE(TAG, "Upload failed with status code: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
  }
  
  fclose(file);
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::download_file(const std::string &remote_path, const std::string &local_path) {
  if (!check_and_renew_auth()) {
    return false;
  }
  
  ESP_LOGI(TAG, "Downloading %s to %s", remote_path.c_str(), local_path.c_str());
  
  std::string download_url = this->base_url_ + "/api/raw" + remote_path;
  esp_http_client_handle_t client = create_client(download_url.c_str());
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  
  FILE *file = fopen(local_path.c_str(), "w");
  if (!file) {
    ESP_LOGE(TAG, "Failed to create local file");
    esp_http_client_cleanup(client);
    return false;
  }
  
  esp_err_t err = esp_http_client_open(client, 0);
  bool success = false;
  
  if (err == ESP_OK) {
    char buffer[1024];
    int read_len;
    
    while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
      if (fwrite(buffer, 1, read_len, file) != (size_t)read_len) {
        ESP_LOGE(TAG, "Failed to write to file");
        break;
      }
    }
    
    success = (read_len == 0);
  } else {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
  }
  
  fclose(file);
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::mount_smb_shares() {
  ESP_LOGI(TAG, "Mounting SMB shares...");
  for (const auto &share : this->smb_shares_) {
    if (!list_smb_files(share)) {
      ESP_LOGE(TAG, "Failed to access SMB share: %s", share.c_str());
      return false;
    }
  }
  return true;
}

bool FileBrowserSDComponent::list_smb_files(const std::string &share, const std::string &path) {
  ESP_LOGI(TAG, "Listing files on SMB share: %s, path: %s", share.c_str(), path.c_str());
  return smb_request("list", share, path);
}

bool FileBrowserSDComponent::smb_request(const std::string &action, const std::string &share, 
                                       const std::string &path, const std::string &data) {
  if (!check_and_renew_auth()) {
    return false;
  }

  std::string url = this->base_url_ + "/api/smb";
  esp_http_client_handle_t client = create_client(url.c_str());
  
  std::string request_data = "{\"action\":\"" + action + 
                            "\",\"share\":\"" + share + 
                            "\",\"path\":\"" + path + "\"";
  
  if (!data.empty()) {
    request_data += ",\"data\":\"" + data + "\"";
  }
  request_data += "}";
  
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, request_data.c_str(), request_data.length());
  
  esp_err_t err = esp_http_client_perform(client);
  bool success = false;
  
  if (err == ESP_OK) {
    int status_code = esp_http_client_get_status_code(client);
    if (status_code == 200) {
      char buffer[1024] = {0};
      int read_len = esp_http_client_read(client, buffer, sizeof(buffer)-1);
      if (read_len > 0) {
        buffer[read_len] = 0;
        ESP_LOGI(TAG, "SMB response: %s", buffer);
        success = true;
      }
    } else {
      ESP_LOGE(TAG, "SMB request failed with status code: %d", status_code);
    }
  } else {
    ESP_LOGE(TAG, "SMB request failed: %s", esp_err_to_name(err));
  }
  
  esp_http_client_cleanup(client);
  return success;
}

bool FileBrowserSDComponent::is_authenticated() {
  return !this->auth_token_.empty();
}

void FileBrowserSDComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "FileBrowser Client:");
  ESP_LOGCONFIG(TAG, "  Base URL: %s", this->base_url_.c_str());
  ESP_LOGCONFIG(TAG, "  Mount Point: %s", this->mount_point_.c_str());
  ESP_LOGCONFIG(TAG, "  Username: %s", this->username_.c_str());
  if (!this->smb_shares_.empty()) {
    ESP_LOGCONFIG(TAG, "  SMB Shares:");
    for (const auto &share : this->smb_shares_) {
      ESP_LOGCONFIG(TAG, "    - %s", share.c_str());
    }
  }
}

}  // namespace filebrowser_sd
}  // namespace esphome

