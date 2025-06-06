#pragma once

#include <mbgl/map/mode.hpp>
#include <mbgl/tile/tile_id.hpp>
#include <mbgl/util/mat4.hpp>
#include <mbgl/style/image_impl.hpp>
#include <mbgl/style/layer_impl.hpp>
#include <mbgl/style/types.hpp>

#include <array>
#include <memory>

namespace mbgl {

namespace gfx {
class Texture2D;
using Texture2DPtr = std::shared_ptr<Texture2D>;

class UploadPass;
} // namespace gfx

class Bucket;
class LayerRenderData;
class Tile;
class TransformState;
class PaintParameters;
class DebugBucket;
class SourcePrepareParameters;
class FeatureIndex;
class TileAtlasTextures;
class TileRenderData;

class RenderTile final {
public:
    RenderTile(UnwrappedTileID, Tile&);
    ~RenderTile();
    RenderTile(const RenderTile&) = delete;
    RenderTile(RenderTile&&) = default;
    RenderTile& operator=(const RenderTile&) = delete;

    UnwrappedTileID id;
    mat4 matrix;
    mat4 nearClippedMatrix;
    // Contains the tile ID string for painting debug information.
    std::unique_ptr<DebugBucket> debugBucket;

    mat4 translatedMatrix(const std::array<float, 2>& translation,
                          style::TranslateAnchorType anchor,
                          const TransformState&) const;

    mat4 translatedClipMatrix(const std::array<float, 2>& translation,
                              style::TranslateAnchorType anchor,
                              const TransformState&) const;

    const OverscaledTileID& getOverscaledTileID() const;
    bool holdForFade() const;

    Bucket* getBucket(const style::Layer::Impl&) const;
    const LayerRenderData* getLayerRenderData(const style::Layer::Impl&) const;
    std::optional<ImagePosition> getPattern(const std::string& pattern) const;

    const std::shared_ptr<TileAtlasTextures>& getAtlasTextures() const;

    bool getNeedsRendering() const { return needsRendering; };

    void upload(gfx::UploadPass&) const;
    void prepare(const SourcePrepareParameters&);

    static mat4 translateVtxMatrix(const UnwrappedTileID& id,
                                   const mat4& tileMatrix,
                                   const std::array<float, 2>& translation,
                                   style::TranslateAnchorType anchor,
                                   const TransformState& state,
                                   bool inViewportPixelUnits);

    mat4 translateVtxMatrix(const mat4& tileMatrix,
                            const std::array<float, 2>& translation,
                            style::TranslateAnchorType anchor,
                            const TransformState& state,
                            bool inViewportPixelUnits) const;

    void setFeatureState(const LayerFeatureStates&);

private:
    Tile& tile;
    // The following members are reset at placement stage.
    std::unique_ptr<TileRenderData> renderData;
    bool needsRendering = false;
};

} // namespace mbgl
