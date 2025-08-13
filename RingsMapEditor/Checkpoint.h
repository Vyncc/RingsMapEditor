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

class Checkpoint : public TriggerVolume
{
public:
    Checkpoint() : TriggerVolume() {
		objectType = ObjectType::Checkpoint;
		name = "Checkpoint";
	}
	~Checkpoint() {}

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

    Vector GetSpawnWorldLocation() const {
        return Vector{ location.X + spawnLocation.X, location.Y + spawnLocation.Y, location.Z + spawnLocation.Z };
    }

	bool IsStartCheckpoint() const {
		return type == CheckpointType::Start;
	}

	bool IsMidCheckpoint() const {
		return type == CheckpointType::Mid;
	}

	bool IsEndCheckpoint() const {
		return type == CheckpointType::End;
	}

    nlohmann::json to_json() const override {
        nlohmann::json triggerVolumeJson{
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"size", size},
            {"vertices", vertices},
            {"id", id},
            {"type", static_cast<uint8_t>(type)},
            {"spawnLocation", spawnLocation},
            {"spawnRotation", spawnRotation},
        };

        if (onTouchCallback)
            triggerVolumeJson["onTouchCallback"] = onTouchCallback->to_json();
        else
            triggerVolumeJson["onTouchCallback"] = "null";

        return triggerVolumeJson;
    }

	int id = 0;
	CheckpointType type = CheckpointType::Mid;
	Vector spawnLocation;
	Rotator spawnRotation;
};