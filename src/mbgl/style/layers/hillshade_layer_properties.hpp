// clang-format off

// This file is generated. Edit scripts/generate-style-code.js, then run `make style-code`.

#pragma once

#include <mbgl/style/types.hpp>
#include <mbgl/style/layer_properties.hpp>
#include <mbgl/style/layers/hillshade_layer.hpp>
#include <mbgl/style/layout_property.hpp>
#include <mbgl/style/paint_property.hpp>
#include <mbgl/style/properties.hpp>
#include <mbgl/shaders/attributes.hpp>
#include <mbgl/shaders/uniforms.hpp>

namespace mbgl {
namespace style {

struct HillshadeAccentColor : PaintProperty<Color> {
    static Color defaultValue() { return Color::black(); }
};

struct HillshadeExaggeration : PaintProperty<float> {
    static float defaultValue() { return 0.5f; }
};

struct HillshadeHighlightColor : PaintProperty<Color> {
    static Color defaultValue() { return Color::white(); }
};

struct HillshadeIlluminationAnchor : PaintProperty<HillshadeIlluminationAnchorType> {
    static HillshadeIlluminationAnchorType defaultValue() { return HillshadeIlluminationAnchorType::Viewport; }
};

struct HillshadeIlluminationDirection : PaintProperty<float> {
    static float defaultValue() { return 335.f; }
};

struct HillshadeShadowColor : PaintProperty<Color> {
    static Color defaultValue() { return Color::black(); }
};

class HillshadePaintProperties : public Properties<
    HillshadeAccentColor,
    HillshadeExaggeration,
    HillshadeHighlightColor,
    HillshadeIlluminationAnchor,
    HillshadeIlluminationDirection,
    HillshadeShadowColor
> {};

class HillshadeLayerProperties final : public LayerProperties {
public:
    explicit HillshadeLayerProperties(Immutable<HillshadeLayer::Impl>);
    HillshadeLayerProperties(
        Immutable<HillshadeLayer::Impl>,
        HillshadePaintProperties::PossiblyEvaluated);
    ~HillshadeLayerProperties() override;

    unsigned long constantsMask() const override;

    expression::Dependency getDependencies() const noexcept override;

    const HillshadeLayer::Impl& layerImpl() const noexcept;
    // Data members.
    HillshadePaintProperties::PossiblyEvaluated evaluated;
};

} // namespace style
} // namespace mbgl

// clang-format on
