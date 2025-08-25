#pragma once
#include "Object.h"
#include "bakkesmod/wrappers/canvaswrapper.h"
#include "bakkesmod/wrappers/GameObject/CameraWrapper.h"
#include "RenderingTools/RenderingTools.h"

#include <cmath>
#include <array>
#define M_PI       3.14159265358979323846   // pi

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

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
    virtual bool RayIntersects(const Vector& rayOrigin, const Vector& rayDir, float maxDist, float& tHit) const = 0;
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

    TriggerVolume_Box() {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = Vector(0);
        rotation = Rotator(0);

        onTouchCallback = nullptr;

        size = Vector{ 200.f, 200.f, 200.f };
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
    }

    TriggerVolume_Box(Vector _size) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = Vector{ 0.f, 0.f , 0.f };
        rotation = Rotator{ 0, 0, 0 };

        onTouchCallback = nullptr;

        size = _size;
    }

    TriggerVolume_Box(Vector _location, Rotator _rotation, Vector _size) {
        objectType = ObjectType::TriggerVolume;
        triggerVolumeType = TriggerVolumeType::Box;
        name = "Trigger Volume Box";
        location = _location;
        rotation = _rotation;

        onTouchCallback = nullptr;

        size = _size;
    }

    ~TriggerVolume_Box() {}

    void SetLocation(const Vector& newLocation) override {
        location = newLocation;
    }

    void SetRotation(const Rotator& newRotation) override {
        rotation = newRotation;
    }

    void SetSize(Vector newSize) {
        size = newSize;
    }

    void SetSizeX(float newSizeX) {
        size.X = newSizeX;
    }

    void SetSizeY(float newSizeY) {
        size.Y = newSizeY;
    }

    void SetSizeZ(float newSizeZ) {
        size.Z = newSizeZ;
    }

    // Check if a point is inside the box
    bool IsPointInside(const Vector& point) const override {
        RT::Box box(location, RotatorToQuat(rotation), size, 1.f);
        return box.IsInBox(point);
    }

    bool RayIntersects(const Vector& rayOrigin, const Vector& rayDir, float maxDist, float& tHit) const override {
        // Build transform for box
        Quat boxRotation = RotatorToQuat(rotation).normalize();
        Quat invRot = boxRotation.conjugate();

        // Transform ray into local box space
        Vector localOrigin = RotateVectorWithQuat(rayOrigin - location, invRot);
        Vector localDir = RotateVectorWithQuat(rayDir, invRot);

        // Normalize direction (for stability)
        localDir = localDir.getNormalized();

        // Half extents
        Vector halfSize = size * 0.5f;

        float tmin = -FLT_MAX;
        float tmax = FLT_MAX;

        // X slab
        if (fabs(localDir.X) < 1e-6f)
        {
            if (localOrigin.X < -halfSize.X || localOrigin.X > halfSize.X)
                return false; // Parallel and outside
        }
        else
        {
            float tx1 = (-halfSize.X - localOrigin.X) / localDir.X;
            float tx2 = (halfSize.X - localOrigin.X) / localDir.X;
            if (tx1 > tx2) std::swap(tx1, tx2);
            tmin = max(tmin, tx1);
            tmax = min(tmax, tx2);
        }

        // Y slab
        if (fabs(localDir.Y) < 1e-6f)
        {
            if (localOrigin.Y < -halfSize.Y || localOrigin.Y > halfSize.Y)
                return false;
        }
        else
        {
            float ty1 = (-halfSize.Y - localOrigin.Y) / localDir.Y;
            float ty2 = (halfSize.Y - localOrigin.Y) / localDir.Y;
            if (ty1 > ty2) std::swap(ty1, ty2);
            tmin = max(tmin, ty1);
            tmax = min(tmax, ty2);
        }

        // Z slab
        if (fabs(localDir.Z) < 1e-6f)
        {
            if (localOrigin.Z < -halfSize.Z || localOrigin.Z > halfSize.Z)
                return false;
        }
        else
        {
            float tz1 = (-halfSize.Z - localOrigin.Z) / localDir.Z;
            float tz2 = (halfSize.Z - localOrigin.Z) / localDir.Z;
            if (tz1 > tz2) std::swap(tz1, tz2);
            tmin = max(tmin, tz1);
            tmax = min(tmax, tz2);
        }

        // Final check
        if (tmax >= tmin && tmin <= maxDist && tmax >= 0.0f)
        {
            tHit = (tmin >= 0.0f) ? tmin : tmax; // pick nearest valid hit
            return true;
        }

        return false;
    }

    void Render(CanvasWrapper canvas, CameraWrapper camera) override {
        RT::Frustum frustum(canvas, camera);
        canvas.SetColor(255, 255, 255, 255);
        RT::Box box(location, RotatorToQuat(rotation), size, 1.f);
        box.Draw(canvas, frustum);
    }

    nlohmann::json to_json() const override {
        nlohmann::json triggerVolumeBoxJson{
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"triggerVolumeType", static_cast<uint8_t>(triggerVolumeType)},
            {"size", size}
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
        return clonedVolume;
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

    bool RayIntersects(const Vector& rayOrigin, const Vector& rayDir, float maxDist, float& tHit) const override {
        // Step 1: Transform ray into local cylinder space
        Quat cylRot = RotatorToQuat(rotation);
        cylRot.normalize();
        Quat invRot = cylRot.conjugate();

        Vector localOrigin = RotateVectorWithQuat(rayOrigin - location, invRot);
        Vector localDir = RotateVectorWithQuat(rayDir, invRot).getNormalized();

        float halfHeight = height * 0.5f;
        float closestT = FLT_MAX;
        bool hit = false;

        //
        // Step 2: Side wall intersection
        //
        float a = localDir.X * localDir.X + localDir.Y * localDir.Y;
        float b = 2.0f * (localOrigin.X * localDir.X + localOrigin.Y * localDir.Y);
        float c = localOrigin.X * localOrigin.X + localOrigin.Y * localOrigin.Y - radius * radius;

        if (fabs(a) > 1e-6f) // ray not parallel to cylinder axis
        {
            float disc = b * b - 4 * a * c;
            if (disc >= 0.0f)
            {
                float sqrtDisc = sqrtf(disc);
                float t1 = (-b - sqrtDisc) / (2 * a);
                float t2 = (-b + sqrtDisc) / (2 * a);

                if (t1 > 0.0f)
                {
                    float z = localOrigin.Z + t1 * localDir.Z;
                    if (z >= -halfHeight && z <= halfHeight && t1 <= maxDist)
                    {
                        closestT = min(closestT, t1);
                        hit = true;
                    }
                }
                if (t2 > 0.0f)
                {
                    float z = localOrigin.Z + t2 * localDir.Z;
                    if (z >= -halfHeight && z <= halfHeight && t2 <= maxDist)
                    {
                        closestT = min(closestT, t2);
                        hit = true;
                    }
                }
            }
        }

        //
        // Step 3: Cap intersections (top and bottom disks)
        //
        if (fabs(localDir.Z) > 1e-6f)
        {
            // Bottom cap (z = -halfHeight)
            float tCap = (-halfHeight - localOrigin.Z) / localDir.Z;
            if (tCap > 0.0f && tCap <= maxDist)
            {
                float x = localOrigin.X + tCap * localDir.X;
                float y = localOrigin.Y + tCap * localDir.Y;
                if (x * x + y * y <= radius * radius)
                {
                    closestT = min(closestT, tCap);
                    hit = true;
                }
            }

            // Top cap (z = +halfHeight)
            tCap = (halfHeight - localOrigin.Z) / localDir.Z;
            if (tCap > 0.0f && tCap <= maxDist)
            {
                float x = localOrigin.X + tCap * localDir.X;
                float y = localOrigin.Y + tCap * localDir.Y;
                if (x * x + y * y <= radius * radius)
                {
                    closestT = min(closestT, tCap);
                    hit = true;
                }
            }
        }

        //
        // Step 4: return result
        //
        if (hit)
        {
            tHit = closestT;
            return true;
        }
        return false;
    }

    void Render(CanvasWrapper canvas, CameraWrapper camera) override {
        RT::Frustum frustum(canvas, camera);
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