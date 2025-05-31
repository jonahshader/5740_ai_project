#pragma once

#include <functional>
#include <random>
#include <span>
#include <stdexcept>
#include <variant>
#include <vector>

#include "model.h"

namespace param_ops {

using model::ParamSpan;
using model::ParamSpans;
using model::ParamVec;
using model::ParamVecs;

// ===== Memory Management Functions =====

// Allocate new backing memory with same structure as template spans
ParamVecs allocate_like(const ParamSpans &template_spans);

// Create backing memory and copy data from existing spans
ParamVecs copy_to_owned(const ParamSpans &spans);

// Zero-initialize backing memory with same structure
ParamVecs zeros_like(const ParamSpans &template_spans);

// ===== Conversion Functions =====

// Convert ParamVecs to ParamSpans (ParamVecs must outlive returned spans)
ParamSpans to_spans(ParamVecs &owned_params);

// Copy data from spans to ParamVecs
void copy_from_spans(const ParamSpans &spans, ParamVecs &dest_vecs);

// Copy data from ParamVecs to spans
void copy_to_spans(const ParamVecs &src_vecs, ParamSpans &dest_spans);

// ===== Compatibility Checking =====

// Check if two span vectors have compatible structure
bool check_compatibility(const ParamSpans &spans1, const ParamSpans &spans2);

// Check if multiple span vectors have compatible structure
bool check_compatibility(const std::vector<ParamSpans> &span_vectors);

// ===== In-Place Arithmetic Operations =====

// In-place addition
ParamSpans &operator+=(ParamSpans &lhs, const ParamSpans &rhs);

// In-place subtraction
ParamSpans &operator-=(ParamSpans &lhs, const ParamSpans &rhs);

// In-place scalar multiplication
ParamSpans &operator*=(ParamSpans &lhs, float scalar);

// In-place scalar division
ParamSpans &operator/=(ParamSpans &lhs, float scalar);

// ===== Explicit Destination Operations =====

// Addition with explicit destination
void add(const ParamSpans &a, const ParamSpans &b, ParamSpans &result);

// Subtraction with explicit destination
void subtract(const ParamSpans &a, const ParamSpans &b, ParamSpans &result);

// Scalar multiplication with explicit destination
void multiply(const ParamSpans &src, float scalar, ParamSpans &result);

// Linear combination: result = a*src1 + b*src2
void linear_combination(const ParamSpans &src1, float a, const ParamSpans &src2, float b,
                        ParamSpans &result);

// AXPY operation: params += alpha * x (fused multiply-add)
void axpy(ParamSpans &params, const ParamSpans &x, float alpha);

// ===== Weighted Averaging Functions =====

// Weighted average of multiple span vectors
// weights.size() must equal span_vectors.size()
void weighted_average(const std::vector<ParamSpans> &span_vectors,
                      const std::vector<float> &weights, ParamSpans &result);

// Weighted average with explicit destination ParamVecs (for better memory management)
void weighted_average(const std::vector<ParamSpans> &span_vectors,
                      const std::vector<float> &weights, ParamVecs &result_vecs);

// Simple average (equal weights)
void average(const std::vector<ParamSpans> &span_vectors, ParamSpans &result);

// Simple average with explicit destination ParamVecs
void average(const std::vector<ParamSpans> &span_vectors, ParamVecs &result_vecs);

// ===== Evolution Strategy Specific Functions =====

// Center of mass calculation (weighted average where weights sum to 1.0)
void center_of_mass(const std::vector<ParamSpans> &population,
                    const std::vector<float> &fitness_weights, ParamVecs &centroid);

// Recombination with truncation selection
// Takes top 'num_parents' individuals and averages them
void truncation_recombination(const std::vector<ParamSpans> &population,
                              const std::vector<float> &fitness_scores, size_t num_parents,
                              ParamVecs &result);

// Weighted recombination where weights are proportional to fitness ranks
void rank_weighted_recombination(const std::vector<ParamSpans> &population,
                                 const std::vector<float> &fitness_scores, ParamVecs &result);

// ===== Utility Functions =====

// Get total number of parameters across all spans
size_t total_param_count(const ParamSpans &spans);

// Copy parameters to a flat vector (useful for debugging/logging)
std::vector<float> flatten_to_float(const ParamSpans &spans);

// Set all parameters to a specific value
void fill(ParamSpans &spans, float value);

// Add Gaussian noise to parameters
void add_noise(ParamSpans &spans, float std_dev, std::mt19937 &rng);

} // namespace param_ops
