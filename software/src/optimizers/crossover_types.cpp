#include "crossover_types.h"

using model::ParamSpan;

namespace ga {

bool check_spans_compatible(const std::vector<ParamSpan> &spans1,
                            const std::vector<ParamSpan> &spans2) {
  // check if number of spans is equal
  if (spans1.size() != spans2.size()) {
    return false;
  }

  // check compatibility of each span
  // TODO: use C++23 and use https://en.cppreference.com/w/cpp/ranges/zip_view
  for (size_t i = 0; i < spans1.size(); ++i) {
    const ParamSpan &span1 = spans1[i];
    const ParamSpan &span2 = spans2[i];

    // check type compatibility
    if (span1.index() != span2.index()) {
      return false;
    }

    // check length compatibility
    bool same_size =
        std::visit([](const auto &s1, const auto &s2) -> bool { return s1.size() == s2.size(); },
                   span1, span2);

    if (!same_size) {
      return false;
    }
  }

  // everything above matched, so they are compatible.
  return true;
}

VectorCrossover to_vec_crossover(SpanCrossover span_cross) {
  return [span_cross](const std::vector<ParamSpan> &spans1, const std::vector<ParamSpan> &spans2,
                      std::vector<ParamSpan> &child_spans, float sol1_relative_perf,
                      std::mt19937 &rng) {
    // apply the span crossover to each span pair
    for (size_t i = 0; i < spans1.size(); ++i) {
      span_cross(spans1[i], spans2[i], child_spans[i], sol1_relative_perf, rng);
    }
  };
}

} // namespace ga
