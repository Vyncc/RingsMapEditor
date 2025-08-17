#include "pch.h"
#include "RingsMapEditor.h"

#include "CustomWidgets.hpp"


void RingsMapEditor::RenderSettings()
{
	if (ImGui::Button("Open Menu"))
	{
		gameWrapper->Execute([&](GameWrapper* gw) {
			cvarManager->executeCommand("openmenu " + GetMenuName());
			});
	}
}

void RingsMapEditor::RenderWindow()
{
	RenderSaveConfigPopup();
	RenderLoadConfigPopup();

	if (ImGui::Button("Save", ImVec2(75.f, 20.f)))
	{
		ImGui::OpenPopup("Save Config");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load", ImVec2(75.f, 20.f)))
	{
		ImGui::OpenPopup("Load Config");
	}

	if (ImGui::Button("Start Editor Mode", ImVec2(120.f, 25.f)))
	{
		StartEditorMode();
	}

	ImGui::SameLine();

	if (ImGui::Button("Start Race Mode", ImVec2(120.f, 25.f)))
	{
		StartRaceMode();
	}

	if (ImGui::BeginChild("##Objects", ImVec2(250, 0), true))
	{
		CustomWidget::CenterNexIMGUItItem(ImGui::CalcTextSize("Objects").x);
		ImGui::Text("Objects");
		ImGui::Separator();

		for (int n = 0; n < objects.size(); n++)
		{
			Object& object = *objects[n];

			ImGui::PushID(n);
			if (ImGui::Selectable(object.name.c_str(), (selectedObjectIndex == n)))
			{
				selectedObjectIndex = n;
			}

			if (ImGui::BeginPopupContextItem(std::string(object.name + "Context Menu").c_str()))
			{
				if (ImGui::Selectable("Copy"))
				{
					CopyObject(object);
				}

				if (ImGui::Selectable("Remove"))
				{
					gameWrapper->Execute([this, n](GameWrapper* gw) {
						RemoveObject(n);
						});
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();

		}

		ImGui::Separator();

		if (ImGui::Button("Add Object", ImVec2(ImGui::GetContentRegionAvailWidth(), 20.f)))
		{
			ImGui::OpenPopup("Add Object");
		}
		RenderAddObjectPopup();

		ImGui::EndChild();
	}

	ImGui::SameLine();

	if (ImGui::BeginChild("##Selected Object Details", ImVec2(0, 0), true))
	{
		CustomWidget::CenterNexIMGUItItem(ImGui::CalcTextSize("Object Details").x);
		ImGui::Text("Object Details");
		ImGui::Separator();

		if (selectedObjectIndex < objects.size())
		{
			RenderProperties_Object(objects[selectedObjectIndex]);
		}

		ImGui::EndChild();
	}
}

void RingsMapEditor::RenderProperties_Object(std::shared_ptr<Object>& _object)
{
	RenderInputText("Name", &_object->name);

	ImGui::NewLine();

	if (_object->objectType == ObjectType::Mesh)
	{
		RenderProperties_Mesh(*std::static_pointer_cast<Mesh>(_object));
	}
	else if (_object->objectType == ObjectType::TriggerVolume)
	{
		RenderProperties_TriggerVolume(reinterpret_cast<std::shared_ptr<TriggerVolume>&>(_object)); //passing it by reference
	}
	else if (_object->objectType == ObjectType::Checkpoint)
	{
		RenderProperties_Checkpoint(*std::static_pointer_cast<Checkpoint>(_object));
	}
	else if (_object->objectType == ObjectType::Ring)
	{
		RenderProperties_Ring(reinterpret_cast<std::shared_ptr<Ring>&>(_object));
	}
	else
	{
		ImGui::Text("Unknown object type");
	}

	ImGui::NewLine();
	ImGui::Separator();

	if (ImGui::Button("Copy"))
	{
		CopyObject(*_object);
	}

	ImGui::SameLine();

	if (ImGui::Button("Remove"))
	{
		gameWrapper->Execute([this](GameWrapper* gw) {
			RemoveObject(selectedObjectIndex);
			});
	}
}

void RingsMapEditor::RenderProperties_Mesh(Mesh& _mesh)
{
	ImGui::Text("Mesh");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Mesh", _mesh.meshInfos.name.c_str()))
	{
		if (ImGui::Selectable("", (_mesh.meshInfos.name == "")))
		{
			_mesh.meshInfos = MeshInfos();
		}

		for (const auto& mesh : AvailableMeshes)
		{
			if (ImGui::Selectable(mesh.name.c_str(), (_mesh.meshInfos.name == mesh.name)))
			{
				_mesh.meshInfos = mesh;
			}
		}
		ImGui::EndCombo();
	}

	//check if the mesh of the selected object exist in the available meshes
	if (std::find_if(AvailableMeshes.begin(), AvailableMeshes.end(), [&](const MeshInfos& m) { return m.meshPath == _mesh.meshInfos.meshPath; }) == AvailableMeshes.end())
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Mesh not found!");
	}

	RenderInputText("Mesh Path", &_mesh.meshInfos.meshPath, ImGuiInputTextFlags_ReadOnly);

	ImGui::NewLine();
	if (ImGui::DragFloat3("Location", &_mesh.location.X))
	{
		if (_mesh.instance)
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				_mesh.SetLocation(_mesh.location);
				});
		}
	}
	if (ImGui::DragInt3("Rotation", &_mesh.rotation.Pitch))
	{
		if (_mesh.instance)
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				_mesh.SetRotation(_mesh.rotation);
				});
		}
	}
	if (ImGui::DragFloat("Scale", &_mesh.scale, 0.01f, 0.01f, 100.0f))
	{
		if (_mesh.instance)
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				_mesh.SetScale3D(Vector(_mesh.scale, _mesh.scale, _mesh.scale));
				});
		}
	}

	ImGui::NewLine();

	bool CantSpawnObject = !IsInGame() || _mesh.IsMeshPathEmpty() || _mesh.IsSpawned();
	std::string errorMessage = "";

	if (CantSpawnObject)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // fade out
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); // actually disable interaction
	}

	if (!IsInGame())
		errorMessage = "You must be in a game";
	else if (_mesh.IsMeshPathEmpty())
		errorMessage = "Object path is empty";
	else if (_mesh.IsSpawned())
		errorMessage = "Object is already spawned";

	if (ImGui::Button("Spawn Object"))
	{
		gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
			SpawnMesh(_mesh);
			});
	}

	if (CantSpawnObject)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::BeginTooltip();
			ImGui::Text(errorMessage.c_str());
			ImGui::EndTooltip();
		}
	}

	ImGui::SameLine();

	if (!_mesh.IsSpawned())
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // fade out
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); // actually disable interaction
	}

	if (ImGui::Button("Destroy Object"))
	{
		gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
			_mesh.DestroyInstance();
			});
	}

	if (!_mesh.IsSpawned())
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}

	if (_mesh.HasCollisionMesh())
	{
		if (ImGui::Checkbox("Enable Collisions", &_mesh.enableCollisions))
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				if (_mesh.enableCollisions)
					_mesh.EnableCollisions();
				else
					_mesh.DisableCollisions();
				});
		}

		if (ImGui::Checkbox("Enable Physics", &_mesh.enablePhysics))
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				if (_mesh.enablePhysics)
					_mesh.EnablePhysics();
				else
					_mesh.DisablePhysics();
				});
		}

		if (ImGui::Checkbox("Enable Sticky Walls", &_mesh.enableStickyWalls))
		{
			gameWrapper->Execute([this, &_mesh](GameWrapper* gw) {
				if (_mesh.enableStickyWalls)
					_mesh.EnableStickyWalls();
				/*else
					_mesh.DisableStickyWalls();*/
				});
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("Enable this if you want the car to be able to drive on the object");
			ImGui::EndTooltip();
		}
	}
}

void RingsMapEditor::RenderProperties_TriggerVolume(std::shared_ptr<TriggerVolume>& _volume)
{
	ImGui::Text("Type");
	ImGui::SameLine();
	std::string selectedTriggerVolumeType = triggerVolumeTypesMap[_volume->triggerVolumeType];
	if (ImGui::BeginCombo("##Trigger Volume Type", selectedTriggerVolumeType.c_str()))
	{
		for (const auto& type : triggerVolumeTypesMap)
		{
			bool isSelected = (type.first == _volume->triggerVolumeType);
			if (ImGui::Selectable(type.second.c_str(), isSelected))
			{
				if (!isSelected)
				{
					ConvertTriggerVolume(_volume, type.first);
				}
			}
		}

		ImGui::EndCombo();
	}

	ImGui::NewLine();

	if (ImGui::DragFloat3("Location", &_volume->location.X))
	{
		_volume->SetLocation(_volume->location);
	}

	if (ImGui::DragInt3("Rotation", &_volume->rotation.Pitch))
	{
		_volume->SetRotation(_volume->rotation);
	}

	std::string selectedFunction = (_volume->onTouchCallback ? _volume->onTouchCallback->name : "");
	if (ImGui::BeginCombo("On Touch Event", selectedFunction.c_str()))
	{
		for (auto& func : triggerFunctionsMap)
		{
			if (ImGui::Selectable(func.second->name.c_str()))
			{
				_volume->SetOnTouchCallback(func.second->Clone());
			}
		}

		ImGui::EndCombo();
	}

	if (_volume->onTouchCallback)
	{
		_volume->onTouchCallback->RenderParameters();
	}

	ImGui::NewLine();

	if (_volume->triggerVolumeType == TriggerVolumeType::Box)
		RenderProperties_TriggerVolume_Box(*std::static_pointer_cast<TriggerVolume_Box>(_volume));
	else if (_volume->triggerVolumeType == TriggerVolumeType::Cylinder)
		RenderProperties_TriggerVolume_Cylinder(*std::static_pointer_cast<TriggerVolume_Cylinder>(_volume));
}

void RingsMapEditor::RenderProperties_TriggerVolume_Box(TriggerVolume_Box& _volume)
{
	if (ImGui::DragFloat3("Size", &_volume.size.X, 1.f, 0.01f, 1000.0f))
	{
		_volume.SetSize(_volume.size);
	}
}

void RingsMapEditor::RenderProperties_TriggerVolume_Cylinder(TriggerVolume_Cylinder& _volume)
{
	ImGui::DragFloat("Radius", &_volume.radius, 0.5f, 0.01f, 1000.0f);
	ImGui::DragFloat("Height", &_volume.height, 0.5f, 0.01f, 1000.0f);
}

void RingsMapEditor::RenderProperties_Checkpoint(Checkpoint& _checkpoint)
{
	static bool idAlreadyInUse = false;
	ImGui::Text("ID");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::InputInt("##ID", &_checkpoint.checkpointId, 0, 100))
	{
		auto it = std::find_if(checkpoints.begin(), checkpoints.end(),
			[_checkpoint](const std::shared_ptr<Checkpoint>& checkpoint) {
				return checkpoint->checkpointId == _checkpoint.checkpointId;
			});

		idAlreadyInUse = (it != checkpoints.end());
	}

	if (idAlreadyInUse)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ID already in use!");
	}

	ImGui::NewLine();

	std::string selectedCheckpointType = checkpointTypesMap[_checkpoint.checkpointType];
	if (ImGui::BeginCombo("Type", selectedCheckpointType.c_str()))
	{
		for (const auto& checkpointType : checkpointTypesMap)
		{
			if (ImGui::Selectable(checkpointType.second.c_str(), (_checkpoint.checkpointType == checkpointType.first)))
			{
				_checkpoint.checkpointType = checkpointType.first;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::NewLine();

	if (ImGui::DragFloat3("Location", &_checkpoint.location.X))
	{
		_checkpoint.SetLocation(_checkpoint.location);
	}

	if (ImGui::DragInt3("Rotation", &_checkpoint.rotation.Pitch))
	{
		_checkpoint.SetRotation(_checkpoint.rotation);
	}

	if (ImGui::DragFloat3("Size", &_checkpoint.triggerVolume.size.X, 1.f, 0.01f, 1000.0f))
	{
		_checkpoint.SetSize(_checkpoint.triggerVolume.size);
	}

	ImGui::NewLine();

	ImGui::DragFloat3("Spawn Location", &_checkpoint.spawnLocation_offset.X);
	ImGui::DragInt3("Spawn Rotation", &_checkpoint.spawnRotation.Pitch);
}

void RingsMapEditor::RenderProperties_Ring(std::shared_ptr<Ring>& _ring)
{
	ImGui::Text("ID");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	ImGui::InputInt("##ID", &_ring->ringId, 0, 100);

	auto it = std::find_if(rings.begin(), rings.end(),
		[&_ring](const std::shared_ptr<Ring>& ring) {
			return ring != _ring && ring->ringId == _ring->ringId;
		});

	bool ringIdAlreadyInUse = (it != rings.end());
	if (ringIdAlreadyInUse)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ID already in use!");
	}

	if (ImGui::DragFloat3("Location", &_ring->location.X))
	{
		gameWrapper->Execute([this, &_ring](GameWrapper* gw) {
			_ring->SetLocation(_ring->location);
			});
	}

	if (ImGui::DragInt3("Rotation", &_ring->rotation.Pitch))
	{
		gameWrapper->Execute([this, &_ring](GameWrapper* gw) {
			_ring->SetRotation(_ring->rotation);
			});
	}

	ImGui::NewLine();

	ImGui::Text("Mesh");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Mesh", _ring->mesh.meshInfos.name.c_str()))
	{
		if (ImGui::Selectable("", (_ring->mesh.meshInfos.name == "")))
		{
			_ring->mesh.meshInfos = MeshInfos();
		}

		for (const auto& mesh : AvailableMeshes)
		{
			if (ImGui::Selectable(mesh.name.c_str(), (_ring->mesh.meshInfos.name == mesh.name)))
			{
				_ring->mesh.meshInfos = mesh;
			}
		}
		ImGui::EndCombo();
	}

	//check if the mesh of the selected object exist in the available meshes
	if (std::find_if(AvailableMeshes.begin(), AvailableMeshes.end(), [&](const MeshInfos& m) { return m.meshPath == _ring->mesh.meshInfos.meshPath; }) == AvailableMeshes.end())
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Mesh not found!");
	}

	RenderInputText("Mesh Path", &_ring->mesh.meshInfos.meshPath, ImGuiInputTextFlags_ReadOnly);

	ImGui::NewLine();

	ImGui::PushID("In Trigger Volume");
	ImGui::Text("In Trigger Volume");

	if(ImGui::DragFloat3("Location ##In Trigger Volume", &_ring->triggerVolumeIn_offset_location.X))
	{
		_ring->UpdateTriggerVolumes();
	}
	RenderProperties_TriggerVolume_Cylinder(_ring->triggerVolumeIn);
	ImGui::PopID();

	ImGui::NewLine();

	ImGui::PushID("Out Trigger Volume");
	ImGui::Text("Out Trigger Volume");

	if (ImGui::DragFloat3("Location ##Out Trigger Volume", &_ring->triggerVolumeOut_offset_location.X))
	{
		_ring->UpdateTriggerVolumes();
	}
	RenderProperties_TriggerVolume_Box(_ring->triggerVolumeOut);
	ImGui::PopID();

	ImGui::NewLine();

	bool CantSpawnObject = !IsInGame() || _ring->mesh.IsMeshPathEmpty() || _ring->mesh.IsSpawned();
	std::string errorMessage = "";

	if (CantSpawnObject)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // fade out
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true); // actually disable interaction
	}

	if (!IsInGame())
		errorMessage = "You must be in a game";
	else if (_ring->mesh.IsMeshPathEmpty())
		errorMessage = "Object path is empty";
	else if (_ring->mesh.IsSpawned())
		errorMessage = "Object is already spawned";

	if (ImGui::Button("Spawn Object"))
	{
		gameWrapper->Execute([this, &_ring](GameWrapper* gw) {
			SpawnMesh(_ring->mesh);
			});
	}

	if (CantSpawnObject)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::BeginTooltip();
			ImGui::Text(errorMessage.c_str());
			ImGui::EndTooltip();
		}
	}
}

void RingsMapEditor::RenderInputText(std::string _label, std::string* _value, ImGuiInputTextFlags _flags)
{
	ImGui::Text(_label.c_str());
	ImGui::SameLine();
	ImGui::InputText(std::string("##" + _label).c_str(), _value, _flags);
}

std::shared_ptr<Object> RingsMapEditor::CopyObject(Object& _object)
{
	std::shared_ptr<Object> clonedObject = _object.Clone();
	clonedObject->name += " (Copy)";
	objects.emplace_back(clonedObject);

	if (clonedObject->objectType == ObjectType::Mesh)
	{
		meshes.emplace_back(std::static_pointer_cast<Mesh>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::TriggerVolume)
	{
		triggerVolumes.emplace_back(std::static_pointer_cast<TriggerVolume>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::Checkpoint)
	{
		checkpoints.emplace_back(std::static_pointer_cast<Checkpoint>(clonedObject));
	}
	else if (clonedObject->objectType == ObjectType::Ring)
	{
		rings.emplace_back(std::static_pointer_cast<Ring>(clonedObject));
	}

	SelectLastObject();
	return clonedObject;
}

void RingsMapEditor::RenderAddObjectPopup()
{
	if (ImGui::BeginPopupModal("Add Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("Mesh", ImVec2(150.f, 40.f)))
		{
			AddObject(ObjectType::Mesh);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Trigger Volume", ImVec2(150.f, 40.f)))
		{
			AddObject(ObjectType::TriggerVolume);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Checkpoint", ImVec2(150.f, 40.f)))
		{
			AddObject(ObjectType::Checkpoint);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Ring", ImVec2(150.f, 40.f)))
		{
			AddObject(ObjectType::Ring);
			ImGui::CloseCurrentPopup();
		}

		CustomWidget::CenterNexIMGUItItem(100.f);
		if (ImGui::Button("Cancel", ImVec2(100.f, 25.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void RingsMapEditor::RenderSaveConfigPopup()
{
	if (ImGui::BeginPopupModal("Save Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static std::string fileName = "";
		RenderInputText("File Name", &fileName);

		CustomWidget::CenterNexIMGUItItem(208.f);

		if (ImGui::Button("Save", ImVec2(100.f, 25.f)))
		{
			if (SaveConfig(fileName))
			{
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 25.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void RingsMapEditor::RenderLoadConfigPopup()
{
	if (ImGui::BeginPopupModal("Load Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		//if no configs exist, show a message
		if (std::filesystem::is_empty(DataFolderPath))
		{
			ImGui::Text("No config files found");
			CustomWidget::CenterNexIMGUItItem(100.f);
			if (ImGui::Button("Cancel", ImVec2(100.f, 25.f)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
			return;
		}

		ImGui::BeginChild("##Config List", ImVec2(200, 250), true);

		for (const auto& entry : std::filesystem::directory_iterator(DataFolderPath))
		{
			if (entry.path().extension() == ".json")
			{
				std::string fileName = entry.path().stem().string();
				if (ImGui::Selectable(fileName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
						LoadConfig(entry.path());
						ImGui::CloseCurrentPopup();
					}
				}
			}
		}

		ImGui::EndChild();

		CustomWidget::CenterNexIMGUItItem(100.f);
		if (ImGui::Button("Cancel", ImVec2(100.f, 25.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}