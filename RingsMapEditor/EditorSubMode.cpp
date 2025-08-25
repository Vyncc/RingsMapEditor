#include "pch.h"
#include "EditorSubMode.h"


EditorSubMode::EditorSubMode(std::shared_ptr<ObjectManager> _objectManager)
{
	m_enabled = false;
	m_objectManager = _objectManager;
	m_previewObjectRotation = Rotator(0, 0, 0);

	m_fname_flyCam = FName(_globalGameWrapper->GetFNameIndexByString("Fly"));
	m_fnameIndex_rightShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_RightShoulder");
	m_fnameIndex_leftShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_LeftShoulder");

	BuildEditingProperties();
}



void EditorSubMode::Toggle()
{
	if (!m_enabled)
		Enable();
	else
		Disable();
}

bool EditorSubMode::IsEnabled() const
{
	return m_enabled;
}

bool EditorSubMode::IsInGame() const
{
	return _globalGameWrapper->IsInFreeplay() || _globalGameWrapper->IsInGame() || _globalGameWrapper->IsInOnlineGame();
}

bool EditorSubMode::IsSpectator() const
{
	PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
	if (!pc)
		return false;

	PriWrapper pri = pc.GetPRI();
	if (!pri)
		return false;

	return pri.IsSpectator();
}

//Cancel some bindings when in spectator mode
void EditorSubMode::HookEvents()
{
	_globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode", [this](ActorWrapper caller, void* params, std::string evenName) {
		UGFxData_ReplayViewer_TA_execSetCameraMode_Params* param = reinterpret_cast<UGFxData_ReplayViewer_TA_execSetCameraMode_Params*>(params);
		param->Mode = m_fname_flyCam;
		});

	_globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetFOV", [this](ActorWrapper caller, void* params, std::string evenName) {
		float* param = reinterpret_cast<float*>(params);
		*param = 90.f;
		});
}

void EditorSubMode::UnhookEvents()
{
	_globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode");
	_globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetFOV");
}

void EditorSubMode::Spectate() {
	PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
	if (!pc)
	{
		LOG("[ERROR]pc NULL!");
		return;
	}

	pc.Spectate();
}

void EditorSubMode::SwitchToFlyCam()
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

Vector EditorSubMode::CalculatePreviewActorLocation(CameraWrapper _camera)
{
	return _camera.GetLocation() + RotateVectorWithQuat({ m_previewObject_distance, 0, 0 }, RotatorToQuat(_camera.GetRotation()));
}

void EditorSubMode::BuildEditingProperties()
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

void EditorSubMode::SetCurrentObjectEditingProperties(const std::string& _name)
{
	m_currentObjectEditingPropertiesName = _name;
}

void EditorSubMode::SetCurrentObjectEditingProperties(const ObjectType& _objectType, const TriggerVolumeType& _triggerVolumeTypeOptionnal)
{
	if (_objectType == ObjectType::Mesh)
		m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_MESH;
	else if (_objectType == ObjectType::TriggerVolume)
	{
		if (_triggerVolumeTypeOptionnal == TriggerVolumeType::Box)
			m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX;
		else if (_triggerVolumeTypeOptionnal == TriggerVolumeType::Cylinder)
			m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER;
	}
	else if (_objectType == ObjectType::Checkpoint)
		m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_CHECKPOINT;
	else if (_objectType == ObjectType::Ring)
		m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_RING;
	else
		m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_OBJECT;
}

ObjectEditingProperties& EditorSubMode::GetCurrentObjectEditingProperties()
{
	return m_objectsEditingProperties[m_currentObjectEditingPropertiesName];
}

EditingProperty& EditorSubMode::GetCurrentEditingProperty()
{
	return GetCurrentObjectEditingProperties().GetCurrentEditingProperty();
}

void EditorSubMode::CycleEditingProperty()
{
	int value = GetCurrentObjectEditingProperties().GetIndex();
	value++;

	if (value > GetCurrentObjectEditingProperties().GetPropertiesCount() - 1)
		value = 0;

	GetCurrentObjectEditingProperties().SetIndex(value);
}

void EditorSubMode::ResetCurrentEditingProperty()
{
	GetCurrentEditingProperty().resetFunction();
}

void EditorSubMode::OnRightShoulderPressed(float _deltaTime)
{
	GetCurrentEditingProperty().addFunction(_deltaTime);
}

void EditorSubMode::OnLeftShoulderPressed(float _deltaTime)
{
	GetCurrentEditingProperty().removeFunction(_deltaTime);
}

int EditorSubMode::NormalizeUnrealRotation(int _rot)
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

void EditorSubMode::RotatePreviewObjectAdd(const float& _pitch, const float& _yaw, const float& _roll)
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