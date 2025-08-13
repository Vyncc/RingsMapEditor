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
	None = 0,
    Mesh = 1,
	TriggerVolume = 2,
	Checkpoint = 3
};

class Object
{
public:

    Object() {}
    Object(std::string _name, FVector _location = { 0.f, 0.f, 0.f }, FRotator _rotation = { 0, 0, 0 }, float _scale = 1.f) {
        name = _name;
        location = _location;
        rotation = _rotation;
        scale = _scale;
    }

	virtual ~Object() {}

    virtual nlohmann::json to_json() const = 0;
    virtual std::shared_ptr<Object> Clone() = 0;

	Vector GetVectorLocation() const {
		return Vector{ location.X, location.Y, location.Z };
	}

	Vector GetFVectorLocation() const {
		return Vector{ location.X, location.Y, location.Z };
	}

	ObjectType objectType = ObjectType::None;
    std::string name;
    FVector location;
    FRotator rotation;
    float scale = 1.0f;
};