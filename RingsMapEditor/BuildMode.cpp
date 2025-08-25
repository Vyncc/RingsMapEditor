#include "pch.h"
#include "BuildMode.h"



BuildMode::BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes) : EditorSubMode(_objectManager)
{
    m_enabled = false;
    m_availableMeshes = _availableMeshes;
    m_availableMeshesIndex = 0;
    m_previewObjectType = ObjectType::Mesh;
    m_previewObjectRotation = Rotator(0, 0, 0);

    BuildEditingProperties();
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

void BuildMode::OnTick(float _deltaTime)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

    if (m_previewObject)
    {
        //if R1 pressed
        if (_globalGameWrapper->IsKeyPressed(rightShoulder))
        {
            OnRightShoulderPressed(_deltaTime);
        }

        //if L1 pressed
        if (_globalGameWrapper->IsKeyPressed(leftShoulder))
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

void BuildMode::RenderCanvas(CanvasWrapper _canvas)
{
    if (IsEnabled() && IsSpectator())
    {
        RenderObjectsCanvas(_canvas);

        _canvas.SetColor(255, 255, 255, 255);

        float stringScale = 2.f;
        float newLinePaddingY = 15.f * stringScale;
        Vector2 pos = Vector2{ 20, 80 };

        _canvas.SetPosition(pos);
        _canvas.DrawString("Build Mode", stringScale, stringScale);

        auto newLine = [&]() {
            pos.Y += newLinePaddingY;
            _canvas.SetPosition(pos);
            };

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

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_rotate_add", [&](std::vector<std::string> args) {
        RotatePreviewObjectAdd(0.f, 60.f, 0.f);
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_rotate_remove", [&](std::vector<std::string> args) {
        RotatePreviewObjectAdd(0.f, -60.f, 0.f);
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_editing_property_cycle", [&](std::vector<std::string> args) {
        CycleEditingProperty();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_editing_property_reset", [&](std::vector<std::string> args) {
        ResetCurrentEditingProperty();
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
}

void BuildMode::SetPreviewObjectType(ObjectType _objectType)
{
    if (_objectType == ObjectType::Mesh)
    {
        m_previewObject = std::make_shared<Mesh>(GetCurrentMesh());
        std::static_pointer_cast<Mesh>(m_previewObject)->SpawnInstance();
        SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_MESH);
        LOG("Set preview object type to Mesh");
    }
    else if (_objectType == ObjectType::TriggerVolume)
    {
        if (m_triggerVolumeType == TriggerVolumeType::Box)
        {
            m_previewObject = std::make_shared<TriggerVolume_Box>();
            SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX);
            LOG("Set preview object type to TriggerVolume Box");
        }
        else if(m_triggerVolumeType == TriggerVolumeType::Cylinder)
        {
            m_previewObject = std::make_shared<TriggerVolume_Cylinder>();
            SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER);
            LOG("Set preview object type to TriggerVolume Cylinder");
        }
    }
    else if (_objectType == ObjectType::Checkpoint)
    {
        m_previewObject = std::make_shared<Checkpoint>();
		SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_CHECKPOINT);
        LOG("Set preview object type to Checkpoint");
    }
    else if (_objectType == ObjectType::Ring)
    {
        m_previewObject = std::make_shared<Ring_Small>(m_objectManager->GetRings().size());
        std::static_pointer_cast<Ring>(m_previewObject)->mesh.SpawnInstance();
        SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_RING);
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

    uint8_t value = static_cast<std::uint8_t>(m_triggerVolumeType);
    value--;

    if (value < static_cast<std::uint8_t>(TriggerVolumeType::Box))
        return;

    SetPreviewObjectTriggerVolumeType(static_cast<TriggerVolumeType>(value));
}

void BuildMode::NextTriggerVolume()
{
    if (m_previewObjectType != ObjectType::TriggerVolume) return;

    uint8_t value = static_cast<std::uint8_t>(m_triggerVolumeType);
    value++;

    if (value > static_cast<std::uint8_t>(TriggerVolumeType::Cylinder))
        return;

    SetPreviewObjectTriggerVolumeType(static_cast<TriggerVolumeType>(value));
}

void BuildMode::SetPreviewObjectTriggerVolumeType(const TriggerVolumeType& _triggerVolumeType)
{
    if (m_triggerVolumeType == TriggerVolumeType::Box)
    {
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX);
    }
    else if (m_triggerVolumeType == TriggerVolumeType::Cylinder)
    {
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
		SetCurrentObjectEditingPropertiesName(OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER);
    }
    else
    {
        LOG("[ERROR]Couldn't set trigger volume, invalid trigger volume type : {}", static_cast<uint8_t>(_triggerVolumeType));
        return;
    }

	m_triggerVolumeType = _triggerVolumeType;
}

void BuildMode::CycleEditingProperty()
{
    int value = GetCurrentObjectEditingProperties().GetIndex();
    value++;

    if (value > GetCurrentObjectEditingProperties().GetPropertiesCount() - 1)
        value = 0;

    GetCurrentObjectEditingProperties().SetIndex(value);
}

void BuildMode::ResetCurrentEditingProperty()
{
    GetCurrentEditingProperty().resetFunction();
}

int BuildMode::NormalizeUnrealRotation(int _rot)
{
    // Wrap to [0, 65536)
    _rot = _rot % 65536;
    if (_rot < 0)
        _rot += 65536;

    // Shift to [-32768, 32767]
    if (_rot > 32767)
        _rot -= 65536;

    return _rot;
}

void BuildMode::RotatePreviewObjectAdd(const float& _pitch, const float& _yaw, const float& _roll)
{
    if (m_previewObject)
    {
        int rotationToAdd_pitch = static_cast<int>(_pitch * 182.044449f);
        int rotationToAdd_yaw = static_cast<int>(_yaw * 182.044449f);
        int rotationToAdd_roll = static_cast<int>(_roll * 182.044449f);

        LOG("p {} | y {} | r {}", rotationToAdd_pitch, rotationToAdd_yaw, rotationToAdd_roll);

        m_previewObjectRotation.Pitch = NormalizeUnrealRotation(m_previewObjectRotation.Pitch + rotationToAdd_pitch);
        m_previewObjectRotation.Yaw = NormalizeUnrealRotation(m_previewObjectRotation.Yaw + rotationToAdd_yaw);
        m_previewObjectRotation.Roll = NormalizeUnrealRotation(m_previewObjectRotation.Roll + rotationToAdd_roll);
    }
}

Vector BuildMode::CalculatePreviewActorLocation(CameraWrapper _camera)
{
    return _camera.GetLocation() + RotateVectorWithQuat({ m_previewObjectDistance, 0, 0 }, RotatorToQuat(_camera.GetRotation()));
}

MeshInfos BuildMode::GetCurrentMesh()
{
    return m_availableMeshes[m_availableMeshesIndex];
}

void BuildMode::OnRightShoulderPressed(float _deltaTime)
{
    GetCurrentEditingProperty().addFunction(_deltaTime);
}

void BuildMode::OnLeftShoulderPressed(float _deltaTime)
{
    GetCurrentEditingProperty().removeFunction(_deltaTime);
}
void BuildMode::BuildEditingProperties()
{
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_OBJECT] = ObjectEditingProperties();
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_OBJECT].AddProperty(m_editingPropertyMap["Object Rotation Pitch"]);
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_OBJECT].AddProperty(m_editingPropertyMap["Object Rotation Yaw"]);
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_OBJECT].AddProperty(m_editingPropertyMap["Object Rotation Roll"]);

    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_MESH] = ObjectEditingProperties();
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_MESH].SetProperties(m_objectsEditingProperties["Object"].GetProperties()); //copy object properties
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_MESH].AddProperty(m_editingPropertyMap["Mesh Scale"]);

	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX] = ObjectEditingProperties();
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX].SetProperties(m_objectsEditingProperties["Object"].GetProperties()); //copy object properties
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX].AddProperty(m_editingPropertyMap["TriggerVolumeBox Size X"]);
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX].AddProperty(m_editingPropertyMap["TriggerVolumeBox Size Y"]);
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX].AddProperty(m_editingPropertyMap["TriggerVolumeBox Size Z"]);

	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER] = ObjectEditingProperties();
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER].SetProperties(m_objectsEditingProperties["Object"].GetProperties()); //copy object properties
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER].AddProperty(m_editingPropertyMap["TriggerVolumeCylinder Radius"]);
	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER].AddProperty(m_editingPropertyMap["TriggerVolumeCylinder Height"]);

	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_CHECKPOINT] = ObjectEditingProperties();
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_CHECKPOINT].SetProperties(m_objectsEditingProperties["Object"].GetProperties()); //copy object properties

	m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_RING] = ObjectEditingProperties();
    m_objectsEditingProperties[OBJECT_EDITING_PROPERTIES_RING].SetProperties(m_objectsEditingProperties["Object"].GetProperties()); //copy object properties
}

void BuildMode::SetCurrentObjectEditingPropertiesName(const std::string& _name)
{
    m_currentObjectEditingPropertiesName = _name;
}

ObjectEditingProperties& BuildMode::GetCurrentObjectEditingProperties()
{
    return m_objectsEditingProperties[m_currentObjectEditingPropertiesName];
}

EditingProperty& BuildMode::GetCurrentEditingProperty()
{
	return GetCurrentObjectEditingProperties().GetCurrentEditingProperty();
}
