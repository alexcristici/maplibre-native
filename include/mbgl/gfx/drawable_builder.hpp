#pragma once

#include <mbgl/gfx/drawable.hpp>

#include <vector>

namespace mbgl {
namespace gfx {

/**
    Base class for drawable builders, which construct Drawables from primitives
 */
class DrawableBuilder {
protected:
    DrawableBuilder();

public:
    virtual ~DrawableBuilder();

    /// Get the drawable we're currently working on, if any
    DrawablePtr getCurrentDrawable(bool createIfNone);

    /// Close the current drawable, using a new one for any further work
    void flush();

    /// Get all the completed drawables
    const std::vector<DrawablePtr>& getDrawables() const { return drawables; }

    /// Get all the completed drawables and release ownership
    std::vector<DrawablePtr> clearDrawables() const { return std::move(drawables); }

    /// Get the ID of the drawable we're currently working on, if any
    util::SimpleIdentity getDrawableId();

    /// Get the draw priority assigned to generated drawables
    DrawPriority getDrawPriority() const;
    /// Set the draw priority assigned to generated drawables
    void setDrawPriority(DrawPriority);

    /// Set the draw priority on all drawables including those already generated
    void resetDrawPriority(DrawPriority);

    /// Add a triangle
    void addTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    /// Add another triangle based on the previous two points
    void appendTriangle(int16_t x0, int16_t y0);

    /// Add a rectangle consisting of two triangles
    void addQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

protected:
    /// Create an instance of the appropriate drawable type
    virtual DrawablePtr createDrawable() const = 0;

protected:
    DrawPriority drawPriority = 0;

    DrawablePtr currentDrawable;
    std::vector<DrawablePtr> drawables;
    
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace gfx
} // namespace mbgl