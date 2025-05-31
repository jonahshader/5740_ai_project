#include "param_ops.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace param_ops {

// ===== Memory Management Functions =====

ParamVecs allocate_like(const ParamSpans &template_spans) {
  ParamVecs result;
  result.reserve(template_spans.size());

  for (const auto &span : template_spans) {
    result.push_back(std::visit(
        [](const auto &s) -> ParamVec {
          using T = typename std::decay_t<decltype(s)>::element_type;
          return std::vector<T>(s.size()); // Zero-initialized
        },
        span));
  }
  return result;
}

ParamVecs copy_to_owned(const ParamSpans &spans) {
  ParamVecs result;
  result.reserve(spans.size());

  for (const auto &span : spans) {
    result.push_back(std::visit(
        [](const auto &s) -> ParamVec {
          using T = typename std::decay_t<decltype(s)>::element_type;
          return std::vector<T>(s.begin(), s.end());
        },
        span));
  }
  return result;
}

ParamVecs zeros_like(const ParamSpans &template_spans) {
  return allocate_like(template_spans); // Same as allocate_like since vectors are zero-initialized
}

// ===== Conversion Functions =====

ParamSpans to_spans(ParamVecs &owned_params) {
  ParamSpans result;
  result.reserve(owned_params.size());

  for (auto &param_vec : owned_params) {
    result.push_back(std::visit([](auto &vec) -> ParamSpan { return std::span(vec); }, param_vec));
  }
  return result;
}

void copy_from_spans(const ParamSpans &spans, ParamVecs &dest_vecs) {
  for (size_t i = 0; i < spans.size(); ++i) {
    std::visit(
        [](const auto &span, auto &vec) {
          using SpanT = std::decay_t<decltype(span)>;
          using VecT = std::decay_t<decltype(vec)>;
          using SpanValueT = typename SpanT::element_type;
          using VecValueT = typename VecT::value_type;

          if constexpr (std::is_same_v<SpanValueT, VecValueT>) {
            vec.resize(span.size());
            std::copy(span.begin(), span.end(), vec.begin());
          } else {
            throw std::runtime_error("Type mismatch in copy_from_spans");
          }
        },
        spans[i], dest_vecs[i]);
  }
}

void copy_to_spans(const ParamVecs &src_vecs, ParamSpans &dest_spans) {
  for (size_t i = 0; i < src_vecs.size(); ++i) {
    std::visit(
        [](const auto &vec, auto &span) {
          using VecT = std::decay_t<decltype(vec)>;
          using SpanT = std::decay_t<decltype(span)>;
          using VecValueT = typename VecT::value_type;
          using SpanValueT = typename SpanT::element_type;

          if constexpr (std::is_same_v<VecValueT, SpanValueT>) {
            if (vec.size() != span.size()) {
              throw std::runtime_error("Size mismatch in copy_to_spans");
            }
            std::copy(vec.begin(), vec.end(), span.begin());
          } else {
            throw std::runtime_error("Type mismatch in copy_to_spans");
          }
        },
        src_vecs[i], dest_spans[i]);
  }
}

// ===== Compatibility Checking =====

bool check_compatibility(const ParamSpans &spans1, const ParamSpans &spans2) {
  if (spans1.size() != spans2.size()) {
    return false;
  }

  for (size_t i = 0; i < spans1.size(); ++i) {
    if (spans1[i].index() != spans2[i].index()) {
      return false;
    }

    bool same_size =
        std::visit([](const auto &s1, const auto &s2) { return s1.size() == s2.size(); }, spans1[i],
                   spans2[i]);

    if (!same_size) {
      return false;
    }
  }
  return true;
}

bool check_compatibility(const std::vector<ParamSpans> &span_vectors) {
  if (span_vectors.empty()) {
    return true;
  }

  const auto &reference = span_vectors[0];
  for (size_t i = 1; i < span_vectors.size(); ++i) {
    if (!check_compatibility(reference, span_vectors[i])) {
      return false;
    }
  }
  return true;
}

// ===== In-Place Arithmetic Operations =====

ParamSpans &operator+=(ParamSpans &lhs, const ParamSpans &rhs) {
  if (!check_compatibility(lhs, rhs)) {
    throw std::runtime_error("Incompatible spans in operator+=");
  }

  for (size_t i = 0; i < lhs.size(); ++i) {
    std::visit(
        [](auto &l, const auto &r) {
          for (size_t j = 0; j < l.size(); ++j) {
            l[j] += r[j];
          }
        },
        lhs[i], rhs[i]);
  }
  return lhs;
}

ParamSpans &operator-=(ParamSpans &lhs, const ParamSpans &rhs) {
  if (!check_compatibility(lhs, rhs)) {
    throw std::runtime_error("Incompatible spans in operator-=");
  }

  for (size_t i = 0; i < lhs.size(); ++i) {
    std::visit(
        [](auto &l, const auto &r) {
          for (size_t j = 0; j < l.size(); ++j) {
            l[j] -= r[j];
          }
        },
        lhs[i], rhs[i]);
  }
  return lhs;
}

ParamSpans &operator*=(ParamSpans &lhs, float scalar) {
  for (auto &span : lhs) {
    std::visit(
        [scalar](auto &s) {
          for (auto &val : s) {
            val = static_cast<typename std::decay_t<decltype(val)>>(val * scalar);
          }
        },
        span);
  }
  return lhs;
}

ParamSpans &operator/=(ParamSpans &lhs, float scalar) {
  if (std::abs(scalar) < 1e-10f) {
    throw std::runtime_error("Division by zero in operator/=");
  }
  return lhs *= (1.0f / scalar);
}

// ===== Explicit Destination Operations =====

void add(const ParamSpans &a, const ParamSpans &b, ParamSpans &result) {
  if (!check_compatibility(a, b) || !check_compatibility(a, result)) {
    throw std::runtime_error("Incompatible spans in add");
  }

  for (size_t i = 0; i < a.size(); ++i) {
    std::visit(
        [](const auto &sa, const auto &sb, auto &sr) {
          for (size_t j = 0; j < sa.size(); ++j) {
            sr[j] = sa[j] + sb[j];
          }
        },
        a[i], b[i], result[i]);
  }
}

void subtract(const ParamSpans &a, const ParamSpans &b, ParamSpans &result) {
  if (!check_compatibility(a, b) || !check_compatibility(a, result)) {
    throw std::runtime_error("Incompatible spans in subtract");
  }

  for (size_t i = 0; i < a.size(); ++i) {
    std::visit(
        [](const auto &sa, const auto &sb, auto &sr) {
          for (size_t j = 0; j < sa.size(); ++j) {
            sr[j] = sa[j] - sb[j];
          }
        },
        a[i], b[i], result[i]);
  }
}

void multiply(const ParamSpans &src, float scalar, ParamSpans &result) {
  if (!check_compatibility(src, result)) {
    throw std::runtime_error("Incompatible spans in multiply");
  }

  for (size_t i = 0; i < src.size(); ++i) {
    std::visit(
        [scalar](const auto &s, auto &r) {
          for (size_t j = 0; j < s.size(); ++j) {
            r[j] = static_cast<typename std::decay_t<decltype(r[j])>>(s[j] * scalar);
          }
        },
        src[i], result[i]);
  }
}

void linear_combination(const ParamSpans &src1, float a, const ParamSpans &src2, float b,
                        ParamSpans &result) {
  if (!check_compatibility(src1, src2) || !check_compatibility(src1, result)) {
    throw std::runtime_error("Incompatible spans in linear_combination");
  }

  for (size_t i = 0; i < src1.size(); ++i) {
    std::visit(
        [a, b](const auto &s1, const auto &s2, auto &r) {
          for (size_t j = 0; j < s1.size(); ++j) {
            r[j] = static_cast<typename std::decay_t<decltype(r[j])>>(a * s1[j] + b * s2[j]);
          }
        },
        src1[i], src2[i], result[i]);
  }
}

void axpy(ParamSpans &params, const ParamSpans &x, float alpha) {
  if (!check_compatibility(params, x)) {
    throw std::runtime_error("Incompatible spans in axpy");
  }

  for (size_t i = 0; i < params.size(); ++i) {
    std::visit(
        [alpha](auto &p, const auto &x_vals) {
          for (size_t j = 0; j < p.size(); ++j) {
            p[j] = static_cast<typename std::decay_t<decltype(p[j])>>(p[j] + alpha * x_vals[j]);
          }
        },
        params[i], x[i]);
  }
}

// ===== Weighted Averaging Functions =====

void weighted_average(const std::vector<ParamSpans> &span_vectors,
                      const std::vector<float> &weights, ParamSpans &result) {
  if (span_vectors.empty()) {
    throw std::runtime_error("Empty span_vectors in weighted_average");
  }

  if (span_vectors.size() != weights.size()) {
    throw std::runtime_error("Size mismatch between span_vectors and weights");
  }

  if (!check_compatibility(span_vectors)) {
    throw std::runtime_error("Incompatible span vectors in weighted_average");
  }

  if (!check_compatibility(span_vectors[0], result)) {
    throw std::runtime_error("Result incompatible with input spans");
  }

  // Initialize result to zero
  fill(result, 0.0f);

  // Accumulate weighted sum
  for (size_t vec_idx = 0; vec_idx < span_vectors.size(); ++vec_idx) {
    const float weight = weights[vec_idx];
    const auto &spans = span_vectors[vec_idx];

    for (size_t span_idx = 0; span_idx < spans.size(); ++span_idx) {
      std::visit(
          [weight](const auto &src, auto &dest) {
            for (size_t j = 0; j < src.size(); ++j) {
              dest[j] =
                  static_cast<typename std::decay_t<decltype(dest[j])>>(dest[j] + weight * src[j]);
            }
          },
          spans[span_idx], result[span_idx]);
    }
  }
}

void weighted_average(const std::vector<ParamSpans> &span_vectors,
                      const std::vector<float> &weights, ParamVecs &result_vecs) {
  if (span_vectors.empty()) {
    throw std::runtime_error("Empty span_vectors in weighted_average");
  }

  // Initialize result_vecs with correct structure
  result_vecs = allocate_like(span_vectors[0]);
  auto result_spans = to_spans(result_vecs);

  weighted_average(span_vectors, weights, result_spans);
}

void average(const std::vector<ParamSpans> &span_vectors, ParamSpans &result) {
  std::vector<float> equal_weights(span_vectors.size(), 1.0f / span_vectors.size());
  weighted_average(span_vectors, equal_weights, result);
}

void average(const std::vector<ParamSpans> &span_vectors, ParamVecs &result_vecs) {
  std::vector<float> equal_weights(span_vectors.size(), 1.0f / span_vectors.size());
  weighted_average(span_vectors, equal_weights, result_vecs);
}

// ===== Evolution Strategy Specific Functions =====

void center_of_mass(const std::vector<ParamSpans> &population,
                    const std::vector<float> &fitness_weights, ParamVecs &centroid) {
  // Normalize weights to sum to 1.0
  float weight_sum = std::accumulate(fitness_weights.begin(), fitness_weights.end(), 0.0f);
  if (weight_sum <= 0.0f) {
    throw std::runtime_error("Non-positive weight sum in center_of_mass");
  }

  std::vector<float> normalized_weights;
  normalized_weights.reserve(fitness_weights.size());
  for (float w : fitness_weights) {
    normalized_weights.push_back(w / weight_sum);
  }

  weighted_average(population, normalized_weights, centroid);
}

void truncation_recombination(const std::vector<ParamSpans> &population,
                              const std::vector<float> &fitness_scores, size_t num_parents,
                              ParamVecs &result) {
  if (num_parents > population.size()) {
    throw std::runtime_error("num_parents exceeds population size");
  }

  // Create indices and sort by fitness (descending)
  std::vector<size_t> indices(population.size());
  std::iota(indices.begin(), indices.end(), 0);

  std::sort(indices.begin(), indices.end(), [&fitness_scores](size_t i, size_t j) {
    return fitness_scores[i] > fitness_scores[j];
  });

  // Take top num_parents individuals
  std::vector<ParamSpans> selected_population;
  selected_population.reserve(num_parents);

  for (size_t i = 0; i < num_parents; ++i) {
    selected_population.push_back(population[indices[i]]);
  }

  // Average them
  average(selected_population, result);
}

void rank_weighted_recombination(const std::vector<ParamSpans> &population,
                                 const std::vector<float> &fitness_scores, ParamVecs &result) {
  // Create rank-based weights (higher rank = higher weight)
  std::vector<size_t> indices(population.size());
  std::iota(indices.begin(), indices.end(), 0);

  std::sort(indices.begin(), indices.end(), [&fitness_scores](size_t i, size_t j) {
    return fitness_scores[i] > fitness_scores[j];
  });

  std::vector<float> rank_weights(population.size());
  for (size_t i = 0; i < population.size(); ++i) {
    // Linear ranking: rank 0 (best) gets weight N, rank N-1 (worst) gets weight 1
    rank_weights[indices[i]] = static_cast<float>(population.size() - i);
  }

  weighted_average(population, rank_weights, result);
}

// ===== Utility Functions =====

size_t total_param_count(const ParamSpans &spans) {
  size_t total = 0;
  for (const auto &span : spans) {
    total += std::visit([](const auto &s) { return s.size(); }, span);
  }
  return total;
}

std::vector<float> flatten_to_float(const ParamSpans &spans) {
  std::vector<float> result;
  result.reserve(total_param_count(spans));

  for (const auto &span : spans) {
    std::visit(
        [&result](const auto &s) {
          for (const auto &val : s) {
            result.push_back(static_cast<float>(val));
          }
        },
        span);
  }

  return result;
}

void fill(ParamSpans &spans, float value) {
  for (auto &span : spans) {
    std::visit(
        [value](auto &s) {
          using T = typename std::decay_t<decltype(s)>::element_type;
          T typed_value = static_cast<T>(value);
          std::fill(s.begin(), s.end(), typed_value);
        },
        span);
  }
}

void add_noise(ParamSpans &spans, float std_dev, std::mt19937 &rng) {
  std::normal_distribution<float> dist(0.0f, std_dev);

  for (auto &span : spans) {
    std::visit(
        [&dist, &rng](auto &s) {
          using T = typename std::decay_t<decltype(s)>::element_type;
          for (auto &val : s) {
            float noise = dist(rng);
            val = static_cast<T>(val + noise);
          }
        },
        span);
  }
}

} // namespace param_ops
