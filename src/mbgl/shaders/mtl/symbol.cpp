#include <mbgl/shaders/mtl/symbol.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

//
// Symbol icon

const std::array<UniformBlockInfo, 4> ShaderSource<BuiltIn::SymbolIconShader, gfx::Backend::Type::Metal>::uniforms = {
    UniformBlockInfo{true, false, sizeof(GlobalPaintParamsUBO), idGlobalPaintParamsUBO},
    UniformBlockInfo{true, false, sizeof(SymbolDrawableUBO), idSymbolDrawableUBO},
    UniformBlockInfo{false, true, sizeof(SymbolTilePropsUBO), idSymbolTilePropsUBO},
    UniformBlockInfo{false, true, sizeof(SymbolEvaluatedPropsUBO), idSymbolEvaluatedPropsUBO},
};
const std::array<AttributeInfo, 6> ShaderSource<BuiltIn::SymbolIconShader, gfx::Backend::Type::Metal>::attributes = {
    // always attributes
    AttributeInfo{symbolUBOCount + 0, gfx::AttributeDataType::Short4, idSymbolPosOffsetVertexAttribute},
    AttributeInfo{symbolUBOCount + 1, gfx::AttributeDataType::UShort4, idSymbolDataVertexAttribute},
    AttributeInfo{symbolUBOCount + 2, gfx::AttributeDataType::Short4, idSymbolPixelOffsetVertexAttribute},
    AttributeInfo{symbolUBOCount + 3, gfx::AttributeDataType::Float3, idSymbolProjectedPosVertexAttribute},
    AttributeInfo{symbolUBOCount + 4, gfx::AttributeDataType::Float, idSymbolFadeOpacityVertexAttribute},

    // sometimes uniforms
    AttributeInfo{symbolUBOCount + 5, gfx::AttributeDataType::Float, idSymbolOpacityVertexAttribute},
};
const std::array<TextureInfo, 1> ShaderSource<BuiltIn::SymbolIconShader, gfx::Backend::Type::Metal>::textures = {
    TextureInfo{0, idSymbolImageTexture},
};

//
// Symbol sdf

const std::array<UniformBlockInfo, 4> ShaderSource<BuiltIn::SymbolSDFIconShader, gfx::Backend::Type::Metal>::uniforms =
    {
        UniformBlockInfo{true, false, sizeof(GlobalPaintParamsUBO), idGlobalPaintParamsUBO},
        UniformBlockInfo{true, false, sizeof(SymbolDrawableUBO), idSymbolDrawableUBO},
        UniformBlockInfo{false, true, sizeof(SymbolTilePropsUBO), idSymbolTilePropsUBO},
        UniformBlockInfo{false, true, sizeof(SymbolEvaluatedPropsUBO), idSymbolEvaluatedPropsUBO},
};
const std::array<AttributeInfo, 10> ShaderSource<BuiltIn::SymbolSDFIconShader, gfx::Backend::Type::Metal>::attributes =
    {
        // always attributes
        AttributeInfo{symbolUBOCount + 0, gfx::AttributeDataType::Short4, idSymbolPosOffsetVertexAttribute},
        AttributeInfo{symbolUBOCount + 1, gfx::AttributeDataType::UShort4, idSymbolDataVertexAttribute},
        AttributeInfo{symbolUBOCount + 2, gfx::AttributeDataType::Short4, idSymbolPixelOffsetVertexAttribute},
        AttributeInfo{symbolUBOCount + 3, gfx::AttributeDataType::Float3, idSymbolProjectedPosVertexAttribute},
        AttributeInfo{symbolUBOCount + 4, gfx::AttributeDataType::Float, idSymbolFadeOpacityVertexAttribute},

        // sometimes uniforms
        AttributeInfo{symbolUBOCount + 5, gfx::AttributeDataType::Float4, idSymbolColorVertexAttribute},
        AttributeInfo{symbolUBOCount + 6, gfx::AttributeDataType::Float4, idSymbolHaloColorVertexAttribute},
        AttributeInfo{symbolUBOCount + 7, gfx::AttributeDataType::Float, idSymbolOpacityVertexAttribute},
        AttributeInfo{symbolUBOCount + 8, gfx::AttributeDataType::Float, idSymbolHaloWidthVertexAttribute},
        AttributeInfo{symbolUBOCount + 9, gfx::AttributeDataType::Float, idSymbolHaloBlurVertexAttribute},
};
const std::array<TextureInfo, 1> ShaderSource<BuiltIn::SymbolSDFIconShader, gfx::Backend::Type::Metal>::textures = {
    TextureInfo{0, idSymbolImageTexture},
};

//
// Symbol icon and text

const std::array<UniformBlockInfo, 4>
    ShaderSource<BuiltIn::SymbolTextAndIconShader, gfx::Backend::Type::Metal>::uniforms = {
        UniformBlockInfo{true, false, sizeof(GlobalPaintParamsUBO), idGlobalPaintParamsUBO},
        UniformBlockInfo{true, false, sizeof(SymbolDrawableUBO), idSymbolDrawableUBO},
        UniformBlockInfo{false, true, sizeof(SymbolTilePropsUBO), idSymbolTilePropsUBO},
        UniformBlockInfo{false, true, sizeof(SymbolEvaluatedPropsUBO), idSymbolEvaluatedPropsUBO},
};
const std::array<AttributeInfo, 9>
    ShaderSource<BuiltIn::SymbolTextAndIconShader, gfx::Backend::Type::Metal>::attributes = {
        // always attributes
        AttributeInfo{symbolUBOCount + 0, gfx::AttributeDataType::Short4, idSymbolPosOffsetVertexAttribute},
        AttributeInfo{symbolUBOCount + 1, gfx::AttributeDataType::UShort4, idSymbolDataVertexAttribute},
        AttributeInfo{symbolUBOCount + 2, gfx::AttributeDataType::Float3, idSymbolProjectedPosVertexAttribute},
        AttributeInfo{symbolUBOCount + 3, gfx::AttributeDataType::Float, idSymbolFadeOpacityVertexAttribute},

        // sometimes uniforms
        AttributeInfo{symbolUBOCount + 4, gfx::AttributeDataType::Float4, idSymbolColorVertexAttribute},
        AttributeInfo{symbolUBOCount + 5, gfx::AttributeDataType::Float4, idSymbolHaloColorVertexAttribute},
        AttributeInfo{symbolUBOCount + 6, gfx::AttributeDataType::Float, idSymbolOpacityVertexAttribute},
        AttributeInfo{symbolUBOCount + 7, gfx::AttributeDataType::Float, idSymbolHaloWidthVertexAttribute},
        AttributeInfo{symbolUBOCount + 8, gfx::AttributeDataType::Float, idSymbolHaloBlurVertexAttribute},
};
const std::array<TextureInfo, 2> ShaderSource<BuiltIn::SymbolTextAndIconShader, gfx::Backend::Type::Metal>::textures = {
    TextureInfo{0, idSymbolImageTexture},
    TextureInfo{1, idSymbolImageIconTexture},
};

} // namespace shaders
} // namespace mbgl
