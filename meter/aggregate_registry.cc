#include "aggregate_registry.h"
#include "../util/logger.h"

namespace atlas {
namespace meter {

enum class AggrOp { Add, Max };

static AggrOp op_for_statistic(util::StrRef stat) {
  static std::array<util::StrRef, 5> counters{
      util::intern_str("count"), util::intern_str("totalAmount"),
      util::intern_str("totalTime"), util::intern_str("totalOfSquares"),
      util::intern_str("percentile")};

  auto c = std::find(counters.begin(), counters.end(), stat);
  return c == counters.end() ? AggrOp::Max : AggrOp::Add;
}

static AggrOp aggregate_op(const Measurement& measurement) {
  static util::StrRef statistic = util::intern_str("statistic");
  const auto& tags = measurement.id->GetTags();
  auto stat = tags.at(statistic);
  return op_for_statistic(stat);
}

void AggregateRegistry::update_from(const Measurements& measurements) noexcept {
  std::lock_guard<std::mutex> guard{mutex};

  for (const auto& m : measurements) {
    auto& my_m = my_measures[m.id];
    my_m.id = m.id;
    my_m.timestamp = m.timestamp;
    auto op = aggregate_op(m);
    switch (op) {
      case AggrOp::Add:
        my_m.value += m.value;
        break;
      case AggrOp::Max:
        my_m.value = std::max(my_m.value, m.value);
        break;
    }
  }
}

Measurements AggregateRegistry::measurements() noexcept {
  std::lock_guard<std::mutex> guard{mutex};

  Measurements result;
  result.reserve(my_measures.size());
  for (const auto& m : my_measures) {
    result.push_back(m.second);
  }
  my_measures.clear();
  return result;
}

}  // namespace meter
}  // namespace atlas
