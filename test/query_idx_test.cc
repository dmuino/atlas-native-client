#include "../interpreter/query_index.h"
#include <gtest/gtest.h>

using atlas::interpreter::Query;
using atlas::interpreter::QueryIndex;
using atlas::interpreter::query::and_q;
using atlas::interpreter::query::eq;
using atlas::meter::Tags;

using QIdx = QueryIndex<std::shared_ptr<Query>>;

TEST(QueryIndex, Empty) {
  QIdx::Queries queries;
  auto qi = QIdx::Build(queries);

  EXPECT_TRUE(qi->MatchingEntries(Tags{}).empty());
  EXPECT_TRUE(qi->MatchingEntries(Tags{{"a", "1"}}).empty());
}

TEST(QueryIndex, SingleQuerySimple) {
  QIdx::Queries queries;
  queries.emplace_back(and_q(eq("a", "1"), eq("b", "2")));

  auto qi = QIdx::Build(queries);

  // not all tags are present
  EXPECT_TRUE(qi->MatchingEntries(Tags{}).empty());
  EXPECT_TRUE(qi->MatchingEntries(Tags{{"a", "1"}}).empty());

  // matches
  EXPECT_EQ(qi->MatchingEntries(Tags{{"a", "1"}, {"b", "2"}}), queries);
  EXPECT_EQ(qi->MatchingEntries(Tags{{"a", "1"}, {"b", "2"}, {"c", "3"}}),
            queries);
}
