#include <mbgl/shaders/mtl/widevector.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

const std::array<UniformBlockInfo, 2> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::uniforms = {
    UniformBlockInfo{true, false, sizeof(WideVectorUniformsUBO), idWideVectorUniformsUBO},
    UniformBlockInfo{true, false, sizeof(WideVectorUniformWideVecUBO), idWideVectorUniformWideVecUBO},
};
const std::array<AttributeInfo, 3> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::attributes = {
    AttributeInfo{wideVectorDrawableUBOCount + 0, gfx::AttributeDataType::Float3, idWideVectorScreenPos},
    AttributeInfo{wideVectorDrawableUBOCount + 1, gfx::AttributeDataType::Float4, idWideVectorColor},
    AttributeInfo{wideVectorDrawableUBOCount + 2, gfx::AttributeDataType::Int, idWideVectorIndex},
};
const std::array<AttributeInfo, 4> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::instanceAttributes = {
    AttributeInfo{wideVectorDrawableUBOCount + 3, gfx::AttributeDataType::Float3, idWideVectorInstanceCenter},
    AttributeInfo{wideVectorDrawableUBOCount + 3, gfx::AttributeDataType::Float4, idWideVectorInstanceColor},
    AttributeInfo{wideVectorDrawableUBOCount + 3, gfx::AttributeDataType::Int, idWideVectorInstancePrevious},
    AttributeInfo{wideVectorDrawableUBOCount + 3, gfx::AttributeDataType::Int, idWideVectorInstanceNext},
};
const std::array<TextureInfo, 0> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::textures = {};

} // namespace shaders
} // namespace mbgl
