#pragma once

#include "EditorSubMode.h"

class BuildMode : public EditorSubMode
{
public:
    BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes);
    ~BuildMode();

public:
    void Enable() override;
    void Disable() override;
    void RegisterCommands() override;
    void UnregisterCommands() override;
    void OnTick(float _deltaTime) override;
    void RenderCanvas(CanvasWrapper _canvas) override;
    void PlaceObject() override;

public:
    void RenderObjectsCanvas(CanvasWrapper _canvas);
    void SetPreviewObjectType(ObjectType _objectType);
    void PreviousObjectType();
    void NextObjectType();
    void PreviousMesh();
    void NextMesh();
    void PreviousTriggerVolume();
    void NextTriggerVolume();
	void SetPreviewObjectTriggerVolumeType(const TriggerVolumeType& _triggerVolumeType);
    MeshInfos GetCurrentMesh();

private:
    ObjectType m_previewObjectType = ObjectType::Mesh;
    TriggerVolumeType m_previewObject_triggerVolumeType = TriggerVolumeType::Box;
    std::vector<MeshInfos> m_availableMeshes;
    int m_availableMeshesIndex = 0;
};