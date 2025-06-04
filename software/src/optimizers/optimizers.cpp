#include "optimizers.h"

es::Optimizer make_adam(const model::ParamSpans &param_shape, float b1, float b2) {
  auto m = param_ops::allocate_like(param_shape);
  auto v = param_ops::allocate_like(param_shape);

  return [m = std::move(m), v = std::move(v), b1, b2](model::ParamSpans &params,
                                                      const model::ParamSpans &grad, float lr) mutable {
    if (!param_ops::check_compatibility(params, grad)) {
      throw std::runtime_error("Incompatible spans in adam");
    }

    for (size_t i = 0; i < params.size(); ++i) {
      std::visit(
        [&](auto &m_span, auto &v_span, const auto &grad_span, auto &param_span) {
          for (size_t j = 0; j < m_span.size(); ++j) {
            auto &m_val = m_span[j];
            auto &v_val = v_span[j];
            const auto &grad_val = grad_span[j];
            auto &param_val = param_span[j];

            m_val = static_cast<typename std::decay_t<decltype(m_val)>>((b1 * m_val) + (1 - b1) * grad_val);
            v_val = static_cast<typename std::decay_t<decltype(v_val)>>((b2 * v_val) + (1 - b2) * grad_val * grad_val);

            // Bias correction
            auto m_hat = m_val / (1 - b1);
            auto v_hat = v_val / (1 - b2);

            // Update parameters
            param_val += static_cast<typename std::decay_t<decltype(param_val)>>(
                lr * m_hat / (std::sqrt(v_hat) + 1e-8f));
          }
        },
        m[i], v[i], grad[i], params[i]
      );
    }
  };
}
