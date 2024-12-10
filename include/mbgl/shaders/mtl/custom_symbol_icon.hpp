#pragma once

#include <mbgl/shaders/custom_drawable_layer_ubo.hpp>
#include <mbgl/shaders/shader_source.hpp>
#include <mbgl/shaders/mtl/shader_program.hpp>

namespace mbgl {
namespace shaders {

#define CUSTOM_SYMBOL_ICON_SHADER_PRELUDE R"(

/// Custom Symbol Icon matrix
struct alignas(16) CustomSymbolIconDrawableUBO {
    /*   0 */ float4x4 matrix;
    /*  64 */
};
static_assert(sizeof(CustomSymbolIconDrawableUBO) == 4 * 16, "wrong size");

/// Custom Symbol Icon Parameters
struct alignas(16) CustomSymbolIconParametersUBO {
    /*  0 */ float2 extrude_scale;
    /*  8 */ float2 anchor;
    /* 16 */ float angle_degrees;
    /* 20 */ uint32_t scale_with_map;
    /* 24 */ uint32_t pitch_with_map;
    /* 28 */ float camera_to_center_distance;
    /* 32 */ float aspect_ratio;
    /* 36 */ float pad1;
    /* 40 */ float pad2;
    /* 44 */ float pad3;
    /* 48 */
};
static_assert(sizeof(CustomSymbolIconParametersUBO) == 3 * 16, "wrong size");

enum {
    idCustomSymbolDrawableUBO = globalUBOCount,
    idCustomSymbolParametersUBO,
    customSymbolUBOCount
};

)"

template <>
struct ShaderSource<BuiltIn::CustomSymbolIconShader, gfx::Backend::Type::Metal> {
    static constexpr auto name = "CustomSymbolIconShader";
    static constexpr auto vertexMainFunction = "vertexMain";
    static constexpr auto fragmentMainFunction = "fragmentMain";

    static const std::array<UniformBlockInfo, 2> uniforms;
    static const std::array<AttributeInfo, 2> attributes;
    static constexpr std::array<AttributeInfo, 0> instanceAttributes{};
    static const std::array<TextureInfo, 1> textures;

    static constexpr auto source = CUSTOM_SYMBOL_ICON_SHADER_PRELUDE R"(
struct VertexStage {
    float2 a_pos [[attribute(customSymbolUBOCount + 0)]];
    float2 a_tex [[attribute(customSymbolUBOCount + 1)]];
};

struct FragmentStage {
    float4 position [[position, invariant]];
    half2 tex;
};

float2 rotateVec2(float2 v, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    return float2(v.x * cosA - v.y * sinA, v.x * sinA + v.y * cosA);
}

float2 ellipseRotateVec2(float2 v, float angle, float radiusRatio /* A/B */) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    float invRatio = 1.0 / radiusRatio;
    return float2(v.x * cosA - radiusRatio * v.y * sinA, invRatio * v.x * sinA + v.y * cosA);
}

FragmentStage vertex vertexMain(thread const VertexStage vertx [[stage_in]],
                                device const CustomSymbolIconDrawableUBO& drawable [[buffer(idCustomSymbolDrawableUBO)]],
                                device const CustomSymbolIconParametersUBO& parameters [[buffer(idCustomSymbolParametersUBO)]]) {

    const float2 extrude = glMod(float2(vertx.a_pos), 2.0) * 2.0 - 1.0;
    const float2 anchor = (parameters.anchor - float2(0.5, 0.5)) * 2.0;
    const float2 center = floor(float2(vertx.a_pos) * 0.5);
    const float angle = radians(-parameters.angle_degrees);
    float2 corner = extrude - anchor;

    float4 position;
    if (parameters.pitch_with_map) {
        if (parameters.scale_with_map) {
            corner *= parameters.extrude_scale;
        } else {
            float4 projected_center = drawable.matrix * float4(center, 0, 1);
            corner *= parameters.extrude_scale * (projected_center.w / parameters.camera_to_center_distance);
        }
        corner = center + rotateVec2(corner, angle);
        position = drawable.matrix * float4(corner, 0, 1);
    } else {
        position = drawable.matrix * float4(center, 0, 1);
        const float factor = parameters.scale_with_map ? parameters.camera_to_center_distance : position.w;
        position.xy += ellipseRotateVec2(corner * parameters.extrude_scale * factor, angle, parameters.aspect_ratio);
    }

    return {
        .position   = position,
        .tex        = half2(vertx.a_tex)
    };
}

half4 fragment fragmentMain(FragmentStage in [[stage_in]],
                            texture2d<float, access::sample> image [[texture(0)]],
                            sampler image_sampler [[sampler(0)]]) {
#if defined(OVERDRAW_INSPECTOR)
    return half4(1.0);
#endif

    return half4(image.sample(image_sampler, float2(in.tex)));
}
)";
};

} // namespace shaders
} // namespace mbgl
