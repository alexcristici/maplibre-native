#pragma once

#include <mbgl/gfx/uniform_block.hpp>
#include <mbgl/gfx/uniform_buffer.hpp>

namespace mbgl {
namespace gl {

class UniformBlockGL final : public gfx::UniformBlock {
public:
    UniformBlockGL(int index_, std::size_t size_)
        : UniformBlock(index_, size_) {}
    UniformBlockGL(const UniformBlockGL& other)
        : UniformBlock(other) {}
    UniformBlockGL(UniformBlockGL&& other)
        : UniformBlock(std::move(other)) {}
    ~UniformBlockGL() override = default;
    void bindBuffer(const gfx::UniformBuffer& uniformBuffer) override;
    void unbindBuffer() override;
};

/// Stores a collection of uniform blocks by name
class UniformBlockArrayGL final : public gfx::UniformBlockArray {
public:
    UniformBlockArrayGL() = default;
    UniformBlockArrayGL(UniformBlockArrayGL&& other)
        : UniformBlockArray(std::move(other)) {}
    UniformBlockArrayGL(const UniformBlockArrayGL&) = delete;

    UniformBlockArrayGL& operator=(UniformBlockArrayGL&& other) {
        UniformBlockArray::operator=(std::move(other));
        return *this;
    }
    UniformBlockArrayGL& operator=(const UniformBlockArrayGL& other) {
        UniformBlockArray::operator=(other);
        return *this;
    }

private:
    std::unique_ptr<gfx::UniformBlock> create(int index, std::size_t size) override {
        return std::make_unique<UniformBlockGL>(index, size);
    }
    std::unique_ptr<gfx::UniformBlock> copy(const gfx::UniformBlock& uniformBlocks) override {
        return std::make_unique<UniformBlockGL>(static_cast<const UniformBlockGL&>(uniformBlocks));
    }
};

} // namespace gl
} // namespace mbgl
