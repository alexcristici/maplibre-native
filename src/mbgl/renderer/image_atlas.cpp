#include <mbgl/renderer/image_atlas.hpp>
#include <mbgl/renderer/image_manager.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/util/hash.hpp>

#include <mapbox/shelf-pack.hpp>

namespace mbgl {

ImagePosition::ImagePosition(const mapbox::Bin& bin,
                             const style::Image::Impl& image,
                             uint32_t version_,
                             std::optional<gfx::TextureHandle> handle_)
    : handle(handle_),
      pixelRatio(image.pixelRatio),
      paddedRect(bin.x, bin.y, bin.w, bin.h),
      version(version_),
      stretchX(image.stretchX),
      stretchY(image.stretchY),
      content(image.content),
      textFitWidth(image.textFitWidth),
      textFitHeight(image.textFitHeight) {}

void populateImagePatches(ImagePositions& imagePositions,
                          const ImageManager& imageManager,
                          std::vector<ImagePatch>& /*out*/ patches) {
    if (imagePositions.empty()) {
        imagePositions.reserve(imageManager.updatedImageVersions.size());
    }
    for (auto& updatedImageVersion : imageManager.updatedImageVersions) {
        const std::string& name = updatedImageVersion.first;
        const uint32_t version = updatedImageVersion.second;
        const auto it = imagePositions.find(updatedImageVersion.first);
        if (it != imagePositions.end()) {
            auto& position = it->second;
            if (position.version == version) continue;

            const auto updatedImage = imageManager.getSharedImage(name);
            if (updatedImage == nullptr) continue;

            patches.emplace_back(*updatedImage, position.paddedRect);
            position.version = version;
        }
    }
}

ImagePositions uploadImages(const ImageMap& images, const ImageVersionMap& versionMap) {
    ImagePositions imagePositions;

    auto& dynamicTextureRGBA = gfx::Context::getDynamicTextureRGBA();
    if (!dynamicTextureRGBA) {
        return imagePositions;
    }
    
    imagePositions.reserve(images.size());
    for (const auto& entry : images) {
        const style::Image::Impl& image = *entry.second;
        
        auto imageHash = util::hash(image.id);
        int32_t uniqueId = static_cast<int32_t>(sqrt(imageHash) / 2);
        auto imageHandle = dynamicTextureRGBA->addImage(image.image, uniqueId);
        assert(imageHandle.has_value());

        if (imageHandle.has_value()) {
            const auto it = versionMap.find(entry.first);
            const auto version = it != versionMap.end() ? it->second : 0;
            imagePositions.emplace(image.id, ImagePosition{*imageHandle->getBin(), image, version, imageHandle});
        }
    }

    return imagePositions;
}

} // namespace mbgl
