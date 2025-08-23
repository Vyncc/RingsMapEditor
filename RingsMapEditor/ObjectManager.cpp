#include "pch.h"
#include "ObjectManager.h"


ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
}



void ObjectManager::AddObject(ObjectType _objectType)
{
	if (_objectType == ObjectType::Mesh)
	{
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();
		AddObject(newMesh);
		AddMesh(newMesh);
		//SelectLastObject();
	}
	else if (_objectType == ObjectType::TriggerVolume)
	{
		std::shared_ptr<TriggerVolume_Box> newTriggerVolume = std::make_shared<TriggerVolume_Box>();
		AddObject(newTriggerVolume);
		AddTriggerVolume(newTriggerVolume);
		//SelectLastObject();
	}
	else if (_objectType == ObjectType::Checkpoint)
	{
		std::shared_ptr<Checkpoint> newCheckpoint = std::make_shared<Checkpoint>();
		newCheckpoint->checkpointId = checkpoints.size() + 1; // Assign a new ID based on the current size
		AddObject(newCheckpoint);
		AddCheckpoint(newCheckpoint);
		//SelectLastObject();
	}
	else if (_objectType == ObjectType::Ring)
	{
		std::shared_ptr<Ring_Small> newRing = std::make_shared<Ring_Small>(m_rings.size() + 1);
		AddObject(newRing);
		AddRing(newRing);
		//SelectLastObject();
	}
	else
	{
		LOG("[ERROR] Unsupported object type: {}", static_cast<int>(_objectType));
	}
}

void ObjectManager::AddObject(const std::shared_ptr<Object>& _object)
{
	m_objects.emplace_back(_object);

	if (_object->objectType == ObjectType::Mesh)
		AddMesh(std::static_pointer_cast<Mesh>(_object));
	else if (_object->objectType == ObjectType::TriggerVolume)
		AddTriggerVolume(std::static_pointer_cast<TriggerVolume>(_object));
	else if (_object->objectType == ObjectType::Checkpoint)
		AddCheckpoint(std::static_pointer_cast<Checkpoint>(_object));
	else if (_object->objectType == ObjectType::Ring)
		AddRing(std::static_pointer_cast<Ring>(_object));
}

void ObjectManager::AddMesh(const std::shared_ptr<Mesh>& _mesh)
{
	m_meshes.emplace_back(_mesh);
}

void ObjectManager::AddTriggerVolume(const std::shared_ptr<TriggerVolume>& _triggerVolume)
{
	m_triggerVolumes.emplace_back(_triggerVolume);
}

void ObjectManager::AddCheckpoint(const std::shared_ptr<Checkpoint>& _checkpoint)
{
	checkpoints.emplace_back(_checkpoint);
}

void ObjectManager::AddRing(const std::shared_ptr<Ring>& _ring)
{
	m_rings.emplace_back(_ring);
}

std::shared_ptr<Object> ObjectManager::CopyObject(Object& _object)
{
	std::shared_ptr<Object> clonedObject = _object.Clone();
	clonedObject->name += " (Copy)";
	m_objects.emplace_back(clonedObject);

	if (clonedObject->objectType == ObjectType::Mesh)
	{
		m_meshes.emplace_back(std::static_pointer_cast<Mesh>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::TriggerVolume)
	{
		m_triggerVolumes.emplace_back(std::static_pointer_cast<TriggerVolume>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::Checkpoint)
	{
		checkpoints.emplace_back(std::static_pointer_cast<Checkpoint>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::Ring)
	{
		m_rings.emplace_back(std::static_pointer_cast<Ring>(clonedObject));
	}

	return clonedObject;
}

void ObjectManager::RemoveObject(const int& _objectIndex)
{
	std::shared_ptr<Object> selectedObject = m_objects[_objectIndex];

	if (selectedObject->objectType == ObjectType::Mesh)
	{
		std::shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(selectedObject);
		m_meshes.erase(std::remove(m_meshes.begin(), m_meshes.end(), mesh), m_meshes.end());
		LOG("Removed mesh: {}", mesh->name);
	}
	else if (selectedObject->objectType == ObjectType::TriggerVolume)
	{
		std::shared_ptr<TriggerVolume> triggerVolume = std::static_pointer_cast<TriggerVolume>(selectedObject);
		m_triggerVolumes.erase(std::remove(m_triggerVolumes.begin(), m_triggerVolumes.end(), triggerVolume), m_triggerVolumes.end());
		LOG("Removed trigger volume: {}", triggerVolume->name);
	}
	else if (selectedObject->objectType == ObjectType::Checkpoint)
	{
		std::shared_ptr<Checkpoint> chekpoint = std::static_pointer_cast<Checkpoint>(selectedObject);
		checkpoints.erase(std::remove(checkpoints.begin(), checkpoints.end(), chekpoint), checkpoints.end());
		LOG("Removed checkpoint: {}", chekpoint->name);
	}
	else if (selectedObject->objectType == ObjectType::Ring)
	{
		std::shared_ptr<Ring> ring = std::static_pointer_cast<Ring>(selectedObject);
		m_rings.erase(std::remove(m_rings.begin(), m_rings.end(), ring), m_rings.end());
		LOG("Removed ring: {}", ring->name);
	}

	m_objects.erase(m_objects.begin() + _objectIndex);
}

void ObjectManager::ClearObjects()
{
	m_objects.clear();
	m_triggerVolumes.clear();
	checkpoints.clear();
	m_rings.clear();
}

void ObjectManager::ConvertTriggerVolume(std::shared_ptr<TriggerVolume> _triggerVolume, TriggerVolumeType _triggerVolumeType)
{
	std::shared_ptr<TriggerVolume> oldPtr = _triggerVolume;

	if (_triggerVolumeType == TriggerVolumeType::Box)
	{
		_triggerVolume = std::make_shared<TriggerVolume_Box>(*_triggerVolume);
	}
	else if (_triggerVolumeType == TriggerVolumeType::Cylinder)
	{
		_triggerVolume = std::make_shared<TriggerVolume_Cylinder>(*_triggerVolume);
	}

	auto objects_it = std::find(m_objects.begin(), m_objects.end(), oldPtr);
	if (objects_it != m_objects.end())
	{
		*objects_it = _triggerVolume; // point to new object
	}

	auto triggerVolumes_it = std::find(m_triggerVolumes.begin(), m_triggerVolumes.end(), oldPtr);
	if (triggerVolumes_it != m_triggerVolumes.end())
	{
		*triggerVolumes_it = _triggerVolume; // point to new object
	}
}

std::vector<std::shared_ptr<Object>>& ObjectManager::GetObjects()
{
	return m_objects;
}

std::vector<std::shared_ptr<Mesh>>& ObjectManager::GetMeshes()
{
	return m_meshes;
}

std::vector<std::shared_ptr<TriggerVolume>>& ObjectManager::GetTriggerVolumes()
{
	return m_triggerVolumes;
}

std::vector<std::shared_ptr<Checkpoint>>& ObjectManager::GetTriggerCheckpoints()
{
	return checkpoints;
}

std::vector<std::shared_ptr<Ring>>& ObjectManager::GetRings()
{
	return m_rings;
}

std::map<std::string, std::shared_ptr<TriggerFunction>>& ObjectManager::GetTriggerFunctionsMap()
{
	return m_triggerFunctionsMap;
}
