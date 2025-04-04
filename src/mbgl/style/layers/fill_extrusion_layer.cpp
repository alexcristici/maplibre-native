// clang-format off

// This file is generated. Edit scripts/generate-style-code.js, then run `make style-code`.

#include <mbgl/style/layers/fill_extrusion_layer.hpp>
#include <mbgl/style/layers/fill_extrusion_layer_impl.hpp>
#include <mbgl/style/layer_observer.hpp>
#include <mbgl/style/conversion/color_ramp_property_value.hpp>
#include <mbgl/style/conversion/constant.hpp>
#include <mbgl/style/conversion/property_value.hpp>
#include <mbgl/style/conversion/transition_options.hpp>
#include <mbgl/style/conversion/json.hpp>
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/util/traits.hpp>

#include <mapbox/eternal.hpp>

namespace mbgl {
namespace style {


// static
const LayerTypeInfo* FillExtrusionLayer::Impl::staticTypeInfo() noexcept {
    const static LayerTypeInfo typeInfo{.type="fill-extrusion",
                                        .source=LayerTypeInfo::Source::Required,
                                        .pass3d=LayerTypeInfo::Pass3D::Required,
                                        .layout=LayerTypeInfo::Layout::Required,
                                        .fadingTiles=LayerTypeInfo::FadingTiles::NotRequired,
                                        .crossTileIndex=LayerTypeInfo::CrossTileIndex::NotRequired,
                                        .tileKind=LayerTypeInfo::TileKind::Geometry};
    return &typeInfo;
}

FillExtrusionLayer::FillExtrusionLayer(const std::string& layerID, const std::string& sourceID)
    : Layer(makeMutable<Impl>(layerID, sourceID)) {
}

FillExtrusionLayer::FillExtrusionLayer(Immutable<Impl> impl_)
    : Layer(std::move(impl_)) {
}

FillExtrusionLayer::~FillExtrusionLayer() = default;

const FillExtrusionLayer::Impl& FillExtrusionLayer::impl() const {
    return static_cast<const Impl&>(*baseImpl);
}

Mutable<FillExtrusionLayer::Impl> FillExtrusionLayer::mutableImpl() const {
    return makeMutable<Impl>(impl());
}

std::unique_ptr<Layer> FillExtrusionLayer::cloneRef(const std::string& id_) const {
    auto impl_ = mutableImpl();
    impl_->id = id_;
    impl_->paint = FillExtrusionPaintProperties::Transitionable();
    return std::make_unique<FillExtrusionLayer>(std::move(impl_));
}

void FillExtrusionLayer::Impl::stringifyLayout(rapidjson::Writer<rapidjson::StringBuffer>&) const {
}

// Layout properties


// Paint properties

PropertyValue<float> FillExtrusionLayer::getDefaultFillExtrusionBase() {
    return {0.f};
}

const PropertyValue<float>& FillExtrusionLayer::getFillExtrusionBase() const {
    return impl().paint.template get<FillExtrusionBase>().value;
}

void FillExtrusionLayer::setFillExtrusionBase(const PropertyValue<float>& value) {
    if (value == getFillExtrusionBase())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionBase>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionBaseTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionBase>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionBaseTransition() const {
    return impl().paint.template get<FillExtrusionBase>().options;
}

PropertyValue<Color> FillExtrusionLayer::getDefaultFillExtrusionColor() {
    return {Color::black()};
}

const PropertyValue<Color>& FillExtrusionLayer::getFillExtrusionColor() const {
    return impl().paint.template get<FillExtrusionColor>().value;
}

void FillExtrusionLayer::setFillExtrusionColor(const PropertyValue<Color>& value) {
    if (value == getFillExtrusionColor())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionColor>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionColorTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionColor>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionColorTransition() const {
    return impl().paint.template get<FillExtrusionColor>().options;
}

PropertyValue<float> FillExtrusionLayer::getDefaultFillExtrusionHeight() {
    return {0.f};
}

const PropertyValue<float>& FillExtrusionLayer::getFillExtrusionHeight() const {
    return impl().paint.template get<FillExtrusionHeight>().value;
}

void FillExtrusionLayer::setFillExtrusionHeight(const PropertyValue<float>& value) {
    if (value == getFillExtrusionHeight())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionHeight>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionHeightTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionHeight>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionHeightTransition() const {
    return impl().paint.template get<FillExtrusionHeight>().options;
}

PropertyValue<float> FillExtrusionLayer::getDefaultFillExtrusionOpacity() {
    return {1.f};
}

const PropertyValue<float>& FillExtrusionLayer::getFillExtrusionOpacity() const {
    return impl().paint.template get<FillExtrusionOpacity>().value;
}

void FillExtrusionLayer::setFillExtrusionOpacity(const PropertyValue<float>& value) {
    if (value == getFillExtrusionOpacity())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionOpacity>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionOpacityTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionOpacity>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionOpacityTransition() const {
    return impl().paint.template get<FillExtrusionOpacity>().options;
}

PropertyValue<expression::Image> FillExtrusionLayer::getDefaultFillExtrusionPattern() {
    return {{}};
}

const PropertyValue<expression::Image>& FillExtrusionLayer::getFillExtrusionPattern() const {
    return impl().paint.template get<FillExtrusionPattern>().value;
}

void FillExtrusionLayer::setFillExtrusionPattern(const PropertyValue<expression::Image>& value) {
    if (value == getFillExtrusionPattern())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionPattern>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionPatternTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionPattern>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionPatternTransition() const {
    return impl().paint.template get<FillExtrusionPattern>().options;
}

PropertyValue<std::array<float, 2>> FillExtrusionLayer::getDefaultFillExtrusionTranslate() {
    return {{{0.f, 0.f}}};
}

const PropertyValue<std::array<float, 2>>& FillExtrusionLayer::getFillExtrusionTranslate() const {
    return impl().paint.template get<FillExtrusionTranslate>().value;
}

void FillExtrusionLayer::setFillExtrusionTranslate(const PropertyValue<std::array<float, 2>>& value) {
    if (value == getFillExtrusionTranslate())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionTranslate>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionTranslateTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionTranslate>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionTranslateTransition() const {
    return impl().paint.template get<FillExtrusionTranslate>().options;
}

PropertyValue<TranslateAnchorType> FillExtrusionLayer::getDefaultFillExtrusionTranslateAnchor() {
    return {TranslateAnchorType::Map};
}

const PropertyValue<TranslateAnchorType>& FillExtrusionLayer::getFillExtrusionTranslateAnchor() const {
    return impl().paint.template get<FillExtrusionTranslateAnchor>().value;
}

void FillExtrusionLayer::setFillExtrusionTranslateAnchor(const PropertyValue<TranslateAnchorType>& value) {
    if (value == getFillExtrusionTranslateAnchor())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionTranslateAnchor>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionTranslateAnchorTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionTranslateAnchor>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionTranslateAnchorTransition() const {
    return impl().paint.template get<FillExtrusionTranslateAnchor>().options;
}

PropertyValue<bool> FillExtrusionLayer::getDefaultFillExtrusionVerticalGradient() {
    return {true};
}

const PropertyValue<bool>& FillExtrusionLayer::getFillExtrusionVerticalGradient() const {
    return impl().paint.template get<FillExtrusionVerticalGradient>().value;
}

void FillExtrusionLayer::setFillExtrusionVerticalGradient(const PropertyValue<bool>& value) {
    if (value == getFillExtrusionVerticalGradient())
        return;
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionVerticalGradient>().value = value;
    baseImpl = std::move(impl_);
    observer->onLayerChanged(*this);
}

void FillExtrusionLayer::setFillExtrusionVerticalGradientTransition(const TransitionOptions& options) {
    auto impl_ = mutableImpl();
    impl_->paint.template get<FillExtrusionVerticalGradient>().options = options;
    baseImpl = std::move(impl_);
}

TransitionOptions FillExtrusionLayer::getFillExtrusionVerticalGradientTransition() const {
    return impl().paint.template get<FillExtrusionVerticalGradient>().options;
}

using namespace conversion;

namespace {

constexpr uint8_t kPaintPropertyCount = 16u;

enum class Property : uint8_t {
    FillExtrusionBase,
    FillExtrusionColor,
    FillExtrusionHeight,
    FillExtrusionOpacity,
    FillExtrusionPattern,
    FillExtrusionTranslate,
    FillExtrusionTranslateAnchor,
    FillExtrusionVerticalGradient,
    FillExtrusionBaseTransition,
    FillExtrusionColorTransition,
    FillExtrusionHeightTransition,
    FillExtrusionOpacityTransition,
    FillExtrusionPatternTransition,
    FillExtrusionTranslateTransition,
    FillExtrusionTranslateAnchorTransition,
    FillExtrusionVerticalGradientTransition,
};

template <typename T>
constexpr uint8_t toUint8(T t) noexcept {
    return uint8_t(mbgl::underlying_type(t));
}

constexpr const auto layerProperties = mapbox::eternal::hash_map<mapbox::eternal::string, uint8_t>(
    {{"fill-extrusion-base", toUint8(Property::FillExtrusionBase)},
     {"fill-extrusion-color", toUint8(Property::FillExtrusionColor)},
     {"fill-extrusion-height", toUint8(Property::FillExtrusionHeight)},
     {"fill-extrusion-opacity", toUint8(Property::FillExtrusionOpacity)},
     {"fill-extrusion-pattern", toUint8(Property::FillExtrusionPattern)},
     {"fill-extrusion-translate", toUint8(Property::FillExtrusionTranslate)},
     {"fill-extrusion-translate-anchor", toUint8(Property::FillExtrusionTranslateAnchor)},
     {"fill-extrusion-vertical-gradient", toUint8(Property::FillExtrusionVerticalGradient)},
     {"fill-extrusion-base-transition", toUint8(Property::FillExtrusionBaseTransition)},
     {"fill-extrusion-color-transition", toUint8(Property::FillExtrusionColorTransition)},
     {"fill-extrusion-height-transition", toUint8(Property::FillExtrusionHeightTransition)},
     {"fill-extrusion-opacity-transition", toUint8(Property::FillExtrusionOpacityTransition)},
     {"fill-extrusion-pattern-transition", toUint8(Property::FillExtrusionPatternTransition)},
     {"fill-extrusion-translate-transition", toUint8(Property::FillExtrusionTranslateTransition)},
     {"fill-extrusion-translate-anchor-transition", toUint8(Property::FillExtrusionTranslateAnchorTransition)},
     {"fill-extrusion-vertical-gradient-transition", toUint8(Property::FillExtrusionVerticalGradientTransition)}});

StyleProperty getLayerProperty(const FillExtrusionLayer& layer, Property property) {
    switch (property) {
        case Property::FillExtrusionBase:
            return makeStyleProperty(layer.getFillExtrusionBase());
        case Property::FillExtrusionColor:
            return makeStyleProperty(layer.getFillExtrusionColor());
        case Property::FillExtrusionHeight:
            return makeStyleProperty(layer.getFillExtrusionHeight());
        case Property::FillExtrusionOpacity:
            return makeStyleProperty(layer.getFillExtrusionOpacity());
        case Property::FillExtrusionPattern:
            return makeStyleProperty(layer.getFillExtrusionPattern());
        case Property::FillExtrusionTranslate:
            return makeStyleProperty(layer.getFillExtrusionTranslate());
        case Property::FillExtrusionTranslateAnchor:
            return makeStyleProperty(layer.getFillExtrusionTranslateAnchor());
        case Property::FillExtrusionVerticalGradient:
            return makeStyleProperty(layer.getFillExtrusionVerticalGradient());
        case Property::FillExtrusionBaseTransition:
            return makeStyleProperty(layer.getFillExtrusionBaseTransition());
        case Property::FillExtrusionColorTransition:
            return makeStyleProperty(layer.getFillExtrusionColorTransition());
        case Property::FillExtrusionHeightTransition:
            return makeStyleProperty(layer.getFillExtrusionHeightTransition());
        case Property::FillExtrusionOpacityTransition:
            return makeStyleProperty(layer.getFillExtrusionOpacityTransition());
        case Property::FillExtrusionPatternTransition:
            return makeStyleProperty(layer.getFillExtrusionPatternTransition());
        case Property::FillExtrusionTranslateTransition:
            return makeStyleProperty(layer.getFillExtrusionTranslateTransition());
        case Property::FillExtrusionTranslateAnchorTransition:
            return makeStyleProperty(layer.getFillExtrusionTranslateAnchorTransition());
        case Property::FillExtrusionVerticalGradientTransition:
            return makeStyleProperty(layer.getFillExtrusionVerticalGradientTransition());
    }
    return {};
}

StyleProperty getLayerProperty(const FillExtrusionLayer& layer, const std::string& name) {
    const auto it = layerProperties.find(name.c_str());
    if (it == layerProperties.end()) {
        return {};
    }
    return getLayerProperty(layer, static_cast<Property>(it->second));
}

} // namespace

Value FillExtrusionLayer::serialize() const {
    auto result = Layer::serialize();
    assert(result.getObject());
    for (const auto& property : layerProperties) {
        auto styleProperty = getLayerProperty(*this, static_cast<Property>(property.second));
        if (styleProperty.getKind() == StyleProperty::Kind::Undefined) continue;
        serializeProperty(result, styleProperty, property.first.c_str(), property.second < kPaintPropertyCount);
    }
    return result;
}

std::optional<Error> FillExtrusionLayer::setPropertyInternal(const std::string& name, const Convertible& value) {
    const auto it = layerProperties.find(name.c_str());
    if (it == layerProperties.end()) return Error{"layer doesn't support this property"};

    auto property = static_cast<Property>(it->second);

    if (property == Property::FillExtrusionBase || property == Property::FillExtrusionHeight) {
        Error error;
        const auto& typedValue = convert<PropertyValue<float>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        if (property == Property::FillExtrusionBase) {
            setFillExtrusionBase(*typedValue);
            return std::nullopt;
        }

        if (property == Property::FillExtrusionHeight) {
            setFillExtrusionHeight(*typedValue);
            return std::nullopt;
        }
    }
    if (property == Property::FillExtrusionColor) {
        Error error;
        const auto& typedValue = convert<PropertyValue<Color>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionColor(*typedValue);
        return std::nullopt;
    }
    if (property == Property::FillExtrusionOpacity) {
        Error error;
        const auto& typedValue = convert<PropertyValue<float>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionOpacity(*typedValue);
        return std::nullopt;
    }
    if (property == Property::FillExtrusionPattern) {
        Error error;
        const auto& typedValue = convert<PropertyValue<expression::Image>>(value, error, true, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionPattern(*typedValue);
        return std::nullopt;
    }
    if (property == Property::FillExtrusionTranslate) {
        Error error;
        const auto& typedValue = convert<PropertyValue<std::array<float, 2>>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionTranslate(*typedValue);
        return std::nullopt;
    }
    if (property == Property::FillExtrusionTranslateAnchor) {
        Error error;
        const auto& typedValue = convert<PropertyValue<TranslateAnchorType>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionTranslateAnchor(*typedValue);
        return std::nullopt;
    }
    if (property == Property::FillExtrusionVerticalGradient) {
        Error error;
        const auto& typedValue = convert<PropertyValue<bool>>(value, error, false, false);
        if (!typedValue) {
            return error;
        }

        setFillExtrusionVerticalGradient(*typedValue);
        return std::nullopt;
    }

    Error error;
    std::optional<TransitionOptions> transition = convert<TransitionOptions>(value, error);
    if (!transition) {
        return error;
    }

    if (property == Property::FillExtrusionBaseTransition) {
        setFillExtrusionBaseTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionColorTransition) {
        setFillExtrusionColorTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionHeightTransition) {
        setFillExtrusionHeightTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionOpacityTransition) {
        setFillExtrusionOpacityTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionPatternTransition) {
        setFillExtrusionPatternTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionTranslateTransition) {
        setFillExtrusionTranslateTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionTranslateAnchorTransition) {
        setFillExtrusionTranslateAnchorTransition(*transition);
        return std::nullopt;
    }

    if (property == Property::FillExtrusionVerticalGradientTransition) {
        setFillExtrusionVerticalGradientTransition(*transition);
        return std::nullopt;
    }

    return Error{"layer doesn't support this property"};
}

StyleProperty FillExtrusionLayer::getProperty(const std::string& name) const {
    return getLayerProperty(*this, name);
}

Mutable<Layer::Impl> FillExtrusionLayer::mutableBaseImpl() const {
    return staticMutableCast<Layer::Impl>(mutableImpl());
}

} // namespace style
} // namespace mbgl

// clang-format on
