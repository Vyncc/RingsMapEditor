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
		location = Vector(0);
        rotation = Rotator(0);
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
    Vector size = { 200.f, 200.f, 200.f };     // Full size along X, Y, Z
    std::array<Vector, 8> vertices;            // 8 vertices in WORLD coordinates

    TriggerVolume_Box() {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = Vector(0);
        rotation = Rotator(0);
        size = Vector{ 200.f, 200.f, 200.f };
        UpdateVertices();
    }

    // Conversion constructor from base TriggerVolume
    TriggerVolume_Box(const TriggerVolume& base) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = base.name;
        location = base.location;
        rotation = base.rotation;

        if (base.onTouchCallback)
            onTouchCallback = base.onTouchCallback->Clone();

        size = Vector{ 200.f, 200.f, 200.f };
        UpdateVertices();
    }

    TriggerVolume_Box(Vector _size) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = Vector{ 0.f, 0.f , 0.f };
        rotation = Rotator{ 0, 0, 0 };
        size = _size;
        UpdateVertices();
    }

    TriggerVolume_Box(Vector _location, Rotator _rotation, Vector _size) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = _location;
        rotation = _rotation;
        size = _size;
        UpdateVertices();
    }

    ~TriggerVolume_Box() {}

    // Set the center position
    void SetLocation(const Vector& newLocation) override {
        location = newLocation;
        UpdateVertices();
    }

    // Set the rotation
    void SetRotation(const Rotator& newRotation) override {
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
    void SetSize(Vector newSize) {
        size = newSize;
        UpdateVertices();
    }

    // Recalculate vertex positions in WORLD space
    void UpdateVertices() {
        float hx = size.X / 2.0f;
        float hy = size.Y / 2.0f;
        float hz = size.Z / 2.0f;

        std::array<Vector, 8> local = {
            Vector{ -hx, -hy, -hz },
            Vector{ hx, -hy, -hz },
            Vector{ hx, hy, -hz },
            Vector{ -hx, hy, -hz },
            Vector{ -hx, -hy, hz },
            Vector{ hx, -hy, hz },
            Vector{ hx, hy, hz },
            Vector{ -hx, hy, hz }
        };

        for (size_t i = 0; i < 8; i++) {
            Vector rotatedVector = RotateVector(local[i], rotation);
            vertices[i] = location + rotatedVector;
        }
    }

    // Check if a point is inside the box
    bool IsPointInside(const Vector& point) const override {
        Vector local = point - location;
        local = InverseRotateVector(local, rotation);

        float hx = size.X / 2.0f;
        float hy = size.Y / 2.0f;
        float hz = size.Z / 2.0f;

        return (local.X >= -hx && local.X <= hx &&
            local.Y >= -hy && local.Y <= hy &&
            local.Z >= -hz && local.Z <= hz);
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
            Vector start = vertices[edge[0]];
            Vector end = vertices[edge[1]];
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
    Vector RotateVector(const Vector& v, const Rotator& r) const {
        float pitchRad = r.Pitch * (M_PI / 180.0f);
        float yawRad = r.Yaw * (M_PI / 180.0f);
        float rollRad = r.Roll * (M_PI / 180.0f);

        // Yaw (around Z axis)
        float cosY = cosf(yawRad), sinY = sinf(yawRad);
        Vector res(v.X * cosY - v.Y * sinY,
            v.X * sinY + v.Y * cosY,
            v.Z);

        // Pitch (around Y axis)
        float cosP = cosf(pitchRad), sinP = sinf(pitchRad);
        Vector resP(res.X * cosP + res.Z * sinP,
            res.Y,
            -res.X * sinP + res.Z * cosP);

        // Roll (around X axis)
        float cosR = cosf(rollRad), sinR = sinf(rollRad);
        Vector resR(resP.X,
            resP.Y * cosR - resP.Z * sinR,
            resP.Y * sinR + resP.Z * cosR);

        return resR;
    }

    // Rotate a vector from world -> local space
    Vector InverseRotateVector(const Vector& v, const Rotator& r) const {
        Rotator inv{ -r.Pitch, -r.Yaw, -r.Roll };
        return RotateVector(v, inv);
    }
};

class TriggerVolume_Cylinder : public TriggerVolume
{
public:
    float radius = 50.f;
    float height = 100.f;

    TriggerVolume_Cylinder() {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Cylinder;
        name = "Trigger Volume Cylinder";
        location = Vector(0);
        rotation = Rotator(0);
        radius = 50.f;
        height = 100.f;
    }

    // Conversion constructor from base TriggerVolume
    TriggerVolume_Cylinder(const TriggerVolume& base) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Cylinder;
        name = base.name;
        location = base.location;
        rotation = base.rotation;

        if (base.onTouchCallback)
            onTouchCallback = base.onTouchCallback->Clone();

        radius = 50.f;
        height = 100.f;
    }

    TriggerVolume_Cylinder(float _radius, float _height) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Cylinder;
        name = "Trigger Volume Cylinder";
        location = Vector(0);
        rotation = Rotator(0);
        radius = _radius;
        height = _height;
    }

    TriggerVolume_Cylinder(Vector _location, Rotator _rotation, float _radius, float _height) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Cylinder;
        name = "Trigger Volume Cylinder";
        location = _location;
        rotation = _rotation;
        radius = _radius;
        height = _height;
    }

    ~TriggerVolume_Cylinder() {}

    bool IsPointInside(const Vector& point) const override {
        RT::Cylinder cylinder(location, RotatorToQuat(rotation), radius, height);
        return cylinder.IsInCylinder(point);
    }

    void Render(CanvasWrapper canvas, CameraWrapper camera) override {
        RT::Frustum frustum(canvas, camera);  // Create frustum (use default constructor or initialize as needed)
        canvas.SetColor(255, 255, 255, 255);
        RT::Cylinder cylinder(location, RotatorToQuat(rotation), radius, height);
        cylinder.Draw(canvas, frustum);
    }

    nlohmann::json to_json() const override {
        nlohmann::json triggerVolumeBoxJson{
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"triggerVolumeType", static_cast<uint8_t>(triggerVolumeType)},
            {"radius", radius},
            {"height", height}
        };

        if (onTouchCallback)
            triggerVolumeBoxJson["onTouchCallback"] = onTouchCallback->to_json();
        else
            triggerVolumeBoxJson["onTouchCallback"] = nullptr;

        return triggerVolumeBoxJson;
    }

    std::shared_ptr<Object> Clone() override {
        std::shared_ptr<TriggerVolume_Cylinder> clonedVolume = std::make_shared<TriggerVolume_Cylinder>(*this);
        clonedVolume->onTouchCallback = (onTouchCallback ? onTouchCallback->Clone() : nullptr);
        return clonedVolume;
    }
};