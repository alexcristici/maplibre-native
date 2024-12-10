#include <mbgl/shaders/mtl/widevector.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

const std::array<UniformBlockInfo, 2> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::uniforms = {
    UniformBlockInfo{true, false, sizeof(WideVectorUniformsUBO), idWideVectorUniformsUBO},
    UniformBlockInfo{true, false, sizeof(WideVectorUniformWideVecUBO), idWideVectorUniformWideVecUBO},
};
const std::array<AttributeInfo, 3> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::attributes = {
    AttributeInfo{wideVectorUBOCount + 0, gfx::AttributeDataType::Float3, idWideVectorScreenPos},
    AttributeInfo{wideVectorUBOCount + 1, gfx::AttributeDataType::Float4, idWideVectorColor},
    AttributeInfo{wideVectorUBOCount + 2, gfx::AttributeDataType::Int, idWideVectorIndex},
};
const std::array<AttributeInfo, 4> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::instanceAttributes = {
    AttributeInfo{wideVectorUBOCount + 3, gfx::AttributeDataType::Float3, idWideVectorInstanceCenter},
    AttributeInfo{wideVectorUBOCount + 3, gfx::AttributeDataType::Float4, idWideVectorInstanceColor},
    AttributeInfo{wideVectorUBOCount + 3, gfx::AttributeDataType::Int, idWideVectorInstancePrevious},
    AttributeInfo{wideVectorUBOCount + 3, gfx::AttributeDataType::Int, idWideVectorInstanceNext},
};
const std::array<TextureInfo, 0> ShaderSource<BuiltIn::WideVectorShader, gfx::Backend::Type::Metal>::textures = {};

} // namespace shaders
} // namespace mbgl
