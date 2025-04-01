#include <mbgl/gfx/dynamic_texture.hpp>
#include <mbgl/gfx/context.hpp>

namespace mbgl {
namespace gfx {

std::mutex mutex;
std::mutex mutex2;

DynamicTexture::DynamicTexture(Context& context, Size size, TexturePixelType pixelType) {
    mapbox::ShelfPack::ShelfPackOptions options;
    options.autoResize = false;
    shelfPack = mapbox::ShelfPack(size.width, size.height, options);

    texture = context.createTexture2D();
    texture->setSize(size);
    texture->setFormat(pixelType, TextureChannelDataType::UnsignedByte);
    texture->setSamplerConfiguration({gfx::TextureFilterType::Linear, gfx::TextureWrapType::Clamp, gfx::TextureWrapType::Clamp});
    texture->create();
}

std::optional<TextureHandle> DynamicTexture::addImage(const uint8_t* pixelData, const Size& imageSize, int32_t id) {
    mutex.lock();
    mapbox::Bin* bin = shelfPack.packOne(id, imageSize.width + 2, imageSize.height + 2);
    if (!bin) {
        mutex.unlock();
        return std::nullopt;
    }

    if (bin->refcount() == 1) {
#if MLN_DEFER_UPLOAD_ON_RENDER_THREAD
        auto size = imageSize.area() * texture->getPixelStride();
        auto imageData = std::make_unique<uint8_t[]>(size);
        std::copy(pixelData, pixelData + size, imageData.get());
        imagesToUpload.emplace(bin, std::move(imageData));
#else
        texture->uploadSubRegion(pixelData, imageSize, bin->x + 1, bin->y + 1);
#endif
    }
    mutex.unlock();
    return TextureHandle(bin);
}

void DynamicTexture::uploadDeferredImages() {
    mutex.lock();
    for (auto& pair : imagesToUpload) {
        const auto& bin = pair.first;
        texture->uploadSubRegion(pair.second.get(), Size(bin->w - 2, bin->h - 2), bin->x + 1, bin->y + 1);
    }
    imagesToUpload.clear();
    mutex.unlock();
}

void DynamicTexture::removeTexture(const TextureHandle& texHandle) {
    mutex.lock();
    const auto& bin = texHandle.getBin();
    auto refcount = shelfPack.unref(*bin);
    if (refcount == 0) {
        imagesToUpload.erase(bin);
#if !defined(NDEBUG)
        Size size = Size(bin->w, bin->h);
        std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(size.area() * texture->numChannels());
        memset(data.get(), 0, size.area() * texture->numChannels());
        texture->uploadSubRegion(data.get(), size, bin->x, bin->y);
#endif
    }
    mutex.unlock();
}

GlyphTexturePack DynamicTextureAtlas::uploadGlyphs(const GlyphMap& glyphs) {
    GlyphTexturePack glyphTexPack;

    mutex2.lock();
    for (const auto& dynamicTexture: dynamicTextures) {
        if (dynamicTexture->getPixelFormat() != TexturePixelType::Alpha) {
            continue;
        }
        bool hasSpace = true;
        for (const auto& glyphMapEntry : glyphs) {
            FontStackHash fontStack = glyphMapEntry.first;
            
            for (const auto& glyphEntry : glyphMapEntry.second) {
                if (glyphEntry.second && (*glyphEntry.second)->bitmap.valid()) {
                    const Glyph& glyph = **glyphEntry.second;
                    
                    int32_t uniqueId = static_cast<int32_t>(sqrt(fontStack) / 2 + glyph.id);
                    auto glyphHandle = dynamicTexture->addImage(glyph.bitmap, uniqueId);
                    if (!glyphHandle) {
                        hasSpace = false;
                        break;
                    }
                    glyphTexPack.handle.bins.emplace_back(glyphHandle->getBin());
                }
            }
            if (!hasSpace) {
                for (const auto& bin : glyphTexPack.handle.bins) {
                    dynamicTexture->removeTexture(TextureHandle(bin));
                }
                glyphTexPack.handle.bins.clear();
                break;
            }
        }
        if (hasSpace) {
            glyphTexPack.handle.dynamicTexture = dynamicTexture;
            break;
        }
    }

    if (!glyphTexPack.handle.dynamicTexture) {
        glyphTexPack.handle.dynamicTexture = std::make_shared<gfx::DynamicTexture>(context, Size{2048, 2048}, TexturePixelType::Alpha);
        dynamicTextures.emplace_back(glyphTexPack.handle.dynamicTexture);
    }
    for (const auto& glyphMapEntry : glyphs) {
        FontStackHash fontStack = glyphMapEntry.first;
        GlyphPositionMap& positions = glyphTexPack.positions[fontStack];

        for (const auto& entry : glyphMapEntry.second) {
            if (entry.second && (*entry.second)->bitmap.valid()) {
                const Glyph& glyph = **entry.second;

                int32_t uniqueId = static_cast<int32_t>(sqrt(fontStack) / 2 + glyph.id);
                auto glyphHandle = glyphTexPack.handle.dynamicTexture->addImage(glyph.bitmap, uniqueId);
                assert(glyphHandle.has_value());
                
                if (glyphHandle.has_value()) {
                    const auto& bin = glyphHandle->getBin();
                    glyphTexPack.handle.bins.emplace_back(bin);
                    positions.emplace(glyph.id,
                                      GlyphPosition{Rect<uint16_t>{static_cast<uint16_t>(bin->x),
                                                                   static_cast<uint16_t>(bin->y),
                                                                   static_cast<uint16_t>(bin->w),
                                                                   static_cast<uint16_t>(bin->h)},
                                                    glyph.metrics});
                }
            }
        }
    }
    mutex2.unlock();
    return glyphTexPack;
}

} // namespace gfx
} // namespace mbgl
