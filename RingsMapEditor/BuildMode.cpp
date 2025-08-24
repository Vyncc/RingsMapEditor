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
        LOG("Set preview object type to Mesh");

        editingProperties_current_index = &editingProperties_mesh_index;
        editingProperties_current = editingProperties_mesh;
    }
    else if (_objectType == ObjectType::TriggerVolume)
    {
        if (m_triggerVolumeType == TriggerVolumeType::Box)
        {
            m_previewObject = std::make_shared<TriggerVolume_Box>();
            LOG("Set preview object type to TriggerVolume Box");

            editingProperties_current_index = &editingProperties_triggerVolumeBox_index;
            editingProperties_current = editingProperties_triggerVolumeBox;
        }
        else if(m_triggerVolumeType == TriggerVolumeType::Cylinder)
        {
            m_previewObject = std::make_shared<TriggerVolume_Cylinder>();
            LOG("Set preview object type to TriggerVolume Cylinder");

            editingProperties_current_index = &editingProperties_triggerVolumeCylinder_index;
            editingProperties_current = editingProperties_triggerVolumeCylinder;
        }
    }
    else if (_objectType == ObjectType::Checkpoint)
    {
        m_previewObject = std::make_shared<Checkpoint>();
        LOG("Set preview object type to Checkpoint");

        editingProperties_current_index = &editingProperties_checkpoint_index;
        editingProperties_current = editingProperties_checkpoint;
    }
    else if (_objectType == ObjectType::Ring)
    {
        m_previewObject = std::make_shared<Ring_Small>(m_objectManager->GetRings().size());
        std::static_pointer_cast<Ring>(m_previewObject)->mesh.SpawnInstance();
        LOG("Set preview object type to Ring");

        editingProperties_current_index = &editingProperties_ring_index;
        editingProperties_current = editingProperties_ring;
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

    m_triggerVolumeType = static_cast<TriggerVolumeType>(value);

    if (m_triggerVolumeType == TriggerVolumeType::Box)
    {
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        editingProperties_current_index = &editingProperties_triggerVolumeBox_index;
        editingProperties_current = editingProperties_triggerVolumeBox;
    }
    else if (m_triggerVolumeType == TriggerVolumeType::Cylinder)
    {
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        editingProperties_current_index = &editingProperties_triggerVolumeCylinder_index;
        editingProperties_current = editingProperties_triggerVolumeCylinder;
    }
}

void BuildMode::NextTriggerVolume()
{
    if (m_previewObjectType != ObjectType::TriggerVolume) return;

    uint8_t value = static_cast<std::uint8_t>(m_triggerVolumeType);
    value++;

    if (value > static_cast<std::uint8_t>(TriggerVolumeType::Cylinder))
        return;

    m_triggerVolumeType = static_cast<TriggerVolumeType>(value);

    if (m_triggerVolumeType == TriggerVolumeType::Box)
    {
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        editingProperties_current_index = &editingProperties_triggerVolumeBox_index;
        editingProperties_current = editingProperties_triggerVolumeBox;
    }
    else if (m_triggerVolumeType == TriggerVolumeType::Cylinder)
    {
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
        editingProperties_current_index = &editingProperties_triggerVolumeCylinder_index;
        editingProperties_current = editingProperties_triggerVolumeCylinder;
    }
}

void BuildMode::CycleEditingProperty()
{
    int value = *editingProperties_current_index;
    value++;

    if (value > editingProperties_current->size() - 1)
        value = 0;

    *editingProperties_current_index = value;
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

void BuildMode::BuildObjectEditingProperties()
{
    editingProperties_object = std::make_shared<std::vector<EditingProperty>>();

    editingProperties_object->push_back(
        {
            "Rotation Pitch",
            [this](float _deltaTime) {
                float degreesToAdd = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(degreesToAdd, 0.f, 0.f);
            },
            [this](float _deltaTime) {
                float degreesToRemove = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(-degreesToRemove, 0.f, 0.f);
            },
            [this]() {
                m_previewObjectRotation.Pitch = 0;
            }
        }
    );
    editingProperties_object->push_back(
        {
            "Rotation Yaw",
            [this](float _deltaTime) {
                float degreesToAdd = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(0.f, degreesToAdd, 0.f);
            },
            [this](float _deltaTime) {
                float degreesToRemove = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(0.f, -degreesToRemove, 0.f);
            },
            [this]() {
                m_previewObjectRotation.Yaw = 0;
            },
        }
    );
    editingProperties_object->push_back(
        {
            "Rotation Roll",
            [this](float _deltaTime) {
                float degreesToAdd = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(0.f, 0.f, degreesToAdd);
            },
            [this](float _deltaTime) {
                float degreesToRemove = m_rotationDegreesPerSec * _deltaTime;
                RotatePreviewObjectAdd(0.f, 0.f, -degreesToRemove);
            },
            [this]() {
                m_previewObjectRotation.Roll = 0;
            },
        }
    );
}

void BuildMode::BuildEditingPropertiesFor(std::shared_ptr<std::vector<EditingProperty>>& _editingProperties)
{
    _editingProperties = std::make_shared<std::vector<EditingProperty>>();
    for (auto& editingProperty : *editingProperties_object)
    {
        _editingProperties->emplace_back(editingProperty);
    }
}

void BuildMode::BuildEditingProperties()
{
    BuildObjectEditingProperties();

    //Add common Object editing properties
    BuildEditingPropertiesFor(editingProperties_mesh);
    BuildEditingPropertiesFor(editingProperties_triggerVolumeBox);
    BuildEditingPropertiesFor(editingProperties_triggerVolumeCylinder);
    BuildEditingPropertiesFor(editingProperties_checkpoint);
    BuildEditingPropertiesFor(editingProperties_ring);

    //Add special editing properties for each object type
    //Mesh
    editingProperties_mesh->push_back(
        {
            "Scale",
            [this](float _deltaTime) {
                std::shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(m_previewObject);
                float scaleToAdd = m_scalePerSec * _deltaTime;
                float newScale = mesh->scale + scaleToAdd;
                mesh->SetScale3D(Vector(newScale, newScale, newScale));
            },
            [this](float _deltaTime) {
                std::shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(m_previewObject);
                float scaleToRemove = m_scalePerSec * _deltaTime;
                float newScale = mesh->scale - scaleToRemove;
                mesh->SetScale3D(Vector(newScale, newScale, newScale));
            },
            [this]() {
                std::static_pointer_cast<Mesh>(m_previewObject)->SetScale3D(Vector(1.f, 1.f, 1.f));
            }
        }
    );

    //TriggerVolume Box
    editingProperties_triggerVolumeBox->push_back(
        {
            "Size X",
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToAdd = m_sizeUnitsPerSec * _deltaTime;
                float newSizeX = triggerVolumeBox->size.X + sizeToAdd;
                triggerVolumeBox->SetSizeX(newSizeX);
            },
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToRemove = m_sizeUnitsPerSec * _deltaTime;
                float newSizeX = triggerVolumeBox->size.X - sizeToRemove;
                triggerVolumeBox->SetSizeX(newSizeX);
            },
            [this]() {
                std::static_pointer_cast<TriggerVolume_Box>(m_previewObject)->SetSizeX(200.f);
            }
        }
    );
    editingProperties_triggerVolumeBox->push_back(
        {
            "Size Y",
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToAdd = m_sizeUnitsPerSec * _deltaTime;
                float newSizeY = triggerVolumeBox->size.Y + sizeToAdd;
                triggerVolumeBox->SetSizeY(newSizeY);
            },
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToRemove = m_sizeUnitsPerSec * _deltaTime;
                float newSizeY = triggerVolumeBox->size.Y - sizeToRemove;
                triggerVolumeBox->SetSizeY(newSizeY);
            },
            [this]() {
                std::static_pointer_cast<TriggerVolume_Box>(m_previewObject)->SetSizeY(200.f);
            }
        }
    );
    editingProperties_triggerVolumeBox->push_back(
        {
            "Size Z",
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToAdd = m_sizeUnitsPerSec * _deltaTime;
                float newSizeZ = triggerVolumeBox->size.Z + sizeToAdd;
                triggerVolumeBox->SetSizeZ(newSizeZ);
            },
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::static_pointer_cast<TriggerVolume_Box>(m_previewObject);
                float sizeToRemove = m_sizeUnitsPerSec * _deltaTime;
                float newSizeZ = triggerVolumeBox->size.Z - sizeToRemove;
                triggerVolumeBox->SetSizeZ(newSizeZ);
            },
            [this]() {
                std::static_pointer_cast<TriggerVolume_Box>(m_previewObject)->SetSizeZ(200.f);
            }
        }
    );

    //triggerVolume Cylinder
    editingProperties_triggerVolumeCylinder->push_back(
        {
            "Radius",
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Cylinder> triggerVolumeCylinder = std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject);
                float radiusToAdd = m_radiusUnitsPerSec * _deltaTime;
                triggerVolumeCylinder->radius += radiusToAdd;
            },
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Cylinder> triggerVolumeCylinder = std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject);
                float radiusToRemove = m_radiusUnitsPerSec * _deltaTime;
                triggerVolumeCylinder->radius -= radiusToRemove;
            },
            [this]() {
                std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject)->radius = 50.f;
            }
        }
    );
    editingProperties_triggerVolumeCylinder->push_back(
        {
            "Height",
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Cylinder> triggerVolumeCylinder = std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject);
                float heightToAdd = m_radiusUnitsPerSec * _deltaTime;
                triggerVolumeCylinder->height += heightToAdd;
            },
            [this](float _deltaTime) {
                std::shared_ptr<TriggerVolume_Cylinder> triggerVolumeCylinder = std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject);
                float heightToRemove = m_radiusUnitsPerSec * _deltaTime;
                triggerVolumeCylinder->height -= heightToRemove;
            },
            [this]() {
                std::static_pointer_cast<TriggerVolume_Cylinder>(m_previewObject)->height = 100.f;
            }
        }
    );
}

EditingProperty BuildMode::GetCurrentEditingProperty()
{
    return editingProperties_current->at(*editingProperties_current_index);
}