#pragma once

#include "ObjectManager.h"

enum class EditingProperty : uint8_t
{
    Rotation_Pitch = 0,
    Rotation_Yaw = 1,
    Rotation_Roll = 2,
};

class BuildMode
{
public:
    BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes);
    ~BuildMode();

    bool IsEnabled();
    void ToggleBuildMode();
    void EnableBuildMode();
    void DisableBuildMode();
    void RegisterCommands();
    void UnregisterCommands();
    void HookEvents();
    void UnhookEvents();
    void Spectate();
    void SwitchToFlyCam();
    void SetPreviewObjectType(ObjectType _objectType);
    void PreviousObjectType();
    void NextObjectType();
    void PreviousMesh();
    void NextMesh();
    void PreviousTriggerVolume();
    void NextTriggerVolume();
    void CycleEditingProperty();
    void PlaceObject();
    int NormalizeUnrealRotation(int _rot);
    void RotatePreviewObjectAdd(const float& _pitch, const float& _yaw, const float& _roll);
    Vector CalculatePreviewActorLocation(CameraWrapper _camera);
    bool IsInGame() const;
    MeshInfos GetCurrentMesh();
    bool IsSpectator() const;
    void OnTick(float _deltaTime);
    void RenderObjectsCanvas(CanvasWrapper _canvas);
    void RenderCanvas(CanvasWrapper _canvas);

private:
    bool m_enabled = false;
    std::shared_ptr<ObjectManager> m_objectManager = nullptr;
    ObjectType m_previewObjectType = ObjectType::Mesh;
    TriggerVolumeType m_triggerVolumeType = TriggerVolumeType::Box;
    std::vector<MeshInfos> m_availableMeshes;
    int m_meshIndex = 0;
    std::shared_ptr<Object> m_previewObject = nullptr;
    Rotator m_previewObjectRotation = Rotator(0, 0, 0);

    float m_previewObjectDistance = 1400.f;
    float m_roationDegreesPerSeconds = 90.f;

    EditingProperty m_editingProperty = EditingProperty::Rotation_Yaw;

    FName fname_flyCam;
    int rightShoulder = 0;
    int leftShoulder = 0;
};