#pragma once
#include "bakkesmod/wrappers/wrapperstructs.h"
#include "Frustum.h"

class CanvasWrapper;

namespace RT
{
    class Box
    {
    public:
        Vector location;
        Quat orientation;
        Vector size;         // X, Y, Z extents
        float lineThickness;

        // CONSTRUCTORS
        explicit Box();
        explicit Box(Vector loc);
        explicit Box(Vector loc, Vector size);
        explicit Box(Vector loc, Quat rot, Vector size, float slineThickness);

        // FUNCTIONS
        void Draw(CanvasWrapper canvas, Frustum& frustum) const;
        bool IsInBox(const Vector& point) const;
    };
}
