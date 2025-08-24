#pragma once

#include "EditorSubMode.h"

enum class EditingPropertyType : uint8_t
{
    Rotation_Pitch = 0,
    Rotation_Yaw = 1,
    Rotation_Roll = 2,
    Scale = 3,
    TriggerVolume_Box_Size_X = 4,
    TriggerVolume_Box_Size_Y = 5,
    TriggerVolume_Box_Size_Z = 6,
    TriggerVolume_Cylinder_Radius = 7,
    TriggerVolume_Cylinder_Height = 8,
};

struct EditingProperty
{
    std::string name;
    std::function<void(float _deltaTime)> addFunction;
    std::function<void(float _deltaTime)> removeFunction;
    std::function<void()> resetFunction;
};

class BuildMode : public EditorSubMode
{
public:
    BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes);
    ~BuildMode();

    void Enable() override;
    void Disable() override;
    void OnTick(float _deltaTime) override;
    void RenderObjectsCanvas(CanvasWrapper _canvas) override;
    void RenderCanvas(CanvasWrapper _canvas) override;
    void PlaceObject() override;

    void RegisterCommands();
    void UnregisterCommands();
    void SetPreviewObjectType(ObjectType _objectType);
    void PreviousObjectType();
    void NextObjectType();
    void PreviousMesh();
    void NextMesh();
    void PreviousTriggerVolume();
    void NextTriggerVolume();
    void CycleEditingProperty();
    void ResetCurrentEditingProperty();
    int NormalizeUnrealRotation(int _rot);
    void RotatePreviewObjectAdd(const float& _pitch, const float& _yaw, const float& _roll);
    Vector CalculatePreviewActorLocation(CameraWrapper _camera);
    MeshInfos GetCurrentMesh();

    void OnRightShoulderPressed(float _deltaTime);
    void OnLeftShoulderPressed(float _deltaTime);

    void BuildObjectEditingProperties();
    void BuildEditingPropertiesFor(std::shared_ptr<std::vector<EditingProperty>>& _editingProperties);
    void BuildEditingProperties();

    EditingProperty GetCurrentEditingProperty();

private:
    ObjectType m_previewObjectType = ObjectType::Mesh;
    TriggerVolumeType m_triggerVolumeType = TriggerVolumeType::Box;
    std::vector<MeshInfos> m_availableMeshes;
    int m_availableMeshesIndex = 0;
    std::shared_ptr<Object> m_previewObject = nullptr;
    Rotator m_previewObjectRotation = Rotator(0, 0, 0);

    int* editingProperties_current_index = nullptr;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_current;

    std::shared_ptr<std::vector<EditingProperty>> editingProperties_object;

    int editingProperties_mesh_index = 0;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_mesh;

    int editingProperties_triggerVolumeBox_index = 0;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_triggerVolumeBox;

    int editingProperties_triggerVolumeCylinder_index = 0;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_triggerVolumeCylinder;

    int editingProperties_checkpoint_index = 0;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_checkpoint;

    int editingProperties_ring_index = 0;
    std::shared_ptr<std::vector<EditingProperty>> editingProperties_ring;
};