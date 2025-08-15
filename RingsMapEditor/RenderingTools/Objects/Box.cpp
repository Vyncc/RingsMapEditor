#include "pch.h"

#include "bakkesmod/wrappers/canvaswrapper.h"
#include "Box.h"
#include "Matrix3.h"

#include "Line.h"

// IMPLEMENTATION
RT::Box::Box()
    : location(Vector()), orientation(Quat()), size(Vector(50.0f, 50.0f, 50.0f)), lineThickness(1.0f) {
}

RT::Box::Box(Vector loc)
    : location(loc), orientation(Quat()), size(Vector(50.0f, 50.0f, 50.0f)), lineThickness(1.0f) {
}

RT::Box::Box(Vector loc, Vector size)
    : location(loc), orientation(Quat()), size(size), lineThickness(1.0f) {
}

RT::Box::Box(Vector loc, Quat rot, Vector size, float slineThickness)
    : location(loc), orientation(rot), size(size), lineThickness(slineThickness) {
}

void RT::Box::Draw(CanvasWrapper canvas, Frustum& frustum) const
{
    // Half extents for correct positioning
    Vector halfSize = size * 0.5f;

    Matrix3 matrix(orientation);
    Vector fwd = matrix.forward * halfSize.X;  // X-axis
    Vector right = matrix.right * halfSize.Y;  // Y-axis
    Vector up = matrix.up * halfSize.Z;  // Z-axis

    Vector points[8];
    points[0] = location + fwd + right + up; // Front Right Top
    points[1] = location + fwd + right - up; // Front Right Bottom
    points[2] = location + fwd - right - up; // Front Left Bottom
    points[3] = location + fwd - right + up; // Front Left Top
    points[4] = location - fwd + right + up; // Back Right Top
    points[5] = location - fwd + right - up; // Back Right Bottom
    points[6] = location - fwd - right - up; // Back Left Bottom
    points[7] = location - fwd - right + up; // Back Left Top

    if (lineThickness != 1)
    {
        for (int32_t i = 0; i < 4; ++i)
        {
            if (i == 3)
            {
                RT::Line line1(points[i], points[0], lineThickness);
                line1.DrawWithinFrustum(canvas, frustum);

                RT::Line line2(points[i + 4], points[4], lineThickness);
                line1.DrawWithinFrustum(canvas, frustum);

                RT::Line line3(points[0], points[4], lineThickness);
                line3.DrawWithinFrustum(canvas, frustum);
            }
            else
            {
                RT::Line line4(points[i], points[i + 1], lineThickness);
                line4.DrawWithinFrustum(canvas, frustum);

                RT::Line line5(points[i + 4], points[i + 5], lineThickness);
                line5.DrawWithinFrustum(canvas, frustum);

                RT::Line line6(points[i + 1], points[i + 5], lineThickness);
                line6.DrawWithinFrustum(canvas, frustum);
            }
        }
    }
    else
    {
        for (int32_t i = 0; i < 4; ++i)
        {
            if (i == 3)
            {
                RT::Line line7(points[i], points[0]);
                line7.DrawWithinFrustum(canvas, frustum);

                RT::Line line8(points[i + 4], points[4]);
                line8.DrawWithinFrustum(canvas, frustum);

                RT::Line line9(points[0], points[4]);
                line9.DrawWithinFrustum(canvas, frustum);
            }
            else
            {
                RT::Line line10(points[i], points[i + 1]);
                line10.DrawWithinFrustum(canvas, frustum);

                RT::Line line11(points[i + 4], points[i + 5]);
                line11.DrawWithinFrustum(canvas, frustum);

                RT::Line line12(points[i + 1], points[i + 5]);
                line12.DrawWithinFrustum(canvas, frustum);
            }
        }
    }
}

bool RT::Box::IsInBox(const Vector& point) const
{
    // Step 1: move point relative to box center
    Vector localPoint = point - location;

    // Step 2: rotate into box's local orientation space (using conjugate for inverse rotation)
    Vector rotated = RotateVectorWithQuat(localPoint, orientation.conjugate());

    // Step 3: check against half extents
    Vector halfSize = size * 0.5f;

    return (fabsf(rotated.X) <= halfSize.X &&
        fabsf(rotated.Y) <= halfSize.Y &&
        fabsf(rotated.Z) <= halfSize.Z);
}