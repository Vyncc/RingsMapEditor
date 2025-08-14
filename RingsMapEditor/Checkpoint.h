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
        location = FVector{ 0.f, 0.f, 0.f };
        rotation = FRotator{ 0, 0, 0 };
        checkpointType = CheckpointType::Mid;
	}
	~Checkpoint() {}

	void Render(CanvasWrapper canvas, CameraWrapper camera) {
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
            Vector start = Vector{ triggerVolume.vertices[edge[0]].X, triggerVolume.vertices[edge[0]].Y, triggerVolume.vertices[edge[0]].Z };
            Vector end = Vector{ triggerVolume.vertices[edge[1]].X, triggerVolume.vertices[edge[1]].Y, triggerVolume.vertices[edge[1]].Z };
            RT::Line line(start, end);
            line.DrawWithinFrustum(canvas, frustum);
        }

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

    void SetLocation(FVector _newLocation) {
        location = _newLocation;
        triggerVolume.SetLocation(_newLocation);
    }

    void SetRotation(FRotator _newRotation) {
        rotation = _newRotation;
        triggerVolume.SetRotation(_newRotation);
    }

    void SetSize(FVector _newSize) {
        triggerVolume.SetSize(_newSize);
    }

    Vector GetSpawnWorldLocation() const {
        return Vector{ location.X + spawnLocation.X, location.Y + spawnLocation.Y, location.Z + spawnLocation.Z };
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
            {"spawnLocation", spawnLocation},
            {"spawnRotation", spawnRotation},
        };
    }

    std::shared_ptr<Object> Clone() override {
        std::shared_ptr<Checkpoint> clonedCheckpoint = std::make_shared<Checkpoint>(*this);
        clonedCheckpoint->triggerVolume.onTouchCallback = (clonedCheckpoint->triggerVolume.onTouchCallback ? clonedCheckpoint->triggerVolume.onTouchCallback->Clone() : nullptr);
        clonedCheckpoint->triggerVolume.UpdateVertices(); // Ensure vertices are updated after cloning
        return clonedCheckpoint;
    }

	int id = 0;
	CheckpointType checkpointType = CheckpointType::Mid;
    TriggerVolume_Box triggerVolume;
	Vector spawnLocation;
	Rotator spawnRotation;
};