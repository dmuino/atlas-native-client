#pragma once

#include <ska/flat_hash_map.hpp>
#include <unordered_set>
#include "query.h"
namespace atlas {
namespace interpreter {

template <typename T>
struct QueryIndexEntry {
  std::shared_ptr<Query> query;
  T value;
};

template <typename T>
class QueryIndexAnnotated {
 public:
  QueryIndexAnnotated(
      QueryIndexEntry<T> e,
      ska::flat_hash_set<std::shared_ptr<RelopQuery>> f) noexcept
      : entry{std::move(e)}, filters{std::move(f)} {}

  QueryIndexEntry<T> entry;
  ska::flat_hash_set<std::shared_ptr<RelopQuery>> filters;

  std::vector<std::pair<std::shared_ptr<RelopQuery>, QueryIndexAnnotated<T>>>
  to_query_anno_vector() const {
    std::vector<std::pair<std::shared_ptr<RelopQuery>, QueryIndexAnnotated<T>>>
        res;
    res.reserve(filters.size());
    for (const auto& filter : filters) {
      ska::flat_hash_set<std::shared_ptr<RelopQuery>> new_filters{filters};
      new_filters.erase(filter);
      res.emplace_back(
          std::make_pair(filter, QueryIndexAnnotated{entry, new_filters}));
    }
    return res;
  };
};

}  // namespace interpreter
}  // namespace atlas

namespace std {
template <typename T>
struct hash<atlas::interpreter::QueryIndexEntry<T>> {
  size_t operator()(const atlas::interpreter::QueryIndexEntry<T>& a) const {
    return a.query->Hash();
  }
};

template <typename T>
struct equal_to<atlas::interpreter::QueryIndexEntry<T>> {
  bool operator()(const atlas::interpreter::QueryIndexEntry<T>& lhs,
                  const atlas::interpreter::QueryIndexEntry<T>& rhs) const {
    return lhs.query->Equals(*rhs.query);
  }
};

template <typename T>
struct hash<atlas::interpreter::QueryIndexAnnotated<T>> {
  size_t operator()(const atlas::interpreter::QueryIndexAnnotated<T>& a) const {
    return hash<atlas::interpreter::QueryIndexEntry<T>>()(a.entry);
  }
};

template <typename T>
struct equal_to<atlas::interpreter::QueryIndexAnnotated<T>> {
  bool operator()(const atlas::interpreter::QueryIndexAnnotated<T>& lhs,
                  const atlas::interpreter::QueryIndexAnnotated<T>& rhs) const {
    return equal_to<atlas::interpreter::QueryIndexEntry<T>>()(lhs.entry,
                                                              rhs.entry);
  }
};

template <typename T>
struct hash<vector<atlas::interpreter::QueryIndexAnnotated<T>>> {
  size_t operator()(
      const vector<atlas::interpreter::QueryIndexAnnotated<T>>& v) const {
    auto res = static_cast<size_t>(0);
    for (const auto& e : v) {
      res = (res << 4u) ^
            std::hash<atlas::interpreter::QueryIndexAnnotated<T>>()(e);
    }
    return res;
  }
};

template <typename T>
struct equal_to<vector<atlas::interpreter::QueryIndexAnnotated<T>>> {
  bool operator()(
      const vector<atlas::interpreter::QueryIndexAnnotated<T>>& lhs,
      const vector<atlas::interpreter::QueryIndexAnnotated<T>>& rhs) const {
    if (lhs.size() != rhs.size()) return false;
    for (auto i = 0u; i < lhs.size(); ++i) {
      if (!equal_to<atlas::interpreter::QueryIndexAnnotated<T>>()(lhs[i],
                                                                  rhs[i]))
        return false;
    }
    return true;
  }
};
}  // namespace std

namespace atlas {
namespace interpreter {
template <typename T>
class QueryIndex {
 public:
  using QueryPtr = std::shared_ptr<Query>;
  using Queries = std::vector<QueryPtr>;
  using IndexMap = std::unordered_map<QueryPtr, std::shared_ptr<QueryIndex<T>>>;
  using EntryList = std::vector<QueryIndexEntry<T>>;
  using AnnotatedList = std::vector<QueryIndexAnnotated<T>>;
  using AnnotatedIdxMap =
      ska::flat_hash_map<AnnotatedList, std::shared_ptr<QueryIndex<T>>>;

  QueryIndex(IndexMap indexes, EntryList entries)
      : indexes_(std::move(indexes)), entries_(std::move(entries)) {}

  static std::shared_ptr<QueryIndex<T>> Create(const EntryList& entries) {
    std::vector<QueryIndexAnnotated<T>> annotated;
    annotated.reserve(entries.size() * 2);
    /*
    for (const auto& entry: entries) {
      Queries to_split = query::dnf_list(entry.query);


    }
     */
    AnnotatedIdxMap idx_map;
    return create_impl(&idx_map, annotated);
  }

  static std::shared_ptr<QueryIndex<T>> create_impl(
      AnnotatedIdxMap* idx_map, const AnnotatedList& annotated) {
    const auto& maybe_idx = idx_map->find(annotated);
    if (maybe_idx != idx_map->end()) {
      return maybe_idx->second;
    }

    AnnotatedList children;
    AnnotatedList leaf;
    std::partition_copy(annotated.begin(), annotated.end(), children.begin(),
                        leaf.end(), [](const QueryIndexAnnotated<T>& entry) {
                          return !entry.filters.empty();
                        });

    IndexMap trees;
    for (const auto& entry : children) {
      auto f = entry.to_query_anno_vector();
      std::unordered_map<std::shared_ptr<RelopQuery>, AnnotatedList>
          query_anno_map;
      for (const auto& query_anno : f) {
        query_anno_map[query_anno.first].emplace_back(
            std::move(query_anno.second));
      }
      for (const auto& q_lst : query_anno_map) {
        trees[q_lst.first] = create_impl(idx_map, q_lst.second);
      }
    }
    return std::make_unique<QueryIndex<T>>(std::move(trees), EntryList{});
  }

  static std::shared_ptr<QueryIndex<QueryPtr>> Build(const Queries& queries) {
    EntryList entries;
    entries.reserve(queries.size());
    std::transform(queries.begin(), queries.end(), std::back_inserter(entries),
                   [](const QueryPtr& q) {
                     return QueryIndexEntry<QueryPtr>{q, q};
                   });
    return QueryIndex<QueryPtr>::Create(entries);
  }

  std::vector<T> MatchingEntries(const meter::Tags& tags) noexcept {
    Queries queries;
    std::transform(tags.begin(), tags.end(), std::back_inserter(queries),
                   [](const std::pair<util::StrRef, util::StrRef>& tag) {
                     return std::make_shared<RelopQuery>(tag.first, tag.second,
                                                         RelOp::EQ);
                   });
    return matching_entries(tags, queries.begin(), queries.end());
  }

 private:
  IndexMap indexes_;
  EntryList entries_;

  std::vector<T> entries_to_t(const meter::Tags& tags) noexcept {
    std::vector<T> res;
    for (const auto& e : entries_) {
      if (e.query->Matches(tags)) {
        res.emplace_back(e.value);
      }
    }
    return res;
  }

  std::vector<T> matching_entries(const meter::Tags& tags,
                                  Queries::const_iterator begin,
                                  Queries::const_iterator end) noexcept {
    if (begin == end) {
      return entries_to_t(tags);
    } else {
      auto q = *begin;
      begin++;
      std::vector<T> children;
      const auto& matching = indexes_.find(q);
      if (matching != indexes_.end()) {
        children = matching->second->matching_entries(tags, begin, end);
      }
      auto slower = entries_to_t(tags);
      children.reserve(children.size() + slower.size());
      children.insert(children.end(), slower.begin(), slower.end());
      auto rest = matching_entries(tags, begin, end);
      children.reserve(children.size() + rest.size());
      children.insert(children.end(), rest.begin(), rest.end());
      return children;
    }
  }
};

}  // namespace interpreter
}  // namespace atlas
