#pragma once

#include "models/model.h"
#include "param_ops.h"
#include "openai_es.h"

es::Optimizer make_adam(const model::ParamSpans &param_shape, float b1=0.9f, float b2=0.999f);
