#include "pch.h"
#include "BuildMode.h"



BuildMode::BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes)
{
    m_enabled = false;
    m_objectManager = _objectManager;
    m_availableMeshes = _availableMeshes;
    m_meshIndex = 0;
    m_previewObjectType = ObjectType::Mesh;
    m_previewObjectRotation = Rotator(0, 0, 0);

    fname_flyCam = FName(_globalGameWrapper->GetFNameIndexByString("Fly"));
    rightShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_RightShoulder");
    leftShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_LeftShoulder");
}

BuildMode::~BuildMode()
{

}



bool BuildMode::IsEnabled() {
    return m_enabled;
}

void BuildMode::ToggleBuildMode()
{
    if (m_enabled)
        DisableBuildMode();
    else
        EnableBuildMode();
}

void BuildMode::EnableBuildMode()
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

void BuildMode::DisableBuildMode()
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

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_rotate_add", [&](std::vector<std::string> args) {
        RotatePreviewObjectAdd(0.f, 60.f, 0.f);
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_buildmode_rotate_remove", [&](std::vector<std::string> args) {
        RotatePreviewObjectAdd(0.f, -60.f, 0.f);
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
}

void BuildMode::HookEvents()
{
    _globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode", [this](ActorWrapper caller, void* params, std::string evenName) {
        UGFxData_ReplayViewer_TA_execSetCameraMode_Params* param = reinterpret_cast<UGFxData_ReplayViewer_TA_execSetCameraMode_Params*>(params);
        param->Mode = fname_flyCam;
        });

    _globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetFOV", [this](ActorWrapper caller, void* params, std::string evenName) {
        float* param = reinterpret_cast<float*>(params);
        *param = 90.f;
        });
}

void BuildMode::UnhookEvents()
{
    _globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode");
    _globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetFOV");
}

void BuildMode::Spectate()
{
    PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
    if (!pc)
    {
        LOG("[ERROR]pc NULL!");
        return;
    }

    pc.Spectate();
}

void BuildMode::SwitchToFlyCam()
{
    PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
    if (!pc)
    {
        LOG("[ERROR]pc NULL!");
        return;
    }

    SpectatorHUDWrapper spectatorHUD = pc.GetSpectatorHud();
    if (!spectatorHUD)
    {
        LOG("[ERROR]spectatorHUD NULL!");
        return;
    }

    ReplayViewerDataWrapper replayViewer = spectatorHUD.GetViewerData();
    if (!replayViewer)
    {
        LOG("[ERROR]replayViewer NULL!");
        return;
    }

    replayViewer.SetCameraMode("Fly");
}

void BuildMode::SetPreviewObjectType(ObjectType _objectType)
{
    if (_objectType == ObjectType::Mesh)
    {
        m_previewObject = std::make_shared<Mesh>(GetCurrentMesh());
        std::static_pointer_cast<Mesh>(m_previewObject)->SpawnInstance();
        LOG("Set preview object type to Mesh");
    }
    else if (_objectType == ObjectType::TriggerVolume)
    {
        if (m_triggerVolumeType == TriggerVolumeType::Box)
        {
            m_previewObject = std::make_shared<TriggerVolume_Box>();
            LOG("Set preview object type to TriggerVolume Box");
        }
        else if(m_triggerVolumeType == TriggerVolumeType::Cylinder)
        {
            m_previewObject = std::make_shared<TriggerVolume_Cylinder>();
            LOG("Set preview object type to TriggerVolume Cylinder");
        }
    }
    else if (_objectType == ObjectType::Checkpoint)
    {
        m_previewObject = std::make_shared<Checkpoint>();
        LOG("Set preview object type to Checkpoint");
    }
    else if (_objectType == ObjectType::Ring)
    {
        m_previewObject = std::make_shared<Ring_Small>(m_objectManager->GetRings().size());
        std::static_pointer_cast<Ring>(m_previewObject)->mesh.SpawnInstance();
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

    if (m_meshIndex <= 0)
    {
        LOG("[ERROR]No more meshes!");
        return;
    }

    m_meshIndex--;
    MeshInfos meshInfos = GetCurrentMesh();
    LOG("Setting mesh : {} | {}", meshInfos.name, meshInfos.meshPath);
    m_previewObject->name = "Mesh " + meshInfos.name;
    std::static_pointer_cast<Mesh>(m_previewObject)->SetStaticMesh(meshInfos);
}

void BuildMode::NextMesh()
{
    if (m_previewObjectType != ObjectType::Mesh) return;

    if (m_meshIndex >= m_availableMeshes.size() - 1)
    {
        LOG("[ERROR]No more meshes!");
        return;
    }

    m_meshIndex++;
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

    if(m_triggerVolumeType == TriggerVolumeType::Box)
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
    else if (m_triggerVolumeType == TriggerVolumeType::Cylinder)
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
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
        m_previewObject = std::make_shared<TriggerVolume_Box>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
    else if (m_triggerVolumeType == TriggerVolumeType::Cylinder)
        m_previewObject = std::make_shared<TriggerVolume_Cylinder>(*std::static_pointer_cast<TriggerVolume>(m_previewObject)); //convert trigger volume
}

void BuildMode::CycleEditingProperty()
{
    uint8_t value = static_cast<std::uint8_t>(m_editingProperty);
    value++;

    if (value > static_cast<std::uint8_t>(EditingProperty::Rotation_Roll))
        value = 0;

    m_editingProperty = static_cast<EditingProperty>(value);
}

void BuildMode::PlaceObject()
{
    m_objectManager->AddObject(m_previewObject);
    LOG("Placed object : {}", m_previewObject->name);
    m_previewObject = m_previewObject->Clone();
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

bool BuildMode::IsInGame() const
{
    return _globalGameWrapper->IsInFreeplay() || _globalGameWrapper->IsInGame() || _globalGameWrapper->IsInOnlineGame();
}

MeshInfos BuildMode::GetCurrentMesh()
{
    return m_availableMeshes[m_meshIndex];
}

bool BuildMode::IsSpectator() const
{
    PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
    if (!pc)
        return false;

    PriWrapper pri = pc.GetPRI();
    if (!pri)
        return false;

    return pri.IsSpectator();
}

void BuildMode::OnTick(float _deltaTime)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

    if (m_previewObject)
    {
        //if R1 pressed
        if (_globalGameWrapper->IsKeyPressed(rightShoulder))
        {
            float degreesToAdd = m_roationDegreesPerSeconds * _deltaTime;

            if (m_editingProperty == EditingProperty::Rotation_Pitch)
                RotatePreviewObjectAdd(degreesToAdd, 0.f, 0.f);
            else if (m_editingProperty == EditingProperty::Rotation_Yaw)
                RotatePreviewObjectAdd(0.f, degreesToAdd, 0.f);
            else if (m_editingProperty == EditingProperty::Rotation_Roll)
                RotatePreviewObjectAdd(0.f, 0.f, degreesToAdd);
        }

        //if L1 pressed
        if (_globalGameWrapper->IsKeyPressed(leftShoulder))
        {
            float degreesToAdd = m_roationDegreesPerSeconds * _deltaTime;

            if (m_editingProperty == EditingProperty::Rotation_Pitch)
                RotatePreviewObjectAdd(-degreesToAdd, 0.f, 0.f);
            else if (m_editingProperty == EditingProperty::Rotation_Yaw)
                RotatePreviewObjectAdd(0.f, -degreesToAdd, 0.f);
            else if (m_editingProperty == EditingProperty::Rotation_Roll)
                RotatePreviewObjectAdd(0.f, 0.f, -degreesToAdd);
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
            std::string meshIndexText = "Mesh Index : " + std::to_string(m_meshIndex);
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
        if (m_editingProperty == EditingProperty::Rotation_Pitch)
            editingText += "Rotation Pitch";
        else if (m_editingProperty == EditingProperty::Rotation_Yaw)
            editingText += "Rotation Yaw";
        else if (m_editingProperty == EditingProperty::Rotation_Roll)
            editingText += "Rotation Roll";

        _canvas.DrawString(editingText, stringScale, stringScale);
    }
}