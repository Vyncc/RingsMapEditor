#pragma once
#include <string>

#include "RLSDK/SdkHeaders.hpp"
#include "RLSDK/Utils.hpp"

#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FVector, X, Y, Z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vector, X, Y, Z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FRotator, Pitch, Yaw, Roll)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rotator, Pitch, Yaw, Roll)

enum class ObjectType : uint8_t
{
	Unknown = 0,
    Mesh = 1,
	TriggerVolume = 2,
	Checkpoint = 3,
    Ring = 4
};

class Object
{
public:

    Object() {}
    Object(std::string _name, Vector _location = { 0.f, 0.f, 0.f }, Rotator _rotation = { 0, 0, 0 }, float _scale = 1.f) {
        name = _name;
        location = _location;
        rotation = _rotation;
        scale = _scale;
    }

	virtual ~Object() {}

    virtual nlohmann::json to_json() const = 0;
    virtual std::shared_ptr<Object> Clone() = 0;


    virtual Vector GetLocation() const {
        return location;
    }

    virtual void SetLocation(const Vector& _newLocation) {
        location = _newLocation;
    }

    virtual Rotator GetRotation() const {
        return rotation;
    }

    virtual void SetRotation(const Rotator& _newRotation) {
        rotation = _newRotation;
    }

    FVector GetFVectorLocation() const {
		return VectorToFVector(location);
	}

    FRotator GetFRotatorRotation() const {
		return RotatorToFRotator(rotation);
	}

    static FVector VectorToFVector(const Vector& _vector) {
        return FVector{ _vector.X, _vector.Y, _vector.Z };
    }

    static Vector FVectorToVector(const FVector& _vector) {
        return Vector{ _vector.X, _vector.Y, _vector.Z };
    }

    static FRotator RotatorToFRotator(const Rotator& _rotator) {
        return FRotator{ _rotator.Pitch, _rotator.Yaw, _rotator.Roll };
    }

    static Rotator FRotatorToRotator(const FRotator& _rotator) {
        return Rotator{ _rotator.Pitch, _rotator.Yaw, _rotator.Roll };
    }

	ObjectType objectType = ObjectType::Unknown;
    std::string name;
    Vector location;
    Rotator rotation;
    float scale = 1.0f;
};