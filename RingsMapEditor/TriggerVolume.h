#pragma once
#include "Object.h"
#include "bakkesmod/wrappers/canvaswrapper.h"
#include "bakkesmod/wrappers/GameObject/CameraWrapper.h"
#include "RenderingTools/RenderingTools.h"

#include <cmath>
#include <array>
#define M_PI       3.14159265358979323846   // pi

class TriggerFunction
{
public:
    TriggerFunction() = default;
    virtual ~TriggerFunction() = default;

    virtual void Execute(ActorWrapper actor) = 0;

    virtual nlohmann::json to_json() const = 0;
    virtual void RenderParameters() = 0; // Render parameters in ImGui
    virtual std::shared_ptr<TriggerFunction> Clone() = 0;
    virtual std::shared_ptr<TriggerFunction> CloneFromJson(const nlohmann::json& j) = 0;

    std::string name;
    std::string description;
};

enum class TriggerVolumeType : uint8_t
{
    Unknown = 0,
    Box = 1,
    Cylinder = 2
};

inline std::map< TriggerVolumeType, std::string> triggerVolumeTypesMap = {
    { TriggerVolumeType::Box, "Box" },
    { TriggerVolumeType::Cylinder, "Cylinder" }
};

class TriggerVolume : public Object
{
public:
    TriggerVolume() {
		objectType = ObjectType::TriggerVolume;
		name = "Trigger Volume";
		location = FVector{ 0.f, 0.f, 0.f };
		rotation = FRotator{ 0, 0, 0 };
        triggerVolumeType = TriggerVolumeType::Unknown;
    }
    virtual ~TriggerVolume() {}

    void OnTouch(ActorWrapper actor) {
        if (onTouchCallback)
        {
            onTouchCallback->Execute(actor);
        }
    }

    void SetOnTouchCallback(std::shared_ptr<TriggerFunction> callback) {
        onTouchCallback = callback;
    }

    virtual bool IsPointInside(const FVector& point) const = 0;
    virtual bool IsPointInside(const Vector& point) const = 0;
    virtual void Render(CanvasWrapper canvas, CameraWrapper camera) = 0;

    virtual nlohmann::json to_json() const override = 0;
    virtual std::shared_ptr<Object> Clone() override = 0;

    TriggerVolumeType triggerVolumeType = TriggerVolumeType::Unknown;
    std::shared_ptr<TriggerFunction> onTouchCallback = nullptr; // Callback function to execute when touched
};

class TriggerVolume_Box : public TriggerVolume
{
public:
    // Rotation is in degrees
    FVector size = { 200.f, 200.f, 200.f };     // Full size along X, Y, Z
    std::array<FVector, 8> vertices;            // 8 vertices in WORLD coordinates

    TriggerVolume_Box() {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = FVector{ 0.f, 0.f, 0.f };
        rotation = FRotator{ 0, 0, 0 };
        size = FVector{ 200.f, 200.f, 200.f };
        UpdateVertices();
    }

    // Conversion constructor from base TriggerVolume
    TriggerVolume_Box(const TriggerVolume& base) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = base.name;
        location = base.location;
        rotation = base.rotation;

        size = FVector{ 200.f, 200.f, 200.f };
        UpdateVertices();
    }

    TriggerVolume_Box(FVector _size) {
        location = FVector{ 0.f, 0.f , 0.f };
        rotation = FRotator{ 0, 0, 0 };
        size = _size;
        UpdateVertices();
    }

    TriggerVolume_Box(FVector _location, FRotator _rotation, FVector _size) {
        location = _location;
        rotation = _rotation;
        size = _size;
        UpdateVertices();
    }

    ~TriggerVolume_Box() {}

    // Set the center position
    void SetLocation(const FVector& newLocation) {
        location = newLocation;
        UpdateVertices();
    }

    // Set the rotation
    void SetRotation(const FRotator& newRotation) {
        rotation = newRotation;
        UpdateVertices();
    }

    // Resize and update vertices
    void SetSizeX(float newX) {
        size.X = newX;
        UpdateVertices();
    }

    // Resize and update vertices
    void SetSizeY(float newY) {
        size.Y = newY;
        UpdateVertices();
    }

    // Resize and update vertices
    void SetSizeZ(float newZ) {
        size.Z = newZ;
        UpdateVertices();
    }

    // Resize and update vertices
    void SetSize(FVector newSize) {
        size = newSize;
        UpdateVertices();
    }

    // Recalculate vertex positions in WORLD space
    void UpdateVertices() {
        float hx = size.X / 2.0f;
        float hy = size.Y / 2.0f;
        float hz = size.Z / 2.0f;

        std::array<FVector, 8> local = {
            FVector{ -hx, -hy, -hz },
            FVector{ hx, -hy, -hz },
            FVector{ hx, hy, -hz },
            FVector{ -hx, hy, -hz },
            FVector{ -hx, -hy, hz },
            FVector{ hx, -hy, hz },
            FVector{ hx, hy, hz },
            FVector{ -hx, hy, hz }
        };


        for (size_t i = 0; i < 8; i++) {
            FVector rotatedVector = RotateVector(local[i], rotation);
            vertices[i] = FVector{ location.X + rotatedVector.X, location.Y + rotatedVector.Y, location.Z + rotatedVector.Z };
        }
    }

    // Check if a point is inside the box
    bool IsPointInside(const FVector& point) const override {
        FVector local = { point.X - location.X, point.Y - location.Y, point.Z - location.Z };
        local = InverseRotateVector(local, rotation);

        float hx = size.X / 2.0f;
        float hy = size.Y / 2.0f;
        float hz = size.Z / 2.0f;

        return (local.X >= -hx && local.X <= hx &&
            local.Y >= -hy && local.Y <= hy &&
            local.Z >= -hz && local.Z <= hz);
    }

    // Check if a point is inside the box
    bool IsPointInside(const Vector& point) const override {
        return IsPointInside(FVector{ point.X, point.Y, point.Z });
    }

    void Render(CanvasWrapper canvas, CameraWrapper camera) override {
        RT::Frustum frustum(canvas, camera);  // Create frustum (use default constructor or initialize as needed)

        // Edges of a box defined by vertex indices
        constexpr int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom face edges
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top face edges
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges connecting top and bottom faces
        };

        // Set the draw color to white, fully opaque
        canvas.SetColor(255, 255, 255, 255);

        // Draw each edge as a line within the frustum
        for (auto& edge : edges)
        {
            Vector start = Vector{ vertices[edge[0]].X, vertices[edge[0]].Y, vertices[edge[0]].Z };
            Vector end = Vector{ vertices[edge[1]].X, vertices[edge[1]].Y, vertices[edge[1]].Z };
            RT::Line line(start, end);
            line.DrawWithinFrustum(canvas, frustum);
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json triggerVolumeBoxJson{
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"triggerVolumeType", static_cast<uint8_t>(triggerVolumeType)},
            {"size", size},
            {"vertices", vertices}
        };

        if (onTouchCallback)
            triggerVolumeBoxJson["onTouchCallback"] = onTouchCallback->to_json();
        else
            triggerVolumeBoxJson["onTouchCallback"] = nullptr;

        return triggerVolumeBoxJson;
    }

    std::shared_ptr<Object> Clone() override {
        std::shared_ptr<TriggerVolume_Box> clonedVolume = std::make_shared<TriggerVolume_Box>(*this);
        clonedVolume->onTouchCallback = (onTouchCallback ? onTouchCallback->Clone() : nullptr);
        clonedVolume->UpdateVertices(); // Ensure vertices are updated after cloning
        return clonedVolume;
    }

private:
    // Rotate a vector from local -> world space
    FVector RotateVector(const FVector& v, const FRotator& r) const {
        float pitchRad = r.Pitch * (M_PI / 180.0f);
        float yawRad = r.Yaw * (M_PI / 180.0f);
        float rollRad = r.Roll * (M_PI / 180.0f);

        // Yaw (around Z axis)
        float cosY = cosf(yawRad), sinY = sinf(yawRad);
        FVector res(v.X * cosY - v.Y * sinY,
            v.X * sinY + v.Y * cosY,
            v.Z);

        // Pitch (around Y axis)
        float cosP = cosf(pitchRad), sinP = sinf(pitchRad);
        FVector resP(res.X * cosP + res.Z * sinP,
            res.Y,
            -res.X * sinP + res.Z * cosP);

        // Roll (around X axis)
        float cosR = cosf(rollRad), sinR = sinf(rollRad);
        FVector resR(resP.X,
            resP.Y * cosR - resP.Z * sinR,
            resP.Y * sinR + resP.Z * cosR);

        return resR;
    }

    // Rotate a vector from world -> local space
    FVector InverseRotateVector(const FVector& v, const FRotator& r) const {
        FRotator inv{ -r.Pitch, -r.Yaw, -r.Roll };
        return RotateVector(v, inv);
    }
};