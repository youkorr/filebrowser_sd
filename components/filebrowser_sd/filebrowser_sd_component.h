#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esp_vfs.h"

namespace esphome {
namespace filebrowser_sd {

class FileBrowserSDComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  
  void set_base_path(const std::string &base_path) { this->base_path_ = base_path; }
  void set_mount_point(const std::string &mount_point) { this->mount_point_ = mount_point; }
  void set_max_files(int max_files) { this->max_files_ = max_files; }
  void set_format_if_mount_failed(bool format_if_mount_failed) { this->format_if_mount_failed_ = format_if_mount_failed; }
  
  bool list_dir(const std::string &path);
  bool create_dir(const std::string &path);
  bool delete_file(const std::string &path);
  bool rename_file(const std::string &old_path, const std::string &new_path);
  bool get_file_info(const std::string &path);
  
 protected:
  std::string base_path_;
  std::string mount_point_;
  int max_files_{5};
  bool format_if_mount_failed_{false};
};

}  // namespace filebrowser_sd
}  // namespace esphome
