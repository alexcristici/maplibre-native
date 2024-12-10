#pragma once

#include <mbgl/shaders/layer_ubo.hpp>

namespace mbgl {
namespace shaders {

struct alignas(16) CollisionDrawableUBO {
    /*  0 */ std::array<float, 4 * 4> matrix;
    /* 64 */ std::array<float, 2> extrude_scale;
    /* 72 */ float overscale_factor;
    /* 76 */ float pad1;
    /* 80 */
};
static_assert(sizeof(CollisionDrawableUBO) == 5 * 16);

} // namespace shaders
} // namespace mbgl
