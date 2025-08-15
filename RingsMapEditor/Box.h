#pragma once

#include "bakkesmod/wrappers/wrapperstructs.h"

class CanvasWrapper;

namespace RT
{
    class Box
    {
    public:
        Vector location;
        Quat orientation;
        Vector size;
        float lineThickness;

        // CONSTRUCTORS
        explicit Box();
        explicit Box(Vector loc);
        explicit Box(Vector loc, Vector sz);
        explicit Box(Vector loc, Quat rot, Vector sz, float slineThickness);

        // FUNCTIONS
        void Draw(CanvasWrapper canvas) const;
    };
}