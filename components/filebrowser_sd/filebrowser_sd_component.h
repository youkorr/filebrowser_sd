#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "../sd_mmc_card/sd_mmc_card.h"
#include "esphome/components/http_client/http_client.h"

namespace esphome {
namespace filebrowser_sd {

class FileBrowserSDComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  
  void set_webdav_url(const std::string &url) { this->webdav_url_ = url; }
  void set_username(const std::string &username) { this->username_ = username; }
  void set_password(const std::string &password) { this->password_ = password; }
  void set_mount_point(const std::string &mount_point) { this->mount_point_ = mount_point; }
  
  // WebDAV operations
  void sync_to_filebrowser();
  void sync_from_filebrowser();
  
 protected:
  std::string webdav_url_;
  std::string username_;
  std::string password_;
  std::string mount_point_;
  http_client::HTTPClient *client_{nullptr};

  void upload_file(const std::string &local_path, const std::string &remote_path);
  void download_file(const std::string &remote_path, const std::string &local_path);
  void list_remote_files();
};

}  // namespace filebrowser_sd
}  // namespace esphome

