#pragma once
#include "TriggerVolume.h"

enum class CheckpointType : uint8_t
{
	Start = 0,
	Mid = 1,
	End = 2
};

inline std::map<CheckpointType, std::string> checkpointTypesMap = {
    { CheckpointType::Start, "Start" },
    { CheckpointType::Mid, "Mid" },
    { CheckpointType::End, "End" }
};

class Checkpoint : public Object
{
public:
    Checkpoint() {
		objectType = ObjectType::Checkpoint;
		name = "Checkpoint";
        location = Vector(0);
        rotation = Rotator(0);
        checkpointType = CheckpointType::Mid;
	}
	~Checkpoint() {}

	void Render(CanvasWrapper canvas, CameraWrapper camera) {
        triggerVolume.Render(canvas, camera);

        RT::Frustum frustum(canvas, camera);  // Create frustum (use default constructor or initialize as needed)

        //render spawn location
        canvas.SetColor(0, 255, 0, 255); // Green color for spawn location
		Vector spawnWorldLocation = GetSpawnWorldLocation();
		RT::Cone spawnCone(spawnWorldLocation, RotatorToVector(spawnRotation));
		spawnCone.radius = 10.0f;
		spawnCone.height = 40.0f;

        if (frustum.IsInFrustum(spawnWorldLocation))
        {
            spawnCone.Draw(canvas);
        }
	}

    void SetLocation(const Vector& _newLocation) override {
        location = _newLocation;
        triggerVolume.SetLocation(_newLocation);
    }

    void SetRotation(const Rotator& _newRotation) override {
        rotation = _newRotation;
        triggerVolume.SetRotation(_newRotation);
    }

    void SetSize(Vector _newSize) {
        triggerVolume.SetSize(_newSize);
    }

    Vector GetSpawnWorldLocation() const {
        return location + spawnLocation_offset;
    }

	bool IsStartCheckpoint() const {
		return checkpointType == CheckpointType::Start;
	}

	bool IsMidCheckpoint() const {
		return checkpointType == CheckpointType::Mid;
	}

	bool IsEndCheckpoint() const {
		return checkpointType == CheckpointType::End;
	}

    nlohmann::json to_json() const override {
        return {
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"id", id},
            {"checkpointType", static_cast<uint8_t>(checkpointType)},
            {"triggerVolume", triggerVolume.to_json()},
            {"spawnLocation_offset", spawnLocation_offset},
            {"spawnRotation", spawnRotation},
        };
    }

    std::shared_ptr<Object> Clone() override {
        std::shared_ptr<Checkpoint> clonedCheckpoint = std::make_shared<Checkpoint>(*this);
        clonedCheckpoint->triggerVolume.onTouchCallback = (clonedCheckpoint->triggerVolume.onTouchCallback ? clonedCheckpoint->triggerVolume.onTouchCallback->Clone() : nullptr);
        return clonedCheckpoint;
    }

	int id = 0;
	CheckpointType checkpointType = CheckpointType::Mid;
    TriggerVolume_Box triggerVolume;
	Vector spawnLocation_offset;
	Rotator spawnRotation;
};