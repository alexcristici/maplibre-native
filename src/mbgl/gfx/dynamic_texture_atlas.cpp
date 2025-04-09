#include <mbgl/gfx/dynamic_texture_atlas.hpp>
#include <mbgl/gfx/context.hpp>

namespace mbgl {
namespace gfx {

constexpr const uint16_t padding = ImagePosition::padding;
constexpr const Size startSize = {512, 512};

GlyphAtlas DynamicTextureAtlas::uploadGlyphs(const GlyphMap& glyphs) {
    using GlyphsToUpload = std::vector<std::tuple<TextureHandle, Immutable<Glyph>, FontStackHash>>;
   
    GlyphAtlas glyphAtlas;
    if (!glyphs.size()) {
        return glyphAtlas;
    }
    
    mutex.lock();

    size_t dynTexIndex = 0;
    Size dynTexSize = startSize;
    GlyphsToUpload glyphsToUpload;
    
    while (!glyphAtlas.dynamicTexture) {
        if (dynTexIndex < dynamicTextures.size()) {
            glyphAtlas.dynamicTexture = dynamicTextures[dynTexIndex++];
        } else {
            glyphAtlas.dynamicTexture = std::make_shared<gfx::DynamicTexture>(context, dynTexSize, TexturePixelType::Alpha);
            dynTexSize = Size(dynTexSize.width * 2, dynTexSize.height * 2);
            dynTexIndex++;
        }
        
        if (glyphAtlas.dynamicTexture->getPixelFormat() != TexturePixelType::Alpha) {
            glyphAtlas.dynamicTexture = nullptr;
            continue;
        }
        
        bool hasSpace = true;
        for (const auto& glyphMapEntry : glyphs) {
            FontStackHash fontStack = glyphMapEntry.first;

            for (const auto& glyphEntry : glyphMapEntry.second) {
                const auto& glyph = glyphEntry.second;
                
                if (glyph.has_value() && glyph.value()->bitmap.valid()) {
                    int32_t uniqueId = static_cast<int32_t>(sqrt(fontStack) / 2 + glyph.value()->id);
                    const auto size = Size(glyph.value()->bitmap.size.width + 2 * padding, glyph.value()->bitmap.size.height + 2  * padding);
                    const auto& texHandle = glyphAtlas.dynamicTexture->reserveSize(size, uniqueId);
                    if (!texHandle) {
                        hasSpace = false;
                        break;
                    }
                    glyphsToUpload.emplace_back(std::make_tuple(*texHandle, glyph.value(), fontStack));
                }
            }
            if (!hasSpace) {
                for (const auto& tuple : glyphsToUpload) {
                    const auto& texHandle = std::get<0>(tuple);
                    glyphAtlas.dynamicTexture->removeTexture(texHandle);
                }
                glyphsToUpload.clear();
                glyphAtlas.dynamicTexture = nullptr;
                break;
            }
        }
    }
    if (dynTexIndex > dynamicTextures.size()) {
        dynamicTextures.emplace_back(glyphAtlas.dynamicTexture);
    }

    for (const auto& tuple : glyphsToUpload) {
        const auto& texHandle = std::get<0>(tuple);
        const auto& glyph = std::get<1>(tuple);
        const auto fontStack = std::get<2>(tuple);
        
        const auto& rect = texHandle.getRectangle();
        AlphaImage paddedImage(Size{rect.w, rect.h});
        paddedImage.fill(0);
        AlphaImage::copy(glyph->bitmap, paddedImage, {0, 0}, {padding, padding}, glyph->bitmap.size);
        
        glyphAtlas.dynamicTexture->uploadImage(paddedImage.data.get(), texHandle);
        glyphAtlas.textureHandles.emplace_back(texHandle);
        glyphAtlas.glyphPositions[fontStack].emplace(glyph->id, GlyphPosition{rect, glyph->metrics});
    }
    mutex.unlock();
    return glyphAtlas;
}

ImageAtlas DynamicTextureAtlas::uploadIconsAndPatterns(const ImageMap& icons,
                                                       const ImageMap& patterns,
                                                       const ImageVersionMap& versionMap) {
    using ImagesToUpload = std::vector<std::pair<TextureHandle, Immutable<style::Image::Impl>>>;
    
    ImageAtlas imageAtlas;
    if (!icons.size() && !patterns.size()) {
        return imageAtlas;
    }
    
    mutex.lock();

    size_t dynTexIndex = 0;
    Size dynTexSize = startSize;
    ImagesToUpload iconsToUpload;
    ImagesToUpload patternsToUpload;
    
    while (!imageAtlas.dynamicTexture) {
        if (dynTexIndex < dynamicTextures.size()) {
            imageAtlas.dynamicTexture = dynamicTextures[dynTexIndex++];
        } else {
            imageAtlas.dynamicTexture = std::make_shared<gfx::DynamicTexture>(context, dynTexSize, TexturePixelType::RGBA);
            dynTexSize = Size(dynTexSize.width * 2, dynTexSize.height * 2);
            dynTexIndex++;
        }
        
        if (imageAtlas.dynamicTexture->getPixelFormat() != TexturePixelType::RGBA) {
            imageAtlas.dynamicTexture = nullptr;
            continue;
        }

        bool hasSpace = true;
        for (const auto& iconEntry : icons) {
            const auto& icon = iconEntry.second;

            auto imageHash = util::hash(icon->id);
            int32_t uniqueId = static_cast<int32_t>(sqrt(imageHash) / 2);
            const auto size = Size(icon->image.size.width + 2 * padding, icon->image.size.height + 2  * padding);
            const auto& texHandle = imageAtlas.dynamicTexture->reserveSize(size, uniqueId);
            if (!texHandle) {
                hasSpace = false;
                break;
            }
            iconsToUpload.emplace_back(std::make_tuple(*texHandle, icon));
        }
        if (hasSpace) {
            for (const auto& patternEntry : patterns) {
                const auto& pattern = patternEntry.second;
                
                auto patternHash = util::hash(pattern->id);
                int32_t uniqueId = static_cast<int32_t>(sqrt(patternHash) / 2);
                const auto size = Size(pattern->image.size.width + 2 * padding, pattern->image.size.height + 2  * padding);
                const auto& texHandle = imageAtlas.dynamicTexture->reserveSize(size, uniqueId);
                if (!texHandle) {
                    hasSpace = false;
                    break;
                }
                patternsToUpload.emplace_back(std::make_tuple(*texHandle, pattern));
            }
        }
        if (!hasSpace) {
            for (const auto& pair : iconsToUpload) {
                const auto& texHandle = pair.first;
                imageAtlas.dynamicTexture->removeTexture(texHandle);
            }
            iconsToUpload.clear();
            for (const auto& pair : patternsToUpload) {
                const auto& texHandle = pair.first;
                imageAtlas.dynamicTexture->removeTexture(texHandle);
            }
            patternsToUpload.clear();
            imageAtlas.dynamicTexture = nullptr;
            continue;
        }
    }
    if (dynTexIndex > dynamicTextures.size()) {
        dynamicTextures.emplace_back(imageAtlas.dynamicTexture);
    }
    
    imageAtlas.iconPositions.reserve(icons.size());
    for (const auto& pair : iconsToUpload) {
        const auto& texHandle = pair.first;
        const auto& icon = pair.second;
        
        const auto& rect = texHandle.getRectangle();
        PremultipliedImage paddedImage(Size{rect.w, rect.h});
        paddedImage.fill(0);
        PremultipliedImage::copy(icon->image, paddedImage, {0, 0}, {padding, padding}, icon->image.size);
        
        imageAtlas.dynamicTexture->uploadImage(paddedImage.data.get(), texHandle);
        imageAtlas.textureHandles.emplace_back(texHandle);
        const auto it = versionMap.find(icon->id);
        const auto version = it != versionMap.end() ? it->second : 0;
        imageAtlas.iconPositions.emplace(icon->id, ImagePosition{rect, *icon, version});
    }
    
    imageAtlas.patternPositions.reserve(patterns.size());
    for (const auto& pair : patternsToUpload) {
        const auto& texHandle = pair.first;
        const auto& pattern = pair.second;
        
        const auto& rect = texHandle.getRectangle();
        PremultipliedImage paddedImage(Size{rect.w, rect.h});
        paddedImage.fill(0);
        PremultipliedImage::copy(pattern->image, paddedImage, {0, 0}, {padding, padding}, pattern->image.size);
        
        const uint32_t x = padding;
        const uint32_t y = padding;
        const uint32_t w = pattern->image.size.width;
        const uint32_t h = pattern->image.size.height;

        // Add 1 pixel wrapped padding on each side of the image.
        PremultipliedImage::copy(pattern->image, paddedImage, {0, h - 1}, {x, y - 1}, {w, 1}); // T
        PremultipliedImage::copy(pattern->image, paddedImage, {0, 0},     {x, y + h}, {w, 1}); // B
        PremultipliedImage::copy(pattern->image, paddedImage, {w - 1, 0}, {x - 1, y}, {1, h}); // L
        PremultipliedImage::copy(pattern->image, paddedImage, {0, 0},     {x + w, y}, {1, h}); // R
        
        imageAtlas.dynamicTexture->uploadImage(paddedImage.data.get(), texHandle);
        imageAtlas.textureHandles.emplace_back(texHandle);
        const auto it = versionMap.find(pattern->id);
        const auto version = it != versionMap.end() ? it->second : 0;
        imageAtlas.patternPositions.emplace(pattern->id, ImagePosition{rect, *pattern, version});
    }

    mutex.unlock();
    return imageAtlas;
}

void DynamicTextureAtlas::uploadDeferredImages() {
    mutex.lock();
    for (const auto& dynamicTexture : dynamicTextures) {
        dynamicTexture->uploadDeferredImages();
    }
    mutex.unlock();
}

void DynamicTextureAtlas::removeTextures(const std::vector<TextureHandle>& textureHandles,
                                         const DynamicTexturePtr& dynamicTexture) {
    if (!dynamicTexture) {
        return;
    }
    mutex.lock();
    for (const auto& texHandle : textureHandles) {
        dynamicTexture->removeTexture(texHandle);
    }
    if (dynamicTexture->isEmpty()) {
        auto iterator = std::find(dynamicTextures.begin(), dynamicTextures.end(), dynamicTexture);
        if(iterator != dynamicTextures.end()) {
            dynamicTextures.erase(iterator);
        }
    }
    mutex.unlock();
}

} // namespace gfx
} // namespace mbgl
