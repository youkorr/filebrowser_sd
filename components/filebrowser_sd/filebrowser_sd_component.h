#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "../sd_mmc_card/sd_mmc_card.h"
#include "esp_http_client.h"

namespace esphome {
namespace filebrowser_sd {

class FileBrowserSDComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  
  void set_base_url(const std::string &url) { this->base_url_ = url; }
  void set_username(const std::string &username) { this->username_ = username; }
  void set_password(const std::string &password) { this->password_ = password; }
  void set_mount_point(const std::string &mount_point) { this->mount_point_ = mount_point; }
  
  bool login();
  bool renew_token();
  bool list_files(const std::string &path = "/");
  bool upload_file(const std::string &local_path, const std::string &remote_path);
  bool download_file(const std::string &remote_path, const std::string &local_path);
  
 protected:
  std::string base_url_;
  std::string username_;
  std::string password_;
  std::string mount_point_;
  std::string auth_token_;

  esp_http_client_handle_t create_client(const char* url);
  bool is_authenticated();
  void set_auth_header(esp_http_client_handle_t client);
  bool check_and_renew_auth();
};

}  // namespace filebrowser_sd
}  // namespace esphome


