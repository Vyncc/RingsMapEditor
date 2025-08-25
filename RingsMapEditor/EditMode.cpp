#include "pch.h"
#include "EditMode.h"


EditMode::EditMode(std::shared_ptr<ObjectManager> _objectManager) : EditorSubMode(_objectManager)
{
	m_enabled = false;
}



void EditMode::Enable()
{
    if (!IsInGame())
    {
        LOG("[ERROR]You need to be in a game to enable Edit Mode!");
        return;
    }

    Spectate();
    SwitchToFlyCam();
    RegisterCommands();
    HookEvents();

    m_enabled = true;
    LOG("Edit mode activated");
}

void EditMode::Disable()
{
    m_previewObject = nullptr;
    UnregisterCommands();
    UnhookEvents();
    m_enabled = false;
    LOG("Edit mode disabled");
}

void EditMode::RegisterCommands()
{
    _globalCvarManager->registerNotifier("ringsmapeditor_editmode_place_object", [&](std::vector<std::string> args) {
        PlaceObject();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_editmode_select_object", [&](std::vector<std::string> args) {
        SelectObject();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_editmode_editing_property_reset", [&](std::vector<std::string> args) {
        ResetCurrentEditingProperty();
        }, "", 0);

    _globalCvarManager->registerNotifier("ringsmapeditor_editmode_editing_property_cycle", [&](std::vector<std::string> args) {
        CycleEditingProperty();
        }, "", 0);
}

void EditMode::UnregisterCommands()
{
    _globalCvarManager->removeNotifier("ringsmapeditor_editmode_place_object");
    _globalCvarManager->removeNotifier("ringsmapeditor_editmode_select_object");
    _globalCvarManager->removeNotifier("ringsmapeditor_editmode_editing_property_reset");
    _globalCvarManager->removeNotifier("ringsmapeditor_editmode_editing_property_cycle");
}

void EditMode::OnTick(float _deltaTime)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

    if (m_previewObject)
        m_objectUnderCursor = nullptr;
    else
        m_objectUnderCursor = CheckForObjectUnderCursor();

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

void EditMode::RenderCanvas(CanvasWrapper _canvas)
{
    if (!IsEnabled() || !IsInGame() || !IsSpectator()) return;

	RenderCrosshair(_canvas);

    _canvas.SetColor(255, 255, 255, 255);

    float stringScale = 2.f;
    float newLinePaddingY = 15.f * stringScale;
    Vector2 pos = Vector2{ 20, 80 };

    auto newLine = [&]() {
        pos.Y += newLinePaddingY;
        _canvas.SetPosition(pos);
        };

    _canvas.SetPosition(pos);
    _canvas.DrawString("Edit Mode", stringScale, stringScale);
    newLine();

    if (m_previewObject)
    {
        std::string objectNameText = "Object : " + m_previewObject->name;
        _canvas.DrawString(objectNameText, stringScale, stringScale);

        if (m_previewObject->objectType == ObjectType::Mesh)
        {
            newLine();
            _canvas.DrawString("Mesh", stringScale, stringScale);

            newLine();
            std::string meshText = "Mesh : " + std::static_pointer_cast<Mesh>(m_previewObject)->meshInfos.name;
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
    else if (m_objectUnderCursor)
    {
        std::string objectNameText = "Object : " + m_objectUnderCursor->name;
        _canvas.DrawString(objectNameText, stringScale, stringScale);

        if (m_objectUnderCursor->objectType == ObjectType::Mesh)
        {
            newLine();
            _canvas.DrawString("Mesh", stringScale, stringScale);

            newLine();
            std::string meshText = "Mesh : " + std::static_pointer_cast<Mesh>(m_objectUnderCursor)->meshInfos.name;
            _canvas.DrawString(meshText, stringScale, stringScale);
        }
        else if (m_objectUnderCursor->objectType == ObjectType::TriggerVolume)
        {
            newLine();
            _canvas.DrawString("Trigger Volume", stringScale, stringScale);

            newLine();
            newLine();
            std::string triggerVolumeTypeText = "Trigger Volume Type : ";
            if (std::static_pointer_cast<TriggerVolume>(m_objectUnderCursor)->triggerVolumeType == TriggerVolumeType::Box)
                triggerVolumeTypeText += "Box";
            else if (std::static_pointer_cast<TriggerVolume>(m_objectUnderCursor)->triggerVolumeType == TriggerVolumeType::Cylinder)
                triggerVolumeTypeText += "Cylinder";
            else
                triggerVolumeTypeText += "Unknown";

            _canvas.DrawString(triggerVolumeTypeText, stringScale, stringScale);
        }
        else if (m_objectUnderCursor->objectType == ObjectType::Checkpoint)
        {
            newLine();
            _canvas.DrawString("Checkpoint", stringScale, stringScale);
        }
        else if (m_objectUnderCursor->objectType == ObjectType::Ring)
        {
            newLine();
            _canvas.DrawString("Ring", stringScale, stringScale);
        }
    }
}

void EditMode::PlaceObject()
{
	m_previewObject = nullptr;
}

void EditMode::RenderCrosshair(CanvasWrapper _canvas)
{
	_canvas.SetColor(255, 0, 0, 255);
	Vector2 size = Vector2(10, 10);
    _canvas.SetPosition(Vector2((1920 / 2) - (size.X / 2), (1080 / 2) - (size.Y / 2)));
	_canvas.DrawBox(size);
}

float EditMode::CalculateDistance(const Vector& pos1, const Vector& pos2)
{
    float dx = pos2.X - pos1.X;
    float dy = pos2.Y - pos1.Y;
    float dz = pos2.Z - pos1.Z;

    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return distance;
}

float EditMode::CalculateDistanceToObject(const std::shared_ptr<Object>& _object)
{
    CameraWrapper camera = _globalGameWrapper->GetCamera();
    if (!camera)
    {
		LOG("[ERROR]camera is NULL!");
		return -1.f;
    }

	return CalculateDistance(camera.GetLocation(), _object->GetLocation());
}

void EditMode::SelectObject()
{
	if (!m_objectUnderCursor)
	{
		LOG("[ERROR]No object under cursor to select!");
		return;
	}

	m_previewObject_distance = CalculateDistanceToObject(m_objectUnderCursor);
    m_previewObjectRotation = m_objectUnderCursor->GetRotation();
    m_previewObject = m_objectUnderCursor;
    SetCurrentObjectEditingProperties(m_objectUnderCursor->objectType);
	LOG("Selected object : {}", m_previewObject->name);
}

RayCastHitResult EditMode::RayCastActorsFromCamera()
{
    RayCastHitResult result;

    CameraWrapper camera = _globalGameWrapper->GetCamera();
    if (!camera)
    {
        LOG("[ERROR]camera NULL!");
        return result;
    }

    Vector traceStart = camera.GetLocation();
    Vector traceEnd = traceStart + RotateVectorWithQuat({ m_rayCast_distance, 0, 0 }, RotatorToQuat(camera.GetRotation()));
    FVector extent = { 0.f, 0.f, 0.f };
    FVector HitLoc = { 0.f, 0.f, 0.f };
    FVector HitNormal = { 0.f, 0.f, 0.f };
    FTraceHitInfo traceInfo;
    AActor* tracedActor = reinterpret_cast<ACamera*>(camera.memory_address)->Trace(Object::VectorToFVector(traceEnd), Object::VectorToFVector(traceStart), true, extent, 0, HitLoc, HitNormal, traceInfo);

	result.hit = tracedActor != nullptr;
	result.extent = Object::FVectorToVector(extent);
	result.hitLocation = Object::FVectorToVector(HitLoc);
	result.hitNormal = Object::FVectorToVector(HitNormal);
	result.traceInfo = traceInfo;
	result.hitActor = tracedActor;

    return result;
}

std::shared_ptr<Object> EditMode::RayCastFromCamera()
{
    CameraWrapper camera = _globalGameWrapper->GetCamera();
    if (!camera)
    {
        LOG("[ERROR]camera NULL!");
        return nullptr;
    }

    Vector traceStart = camera.GetLocation();
    Vector dir = RotateVectorWithQuat({ 1, 0, 0 }, RotatorToQuat(camera.GetRotation())).getNormalized();
    float maxDist = m_rayCast_distance;

    std::shared_ptr<Object> closestHitObject;
    float closestT = maxDist;

    auto isClosestObject = [&](const float& _tHit) {
        return (_tHit < closestT);
        };

	//Check trigger volumes
    for (std::shared_ptr<TriggerVolume>& triggerVolume : m_objectManager->GetTriggerVolumes())
    {
        float tHit;

        if (triggerVolume->RayIntersects(traceStart, dir, maxDist, tHit))
        {
            LOG("sqdsqdqs");

            if (isClosestObject(tHit))
            {
                LOG("hahahhahahah");
                closestT = tHit;
                closestHitObject = triggerVolume;
            }
        }
    }

    //Check checkpoints
    for (std::shared_ptr<Checkpoint>& checkpoint : m_objectManager->GetCheckpoints())
    {
        float tHit;

        if (checkpoint->triggerVolume.RayIntersects(traceStart, dir, maxDist, tHit))
        {
            if (isClosestObject(tHit))
            {
                closestT = tHit;
                closestHitObject = checkpoint;
            }
        }
    }

    return closestHitObject;
}

std::shared_ptr<Object> EditMode::CheckForObjectUnderCursor()
{
	if (m_objectManager->GetObjects().size() == 0)
		return nullptr;

    RayCastHitResult rayCastResult = RayCastActorsFromCamera();

    //Check for actors
    if (rayCastResult.hit && rayCastResult.hitActor)
    {
        for (std::shared_ptr<Mesh>& mesh : m_objectManager->GetMeshes())
        {
            if (mesh->instance == rayCastResult.hitActor)
                return mesh;
        }

        for (std::shared_ptr<Ring>& ring : m_objectManager->GetRings())
        {
            if (ring->mesh.instance == rayCastResult.hitActor)
                return ring;
        }
    }

	//If no actor found, check for trigger volumes or checkpoints
	std::shared_ptr<Object> objectUnderCursor = RayCastFromCamera();
    if (objectUnderCursor)
        return objectUnderCursor;

    return nullptr;
}
