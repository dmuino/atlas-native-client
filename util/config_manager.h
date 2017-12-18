#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include "config.h"

namespace atlas {
namespace util {

static constexpr const char* kLocalFileName = "./atlas-config.json";
static constexpr int kConfigRefreshMillis = 10000;

class ConfigManager {
 public:
  ConfigManager(std::string local_file_name = kLocalFileName,
                int refresh_ms = kConfigRefreshMillis) noexcept
      : local_file_name_{std::move(local_file_name)},
        refresh_ms_{refresh_ms},
        current_config_{std::shared_ptr<Config>(get_current_config())} {}
  ~ConfigManager() { Stop(); }
  std::shared_ptr<Config> GetConfig() const noexcept;

  void Start() noexcept;

  void Stop() noexcept;

  void AddCommonTag(const char* key, const char* value) noexcept;

  void SetNotifyAlertServer(bool notify) noexcept;

 private:
  mutable std::mutex config_mutex;
  std::string local_file_name_;
  int refresh_ms_;
  std::shared_ptr<Config> current_config_;
  std::atomic<bool> should_run_{false};
  meter::Tags extra_tags_;
  bool default_notify_ = true;

  void refresher() noexcept;
  void refresh_configs() noexcept;
  std::unique_ptr<Config> get_current_config() noexcept;
};
}  // namespace util
}  // namespace atlas
