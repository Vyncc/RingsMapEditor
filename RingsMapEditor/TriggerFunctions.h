#pragma once
#include "Checkpoint.h"



class SetLocation : public TriggerFunction
{
public:
    SetLocation() {
        name = "Set Location";
        description = "Sets the actor's location";
    }

    ~SetLocation() {}

    void Execute(ActorWrapper actor) override {
        actor.SetLocation(location);
    }

    PARAMETERS_IMPL_VECTOR(SetLocation, location, "Location :");
    TriggerFunction_Clone(SetLocation);

private:
    Vector location;
};

class SetRotation : public TriggerFunction
{
public:
    SetRotation() {
        name = "Set Rotation";
        description = "Sets the actor's rotation";
    }

    ~SetRotation() {}

    void Execute(ActorWrapper actor) override {
        actor.SetRotation(rotation);
    }

    void RenderParameters() override {
        ImGui::Text("Rotation :");
        ImGui::SameLine();
        float inputFloatWidth = 70.f;
        ImGui::SetNextItemWidth(inputFloatWidth);
        ImGui::InputInt("Pitch", &rotation.Pitch, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFloatWidth);
        ImGui::InputInt("Yaw", &rotation.Yaw, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFloatWidth);
        ImGui::InputInt("Roll", &rotation.Roll, 0);
    }

    TriggerFunction_Clone(SetRotation);

private:
    Rotator rotation;
};

class Destroy : public TriggerFunction
{
public:
    Destroy() {
        name = "Destroy";
        description = "Destroy the actor";
    }

    ~Destroy() {}

    void Execute(ActorWrapper actor) override {
        AActor* aActor = reinterpret_cast<AActor*>(actor.memory_address);
        aActor->Destroy();
    }

    void RenderParameters() override {}

    TriggerFunction_Clone(Destroy);

private:
};

inline std::vector<std::shared_ptr<Checkpoint>> checkpoints;
inline std::shared_ptr<Checkpoint> currentCheckpoint = nullptr;

class TeleportToCheckpoint : public TriggerFunction
{
public:
    TeleportToCheckpoint() {
        name = "Teleport To Checkpoint";
        description = "Destroy the actor";
    }

    ~TeleportToCheckpoint() {}

    void Execute(ActorWrapper actor) override {
        if (useCurrentCheckpoint && currentCheckpoint)
        {
            actor.SetLocation(currentCheckpoint->GetVectorLocation());
        }
        else
        {
			if(checkpointId < 0 || checkpointId >= checkpoints.size())
			{
				LOG("[ERROR] Invalid checkpoint ID: {}", checkpointId);
				return;
			}

			std::shared_ptr<Checkpoint> checkpoint = checkpoints[checkpointId];
            actor.SetLocation(checkpoint->GetSpawnWorldLocation());
        }
    }

    void RenderParameters() override {
		ImGui::Checkbox("Use Current Checkpoint", &useCurrentCheckpoint);
		if (!useCurrentCheckpoint)
		{
			ImGui::Text("Checkpoint ID");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			ImGui::InputInt("##CheckpointID", &checkpointId);
		}
    }

    TriggerFunction_Clone(TeleportToCheckpoint);

private:
	bool useCurrentCheckpoint = false; // Use current checkpoint if true, otherwise use checkpointId
    int checkpointId = 0;
};