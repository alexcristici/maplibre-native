#include <mbgl/shaders/mtl/collision.hpp>
#include <mbgl/shaders/shader_defines.hpp>

namespace mbgl {
namespace shaders {

//
// Collision box

const std::array<UniformBlockInfo, 1> ShaderSource<BuiltIn::CollisionBoxShader, gfx::Backend::Type::Metal>::uniforms = {
    UniformBlockInfo{true, true, sizeof(CollisionDrawableUBO), idCollisionDrawableUBO},
};
const std::array<AttributeInfo, 5> ShaderSource<BuiltIn::CollisionBoxShader, gfx::Backend::Type::Metal>::attributes = {
    AttributeInfo{collisionDrawableUBOCount + 0, gfx::AttributeDataType::Short2, idCollisionPosVertexAttribute},
    AttributeInfo{collisionDrawableUBOCount + 1, gfx::AttributeDataType::Short2, idCollisionAnchorPosVertexAttribute},
    AttributeInfo{collisionDrawableUBOCount + 2, gfx::AttributeDataType::Short2, idCollisionExtrudeVertexAttribute},
    AttributeInfo{collisionDrawableUBOCount + 3, gfx::AttributeDataType::UShort2, idCollisionPlacedVertexAttribute},
    AttributeInfo{collisionDrawableUBOCount + 4, gfx::AttributeDataType::Float2, idCollisionShiftVertexAttribute},
};
const std::array<TextureInfo, 0> ShaderSource<BuiltIn::CollisionBoxShader, gfx::Backend::Type::Metal>::textures = {};

//
// Collision circle

const std::array<UniformBlockInfo, 1>
    ShaderSource<BuiltIn::CollisionCircleShader, gfx::Backend::Type::Metal>::uniforms = {
        UniformBlockInfo{true, true, sizeof(CollisionDrawableUBO), idCollisionDrawableUBO},
};
const std::array<AttributeInfo, 4> ShaderSource<BuiltIn::CollisionCircleShader, gfx::Backend::Type::Metal>::attributes =
    {
        AttributeInfo{collisionDrawableUBOCount + 0, gfx::AttributeDataType::Short2, idCollisionPosVertexAttribute},
        AttributeInfo{
            collisionDrawableUBOCount + 1, gfx::AttributeDataType::Short2, idCollisionAnchorPosVertexAttribute},
        AttributeInfo{collisionDrawableUBOCount + 2, gfx::AttributeDataType::Short2, idCollisionExtrudeVertexAttribute},
        AttributeInfo{collisionDrawableUBOCount + 3, gfx::AttributeDataType::UShort2, idCollisionPlacedVertexAttribute},
};
const std::array<TextureInfo, 0> ShaderSource<BuiltIn::CollisionCircleShader, gfx::Backend::Type::Metal>::textures = {};

} // namespace shaders
} // namespace mbgl
