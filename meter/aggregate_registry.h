#pragma once

#include "registry.h"
namespace atlas {

namespace meter {

class AggregateRegistry {
 public:
  void update_from(const Measurements& measurements) noexcept;
  Measurements measurements() noexcept;

 private:
  std::unordered_map<IdPtr, Measurement> my_measures;
  std::mutex mutex;
};

}

}

