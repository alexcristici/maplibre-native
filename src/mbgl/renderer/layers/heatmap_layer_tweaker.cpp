#include <mbgl/renderer/layers/heatmap_layer_tweaker.hpp>

#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/drawable.hpp>
#include <mbgl/programs/heatmap_program.hpp>
#include <mbgl/renderer/layer_group.hpp>
#include <mbgl/renderer/paint_parameters.hpp>
#include <mbgl/renderer/render_static_data.hpp>
#include <mbgl/renderer/render_tree.hpp>
#include <mbgl/shaders/heatmap_layer_ubo.hpp>
#include <mbgl/style/layers/heatmap_layer_properties.hpp>
#include <mbgl/util/convert.hpp>

#if MLN_RENDER_BACKEND_METAL
#include <mbgl/shaders/mtl/heatmap.hpp>
#endif

namespace mbgl {

using namespace style;
using namespace shaders;

void HeatmapLayerTweaker::execute(LayerGroupBase& layerGroup, const PaintParameters& parameters) {
    auto& context = parameters.context;
    const auto zoom = static_cast<float>(parameters.state.getZoom());
    const auto& evaluated = static_cast<const HeatmapLayerProperties&>(*evaluatedProperties).evaluated;

    if (layerGroup.empty()) {
        return;
    }

#if !defined(NDEBUG)
    const auto label = layerGroup.getName() + "-update-uniforms";
    const auto debugGroup = parameters.encoder->createDebugGroup(label.c_str());
#endif

    if (!evaluatedPropsUniformBuffer || propertiesUpdated) {
        const HeatmapEvaluatedPropsUBO evaluatedPropsUBO = {
            /* .weight = */ evaluated.get<HeatmapWeight>().constantOr(HeatmapWeight::defaultValue()),
            /* .radius = */ evaluated.get<HeatmapRadius>().constantOr(HeatmapRadius::defaultValue()),
            /* .intensity = */ evaluated.get<HeatmapIntensity>(),
            /* .padding = */ 0};
        parameters.context.emplaceOrUpdateUniformBuffer(evaluatedPropsUniformBuffer, &evaluatedPropsUBO);
        propertiesUpdated = false;
    }
    auto& layerUniforms = layerGroup.mutableUniformBuffers();
    layerUniforms.set(idHeatmapEvaluatedPropsUBO, evaluatedPropsUniformBuffer);

#if MLN_RENDER_BACKEND_METAL || MLN_RENDER_BACKEND_VULKAN
    int i = 0;
    std::vector<HeatmapDrawableUBO> drawableUBOVector(layerGroup.getDrawableCount());
#endif
    
    visitLayerGroupDrawables(layerGroup, [&](gfx::Drawable& drawable) {
        if (!drawable.getTileID() || !checkTweakDrawable(drawable)) {
            return;
        }

        const UnwrappedTileID tileID = drawable.getTileID()->toUnwrapped();

        auto* binders = static_cast<HeatmapProgram::Binders*>(drawable.getBinders());
        const auto* tile = drawable.getRenderTile();
        if (!binders || !tile) {
            assert(false);
            return;
        }

        constexpr bool nearClipped = false;
        constexpr bool inViewportPixelUnits = false;
        const auto matrix = getTileMatrix(
            tileID, parameters, {0.f, 0.f}, TranslateAnchorType::Viewport, nearClipped, inViewportPixelUnits, drawable);
        
#if MLN_RENDER_BACKEND_METAL || MLN_RENDER_BACKEND_VULKAN
        drawableUBOVector[i] = {
#elif MLN_RENDER_BACKEND_OPENGL
        const HeatmapDrawableUBO drawableUBO = {
#endif
            /* .matrix = */ util::cast<float>(matrix),
            /* .extrude_scale = */ tileID.pixelsToTileUnits(1.0f, zoom),

            /* .weight_t = */ std::get<0>(binders->get<HeatmapWeight>()->interpolationFactor(zoom)),
            /* .radius_t = */ std::get<0>(binders->get<HeatmapRadius>()->interpolationFactor(zoom)),
            /* .pad1 = */ 0
        };
#if MLN_RENDER_BACKEND_METAL || MLN_RENDER_BACKEND_VULKAN
        drawable.setUBOIndex(i++);
#elif MLN_RENDER_BACKEND_OPENGL
        auto& drawableUniforms = drawable.mutableUniformBuffers();
        drawableUniforms.createOrUpdate(idHeatmapDrawableUBO, &drawableUBO, context);
#endif
    });
        
#if MLN_RENDER_BACKEND_METAL || MLN_RENDER_BACKEND_VULKAN
    const size_t drawableUBOVectorSize = sizeof(HeatmapDrawableUBO) * drawableUBOVector.size();
    if (!drawableUniformBuffer || drawableUniformBuffer->getSize() < drawableUBOVectorSize) {
        drawableUniformBuffer = context.createUniformBuffer(drawableUBOVector.data(), drawableUBOVectorSize, false, true);
    } else {
        drawableUniformBuffer->update(drawableUBOVector.data(), drawableUBOVectorSize);
    }

    layerUniforms.set(idSymbolDrawableUBO, drawableUniformBuffer);
#endif
}

} // namespace mbgl
