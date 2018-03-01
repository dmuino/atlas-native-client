#include <fstream>
#include <gtest/gtest.h>

#include "../meter/subscription_manager.h"
#include "../util/json.h"
namespace atlas {
namespace meter {
Subscriptions* ParseSubscriptions(const std::string& subs_str);
rapidjson::Document MeasurementsToJson(
    int64_t now_millis,
    const interpreter::TagsValuePairs::const_iterator& first,
    const interpreter::TagsValuePairs::const_iterator& last, bool validate,
    int64_t* added);

rapidjson::Document SubResultsToJson(
    int64_t now_millis, const SubscriptionResults::const_iterator& first,
    const SubscriptionResults::const_iterator& last);
}  // namespace meter
}  // namespace atlas

using namespace atlas::meter;
TEST(SubscriptionsManager, ParseSubs) {
  std::ifstream one_sub("./resources/subs1.json");
  std::stringstream buffer;
  buffer << one_sub.rdbuf();

  auto subs = std::unique_ptr<Subscriptions>(ParseSubscriptions(buffer.str()));
  auto expected = Subscriptions{
      Subscription{"So3yA1c1xN_vzZASUQdORBqd9hM", 60000,
                   "nf.cluster,skan-test,:eq,name,foometric,:eq,:and,:sum"},
      Subscription{"tphgPWqODm0ZUhgUCwj23lpEs1o", 60000,
                   "nf.cluster,skan,:re,name,foometric,:eq,:and,:sum"}};
  EXPECT_EQ(subs->size(), 2);
  EXPECT_EQ(*subs, expected);
}

TEST(SubscriptionsManager, ParseLotsOfSubs) {
  std::ifstream one_sub("./resources/many-subs.json");
  std::stringstream buffer;
  buffer << one_sub.rdbuf();

  auto subs = std::unique_ptr<Subscriptions>(ParseSubscriptions(buffer.str()));
  EXPECT_EQ(subs->size(), 3665);
}

static std::string json_to_str(const rapidjson::Document& json) {
  rapidjson::StringBuffer buffer;
  auto c_str = atlas::util::JsonGetString(buffer, json);
  return std::string{c_str};
}

static void expect_eq_json(rapidjson::Document& expected,
                           rapidjson::Document& actual) {
  EXPECT_EQ(expected, actual) << "expected: " << json_to_str(expected)
                              << ", actual: " << json_to_str(actual);
}

TEST(SubscriptionManager, MeasurementsToJsonInvalid) {
  using namespace atlas::interpreter;
  using namespace rapidjson;

  Tags t1{{"name", "name1"}, {"k1", "v1"}, {"k2", ""}};
  Tags t2{{"name", "name2"}, {"", "v1"}, {"k2", "v2.0"}};
  Tags t3{{"k1", "v1"}, {"k2", "v2.1"}};

  auto m1 = TagsValuePair::of(std::move(t1), 1.1);
  auto m2 = TagsValuePair::of(std::move(t2), 2.2);
  auto m3 = TagsValuePair::of(std::move(t3), 3.3);
  TagsValuePairs ms{std::move(m1), std::move(m2), std::move(m3)};

  int64_t added;
  auto json =
      atlas::meter::MeasurementsToJson(1, ms.begin(), ms.end(), true, &added);
  EXPECT_EQ(added, 0);
}

TEST(SubscriptionManager, MeasurementsToJson) {
  using namespace atlas::interpreter;
  using namespace rapidjson;

  Tags t1{{"name", "name1"}, {"k1", "v1"}, {"k2", "v2"}};
  Tags t2{{"name", "name2"}, {"k1", "v1"}, {"k2", "v2.0"}};
  Tags t3{{"name", "name3"}, {"k1", "v1"}, {"k2", "v2.1"}};

  auto m1 = TagsValuePair::of(std::move(t1), 1.1);
  auto m2 = TagsValuePair::of(std::move(t2), 2.2);
  auto m3 = TagsValuePair::of(std::move(t3), 3.3);
  TagsValuePairs ms{std::move(m1), std::move(m2), std::move(m3)};

  int64_t added;
  auto json =
      atlas::meter::MeasurementsToJson(1, ms.begin(), ms.end(), true, &added);
  EXPECT_EQ(added, 3);

  const char* expected =
      "{\"tags\":{},"
      "\"metrics\":[{\"tags\":{"
      "\"k1\":\"v1\",\"k2\":\"v2\",\"name\":\"name1\"},\"start\":1,\"value\":1."
      "1},"
      "{\"tags\":{\"k1\":\"v1\",\"k2\":\"v2.0\",\"name\":\"name2\"},"
      "\"start\":1,\"value\":2.2},{\"tags\":{\"k1\":\"v1\","
      "\"k2\":\"v2.1\",\"name\":\"name3\"},\"start\":1,\"value\":3.3}]}";

  Document expected_json;
  expected_json.Parse<kParseCommentsFlag | kParseNanAndInfFlag>(expected);
  expect_eq_json(expected_json, json);
}

TEST(SubscriptionManager, SubResToJson) {
  using namespace atlas::interpreter;
  using namespace rapidjson;

  SubscriptionMetric e1{"id1", Tags{{"name", "n1"}, {"k1", "v1"}}, 42.0};
  SubscriptionMetric e2{"id1", Tags{{"name", "n1"}, {"k1", "v2"}}, 24.0};
  SubscriptionResults sub_results{e1, e2};

  auto json = SubResultsToJson(123, sub_results.begin(), sub_results.end());
  const char* expected =
      "{\"timestamp\":123,\"metrics\":[{\"id\":\"id1\",\"tags\":{\"k1\":\"v1\","
      "\"name\":\"n1\"},\"value\":42.0},{\"id\":\"id1\",\"tags\":{\"k1\":"
      "\"v2\",\"name\":\"n1\"},\"value\":24.0}]}";

  Document expected_json;
  expected_json.Parse<kParseCommentsFlag | kParseNanAndInfFlag>(expected);
  expect_eq_json(expected_json, json);
}
