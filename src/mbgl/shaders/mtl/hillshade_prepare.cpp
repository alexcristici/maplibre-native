#include <mbgl/shaders/mtl/hillshade_prepare.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

const std::array<UniformBlockInfo, 2>
    ShaderSource<BuiltIn::HillshadePrepareShader, gfx::Backend::Type::Metal>::uniforms = {
        UniformBlockInfo{true, false, sizeof(HillshadePrepareDrawableUBO), idHillshadePrepareDrawableUBO},
        UniformBlockInfo{true, true, sizeof(HillshadePrepareTilePropsUBO), idHillshadePrepareTilePropsUBO},
};
const std::array<AttributeInfo, 2> ShaderSource<BuiltIn::HillshadePrepareShader,
                                                gfx::Backend::Type::Metal>::attributes = {
    AttributeInfo{hillshadePrepareUBOCount + 0, gfx::AttributeDataType::Short2, idHillshadePosVertexAttribute},
    AttributeInfo{hillshadePrepareUBOCount + 1, gfx::AttributeDataType::Short2, idHillshadeTexturePosVertexAttribute},
};
const std::array<TextureInfo, 1> ShaderSource<BuiltIn::HillshadePrepareShader, gfx::Backend::Type::Metal>::textures = {
    TextureInfo{0, idHillshadeImageTexture},
};

} // namespace shaders
} // namespace mbgl
