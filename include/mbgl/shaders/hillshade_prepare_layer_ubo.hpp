#pragma once

#include <mbgl/shaders/layer_ubo.hpp>

namespace mbgl {
namespace shaders {

struct alignas(16) HillshadePrepareDrawableUBO {
    /*  0 */ std::array<float, 4 * 4> matrix;
    /* 64 */ std::array<float, 4> unpack;
    /* 80 */ std::array<float, 2> dimension;
    /* 88 */ float zoom;
    /* 92 */ float maxzoom;
    /* 96 */
};
static_assert(sizeof(HillshadePrepareDrawableUBO) == 6 * 16);

} // namespace shaders
} // namespace mbgl
