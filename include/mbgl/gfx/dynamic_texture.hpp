#pragma once

#include <mbgl/gfx/texture2d.hpp>
#include <mbgl/gfx/types.hpp>
#include <mbgl/util/containers.hpp>
#include <mbgl/util/image.hpp>

#include <mapbox/shelf-pack.hpp>

namespace mbgl {

namespace gfx {

class Context;
class Texture2D;
using Texture2DPtr = std::shared_ptr<gfx::Texture2D>;
using ImageToUpload = std::pair<std::unique_ptr<uint8_t[]>, mapbox::Bin*>;

class TextureHandle {
public:
    TextureHandle(mapbox::Bin* bin_, bool imageUploadDeferred_)
        : bin(bin_),
        imageUploadDeferred(imageUploadDeferred_) {};
    ~TextureHandle() = default;

    mapbox::Bin* getBin() const { return bin; }
    bool isImageUploadDeferred() const { return imageUploadDeferred; }

private:
    mapbox::Bin* bin;
    bool imageUploadDeferred = false;
};

class DynamicTexture {
public:
    DynamicTexture(Context& context, Size size, TexturePixelType pixelType);
    ~DynamicTexture() = default;

    const Texture2DPtr& getTextureAtlas();

    template <typename Image>
    std::optional<TextureHandle> addImage(const Image& image, int32_t id = -1) {
        return addImage(image.data ? image.data.get() : nullptr, image.size, id);
    }
    std::optional<TextureHandle> addImage(const uint8_t* pixelData, const Size& imageSize, int32_t id = -1);
    void uploadDeferredImages();
    void removeTexture(const TextureHandle& texHandle);

private:
    Texture2DPtr textureAtlas;
    mapbox::ShelfPack shelfPack;
    std::vector<ImageToUpload> imagesToUpload;
};

#define MLN_DEFER_UPLOAD_ON_RENDER_THREAD (MLN_RENDER_BACKEND_OPENGL || MLN_RENDER_BACKEND_VULKAN)

} // namespace gfx
} // namespace mbgl
