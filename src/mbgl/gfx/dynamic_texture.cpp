#include <mbgl/gfx/dynamic_texture.hpp>
#include <mbgl/gfx/context.hpp>

namespace mbgl {
namespace gfx {

std::mutex mutex;

DynamicTexture::DynamicTexture(Context& context, Size size, TexturePixelType pixelType) {
    mapbox::ShelfPack::ShelfPackOptions options;
    options.autoResize = false;
    shelfPack = mapbox::ShelfPack(size.width, size.height, options);

    textureAtlas = context.createTexture2D();
    textureAtlas->setSize(size);
    textureAtlas->setFormat(pixelType, TextureChannelDataType::UnsignedByte);
    textureAtlas->setSamplerConfiguration(
        {gfx::TextureFilterType::Linear, gfx::TextureWrapType::Clamp, gfx::TextureWrapType::Clamp});
    textureAtlas->create();
}

const Texture2DPtr& DynamicTexture::getTextureAtlas() {
    assert(textureAtlas);
    return textureAtlas;
}

std::optional<TextureHandle> DynamicTexture::addImage(const uint8_t* pixelData, const Size& imageSize, int32_t id) {
    mutex.lock();
    mapbox::Bin* bin = shelfPack.packOne(id, imageSize.width + 2, imageSize.height + 2);
    if (!bin) {
        mutex.unlock();
        return std::nullopt;
    }
    bool imageUploadDeferred = false;
    if (bin->refcount() == 1) {
#if MLN_DEFER_UPLOAD_ON_RENDER_THREAD
        auto size = imageSize.area() * textureAtlas->getPixelStride();
        auto imageData = std::make_unique<uint8_t[]>(size);
        std::copy(pixelData, pixelData + size, imageData.get());
        imagesToUpload.emplace_back(std::make_pair(std::move(imageData), bin));
        //imageUploadDeferred = true;
#else
        textureAtlas->uploadSubRegion(pixelData, imageSize, bin->x + 1, bin->y + 1);
#endif
    }
    mutex.unlock();
    return TextureHandle(bin, imageUploadDeferred);
}

void DynamicTexture::uploadDeferredImages() {
    mutex.lock();
    for (auto& pair : imagesToUpload) {
        const auto& bin = pair.second;
        textureAtlas->uploadSubRegion(pair.first.get(), Size(bin->w - 2, bin->h - 2), bin->x + 1, bin->y + 1);
    }
    imagesToUpload.clear();
    mutex.unlock();
}

void DynamicTexture::removeTexture(const TextureHandle& texHandle) {
    mutex.lock();
    auto refcount = shelfPack.unref(*texHandle.getBin());
#if !defined(NDEBUG)
    if (refcount == 0) {
        Size size = Size(texHandle.getBin()->w, texHandle.getBin()->h);
        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(size.area() * textureAtlas->numChannels());
        memset(data.get(), 0, size.area() * textureAtlas->numChannels());
        textureAtlas->uploadSubRegion(data.get(), size, texHandle.getBin()->x, texHandle.getBin()->y);
    }
#endif
    mutex.unlock();
}

} // namespace gfx
} // namespace mbgl
