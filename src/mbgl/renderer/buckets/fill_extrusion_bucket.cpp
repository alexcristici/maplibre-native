#include <mbgl/renderer/buckets/fill_extrusion_bucket.hpp>
#include <mbgl/renderer/bucket_parameters.hpp>
#include <mbgl/style/layers/fill_extrusion_layer_impl.hpp>
#include <mbgl/renderer/layers/render_fill_extrusion_layer.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/math.hpp>
#include <mbgl/util/constants.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#include <mapbox/earcut.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cassert>

namespace mapbox {
namespace util {
template <>
struct nth<0, mbgl::GeometryCoordinate> {
    static int64_t get(const mbgl::GeometryCoordinate& t) { return t.x; };
};

template <>
struct nth<1, mbgl::GeometryCoordinate> {
    static int64_t get(const mbgl::GeometryCoordinate& t) { return t.y; };
};
} // namespace util
} // namespace mapbox

namespace mbgl {

using namespace style;

struct GeometryTooLongException : std::exception {};

FillExtrusionBucket::FillExtrusionBucket(
    const FillExtrusionBucket::PossiblyEvaluatedLayoutProperties&,
    const std::map<std::string, Immutable<style::LayerProperties>>& layerPaintProperties,
    const float zoom,
    const uint32_t) {
    for (const auto& pair : layerPaintProperties) {
        paintPropertyBinders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(pair.first),
            std::forward_as_tuple(getEvaluated<FillExtrusionLayerProperties>(pair.second), zoom));
    }
}

FillExtrusionBucket::~FillExtrusionBucket() {
    sharedVertices->release();
}

void FillExtrusionBucket::addFeature(const GeometryTileFeature& feature,
                                     const GeometryCollection& geometry,
                                     const ImagePositions& patternPositions,
                                     const PatternLayerMap& patternDependencies,
                                     std::size_t index,
                                     const CanonicalTileID& canonical) {
    for (auto& polygon : classifyRings(geometry)) {
        // Optimize polygons with many interior rings for earcut tesselation.
        limitHoles(polygon, 500);

        std::size_t totalVertices = 0;

        for (const auto& ring : polygon) {
            totalVertices += ring.size();
            if (totalVertices > std::numeric_limits<uint16_t>::max()) throw GeometryTooLongException();
        }

        if (totalVertices == 0) continue;

        std::vector<uint32_t> flatIndices;
        flatIndices.reserve(totalVertices);

        std::size_t startVertices = vertices.elements();

        if (triangleSegments.empty() || triangleSegments.back().vertexLength + (5 * (totalVertices - 1) + 1) >
                                            std::numeric_limits<uint16_t>::max()) {
            triangleSegments.emplace_back(startVertices, triangles.elements());
        }

        auto& triangleSegment = triangleSegments.back();
        assert(triangleSegment.vertexLength <= std::numeric_limits<uint16_t>::max());
        auto triangleIndex = static_cast<uint16_t>(triangleSegment.vertexLength);

        assert(triangleIndex + (5 * (totalVertices - 1) + 1) <= std::numeric_limits<uint16_t>::max());

        for (const auto& ring : polygon) {
            std::size_t nVertices = ring.size();

            if (nVertices == 0) continue;

            std::size_t edgeDistance = 0;

            for (std::size_t i = 0; i < nVertices; i++) {
                const auto& p1 = ring[i];

                vertices.emplace_back(
                    FillExtrusionBucket::layoutVertex(p1, 0, 0, 1, 1, static_cast<uint16_t>(edgeDistance)));
                flatIndices.emplace_back(triangleIndex);
                triangleIndex++;

                if (i != 0) {
                    const auto& p2 = ring[i - 1];

                    const auto d1 = convertPoint<double>(p1);
                    const auto d2 = convertPoint<double>(p2);

                    const Point<double> perp = util::unit(util::perp(d1 - d2));
                    const size_t dist = util::dist<int16_t>(d1, d2);
                    if (edgeDistance + dist > static_cast<size_t>(std::numeric_limits<int16_t>::max())) {
                        edgeDistance = 0;
                    }

                    vertices.emplace_back(FillExtrusionBucket::layoutVertex(
                        p1, perp.x, perp.y, 0, 0, static_cast<uint16_t>(edgeDistance)));
                    vertices.emplace_back(FillExtrusionBucket::layoutVertex(
                        p1, perp.x, perp.y, 0, 1, static_cast<uint16_t>(edgeDistance)));

                    edgeDistance += dist;

                    vertices.emplace_back(FillExtrusionBucket::layoutVertex(
                        p2, perp.x, perp.y, 0, 0, static_cast<uint16_t>(edgeDistance)));
                    vertices.emplace_back(FillExtrusionBucket::layoutVertex(
                        p2, perp.x, perp.y, 0, 1, static_cast<uint16_t>(edgeDistance)));

                    // ┌──────┐
                    // │ 0  1 │ Counter-Clockwise winding order.
                    // │      │ Triangle 1: 0 => 2 => 1
                    // │ 2  3 │ Triangle 2: 1 => 2 => 3
                    // └──────┘
                    triangles.emplace_back(triangleIndex, triangleIndex + 2, triangleIndex + 1);
                    triangles.emplace_back(triangleIndex + 1, triangleIndex + 2, triangleIndex + 3);
                    triangleIndex += 4;
                    triangleSegment.vertexLength += 4;
                    triangleSegment.indexLength += 6;
                }
            }
        }

        std::vector<uint32_t> indices = mapbox::earcut(polygon);

        std::size_t nIndices = indices.size();
        assert(nIndices % 3 == 0);

        for (std::size_t i = 0; i < nIndices; i += 3) {
            // Counter-Clockwise winding order.
            triangles.emplace_back(static_cast<uint16_t>(flatIndices[indices[i]]),
                                   static_cast<uint16_t>(flatIndices[indices[i + 2]]),
                                   static_cast<uint16_t>(flatIndices[indices[i + 1]]));
        }

        triangleSegment.vertexLength += totalVertices;
        triangleSegment.indexLength += nIndices;
    }

    for (auto& pair : paintPropertyBinders) {
        const auto it = patternDependencies.find(pair.first);
        if (it != patternDependencies.end()) {
            pair.second.populateVertexVectors(
                feature, vertices.elements(), index, patternPositions, it->second, canonical);
        } else {
            pair.second.populateVertexVectors(feature, vertices.elements(), index, patternPositions, {}, canonical);
        }
    }
}

void FillExtrusionBucket::upload([[maybe_unused]] gfx::UploadPass& uploadPass) {
    uploaded = true;
}

bool FillExtrusionBucket::hasData() const {
    return !triangleSegments.empty();
}

float FillExtrusionBucket::getQueryRadius(const RenderLayer& layer) const {
    const auto& evaluated = getEvaluated<FillExtrusionLayerProperties>(layer.evaluatedProperties);
    const std::array<float, 2>& translate = evaluated.get<FillExtrusionTranslate>();
    return util::length(translate[0], translate[1]);
}

void FillExtrusionBucket::update(const FeatureStates& states,
                                 const GeometryTileLayer& layer,
                                 const std::string& layerID,
                                 const ImagePositions& imagePositions) {
    auto it = paintPropertyBinders.find(layerID);
    if (it != paintPropertyBinders.end()) {
        it->second.updateVertexVectors(states, layer, imagePositions);
        uploaded = false;

        sharedVertices->updateModified();
    }
}

std::array<float, 3> FillExtrusionBucket::lightColor(const EvaluatedLight& light) {
    const auto color = light.get<LightColor>();
    return {{color.r, color.g, color.b}};
}

std::array<float, 3> FillExtrusionBucket::lightPosition(const EvaluatedLight& light, const TransformState& state) {
    auto lightPos = light.get<LightPosition>().getCartesian();
    mat3 lightMat;
    matrix::identity(lightMat);
    if (light.get<LightAnchor>() == LightAnchorType::Viewport) {
        matrix::rotate(lightMat, lightMat, -state.getBearing());
    }
    matrix::transformMat3f(lightPos, lightPos, lightMat);
    return lightPos;
}

float FillExtrusionBucket::lightIntensity(const EvaluatedLight& light) {
    return light.get<LightIntensity>();
}

} // namespace mbgl
