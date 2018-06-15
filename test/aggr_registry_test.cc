#include "../meter/aggregate_registry.h"
#include "test_utils.h"
#include <gtest/gtest.h>

using atlas::meter::AggregateRegistry;
using atlas::meter::Id;
using atlas::meter::Measurement;
using atlas::meter::Measurements;
using atlas::meter::Tags;

TEST(AggregateRegistry, Empty) {
  AggregateRegistry registry;
  auto ms = registry.measurements();
  ASSERT_TRUE(ms.empty());

  registry.update_from(Measurements{});
  ASSERT_TRUE(registry.measurements().empty());
}

bool operator==(const atlas::meter::Measurement& a,
                const atlas::meter::Measurement& b) {
  return a.timestamp == b.timestamp && a.id == b.id &&
         std::abs(a.value - b.value) < 1e-9;
}

TEST(AggregateRegistry, UpdateOne) {
  AggregateRegistry registry;

  auto gauge_id = std::make_shared<Id>("gauge", Tags{{"statistic", "gauge"}});
  auto random_id = std::make_shared<Id>("random", Tags{});
  auto counter_id =
      std::make_shared<Id>("counter", Tags{{"statistic", "count"}});

  auto gauge = Measurement{gauge_id, 1L, 1.0};
  auto random = Measurement{random_id, 1L, 42.0};
  auto counter = Measurement{counter_id, 1L, 1 / 60.0};
  auto measurements = Measurements{gauge, random, counter};

  registry.update_from(measurements);
  auto ms = registry.measurements();
  auto measure_cmp = [](const Measurement& a, const Measurement& b) {
    return *(a.id) < *(b.id);
  };

  std::sort(measurements.begin(), measurements.end(), measure_cmp);
  std::sort(ms.begin(), ms.end(), measure_cmp);
  EXPECT_EQ(ms, measurements);
  EXPECT_TRUE(registry.measurements().empty()) << "Resets after being measured";
}

TEST(AggregateRegistry, Aggregate) {
  AggregateRegistry registry;

  auto gauge_id = std::make_shared<Id>("gauge", Tags{{"statistic", "gauge"}});
  auto random_id = std::make_shared<Id>("random", Tags{});
  auto counter_id =
      std::make_shared<Id>("counter", Tags{{"statistic", "count"}});

  auto gauge = Measurement{gauge_id, 1L, 1.0};
  auto random = Measurement{random_id, 1L, 42.0};
  auto counter = Measurement{counter_id, 1L, 1 / 60.0};
  auto measurements = Measurements{gauge, random, counter};

  registry.update_from(measurements);
  registry.update_from(measurements);

  for (auto& m: measurements) {
    m.value += 1.0;
  }
  registry.update_from(measurements);
  auto ms = registry.measurements();
  auto measure_cmp = [](const Measurement& a, const Measurement& b) {
    return *(a.id) < *(b.id);
  };
  std::sort(ms.begin(), ms.end(), measure_cmp);

  ASSERT_EQ(ms.size(), 3);
  EXPECT_EQ(ms[0].id, counter.id);
  EXPECT_EQ(ms[0].value, 1.0 + 3.0/60);
  EXPECT_EQ(ms[1].id, gauge.id);
  EXPECT_EQ(ms[1].value, 2.0); // max
  EXPECT_EQ(ms[2].id, random.id);
  EXPECT_EQ(ms[2].value, 43.0); // max
}
