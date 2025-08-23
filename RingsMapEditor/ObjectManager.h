#pragma once

#include "Ring.h"
#include "TriggerFunctions.h"


class ObjectManager
{
public:
	ObjectManager();
	~ObjectManager();

    void AddObject(ObjectType _objectType);
    void AddObject(const std::shared_ptr<Object>& _object);
    void AddMesh(const std::shared_ptr<Mesh>& _mesh);
    void AddTriggerVolume(const std::shared_ptr<TriggerVolume>& _triggerVolume);
    void AddCheckpoint(const std::shared_ptr<Checkpoint>& _checkpoint);
    void AddRing(const std::shared_ptr<Ring>& _ring);
    std::shared_ptr<Object> CopyObject(Object& _object);
    void RemoveObject(const int& _objectIndex);
    void ClearObjects();

    void ConvertTriggerVolume(std::shared_ptr<TriggerVolume> _triggerVolume, TriggerVolumeType _triggerVolumeType);

    std::vector<std::shared_ptr<Object>>& GetObjects();
    std::vector<std::shared_ptr<Mesh>>& GetMeshes();
    std::vector<std::shared_ptr<TriggerVolume>>& GetTriggerVolumes();
    std::vector<std::shared_ptr<Checkpoint>>& GetTriggerCheckpoints();
    std::vector<std::shared_ptr<Ring>>& GetRings();
    std::map<std::string, std::shared_ptr<TriggerFunction>>& GetTriggerFunctionsMap();

    std::vector<std::shared_ptr<Object>> m_objects;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<TriggerVolume>> m_triggerVolumes;
    std::map<std::string, std::shared_ptr<TriggerFunction>> m_triggerFunctionsMap = {
        { "Set Location", std::make_shared<SetLocation>()},
        { "Set Rotation", std::make_shared<SetRotation>() },
        { "Destroy", std::make_shared<Destroy>() },
        { "Teleport To Checkpoint", std::make_shared<TeleportToCheckpoint>() }
    };

    std::vector<std::shared_ptr<Ring>> m_rings;

private:
};