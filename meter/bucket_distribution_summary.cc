#include "bucket_distribution_summary.h"

namespace atlas {
namespace meter {

BucketDistributionSummary::BucketDistributionSummary(
    Registry* registry, IdPtr id, BucketFunction bucket_function)
    : Meter{id, registry->clock()},
      registry_{registry},
      bucket_function_{std::move(bucket_function)} {}

std::ostream& BucketDistributionSummary::Dump(std::ostream& os) const {
  os << "BucketDistributionSummary{" << *id_ << "}";
  return os;
}

static const Measurements kEmptyMeasurements;
Measurements BucketDistributionSummary::MeasuresForPoller(
    size_t /*poller_idx*/) const {
  return kEmptyMeasurements;
}

static const std::string kBucket{"bucket"};

void BucketDistributionSummary::Record(int64_t amount) noexcept {
  const auto& bucket = bucket_function_(amount);
  registry_->distribution_summary(id_->WithTag(Tag::of(kBucket, bucket)))
      ->Record(amount);
  Updated();
}

}  // namespace meter
}  // namespace atlas
