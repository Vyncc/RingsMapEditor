#pragma once
#include "Mesh.h"
#include "TriggerVolume.h"

class Ring : public Object
{
public:
	Ring() {
		objectType = ObjectType::Ring;
		name = "Ring";

		ringId = -1;
		mesh = Mesh();
		mesh.enableCollisions = true;
		triggerVolumeIn = TriggerVolume_Cylinder();
		triggerVolumeOut = TriggerVolume_Box();

		SetLocation(Vector(0));
		SetRotation(Rotator(0));
	}

	Ring(int _id) {
		objectType = ObjectType::Ring;
		name = "Ring";

		ringId = _id;
		mesh = Mesh("Ring Mesh");
		mesh.enableCollisions = true;
		triggerVolumeIn = TriggerVolume_Cylinder();
		triggerVolumeOut = TriggerVolume_Box();

		SetLocation(Vector(0));
		SetRotation(Rotator(0));
	}

	virtual ~Ring() {}

	void SetLocation(const Vector& _newLocation) override {
		location = _newLocation;
		mesh.SetLocation(_newLocation);
		UpdateTriggerVolumes();
	}

	void SetRotation(const Rotator& _newRotation) override {
		rotation = _newRotation;
		mesh.SetRotation(_newRotation);
		UpdateTriggerVolumes();
	}

	void UpdateTriggerVolumes() {
		// Rotate the offsets so they pivot around Ring's origin
		Vector rotatedOffsetIn = RotateVectorWithQuat(triggerVolumeIn_offset_location, RotatorToQuat(rotation));
		Vector rotatedOffsetOut = RotateVectorWithQuat(triggerVolumeOut_offset_location, RotatorToQuat(rotation));

		// Compute the final world locations
		Vector worldLocIn = location + rotatedOffsetIn;
		Vector worldLocOut = location + rotatedOffsetOut;

		triggerVolumeIn.SetLocation(worldLocIn);
		triggerVolumeIn.SetRotation(triggerVolumeIn_offset_rotation + rotation);

		triggerVolumeOut.SetLocation(worldLocOut);
		triggerVolumeOut.SetRotation(triggerVolumeOut_offset_rotation + rotation);
	}

	void RenderTriggerVolumes(CanvasWrapper canvas, CameraWrapper camera) {
		triggerVolumeIn.Render(canvas, camera);
		triggerVolumeOut.Render(canvas, camera);
	}

    nlohmann::json to_json() const override {
        return {
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"ringId", ringId},
            {"mesh", mesh.to_json()},
			{"triggerVolumeIn", triggerVolumeIn.to_json()},
			{"triggerVolumeIn_offset_location", triggerVolumeIn_offset_location},
			{"triggerVolumeIn_offset_rotation", triggerVolumeIn_offset_rotation},
            {"triggerVolumeOut", triggerVolumeOut.to_json()},
			{"triggerVolumeOut_offset_location", triggerVolumeOut_offset_location },
			{"triggerVolumeOut_offset_rotation", triggerVolumeOut_offset_rotation}
        };
    }

	//Would be better if I Clone() for mesh, triggerVolumeIn, triggerVolumeOut. But I would first need to store them as shared_ptr
    std::shared_ptr<Object> Clone() override {
        std::shared_ptr<Ring> clonedRing = std::make_shared<Ring>(*this);
		clonedRing->mesh.instance = nullptr;
		clonedRing->triggerVolumeIn.onTouchCallback = (clonedRing->triggerVolumeIn.onTouchCallback ? clonedRing->triggerVolumeIn.onTouchCallback->Clone() : nullptr);
        clonedRing->triggerVolumeOut.onTouchCallback = (clonedRing->triggerVolumeOut.onTouchCallback ? clonedRing->triggerVolumeOut.onTouchCallback->Clone() : nullptr);
		clonedRing->UpdateTriggerVolumes();
        return clonedRing;
    }

    int ringId = -1;
	Mesh mesh;

	TriggerVolume_Cylinder triggerVolumeIn;
	Vector triggerVolumeIn_offset_location;
	Rotator triggerVolumeIn_offset_rotation;

	TriggerVolume_Box triggerVolumeOut;
	Vector triggerVolumeOut_offset_location;
	Rotator triggerVolumeOut_offset_rotation;
};

class Ring_Small : public Ring
{
public:
	Ring_Small(int _id) : Ring(_id) {
		mesh.meshInfos.name = "Ring Small";
		mesh.meshInfos.meshPath = "ringsmapeditor.prop_ring";

		triggerVolumeIn_offset_location = Vector(0.f, -35.f, 0.f);
		triggerVolumeIn_offset_rotation = Rotator(0, 0, 16400);
		triggerVolumeIn.height = 15.f;
		triggerVolumeIn.radius = 195.f;

		triggerVolumeOut_offset_location = Vector(0.f, -90.f, 0.f);
		triggerVolumeOut.SetSize(Vector(640.f, 60.f, 640.f));

		UpdateTriggerVolumes();
	}
	~Ring_Small() {}

private:

};