// defines types for crossover functions
#pragma once

#include "ga.h"

namespace ga {

// file-private declarations
namespace {
using model::ParamSpan;
}

// type aliases for crossover functions
template <typename ObsType>
using SolCrossover = std::function<std::shared_ptr<Model<ObsType>>(
    const Solution<ObsType> &sol1, const Solution<ObsType> &sol2, std::mt19937 &rng)>;

// note: sol1_relative_perf = (fit1 / (fit1 + fit2))
using VectorCrossover =
    std::function<void(const std::vector<ParamSpan> &spans1, const std::vector<ParamSpan> &spans2,
                       std::vector<ParamSpan> &child, float sol1_relative_perf, std::mt19937 &rng)>;

using SpanCrossover =
    std::function<void(const ParamSpan &span1, const ParamSpan &span2, ParamSpan &child,
                       float sol1_relative_perf, std::mt19937 &rng)>;

template <typename T>
using ParamCrossover =
    std::function<T(T param1, T param2, float sol1_relative_perf, std::mt19937 &rng)>;

// helper that turns a VectorCrossover into a SolCrossover
template <typename ObsType>
SolCrossover<ObsType> to_sol_crossover(VectorCrossover vec_cross) {
  return [vec_cross](const Solution<ObsType> &sol1, const Solution<ObsType> &sol2,
                     std::mt19937 &rng) -> std::shared_ptr<Model<ObsType>> {
    // check compatibility
    if (!check_spans_compatible(sol1.model->get_spans(), sol2.model->get_spans())) {
      // parents aren't compatible, so random pick a parent and return a clone
      std::uniform_int_distribution<int> dist(0, 1);
      if (dist(rng) == 0) {
        return sol1.model->clone();
      } else {
        return sol2.model->clone();
      }
    }
    // parents are compatible so move on to the crossover
    auto child = sol1.model->clone();
    float sol1_relative_perf = static_cast<float>(sol1.fitness) / (sol1.fitness + sol2.fitness);
    vec_cross(sol1.model->get_spans(), sol2.model->get_spans(), child->get_spans(),
              sol1_relative_perf, rng);
    // apply child crossover
    child->apply_spans();
    return child;
  };
}

// helper that turns a SpanCrossover into a VectorCrossover
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

// helper that turns a ParamCrossover into a SpanCrossover
template <typename F>
SpanCrossover to_span_crossover(F param_cross) {
  return [param_cross](const ParamSpan &span1, const ParamSpan &span2, ParamSpan &child_span,
                       float sol1_relative_perf, std::mt19937 &rng) {
    // use std::visit to apply the parameter strategy to any span type
    std::visit(
        [&param_cross, &rng, &sol1_relative_perf](auto &&s1, auto &&s2, auto &&sc) {
          // using SpanType = std::decay_t<decltype(s1)>;
          // using ValueType = typename SpanType::value_type;
          for (size_t j = 0; j < s1.size(); ++j) {
            sc[j] = param_cross(s1[j], s2[j], sol1_relative_perf, rng);
          }
        },
        span1, span2, child_span);
  };
}

template <typename ObsType>
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

} // namespace ga
