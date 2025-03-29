#include <mbgl/text/glyph_atlas.hpp>
#include <mbgl/gfx/context.hpp>

#include <mapbox/shelf-pack.hpp>

namespace mbgl {

GlyphsUploadResult uploadGlyphs(const GlyphMap& glyphs) {
    GlyphsUploadResult glyphsUploadResult;

    for (const auto& glyphMapEntry : glyphs) {
        FontStackHash fontStack = glyphMapEntry.first;
        GlyphPositionMap& positions = glyphsUploadResult.glyphPositions[fontStack];

        for (const auto& entry : glyphMapEntry.second) {
            if (entry.second && (*entry.second)->bitmap.valid()) {
                const Glyph& glyph = **entry.second;

                int32_t uniqueId = static_cast<int32_t>(sqrt(fontStack) / 2 + glyph.id);
                auto glyphHandle = gfx::Context::getDynamicTextureAlpha()->addImage(glyph.bitmap, uniqueId);
                if (glyphHandle) {
                    positions.emplace(glyph.id, GlyphPosition{*glyphHandle, glyph.metrics});
                    if (glyphHandle->isImageUploadDeferred()) {
                        glyphsUploadResult.glyphsToUpload.emplace_back(std::make_pair(*entry.second, *glyphHandle));
                    }
                }
            }
        }
    }
    return glyphsUploadResult;
}

} // namespace mbgl
