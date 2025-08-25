#include "pch.h"
#include "BuildMode.h"



BuildMode::BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes) : EditorSubMode(_objectManager)
{
    m_enabled = false;
    m_availableMeshes = _availableMeshes;
    m_availableMeshesIndex = 0;
    m_previewObjectType = ObjectType::Mesh;
}

BuildMode::~BuildMode()
{

}



void BuildMode::Enable()
{
    if (!IsInGame())
    {
        LOG("[ERROR]You need to be in a game to enable Build Mode!");
        return;
    }

    Spectate();
    SwitchToFlyCam();
    RegisterCommands();
    HookEvents();
    SetPreviewObjectType(m_previewObjectType);

    m_enabled = true;
    LOG("Build mode activated");
}

void BuildMode::Disable()
{
    m_previewObject = nullptr;
    UnregisterCommands();
    UnhookEvents();
    m_enabled = false;
    LOG("Build mode disabled");
}

void BuildMode::RegisterCommands()
{
    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_place_object", [&](std::vector<std::string> args) {
        PlaceObject();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_previous_object_type", [&](std::vector<std::string> args) {
        PreviousObjectType();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_next_object_type", [&](std::vector<std::string> args) {
        NextObjectType();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_previous_mesh", [&](std::vector<std::string> args) {
        PreviousMesh();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_next_mesh", [&](std::vector<std::string> args) {
        NextMesh();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_previous_triggervolume", [&](std::vector<std::string> args) {
        PreviousTriggerVolume();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_next_triggervolume", [&](std::vector<std::string> args) {
        NextTriggerVolume();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_editing_property_reset", [&](std::vector<std::string> args) {
        ResetCurrentEditingProperty();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_editing_property_cycle", [&](std::vector<std::string> args) {
        CycleEditingProperty();
        }, "", 0);
}

void BuildMode::UnregisterCommands()
{
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_place_object");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_previous_object_type");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_next_object_type");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_previous_mesh");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_next_mesh");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_previous_triggervolume");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_next_triggervolume");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_editing_property_reset");
    _globalCvarManager->removeNotifier("ringsmapeditor_buildmode_editing_property_cycle");
}

void BuildMode::OnTick(float _deltaTime)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

    if (m_previewObject)
    {
        //if R1 pressed
        if (_globalGameWrapper->IsKeyPressed(m_fnameIndex_rightShoulder))
        {
            OnRightShoulderPressed(_deltaTime);
        }

        //if L1 pressed
        if (_globalGameWrapper->IsKeyPressed(m_fnameIndex_leftShoulder))
        {
            OnLeftShoulderPressed(_deltaTime);
        }

        CameraWrapper camera = _globalGameWrapper->GetCamera();
        if (!camera)
        {
            LOG("[ERROR]camera is NULL!");
            return;
        }

        m_previewObject->SetLocation(CalculatePreviewActorLocation(camera));
        m_previewObject->SetRotation(m_previewObjectRotation);
    }
}

void BuildMode::RenderCanvas(CanvasWrapper _canvas)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

    RenderObjectsCanvas(_canvas);

    _canvas.SetColor(255, 255, 255, 255);

    float stringScale = 2.f;
    float newLinePaddingY = 15.f * stringScale;
    Vector2 pos = Vector2{ 20, 80 };

    auto newLine = [&]() {
        pos.Y += newLinePaddingY;
        _canvas.SetPosition(pos);
        };

    _canvas.SetPosition(pos);
    _canvas.DrawString("Build Mode", stringScale, stringScale);
    newLine();

    if (m_previewObject)
    {
        if (m_previewObject->objectType == ObjectType::Mesh)
        {
            newLine();
            _canvas.DrawString("Mesh", stringScale, stringScale);

            newLine();
            newLine();
            std::string meshIndexText = "Mesh Index : " + std::to_string(m_availableMeshesIndex);
            _canvas.DrawString(meshIndexText, stringScale, stringScale);

            newLine();
            std::string meshText = "Mesh : " + GetCurrentMesh().name;
            _canvas.DrawString(meshText, stringScale, stringScale);
        }
        else if (m_previewObject->objectType == ObjectType::TriggerVolume)
        {
            newLine();
            _canvas.DrawString("Trigger Volume", stringScale, stringScale);

            newLine();
            newLine();
            std::string triggerVolumeTypeText = "Trigger Volume Type : ";
            if (std::static_pointer_cast<TriggerVolume>(m_previewObject)->triggerVolumeType == TriggerVolumeType::Box)
                triggerVolumeTypeText += "Box";
            else if (std::static_pointer_cast<TriggerVolume>(m_previewObject)->triggerVolumeType == TriggerVolumeType::Cylinder)
                triggerVolumeTypeText += "Cylinder";
            else
                triggerVolumeTypeText += "Unknown";

            _canvas.DrawString(triggerVolumeTypeText, stringScale, stringScale);
        }
        else if (m_previewObject->objectType == ObjectType::Checkpoint)
        {
            newLine();
            _canvas.DrawString("Checkpoint", stringScale, stringScale);
        }
        else if (m_previewObject->objectType == ObjectType::Ring)
        {
            newLine();
            _canvas.DrawString("Ring", stringScale, stringScale);
        }

        newLine();
        newLine();
        newLine();
        std::string editingText = "Editing : ";
        editingText += GetCurrentEditingProperty().name;
        _canvas.DrawString(editingText, stringScale, stringScale);
    }
}

void BuildMode::PlaceObject()
{
    m_objectManager->AddObject(m_previewObject);
    LOG("Placed object : {}", m_previewObject->name);
    m_previewObject = m_previewObject->Clone();
}

void BuildMode::RenderObjectsCanvas(CanvasWrapper _canvas)
{
    CameraWrapper camera = _globalGameWrapper->GetCamera();
    if (!camera)
    {
        LOG("[ERROR]camera is NULL!");
        return;
    }

    if (m_previewObject->objectType == ObjectType::TriggerVolume)
        std::static_pointer_cast<TriggerVolume>(m_previewObject)->Render(_canvas, camera);
    else if (m_previewObject->objectType == ObjectType::Checkpoint)
        std::static_pointer_cast<Checkpoint>(m_previewObject)->Render(_canvas, camera);
    else if (m_previewObject->objectType == ObjectType::Ring)
        std::static_pointer_cast<Ring>(m_previewObject)->RenderTriggerVolumes(_canvas, camera);
}

void BuildMode::SetPreviewObjectType(ObjectType _objectType)
{
    if (_objectType == ObjectType::Mesh)
    {
        m_previewObject = std::make_shared<Mesh>(GetCurrentMesh());
        std::static_pointer_cast<Mesh>(m_previewObject)->SpawnInstance();
        SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_MESH);
        LOG("Set preview object type to Mesh");
    }
    else if (_objectType == ObjectType::TriggerVolume)
    {
        if (m_previewObject_triggerVolumeType == TriggerVolumeType::Box)
        {
            m_previewObject = std::make_shared<TriggerVolume_Box>();
            SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX);
            LOG("Set preview object type to TriggerVolume Box");
        }
        else if(m_previewObject_triggerVolumeType == TriggerVolumeType::Cylinder)
        {
            m_previewObject = std::make_shared<TriggerVolume_Cylinder>();
            SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER);
            LOG("Set preview object type to TriggerVolume Cylinder");
        }
    }
    else if (_objectType == ObjectType::Checkpoint)
    {
        m_previewObject = std::make_shared<Checkpoint>();
		SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_CHECKPOINT);
        LOG("Set preview object type to Checkpoint");
    }
    else if (_objectType == ObjectType::Ring)
    {
        m_previewObject = std::make_shared<Ring_Small>(m_objectManager->GetRings().size());
        std::static_pointer_cast<Ring>(m_previewObject)->mesh.SpawnInstance();
        SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_RING);
        LOG("Set preview object type to Ring");
    }
    else
    {
        LOG("[ERROR]Couldn't spawn preview object, invalid object type : {}", static_cast<uint8_t>(_objectType));
        return;
    }

    m_previewObjectType = _objectType;
}

void BuildMode::PreviousObjectType()
{
    uint8_t value = static_cast<std::uint8_t>(m_previewObjectType);
    value--;

    if (value < static_cast<std::uint8_t>(ObjectType::Mesh))
        return;

    m_previewObjectType = static_cast<ObjectType>(value);
    SetPreviewObjectType(m_previewObjectType);
}

void BuildMode::NextObjectType()
{
    uint8_t value = static_cast<std::uint8_t>(m_previewObjectType);
    value++;

    if (value > static_cast<std::uint8_t>(ObjectType::Ring))
        return;

    m_previewObjectType = static_cast<ObjectType>(value);
    SetPreviewObjectType(m_previewObjectType);
}

void BuildMode::PreviousMesh()
{
    if (m_previewObjectType != ObjectType::Mesh) return;

    if (m_availableMeshesIndex <= 0)
    {
        LOG("[ERROR]No more meshes!");
        return;
    }

    m_availableMeshesIndex--;
    MeshInfos meshInfos = GetCurrentMesh();
    LOG("Setting mesh : {} | {}", meshInfos.name, meshInfos.meshPath);
    m_previewObject->name = "Mesh " + meshInfos.name;
    std::static_pointer_cast<Mesh>(m_previewObject)->SetStaticMesh(meshInfos);
}

void BuildMode::NextMesh()
{
    if (m_previewObjectType != ObjectType::Mesh) return;

    if (m_availableMeshesIndex >= m_availableMeshes.size() - 1)
    {
        LOG("[ERROR]No more meshes!");
        return;
    }

    m_availableMeshesIndex++;
    MeshInfos meshInfos = GetCurrentMesh();
    LOG("Setting mesh : {} | {}", meshInfos.name, meshInfos.meshPath);
    m_previewObject->name = "Mesh " + meshInfos.name;
    std::static_pointer_cast<Mesh>(m_previewObject)->SetStaticMesh(meshInfos);
}

void BuildMode::PreviousTriggerVolume()
{
    if (m_previewObjectType != ObjectType::TriggerVolume) return;

    uint8_t value = static_cast<std::uint8_t>(m_previewObject_triggerVolumeType);
    value--;

    if (value < static_cast<std::uint8_t>(TriggerVolumeType::Box))
        return;

    SetPreviewObjectTriggerVolumeType(static_cast<TriggerVolumeType>(value));
}

void BuildMode::NextTriggerVolume()
{
    if (m_previewObjectType != ObjectType::TriggerVolume) return;

    uint8_t value = static_cast<std::uint8_t>(m_previewObject_triggerVolumeType);
    value++;

    if (value > static_cast<std::uint8_t>(TriggerVolumeType::Cylinder))
        return;

    SetPreviewObjectTriggerVolumeType(static_cast<TriggerVolumeType>(value));
}

void BuildMode::SetPreviewObjectTriggerVolumeType(const TriggerVolumeType& _triggerVolumeType)
{
    if (m_previewObject_triggerVolumeType == TriggerVolumeType::Box)
    {
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX);
    }
    else if (m_previewObject_triggerVolumeType == TriggerVolumeType::Cylinder)
    {
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
		SetCurrentObjectEditingProperties(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER);
    }
    else
    {
        LOG("[ERROR]Couldn't set trigger volume, invalid trigger volume type : {}", static_cast<uint8_t>(_triggerVolumeType));
        return;
    }

    m_previewObject_triggerVolumeType = _triggerVolumeType;
}

MeshInfos BuildMode::GetCurrentMesh()
{
    return m_availableMeshes[m_availableMeshesIndex];
}