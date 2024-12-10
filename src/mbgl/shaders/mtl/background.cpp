#include <mbgl/shaders/mtl/background.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

//
// Background

const std::array<UniformBlockInfo, 2> ShaderSource<BuiltIn::BackgroundShader, gfx::Backend::Type::Metal>::uniforms = {
    UniformBlockInfo{true, false, sizeof(BackgroundDrawableUBO), idBackgroundDrawableUBO},
    UniformBlockInfo{false, true, sizeof(BackgroundPropsUBO), idBackgroundPropsUBO},
};
const std::array<AttributeInfo, 1> ShaderSource<BuiltIn::BackgroundShader, gfx::Backend::Type::Metal>::attributes = {
    AttributeInfo{backgroundUBOCount + 0, gfx::AttributeDataType::Float3, idBackgroundPosVertexAttribute},
};
const std::array<TextureInfo, 0> ShaderSource<BuiltIn::BackgroundShader, gfx::Backend::Type::Metal>::textures = {};

//
// Background pattern

const std::array<UniformBlockInfo, 3>
    ShaderSource<BuiltIn::BackgroundPatternShader, gfx::Backend::Type::Metal>::uniforms = {
        UniformBlockInfo{false, true, sizeof(GlobalPaintParamsUBO), idGlobalPaintParamsUBO},
        UniformBlockInfo{true, false, sizeof(BackgroundPatternDrawableUBO), idBackgroundDrawableUBO},
        UniformBlockInfo{true, true, sizeof(BackgroundPatternPropsUBO), idBackgroundPropsUBO},
};
const std::array<AttributeInfo, 1>
    ShaderSource<BuiltIn::BackgroundPatternShader, gfx::Backend::Type::Metal>::attributes = {
        AttributeInfo{backgroundUBOCount + 0, gfx::AttributeDataType::Float3, idBackgroundPosVertexAttribute},
};
const std::array<TextureInfo, 1> ShaderSource<BuiltIn::BackgroundPatternShader, gfx::Backend::Type::Metal>::textures = {
    TextureInfo{0, idBackgroundImageTexture}};

} // namespace shaders
} // namespace mbgl
