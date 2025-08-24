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