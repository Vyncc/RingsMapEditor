#pragma once
#include "Checkpoint.h"


#define PARAMETERS_IMPL_VECTOR(class_name, parameter_vector, parameter_label) \
void RenderParameters() override { \
    ImGui::Text(parameter_label); \
    ImGui::SameLine(); \
\
    float inputFloatWidth = 70.f; \
    ImGui::SetNextItemWidth(inputFloatWidth); \
    ImGui::InputFloat("X", &parameter_vector.X); \
    ImGui::SameLine(); \
    ImGui::SetNextItemWidth(inputFloatWidth); \
    ImGui::InputFloat("Y", &parameter_vector.Y); \
    ImGui::SameLine(); \
    ImGui::SetNextItemWidth(inputFloatWidth); \
    ImGui::InputFloat("Z", &parameter_vector.Z); \
} \

#define TriggerFunction_Clone(class_name) \
std::shared_ptr<TriggerFunction> Clone() override { \
    return std::make_shared<class_name>(*this); \
} \

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

    nlohmann::json to_json() const override {
        return nlohmann::json{
            {"name", name},
            {"description", description},
            {"location", location}
        };
    }

    PARAMETERS_IMPL_VECTOR(SetLocation, location, "Location :");
    TriggerFunction_Clone(SetLocation);

    std::shared_ptr<TriggerFunction> CloneFromJson(const nlohmann::json& j) override {
        std::shared_ptr<SetLocation> cloned = std::make_shared<SetLocation>(*this);
        cloned->location = j["location"].get<Vector>();
        return cloned;
    }

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

    nlohmann::json to_json() const {
        return nlohmann::json{
            {"name", name},
            {"description", description},
            {"rotation", rotation}
        };
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

    std::shared_ptr<TriggerFunction> CloneFromJson(const nlohmann::json& j) override {
        std::shared_ptr<SetRotation> cloned = std::make_shared<SetRotation>(*this);
        cloned->rotation = j["rotation"].get<Rotator>();
        return cloned;
    }

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

    nlohmann::json to_json() const override {
        return nlohmann::json{
            {"name", name},
            {"description", description}
        };
    }

    void RenderParameters() override {}

    TriggerFunction_Clone(Destroy);

    std::shared_ptr<TriggerFunction> CloneFromJson(const nlohmann::json& j) override {
        return std::make_shared<Destroy>(*this);
    }

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

    nlohmann::json to_json() const override {
        return nlohmann::json{
            {"name", name},
            {"description", description},
            {"useCurrentCheckpoint", useCurrentCheckpoint},
            {"checkpointId", checkpointId}
        };
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

    std::shared_ptr<TriggerFunction> CloneFromJson(const nlohmann::json& j) override {
        std::shared_ptr<TeleportToCheckpoint> cloned = std::make_shared<TeleportToCheckpoint>(*this);
        cloned->useCurrentCheckpoint = j["useCurrentCheckpoint"].get<bool>();
        cloned->checkpointId = j["checkpointId"].get<int>();
        return cloned;
    }

private:
	bool useCurrentCheckpoint = false; // Use current checkpoint if true, otherwise use checkpointId
    int checkpointId = 0;
};