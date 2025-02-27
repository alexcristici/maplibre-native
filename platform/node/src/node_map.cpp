#include "node_map.hpp"
#include "node_request.hpp"
#include "node_feature.hpp"
#include "node_conversion.hpp"

#include <mbgl/renderer/renderer.hpp>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/style/conversion/source.hpp>
#include <mbgl/style/conversion/layer.hpp>
#include <mbgl/style/conversion/filter.hpp>
#include <mbgl/style/conversion/light.hpp>

#include <mbgl/style/layers/background_layer.hpp>
#include <mbgl/style/layers/circle_layer.hpp>
#include <mbgl/style/layers/fill_layer.hpp>
#include <mbgl/style/layers/fill_extrusion_layer.hpp>
#include <mbgl/style/layers/heatmap_layer.hpp>
#include <mbgl/style/layers/hillshade_layer.hpp>
#include <mbgl/style/layers/line_layer.hpp>
#include <mbgl/style/layers/raster_layer.hpp>
#include <mbgl/style/layers/symbol_layer.hpp>

#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/storage/file_source_manager.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/style/image.hpp>
#include <mbgl/style/light.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/util/async_request.hpp>
#include <mbgl/util/exception.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/premultiply.hpp>

namespace node_mbgl {

struct NodeMap::RenderOptions {
    double zoom = 0;
    double bearing = 0;
    mbgl::style::Light light;
    double pitch = 0;
    double latitude = 0;
    double longitude = 0;
    mbgl::Size size = {512, 512};
    bool axonometric = false;
    double xSkew = 0;
    double ySkew = 1;
    std::vector<std::string> classes;
    mbgl::MapDebugOptions debugOptions = mbgl::MapDebugOptions::NoDebug;
};

Nan::Persistent<v8::Function> NodeMap::constructor;
Nan::Persistent<v8::Object> NodeMap::parseError;

static const char* releasedMessage() {
    return "Map resources have already been released";
}

void NodeMapObserver::onDidFailLoadingMap(mbgl::MapLoadError error, const std::string& description) {
    switch (error) {
        case mbgl::MapLoadError::StyleParseError:
            Nan::ThrowError(NodeMap::ParseError(description.c_str()));
            break;
        default:
            Nan::ThrowError(description.c_str());
    }
}

void NodeMap::Init(v8::Local<v8::Object> target) {
    // Define a custom error class for parse errors
    auto script = Nan::New<v8::UnboundScript>(Nan::New(R"JS(
class ParseError extends Error {
  constructor(...params) {
    super(...params);
    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, ParseError);
    }
  }
}
ParseError)JS")
                                                  .ToLocalChecked())
                      .ToLocalChecked();
    parseError.Reset(Nan::To<v8::Object>(Nan::RunScript(script).ToLocalChecked()).ToLocalChecked());
    Nan::Set(target, Nan::New("ParseError").ToLocalChecked(), Nan::New(parseError));

    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);

    tpl->SetClassName(Nan::New("Map").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(2);

    Nan::SetPrototypeMethod(tpl, "load", Load);
    Nan::SetPrototypeMethod(tpl, "loaded", Loaded);
    Nan::SetPrototypeMethod(tpl, "render", Render);
    Nan::SetPrototypeMethod(tpl, "release", Release);
    Nan::SetPrototypeMethod(tpl, "cancel", Cancel);

    Nan::SetPrototypeMethod(tpl, "addSource", AddSource);
    Nan::SetPrototypeMethod(tpl, "removeSource", RemoveSource);
    Nan::SetPrototypeMethod(tpl, "addLayer", AddLayer);
    Nan::SetPrototypeMethod(tpl, "removeLayer", RemoveLayer);
    Nan::SetPrototypeMethod(tpl, "addImage", AddImage);
    Nan::SetPrototypeMethod(tpl, "removeImage", RemoveImage);
    Nan::SetPrototypeMethod(tpl, "setLayerZoomRange", SetLayerZoomRange);
    Nan::SetPrototypeMethod(tpl, "setLayoutProperty", SetLayerProperty);
    Nan::SetPrototypeMethod(tpl, "setPaintProperty", SetLayerProperty);
    Nan::SetPrototypeMethod(tpl, "setFilter", SetFilter);
    Nan::SetPrototypeMethod(tpl, "setSize", SetSize);
    Nan::SetPrototypeMethod(tpl, "setCenter", SetCenter);
    Nan::SetPrototypeMethod(tpl, "setZoom", SetZoom);
    Nan::SetPrototypeMethod(tpl, "setBearing", SetBearing);
    Nan::SetPrototypeMethod(tpl, "setPitch", SetPitch);
    Nan::SetPrototypeMethod(tpl, "setLight", SetLight);
    Nan::SetPrototypeMethod(tpl, "setAxonometric", SetAxonometric);
    Nan::SetPrototypeMethod(tpl, "setXSkew", SetXSkew);
    Nan::SetPrototypeMethod(tpl, "setYSkew", SetYSkew);
    Nan::SetPrototypeMethod(tpl, "setFeatureState", SetFeatureState);
    Nan::SetPrototypeMethod(tpl, "getFeatureState", GetFeatureState);
    Nan::SetPrototypeMethod(tpl, "removeFeatureState", RemoveFeatureState);

    Nan::SetPrototypeMethod(tpl, "dumpDebugLogs", DumpDebugLogs);
    Nan::SetPrototypeMethod(tpl, "queryRenderedFeatures", QueryRenderedFeatures);
#if defined NODE_MODULE_VERSION && NODE_MODULE_VERSION < 93
    v8::Local<v8::Context> context = target->CreationContext();
#else
    v8::Local<v8::Context> context = target->GetCreationContext().ToLocalChecked();
#endif

    constructor.Reset(tpl->GetFunction(context).ToLocalChecked());
    Nan::Set(target, Nan::New("Map").ToLocalChecked(), tpl->GetFunction(context).ToLocalChecked());
}

/**
 * A request object, given to the `request` handler of a map, is an
 * encapsulation of a URL and type of a resource that the map asks you to load.
 *
 * The `kind` property is one of
 *
 *     "Unknown": 0,
 *     "Style": 1,
 *     "Source": 2,
 *     "Tile": 3,
 *     "Glyphs": 4,
 *     "SpriteImage": 5,
 *     "SpriteJSON": 6
 *
 * @typedef
 * @name Request
 * @property {string} url
 * @property {number} kind
 */

/**
 * Mapbox GL object: this object loads stylesheets and renders them into
 * images.
 *
 * @class
 * @name Map
 * @param {Object} options
 * @param {Function} options.request a method used to request resources
 * over the internet
 * @param {Function} [options.cancel]
 * @param {number} options.ratio pixel ratio
 * @example
 * var map = new mbgl.Map({ request: function() {} });
 * map.load(require('./test/fixtures/style.json'));
 * map.render({}, function(err, image) {
 *     if (err) throw err;
 *     fs.writeFileSync('image.png', image);
 * });
 */
void NodeMap::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    if (!info.IsConstructCall()) {
        return Nan::ThrowTypeError("Use the new operator to create new Map objects");
    }

    if (info.Length() > 0 && !info[0]->IsObject()) {
        return Nan::ThrowTypeError("Requires an options object as first argument");
    }

    v8::Local<v8::Object> options;

    if (info.Length() > 0) {
        options = Nan::To<v8::Object>(info[0]).ToLocalChecked();
    } else {
        options = Nan::New<v8::Object>();
    }

    // Check that if 'request' is set it must be a function, if 'cancel' is set
    // it must be a function and if 'ratio' is set it must be a number.
    if (Nan::Has(options, Nan::New("request").ToLocalChecked()).FromJust() &&
        !Nan::Get(options, Nan::New("request").ToLocalChecked()).ToLocalChecked()->IsFunction()) {
        return Nan::ThrowError("Options object 'request' property must be a function");
    }

    if (Nan::Has(options, Nan::New("cancel").ToLocalChecked()).FromJust() &&
        !Nan::Get(options, Nan::New("cancel").ToLocalChecked()).ToLocalChecked()->IsFunction()) {
        return Nan::ThrowError("Options object 'cancel' property must be a function");
    }

    if (Nan::Has(options, Nan::New("ratio").ToLocalChecked()).FromJust() &&
        !Nan::Get(options, Nan::New("ratio").ToLocalChecked()).ToLocalChecked()->IsNumber()) {
        return Nan::ThrowError("Options object 'ratio' property must be a number");
    }

    info.This()->SetInternalField(1, options);

    if (Nan::Has(options, Nan::New("request").ToLocalChecked()).FromJust() &&
        Nan::Get(options, Nan::New("request").ToLocalChecked()).ToLocalChecked()->IsFunction()) {
        mbgl::FileSourceManager::get()->registerFileSourceFactory(
            mbgl::FileSourceType::ResourceLoader,
            [](const mbgl::ResourceOptions& resourceOptions, const mbgl::ClientOptions&) {
                return std::make_unique<node_mbgl::NodeFileSource>(
                    reinterpret_cast<node_mbgl::NodeMap*>(resourceOptions.platformContext()));
            });
    }

    try {
        auto nodeMap = new NodeMap(options);
        nodeMap->Wrap(info.This());
    } catch (std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().Set(info.This());
}

/**
 * Load a stylesheet
 *
 * @function
 * @name load
 * @param {string|Object} stylesheet either an object or a JSON representation
 * @param {Object} options
 * @param {boolean} options.defaultStyleCamera if true, sets the default style
 * camera
 * @returns {undefined} loads stylesheet into map
 * @throws {Error} if stylesheet is missing or invalid
 * @example
 * // providing an object
 * map.load(require('./test/fixtures/style.json'));
 *
 * // providing a string
 * map.load(fs.readFileSync('./test/fixtures/style.json', 'utf8'));
 */
void NodeMap::Load(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    // Reset the flag as this could be the second time
    // we are calling this (being the previous successful).
    nodeMap->loaded = false;

    if (info.Length() < 1) {
        return Nan::ThrowError("Requires a map style as first argument");
    }

    std::string style;

    if (info[0]->IsObject()) {
        Nan::JSON JSON;
        style = *Nan::Utf8String(JSON.Stringify(info[0]->ToObject(context).ToLocalChecked()).ToLocalChecked());
    } else if (info[0]->IsString()) {
        style = *Nan::Utf8String(info[0]);
    } else {
        return Nan::ThrowTypeError("First argument must be a string or object");
    }

    try {
        nodeMap->map->getStyle().loadJSON(style);
    } catch (const mbgl::util::StyleParseException& ex) {
        return Nan::ThrowError(ParseError(ex.what()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    if (info.Length() == 2) {
        if (!info[1]->IsObject()) {
            return Nan::ThrowTypeError("Second argument must be an options object");
        }
        auto options = Nan::To<v8::Object>(info[1]).ToLocalChecked();
        if (Nan::Has(options, Nan::New("defaultStyleCamera").ToLocalChecked()).FromJust()) {
            if (!Nan::Get(options, Nan::New("defaultStyleCamera").ToLocalChecked()).ToLocalChecked()->IsBoolean()) {
                return Nan::ThrowError(
                    "Options object 'defaultStyleCamera' property must be a "
                    "boolean");
            }
            if (Nan::To<bool>(Nan::Get(options, Nan::New("cameraMutated").ToLocalChecked()).ToLocalChecked())
                    .ToChecked()) {
                nodeMap->map->jumpTo(nodeMap->map->getStyle().getDefaultCamera());
            }
        }
    }

    nodeMap->loaded = true;

    info.GetReturnValue().SetUndefined();
}

void NodeMap::Loaded(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    bool loaded = false;

    try {
        loaded = nodeMap->map->isFullyLoaded();
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().Set(Nan::New(loaded));
}

NodeMap::RenderOptions NodeMap::ParseOptions(v8::Local<v8::Object> obj) {
    Nan::HandleScope scope;

    NodeMap::RenderOptions options;

    if (Nan::Has(obj, Nan::New("zoom").ToLocalChecked()).FromJust()) {
        options.zoom = Nan::To<double>(Nan::Get(obj, Nan::New("zoom").ToLocalChecked()).ToLocalChecked()).ToChecked();
    }

    if (Nan::Has(obj, Nan::New("bearing").ToLocalChecked()).FromJust()) {
        options.bearing =
            Nan::To<double>(Nan::Get(obj, Nan::New("bearing").ToLocalChecked()).ToLocalChecked()).ToChecked();
    }

    if (Nan::Has(obj, Nan::New("pitch").ToLocalChecked()).FromJust()) {
        options.pitch = Nan::To<double>(Nan::Get(obj, Nan::New("pitch").ToLocalChecked()).ToLocalChecked()).ToChecked();
    }

    if (Nan::Has(obj, Nan::New("light").ToLocalChecked()).FromJust()) {
        auto lightObj = Nan::Get(obj, Nan::New("light").ToLocalChecked()).ToLocalChecked();
        mbgl::style::conversion::Error conversionError;
        if (auto light = mbgl::style::conversion::convert<mbgl::style::Light>(lightObj, conversionError)) {
            options.light = *light;
        } else {
            throw std::move(conversionError);
        }
    }

    if (Nan::Has(obj, Nan::New("axonometric").ToLocalChecked()).FromJust()) {
        options.axonometric =
            Nan::To<bool>(Nan::Get(obj, Nan::New("axonometric").ToLocalChecked()).ToLocalChecked()).ToChecked();
    }

    if (Nan::Has(obj, Nan::New("skew").ToLocalChecked()).FromJust()) {
        auto skewObj = Nan::Get(obj, Nan::New("skew").ToLocalChecked()).ToLocalChecked();
        if (skewObj->IsArray()) {
            auto skew = skewObj.As<v8::Array>();
            if (skew->Length() > 0) {
                options.xSkew = Nan::To<double>(Nan::Get(skew, 0).ToLocalChecked()).ToChecked();
            }
            if (skew->Length() > 1) {
                options.ySkew = Nan::To<double>(Nan::Get(skew, 1).ToLocalChecked()).ToChecked();
            }
        }
    }

    if (Nan::Has(obj, Nan::New("center").ToLocalChecked()).FromJust()) {
        auto centerObj = Nan::Get(obj, Nan::New("center").ToLocalChecked()).ToLocalChecked();
        if (centerObj->IsArray()) {
            auto center = centerObj.As<v8::Array>();
            if (center->Length() > 0) {
                options.longitude = Nan::To<double>(Nan::Get(center, 0).ToLocalChecked()).ToChecked();
            }
            if (center->Length() > 1) {
                options.latitude = Nan::To<double>(Nan::Get(center, 1).ToLocalChecked()).ToChecked();
            }
        }
    }

    if (Nan::Has(obj, Nan::New("width").ToLocalChecked()).FromJust()) {
        options.size.width = static_cast<uint32_t>(
            Nan::To<int64_t>(Nan::Get(obj, Nan::New("width").ToLocalChecked()).ToLocalChecked()).ToChecked());
    }

    if (Nan::Has(obj, Nan::New("height").ToLocalChecked()).FromJust()) {
        options.size.height = static_cast<uint32_t>(
            Nan::To<int64_t>(Nan::Get(obj, Nan::New("height").ToLocalChecked()).ToLocalChecked()).ToChecked());
    }

    if (Nan::Has(obj, Nan::New("classes").ToLocalChecked()).FromJust()) {
        auto classes = Nan::To<v8::Object>(Nan::Get(obj, Nan::New("classes").ToLocalChecked()).ToLocalChecked())
                           .ToLocalChecked()
                           .As<v8::Array>();
        const int length = classes->Length();
        options.classes.reserve(length);
        for (int i = 0; i < length; i++) {
            options.classes.emplace_back(std::string{
                *Nan::Utf8String(Nan::To<v8::String>(Nan::Get(classes, i).ToLocalChecked()).ToLocalChecked())});
        }
    }

    if (Nan::Has(obj, Nan::New("debug").ToLocalChecked()).FromJust()) {
        auto debug =
            Nan::To<v8::Object>(Nan::Get(obj, Nan::New("debug").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
        if (Nan::Has(debug, Nan::New("tileBorders").ToLocalChecked()).FromJust()) {
            if (Nan::To<bool>(Nan::Get(debug, Nan::New("tileBorders").ToLocalChecked()).ToLocalChecked()).ToChecked()) {
                options.debugOptions = options.debugOptions | mbgl::MapDebugOptions::TileBorders;
            }
        }
        if (Nan::Has(debug, Nan::New("parseStatus").ToLocalChecked()).FromJust()) {
            if (Nan::To<bool>(Nan::Get(debug, Nan::New("parseStatus").ToLocalChecked()).ToLocalChecked()).ToChecked()) {
                options.debugOptions = options.debugOptions | mbgl::MapDebugOptions::ParseStatus;
            }
        }
        if (Nan::Has(debug, Nan::New("timestamps").ToLocalChecked()).FromJust()) {
            if (Nan::To<bool>(Nan::Get(debug, Nan::New("timestamps").ToLocalChecked()).ToLocalChecked()).ToChecked()) {
                options.debugOptions = options.debugOptions | mbgl::MapDebugOptions::Timestamps;
            }
        }
        if (Nan::Has(debug, Nan::New("collision").ToLocalChecked()).FromJust()) {
            if (Nan::To<bool>(Nan::Get(debug, Nan::New("collision").ToLocalChecked()).ToLocalChecked()).ToChecked()) {
                options.debugOptions = options.debugOptions | mbgl::MapDebugOptions::Collision;
            }
        }
        if (Nan::Has(debug, Nan::New("overdraw").ToLocalChecked()).FromJust()) {
            if (Nan::To<bool>(Nan::Get(debug, Nan::New("overdraw").ToLocalChecked()).ToLocalChecked()).ToChecked()) {
                options.debugOptions = mbgl::MapDebugOptions::Overdraw;
            }
        }
    }

    return options;
}

class RenderRequest : public Nan::AsyncResource {
public:
    explicit RenderRequest(v8::Local<v8::Function> callback_)
        : AsyncResource("mbgl:RenderRequest") {
        callback.Reset(callback_);
    }
    ~RenderRequest() { callback.Reset(); }

    Nan::Persistent<v8::Function> callback;
};

/**
 * Render an image from the currently-loaded style
 *
 * @name render
 * @param {Object} options
 * @param {number} [options.zoom=0]
 * @param {number} [options.width=512]
 * @param {number} [options.height=512]
 * @param {Array<number>} [options.center=[0,0]] longitude, latitude center
 * of the map
 * @param {number} [options.bearing=0] rotation
 * @param {Array<string>} [options.classes=[]] style classes
 * @param {Function} callback
 * @returns {undefined} calls callback
 * @throws {Error} if stylesheet is not loaded or if map is already rendering
 */
void NodeMap::Render(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || (!info[0]->IsObject() && !info[0]->IsFunction())) {
        return Nan::ThrowTypeError("First argument must be an options object or a callback function");
    }

    if ((info.Length() <= 1 && !info[0]->IsFunction()) || (info.Length() > 1 && !info[1]->IsFunction())) {
        return Nan::ThrowTypeError("Second argument must be a callback function");
    }

    if (!nodeMap->loaded) {
        return Nan::ThrowTypeError("Style is not loaded");
    }

    if (nodeMap->req) {
        return Nan::ThrowError("Map is currently processing a RenderRequest");
    }

    try {
        if (info[0]->IsFunction()) {
            assert(!nodeMap->req);
            assert(!nodeMap->image.data);
            nodeMap->req = std::make_unique<RenderRequest>(Nan::To<v8::Function>(info[0]).ToLocalChecked());

            nodeMap->startRender();
        } else {
            auto options = ParseOptions(Nan::To<v8::Object>(info[0]).ToLocalChecked());
            assert(!nodeMap->req);
            assert(!nodeMap->image.data);
            nodeMap->req = std::make_unique<RenderRequest>(Nan::To<v8::Function>(info[1]).ToLocalChecked());

            nodeMap->startRender(options);
        }
    } catch (const mbgl::style::conversion::Error& err) {
        return Nan::ThrowTypeError(err.message.c_str());
    } catch (const mbgl::util::StyleParseException& ex) {
        return Nan::ThrowError(ParseError(ex.what()));
    } catch (const mbgl::util::Exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::startRender() {
    map->renderStill([this](const std::exception_ptr& eptr) {
        if (eptr) {
            error = eptr;
            uv_async_send(async);
        } else {
            assert(!image.data);
            image = frontend->readStillImage();
            uv_async_send(async);
        }
    });

    // Retain this object, otherwise it might get destructed before we are
    // finished rendering the still image.
    Ref();

    // Similarly, we're now waiting for the async to be called, so we need to
    // make sure that it keeps the loop alive.
    uv_ref(reinterpret_cast<uv_handle_t*>(async));
}

void NodeMap::startRender(const NodeMap::RenderOptions& options) {
    frontend->setSize(options.size);
    map->setSize(options.size);

    mbgl::CameraOptions camera;
    camera.center = mbgl::LatLng{options.latitude, options.longitude};
    camera.zoom = options.zoom;
    camera.bearing = options.bearing;
    camera.pitch = options.pitch;

    auto projectionOptions =
        mbgl::ProjectionMode().withAxonometric(options.axonometric).withXSkew(options.xSkew).withYSkew(options.ySkew);

    map->setProjectionMode(projectionOptions);

    map->renderStill(camera, options.debugOptions, [this](const std::exception_ptr& eptr) {
        if (eptr) {
            error = eptr;
            uv_async_send(async);
        } else {
            assert(!image.data);
            image = frontend->readStillImage();
            uv_async_send(async);
        }
    });

    // Retain this object, otherwise it might get destructed before we are
    // finished rendering the still image.
    Ref();

    // Similarly, we're now waiting for the async to be called, so we need to
    // make sure that it keeps the loop alive.
    uv_ref(reinterpret_cast<uv_handle_t*>(async));
}

v8::Local<v8::Value> NodeMap::ParseError(const char* msg) {
    v8::Local<v8::Value> argv[] = {Nan::New(msg).ToLocalChecked()};
    return Nan::CallAsConstructor(Nan::New(parseError), 1, argv).ToLocalChecked();
}

void NodeMap::renderFinished() {
    assert(req);

    Nan::HandleScope scope;

    // We're done with this render call, so we're unrefing so that the loop could close.
    uv_unref(reinterpret_cast<uv_handle_t*>(async));

    // Move the callback and image out of the way so that the callback can start a new render call.
    auto request = std::move(req);
    auto img = std::move(image);
    assert(request);

    // These have to be empty to be prepared for the next render call.
    assert(!req);
    assert(!image.data);

    v8::Local<v8::Function> callback = Nan::New(request->callback);
    v8::Local<v8::Object> target = Nan::New<v8::Object>();

    if (error) {
        v8::Local<v8::Value> err;

        try {
            std::rethrow_exception(error);
            assert(false);
        } catch (const mbgl::util::StyleParseException& ex) {
            err = ParseError(ex.what());
        } catch (const std::exception& ex) {
            err = Nan::Error(ex.what());
        }

        v8::Local<v8::Value> argv[] = {err};

        // This must be empty to be prepared for the next render call.
        error = nullptr;
        assert(!error);

        request->runInAsyncScope(target, callback, 1, argv);
    } else if (img.data) {
        v8::Local<v8::Object> pixels = Nan::NewBuffer(
                                           reinterpret_cast<char*>(img.data.get()),
                                           img.bytes(),
                                           // Retain the data until the buffer is deleted.
                                           [](char*, void* hint) { delete[] reinterpret_cast<uint8_t*>(hint); },
                                           img.data.get())
                                           .ToLocalChecked();
        if (!pixels.IsEmpty()) {
            img.data.release();
        }

        v8::Local<v8::Value> argv[] = {Nan::Null(), pixels};
        request->runInAsyncScope(target, callback, 2, argv);
    } else {
        v8::Local<v8::Value> argv[] = {Nan::Error("Didn't get an image")};
        request->runInAsyncScope(target, callback, 1, argv);
    }

    // There is no render pending anymore, we the GC could now delete this
    // object if it went out of scope.
    Unref();
}

/**
 * Clean up any resources used by a map instance.options
 * @name release
 * @returns {undefined}
 */
void NodeMap::Release(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    try {
        nodeMap->release();
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::release() {
    if (!map) throw mbgl::util::Exception(releasedMessage());

    uv_close(reinterpret_cast<uv_handle_t*>(async), [](uv_handle_t* h) { delete reinterpret_cast<uv_async_t*>(h); });

    map.reset();
    frontend.reset();
}

/**
 * Cancel an ongoing render request. The callback will be called with
 * the error set to "Canceled". Will throw if no rendering is in progress.
 * @name cancel
 * @returns {undefined}
 */
void NodeMap::Cancel(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());

    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());
    if (!nodeMap->req) return Nan::ThrowError("No render in progress");

    try {
        nodeMap->cancel();
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::cancel() {
    auto style = map->getStyle().getJSON();

    // Reset map explicitly as it resets the renderer frontend
    map.reset();

    // Remove the existing async handle to flush any scheduled calls to renderFinished.
    uv_unref(reinterpret_cast<uv_handle_t*>(async));
    uv_close(reinterpret_cast<uv_handle_t*>(async), [](uv_handle_t* h) { delete reinterpret_cast<uv_async_t*>(h); });
    async = new uv_async_t;
    async->data = this;
    uv_async_init(
        uv_default_loop(), async, [](uv_async_t* h) { reinterpret_cast<NodeMap*>(h->data)->renderFinished(); });

    frontend = std::make_unique<mbgl::HeadlessFrontend>(mbgl::Size{512, 512}, pixelRatio);
    map = std::make_unique<mbgl::Map>(*frontend,
                                      mapObserver,
                                      mbgl::MapOptions()
                                          .withSize(frontend->getSize())
                                          .withPixelRatio(pixelRatio)
                                          .withMapMode(mode)
                                          .withCrossSourceCollisions(crossSourceCollisions),
                                      mbgl::ResourceOptions().withPlatformContext(reinterpret_cast<void*>(this)),
                                      mbgl::ClientOptions());

    // FIXME: Reload the style after recreating the map. We need to find
    // a better way of canceling an ongoing rendering on the core level
    // without resetting the map, which is way too expensive.
    map->getStyle().loadJSON(style);

    error = std::make_exception_ptr(std::runtime_error("Canceled"));
    renderFinished();
}

void NodeMap::AddSource(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 2) {
        return Nan::ThrowTypeError("Two argument required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    if (!info[1]->IsObject()) {
        return Nan::ThrowTypeError("Second argument must be an object");
    }

    Error error;
    std::optional<std::unique_ptr<Source>> source = convert<std::unique_ptr<Source>>(
        info[1], error, *Nan::Utf8String(info[0]));
    if (!source) {
        Nan::ThrowTypeError(error.message.c_str());
        return;
    }

    nodeMap->map->getStyle().addSource(std::move(*source));
}

void NodeMap::RemoveSource(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 1) {
        return Nan::ThrowTypeError("One argument required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    nodeMap->map->getStyle().removeSource(*Nan::Utf8String(info[0]));
}

void NodeMap::AddLayer(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 1) {
        return Nan::ThrowTypeError("One argument required");
    }

    Error error;
    std::optional<std::unique_ptr<Layer>> layer = convert<std::unique_ptr<Layer>>(info[0], error);
    if (!layer) {
        Nan::ThrowTypeError(error.message.c_str());
        return;
    }

    nodeMap->map->getStyle().addLayer(std::move(*layer));
}

void NodeMap::RemoveLayer(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 1) {
        return Nan::ThrowTypeError("One argument required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    nodeMap->map->getStyle().removeLayer(*Nan::Utf8String(info[0]));
}

void NodeMap::AddImage(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 3) {
        return Nan::ThrowTypeError("Three arguments required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    if (!info[1]->IsObject()) {
        return Nan::ThrowTypeError("Second argument must be an object");
    }

    if (!info[2]->IsObject()) {
        return Nan::ThrowTypeError("Third argument must be an object");
    }

    auto optionObject = Nan::To<v8::Object>(info[2]).ToLocalChecked();

    if (!Nan::Get(optionObject, Nan::New("height").ToLocalChecked()).ToLocalChecked()->IsUint32()) {
        return Nan::ThrowTypeError("height parameter required");
    }

    if (!Nan::Get(optionObject, Nan::New("width").ToLocalChecked()).ToLocalChecked()->IsUint32()) {
        return Nan::ThrowTypeError("width parameter required");
    }

    if (!Nan::Get(optionObject, Nan::New("pixelRatio").ToLocalChecked()).ToLocalChecked()->IsNumber()) {
        return Nan::ThrowTypeError("pixelRatio parameter required");
    }

    uint32_t imageHeight =
        Nan::To<uint32_t>(Nan::Get(optionObject, Nan::New("height").ToLocalChecked()).ToLocalChecked()).ToChecked();
    uint32_t imageWidth =
        Nan::To<uint32_t>(Nan::Get(optionObject, Nan::New("width").ToLocalChecked()).ToLocalChecked()).ToChecked();

    if (imageWidth > 1024 || imageHeight > 1024) {
        return Nan::ThrowTypeError("Max height and width is 1024");
    }

    bool sdf = false;
    if (Nan::Get(optionObject, Nan::New("sdf").ToLocalChecked()).ToLocalChecked()->IsBoolean()) {
        sdf = Nan::To<bool>(Nan::Get(optionObject, Nan::New("sdf").ToLocalChecked()).ToLocalChecked()).ToChecked();
    }

    float pixelRatio = (float)Nan::To<double>(
                           Nan::Get(optionObject, Nan::New("pixelRatio").ToLocalChecked()).ToLocalChecked())
                           .ToChecked();
    auto imageBuffer = Nan::To<v8::Object>(info[1]).ToLocalChecked()->ToObject(context).ToLocalChecked();

    char* imageDataBuffer = node::Buffer::Data(imageBuffer);
    size_t imageLength = node::Buffer::Length(imageBuffer);

    if (imageLength != imageHeight * imageWidth * 4) {
        return Nan::ThrowTypeError("Image size does not match buffer size");
    }

    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(imageLength);
    std::copy(imageDataBuffer, imageDataBuffer + imageLength, data.get());

    mbgl::UnassociatedImage cImage({imageWidth, imageHeight}, std::move(data));
    mbgl::PremultipliedImage cPremultipliedImage = mbgl::util::premultiply(std::move(cImage));
    nodeMap->map->getStyle().addImage(std::make_unique<mbgl::style::Image>(
        *Nan::Utf8String(info[0]), std::move(cPremultipliedImage), pixelRatio, sdf));
}

void NodeMap::RemoveImage(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 1) {
        return Nan::ThrowTypeError("One argument required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    nodeMap->map->getStyle().removeImage(*Nan::Utf8String(info[0]));
}

void NodeMap::SetLayerZoomRange(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() != 3) {
        return Nan::ThrowTypeError("Three arguments required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    if (!info[1]->IsNumber() || !info[2]->IsNumber()) {
        return Nan::ThrowTypeError("Second and third arguments must be numbers");
    }

    mbgl::style::Layer* layer = nodeMap->map->getStyle().getLayer(*Nan::Utf8String(info[0]));
    if (!layer) {
        return Nan::ThrowTypeError("layer not found");
    }

    layer->setMinZoom(static_cast<float>(Nan::To<double>(info[1]).ToChecked()));
    layer->setMaxZoom(static_cast<float>(Nan::To<double>(info[2]).ToChecked()));
}

void NodeMap::SetLayerProperty(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() < 3) {
        return Nan::ThrowTypeError("Three arguments required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    mbgl::style::Layer* layer = nodeMap->map->getStyle().getLayer(*Nan::Utf8String(info[0]));
    if (!layer) {
        return Nan::ThrowTypeError("layer not found");
    }

    if (!info[1]->IsString()) {
        return Nan::ThrowTypeError("Second argument must be a string");
    }

    std::optional<Error> error = layer->setProperty(*Nan::Utf8String(info[1]), Convertible(info[2]));
    if (error) {
        return Nan::ThrowTypeError(error->message.c_str());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetFilter(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() < 2) {
        return Nan::ThrowTypeError("Two arguments required");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("First argument must be a string");
    }

    mbgl::style::Layer* layer = nodeMap->map->getStyle().getLayer(*Nan::Utf8String(info[0]));
    if (!layer) {
        return Nan::ThrowTypeError("layer not found");
    }

    Filter filter;

    if (!info[1]->IsNull() && !info[1]->IsUndefined()) {
        Error error;
        std::optional<Filter> converted = convert<Filter>(info[1], error);
        if (!converted) {
            Nan::ThrowTypeError(error.message.c_str());
            return;
        }
        filter = std::move(*converted);
    }

    layer->setFilter(filter);
}

void NodeMap::SetSize(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsArray()) {
        return Nan::ThrowTypeError("First argument must be an array");
    }

    auto size = info[0].As<v8::Array>();
    uint32_t width = 0;
    uint32_t height = 0;
    if (size->Length() > 0) {
        width = Nan::To<uint32_t>(Nan::Get(size, 0).ToLocalChecked()).ToChecked();
    }
    if (size->Length() > 1) {
        height = Nan::To<uint32_t>(Nan::Get(size, 1).ToLocalChecked()).ToChecked();
    }

    try {
        nodeMap->frontend->setSize({width, height});
        nodeMap->map->setSize({width, height});
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetCenter(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsArray()) {
        return Nan::ThrowTypeError("First argument must be an array");
    }

    auto center = info[0].As<v8::Array>();
    double latitude = 0;
    double longitude = 0;
    if (center->Length() > 0) {
        longitude = Nan::To<double>(Nan::Get(center, 0).ToLocalChecked()).ToChecked();
    }
    if (center->Length() > 1) {
        latitude = Nan::To<double>(Nan::Get(center, 1).ToLocalChecked()).ToChecked();
    }

    try {
        nodeMap->map->jumpTo(mbgl::CameraOptions().withCenter(mbgl::LatLng{latitude, longitude}));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetZoom(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsNumber()) {
        return Nan::ThrowTypeError("First argument must be a number");
    }

    try {
        nodeMap->map->jumpTo(mbgl::CameraOptions().withZoom(Nan::To<double>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetBearing(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsNumber()) {
        return Nan::ThrowTypeError("First argument must be a number");
    }

    try {
        nodeMap->map->jumpTo(mbgl::CameraOptions().withBearing(Nan::To<double>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetPitch(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsNumber()) {
        return Nan::ThrowTypeError("First argument must be a number");
    }

    try {
        nodeMap->map->jumpTo(mbgl::CameraOptions().withPitch(Nan::To<double>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetLight(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsObject()) {
        return Nan::ThrowTypeError("First argument must be an object");
    }

    try {
        Error conversionError;
        if (auto light = convert<mbgl::style::Light>(info[0], conversionError)) {
            nodeMap->map->getStyle().setLight(std::make_unique<Light>(*light));
        } else {
            return Nan::ThrowTypeError(conversionError.message.c_str());
        }
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetAxonometric(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsBoolean()) {
        return Nan::ThrowTypeError("First argument must be a boolean");
    }

    try {
        nodeMap->map->setProjectionMode(mbgl::ProjectionMode().withAxonometric(Nan::To<bool>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetXSkew(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsNumber()) {
        return Nan::ThrowTypeError("First argument must be a number");
    }

    try {
        nodeMap->map->setProjectionMode(mbgl::ProjectionMode().withXSkew(Nan::To<double>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetYSkew(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsNumber()) {
        return Nan::ThrowTypeError("First argument must be a number");
    }

    try {
        nodeMap->map->setProjectionMode(mbgl::ProjectionMode().withYSkew(Nan::To<double>(info[0]).ToChecked()));
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::SetFeatureState(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() < 2) {
        return Nan::ThrowTypeError("Two arguments required");
    }

    if (!info[0]->IsObject() || !info[1]->IsObject()) {
        return Nan::ThrowTypeError("Both arguments must be objects");
    }

    std::string sourceID;
    std::string featureID;
    std::optional<std::string> sourceLayerID;
    auto feature = Nan::To<v8::Object>(info[0]).ToLocalChecked();
    if (Nan::Has(feature, Nan::New("source").ToLocalChecked()).FromJust()) {
        auto sourceOption = Nan::Get(feature, Nan::New("source").ToLocalChecked()).ToLocalChecked();
        if (!sourceOption->IsString()) {
            return Nan::ThrowTypeError("Requires feature.source property to be a string");
        }
        sourceID = *Nan::Utf8String(sourceOption);
    } else {
        return Nan::ThrowTypeError("SetFeatureState: Requires feature.source property");
    }

    if (Nan::Has(feature, Nan::New("sourceLayer").ToLocalChecked()).FromJust()) {
        auto sourceLayerOption = Nan::Get(feature, Nan::New("sourceLayer").ToLocalChecked()).ToLocalChecked();
        if (!sourceLayerOption->IsString()) {
            return Nan::ThrowTypeError(
                "SetFeatureState: Requires feature.sourceLayer property to be "
                "a string");
        }
        sourceLayerID = {*Nan::Utf8String(sourceLayerOption)};
    }

    if (Nan::Has(feature, Nan::New("id").ToLocalChecked()).FromJust()) {
        auto idOption = Nan::Get(feature, Nan::New("id").ToLocalChecked()).ToLocalChecked();
        if (!idOption->IsString() && !(idOption->IsNumber() || idOption->IsString())) {
            return Nan::ThrowTypeError("Requires feature.id property to be a string or a number");
        }
        featureID = *Nan::Utf8String(idOption);
    } else {
        return Nan::ThrowTypeError("SetFeatureState: Requires feature.id property");
    }

    Convertible state(info[1]);

    if (!isObject(state)) {
        return Nan::ThrowTypeError("Feature state must be an object");
    }

    std::string sourceLayer = sourceLayerID.value_or(std::string());
    std::string stateKey;
    Value stateValue;
    bool valueParsed = false;
    FeatureState newState;

    const std::function<std::optional<Error>(const std::string&, const Convertible&)> convertFn =
        [&](const std::string& k, const Convertible& v) -> std::optional<Error> {
        std::optional<Value> value = toValue(v);
        if (value) {
            stateValue = std::move(*value);
            valueParsed = true;
        } else if (isArray(v)) {
            std::vector<Value> array;
            std::size_t length = arrayLength(v);
            array.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                std::optional<Value> arrayVal = toValue(arrayMember(v, i));
                if (arrayVal) {
                    array.emplace_back(*arrayVal);
                }
            }
            std::unordered_map<std::string, Value> result;
            result[k] = std::move(array);
            stateValue = std::move(result);
            valueParsed = true;
            return {};

        } else if (isObject(v)) {
            eachMember(v, convertFn);
        }
        if (!valueParsed) {
            Nan::ThrowTypeError("Could not get feature state value");
            return std::nullopt;
        }
        stateKey = k;
        newState[stateKey] = stateValue;
        return std::nullopt;
    };

    eachMember(state, convertFn);

    try {
        nodeMap->frontend->getRenderer()->setFeatureState(sourceID, sourceLayerID, featureID, newState);
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::GetFeatureState(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() < 1) {
        return Nan::ThrowTypeError("One argument required");
    }

    if (!info[0]->IsObject() || !info[1]->IsObject()) {
        return Nan::ThrowTypeError("Argument must be object");
    }

    std::string sourceID;
    std::string featureID;
    std::optional<std::string> sourceLayerID;
    auto feature = Nan::To<v8::Object>(info[0]).ToLocalChecked();
    if (Nan::Has(feature, Nan::New("source").ToLocalChecked()).FromJust()) {
        auto sourceOption = Nan::Get(feature, Nan::New("source").ToLocalChecked()).ToLocalChecked();
        if (!sourceOption->IsString()) {
            return Nan::ThrowTypeError("Requires feature.source property to be a string");
        }
        sourceID = *Nan::Utf8String(sourceOption);
    } else {
        return Nan::ThrowTypeError("GetFeatureState: Requires feature.source property");
    }

    if (Nan::Has(feature, Nan::New("sourceLayer").ToLocalChecked()).FromJust()) {
        auto sourceLayerOption = Nan::Get(feature, Nan::New("sourceLayer").ToLocalChecked()).ToLocalChecked();
        if (!sourceLayerOption->IsString()) {
            return Nan::ThrowTypeError(
                "GetFeatureState: Requires feature.sourceLayer property to be "
                "a string");
        }
        sourceLayerID = {*Nan::Utf8String(sourceLayerOption)};
    }

    if (Nan::Has(feature, Nan::New("id").ToLocalChecked()).FromJust()) {
        auto idOption = Nan::Get(feature, Nan::New("id").ToLocalChecked()).ToLocalChecked();
        if (!idOption->IsString() && !(idOption->IsNumber() || idOption->IsString())) {
            return Nan::ThrowTypeError("Requires feature.id property to be a string or a number");
        }
        featureID = *Nan::Utf8String(idOption);
    } else {
        return Nan::ThrowTypeError("GetFeatureState: Requires feature.id property");
    }

    mbgl::FeatureState state;
    try {
        nodeMap->frontend->getRenderer()->getFeatureState(state, sourceID, sourceLayerID, featureID);
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::RemoveFeatureState(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() < 1) {
        return Nan::ThrowTypeError("At least one argument required");
    }

    if (!info[0]->IsObject()) {
        return Nan::ThrowTypeError("Argument 1 must be object");
    }

    if (info.Length() == 2 && !info[1]->IsString()) {
        return Nan::ThrowTypeError("argument 2 must be string");
    }

    std::string sourceID;
    std::optional<std::string> sourceLayerID;
    std::optional<std::string> featureID;
    std::optional<std::string> stateKey;
    auto feature = Nan::To<v8::Object>(info[0]).ToLocalChecked();
    if (Nan::Has(feature, Nan::New("source").ToLocalChecked()).FromJust()) {
        auto sourceOption = Nan::Get(feature, Nan::New("source").ToLocalChecked()).ToLocalChecked();
        if (!sourceOption->IsString()) {
            return Nan::ThrowTypeError("Requires feature.source property to be a string");
        }
        sourceID = *Nan::Utf8String(sourceOption);
    } else {
        return Nan::ThrowTypeError("RemoveFeatureState: Requires feature.source property");
    }

    if (Nan::Has(feature, Nan::New("sourceLayer").ToLocalChecked()).FromJust()) {
        auto sourceLayerOption = Nan::Get(feature, Nan::New("sourceLayer").ToLocalChecked()).ToLocalChecked();
        if (!sourceLayerOption->IsString()) {
            return Nan::ThrowTypeError(
                "RemoveFeatureState: Requires feature.sourceLayer property to "
                "be a string");
        }
        sourceLayerID = {*Nan::Utf8String(sourceLayerOption)};
    }

    if (Nan::Has(feature, Nan::New("id").ToLocalChecked()).FromJust()) {
        auto idOption = Nan::Get(feature, Nan::New("id").ToLocalChecked()).ToLocalChecked();
        if (!idOption->IsString() && !(idOption->IsNumber() || idOption->IsString())) {
            return Nan::ThrowTypeError("Requires feature.id property to be a string or a number");
        }
        featureID = {*Nan::Utf8String(idOption)};
    }

    if (info.Length() == 2) {
        auto keyParam = Nan::To<v8::String>(info[1]).ToLocalChecked();
        if (!keyParam->IsString()) {
            return Nan::ThrowTypeError(
                "RemoveFeatureState: Requires feature key property to be a "
                "string");
        }
        stateKey = {*Nan::Utf8String(keyParam)};
    }

    try {
        nodeMap->frontend->getRenderer()->removeFeatureState(sourceID, sourceLayerID, featureID, stateKey);
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }

    info.GetReturnValue().SetUndefined();
}

void NodeMap::DumpDebugLogs(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    nodeMap->map->dumpDebugLogs();
    nodeMap->frontend->getRenderer()->dumpDebugLogs();

    info.GetReturnValue().SetUndefined();
}

void NodeMap::QueryRenderedFeatures(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    using namespace mbgl::style;
    using namespace mbgl::style::conversion;

    auto nodeMap = Nan::ObjectWrap::Unwrap<NodeMap>(info.Holder());
    if (!nodeMap->map) return Nan::ThrowError(releasedMessage());

    if (info.Length() <= 0 || !info[0]->IsArray()) {
        return Nan::ThrowTypeError("First argument must be an array");
    }

    auto posOrBox = info[0].As<v8::Array>();
    if (posOrBox->Length() != 2) {
        return Nan::ThrowTypeError("First argument must have two components");
    }

    mbgl::RenderedQueryOptions queryOptions;
    if (!info[1]->IsNull() && !info[1]->IsUndefined()) {
        if (!info[1]->IsObject()) {
            return Nan::ThrowTypeError("options argument must be an object");
        }

        auto options = Nan::To<v8::Object>(info[1]).ToLocalChecked();

        // Check if layers is set. If provided, it must be an array of strings
        if (Nan::Has(options, Nan::New("layers").ToLocalChecked()).FromJust()) {
            auto layersOption = Nan::Get(options, Nan::New("layers").ToLocalChecked()).ToLocalChecked();
            if (!layersOption->IsArray()) {
                return Nan::ThrowTypeError("Requires options.layers property to be an array");
            }
            auto layers = layersOption.As<v8::Array>();
            std::vector<std::string> layersVec;
            for (uint32_t i = 0; i < layers->Length(); i++) {
                layersVec.emplace_back(*Nan::Utf8String(Nan::Get(layers, i).ToLocalChecked()));
            }
            queryOptions.layerIDs = layersVec;
        }

        // Check if filter is provided. If set it must be a valid Filter object
        if (Nan::Has(options, Nan::New("filter").ToLocalChecked()).FromJust()) {
            auto filterOption = Nan::Get(options, Nan::New("filter").ToLocalChecked()).ToLocalChecked();
            Error error;
            std::optional<Filter> converted = convert<Filter>(filterOption, error);
            if (!converted) {
                return Nan::ThrowTypeError(error.message.c_str());
            }
            queryOptions.filter = std::move(*converted);
        }
    }

    try {
        std::vector<mbgl::Feature> optional;

        if (Nan::Get(posOrBox, 0).ToLocalChecked()->IsArray()) {
            auto pos0 = Nan::Get(posOrBox, 0).ToLocalChecked().As<v8::Array>();
            auto pos1 = Nan::Get(posOrBox, 1).ToLocalChecked().As<v8::Array>();

            optional = nodeMap->frontend->getRenderer()->queryRenderedFeatures(
                mbgl::ScreenBox{{Nan::To<double>(Nan::Get(pos0, 0).ToLocalChecked()).ToChecked(),
                                 Nan::To<double>(Nan::Get(pos0, 1).ToLocalChecked()).ToChecked()},
                                {Nan::To<double>(Nan::Get(pos1, 0).ToLocalChecked()).ToChecked(),
                                 Nan::To<double>(Nan::Get(pos1, 1).ToLocalChecked()).ToChecked()}},
                queryOptions);

        } else {
            optional = nodeMap->frontend->getRenderer()->queryRenderedFeatures(
                mbgl::ScreenCoordinate{Nan::To<double>(Nan::Get(posOrBox, 0).ToLocalChecked()).ToChecked(),
                                       Nan::To<double>(Nan::Get(posOrBox, 1).ToLocalChecked()).ToChecked()},
                queryOptions);
        }

        auto array = Nan::New<v8::Array>();
        for (std::size_t i = 0; i < optional.size(); i++) {
            Nan::Set(array, static_cast<uint32_t>(i), toJS(optional[i]));
        }
        info.GetReturnValue().Set(array);
    } catch (const std::exception& ex) {
        return Nan::ThrowError(ex.what());
    }
}

NodeMap::NodeMap(v8::Local<v8::Object> options)
    : pixelRatio([&] {
          Nan::HandleScope scope;
          return Nan::Has(options, Nan::New("ratio").ToLocalChecked()).FromJust()
                     ? static_cast<float>(
                           Nan::To<double>(Nan::Get(options, Nan::New("ratio").ToLocalChecked()).ToLocalChecked())
                               .ToChecked())
                     : 1.0f;
      }()),
      mode([&] {
          Nan::HandleScope scope;
          if (std::string(*v8::String::Utf8Value(
                  v8::Isolate::GetCurrent(), Nan::Get(options, Nan::New("mode").ToLocalChecked()).ToLocalChecked())) ==
              "tile") {
              return mbgl::MapMode::Tile;
          } else {
              return mbgl::MapMode::Static;
          }
      }()),
      crossSourceCollisions([&] {
          Nan::HandleScope scope;
          return Nan::Has(options, Nan::New("crossSourceCollisions").ToLocalChecked()).FromJust()
                     ? Nan::To<bool>(
                           Nan::Get(options, Nan::New("crossSourceCollisions").ToLocalChecked()).ToLocalChecked())
                           .ToChecked()
                     : true;
      }()),
      mapObserver(NodeMapObserver()),
      frontend(std::make_unique<mbgl::HeadlessFrontend>(mbgl::Size{512, 512}, pixelRatio)),
      map(std::make_unique<mbgl::Map>(*frontend,
                                      mapObserver,
                                      mbgl::MapOptions()
                                          .withSize(frontend->getSize())
                                          .withPixelRatio(pixelRatio)
                                          .withMapMode(mode)
                                          .withCrossSourceCollisions(crossSourceCollisions),
                                      mbgl::ResourceOptions().withPlatformContext(reinterpret_cast<void*>(this)),
                                      mbgl::ClientOptions())),
      async(new uv_async_t) {
    async->data = this;
    uv_async_init(
        uv_default_loop(), async, [](uv_async_t* h) { reinterpret_cast<NodeMap*>(h->data)->renderFinished(); });

    // Make sure the async handle doesn't keep the loop alive.
    uv_unref(reinterpret_cast<uv_handle_t*>(async));
}

NodeMap::~NodeMap() {
    try {
        if (map) release();
    } catch (...) {
        mbgl::Log::Error(mbgl::Event::General, "Error release the map object when destroying NodeMap");
    }
}

std::unique_ptr<mbgl::AsyncRequest> NodeFileSource::request(const mbgl::Resource& resource,
                                                            std::function<void(mbgl::Response)> callback_) {
    assert(nodeMap);

    Nan::HandleScope scope;
    // Because this method may be called while this NodeMap is already eligible
    // for garbage collection, we need to explicitly hold onto our own handle
    // here so that GC during a v8 call doesn't destroy *this while we're still
    // executing code.
    nodeMap->handle();

    auto asyncRequest = std::make_unique<node_mbgl::NodeAsyncRequest>();

    v8::Local<v8::Value> argv[] = {Nan::New<v8::External>(nodeMap),
                                   Nan::New<v8::External>(&callback_),
                                   Nan::New<v8::External>(asyncRequest.get()),
                                   Nan::New(resource.url).ToLocalChecked(),
                                   Nan::New<v8::Integer>(resource.kind)};

    Nan::NewInstance(Nan::New(node_mbgl::NodeRequest::constructor), 5, argv).ToLocalChecked();

    return asyncRequest;
}

bool NodeFileSource::canRequest(const mbgl::Resource&) const {
    return true;
}

void NodeFileSource::setResourceOptions(mbgl::ResourceOptions options) {
    this->_resourceOptions = std::move(options);
}

mbgl::ResourceOptions NodeFileSource::getResourceOptions() {
    return this->_resourceOptions.clone();
}

void NodeFileSource::setClientOptions(mbgl::ClientOptions options) {
    this->_clientOptions = std::move(options);
}

mbgl::ClientOptions NodeFileSource::getClientOptions() {
    return this->_clientOptions.clone();
}

} // namespace node_mbgl
