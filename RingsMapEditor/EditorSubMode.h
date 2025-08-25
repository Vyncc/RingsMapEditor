#pragma once

#include "ObjectManager.h"

struct EditingProperty
{
	std::string name;
	std::function<void(float _deltaTime)> addFunction;
	std::function<void(float _deltaTime)> removeFunction;
	std::function<void()> resetFunction;
};

class ObjectEditingProperties
{
public:
	ObjectEditingProperties() = default;
	~ObjectEditingProperties() = default;

	int GetIndex() const { return index; }
	void SetIndex(int _index) { index = _index; }

	std::vector<EditingProperty>& GetProperties() { return properties; }
	void SetProperties(const std::vector<EditingProperty>& _properties) { properties = _properties; }
	void AddProperty(EditingProperty _property) { properties.emplace_back(_property); }
	size_t GetPropertiesCount() const { return properties.size(); }

	EditingProperty& GetCurrentEditingProperty() { return properties[index]; }

private:
	int index = 0;
	std::vector<EditingProperty> properties;
};

#define OBJECT_EDITING_PROPERTIES_OBJECT "Object"
#define OBJECT_EDITING_PROPERTIES_MESH "Mesh"
#define OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_BOX "TriggerVolume Box"
#define OBJECT_EDITING_PROPERTIES_TRIGGER_VOLUME_CYLINDER "TriggerVolume Cylinder"
#define OBJECT_EDITING_PROPERTIES_CHECKPOINT "Checkpoint"
#define OBJECT_EDITING_PROPERTIES_RING "Ring"

class EditorSubMode
{
public:
	EditorSubMode(std::shared_ptr<ObjectManager> _objectManager);
	virtual ~EditorSubMode() = default;

public:
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual void RegisterCommands() = 0;
	virtual void UnregisterCommands() = 0;
	virtual void OnTick(float _deltaTime) = 0;
	virtual void RenderCanvas(CanvasWrapper _canvas) = 0;
	virtual void PlaceObject() = 0;

public:
	void Toggle();
	bool IsEnabled() const;
	bool IsInGame() const;
	bool IsSpectator() const;
	void HookEvents();
	void UnhookEvents();
	void Spectate();
	void SwitchToFlyCam();

	Vector CalculatePreviewActorLocation(CameraWrapper _camera);

	//Editing Properties
	void BuildEditingProperties();
	void SetCurrentObjectEditingProperties(const std::string& _name);
	void SetCurrentObjectEditingProperties(const ObjectType& _objectType, const TriggerVolumeType& _triggerVolumeTypeOptionnal = TriggerVolumeType::Box);
	ObjectEditingProperties& GetCurrentObjectEditingProperties();
	EditingProperty& GetCurrentEditingProperty();
	void CycleEditingProperty();
	void ResetCurrentEditingProperty();

	void OnRightShoulderPressed(float _deltaTime); //On R1 pressed
	void OnLeftShoulderPressed(float _deltaTime); //On L1 pressed

	int NormalizeUnrealRotation(int _rot);
	void RotatePreviewObjectAdd(const float& _pitch, const float& _yaw, const float& _roll);

protected:
	bool m_enabled = false;
	std::shared_ptr<ObjectManager> m_objectManager;

	//Preview Object
	std::shared_ptr<Object> m_previewObject;
	Rotator m_previewObjectRotation = Rotator(0, 0, 0);

	//Settings
	float m_previewObject_distance = 1400.f;
	float m_scalePerSec = 0.5f;
	float m_rotationDegreesPerSec = 90.f;
	float m_sizeUnitsPerSec = 500.f;
	float m_radiusUnitsPerSec = 500.f;
	float m_heightUnitsPerSec = 500.f;

	//UE3 Names
	FName m_fname_flyCam;
	int m_fnameIndex_rightShoulder = 0;
	int m_fnameIndex_leftShoulder = 0;

	//Editing Properties
	std::string m_currentObjectEditingPropertiesName = OBJECT_EDITING_PROPERTIES_MESH;
	std::map<std::string, ObjectEditingProperties> m_objectsEditingProperties;
	std::map<std::string, EditingProperty> m_editingPropertyMap = {
		{
			"Object Rotation Pitch",
			EditingProperty{
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
		},
		{
			"Object Rotation Yaw",
			EditingProperty{
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
				}
			}
		},
		{
			"Object Rotation Roll",
			EditingProperty{
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
				}
			}
		},
		{
			"Mesh Scale",
			EditingProperty{
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
		},
		{
			"TriggerVolumeBox Size X",
			EditingProperty{
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
		},
		{
			"TriggerVolumeBox Size Y",
			EditingProperty{
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
		},
		{
			"TriggerVolumeBox Size Z",
			EditingProperty{
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
		},
		{
			"TriggerVolumeCylinder Radius",
			EditingProperty{
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
		},
		{
			"TriggerVolumeCylinder Height",
			EditingProperty{
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
		}
	};
};