#include "pch.h"
#include "RingsMapEditor.h"

#include <fstream>

BAKKESMOD_PLUGIN(RingsMapEditor, "RingsMapEditor", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
std::shared_ptr<GameWrapper> _globalGameWrapper;

void RingsMapEditor::onLoad()
{
	_globalCvarManager = cvarManager;
	_globalGameWrapper = gameWrapper;

	InitRLSDK();
	InitPaths();

	AvailableMeshes = GetAvailableMeshes();
	objectManager = std::make_shared<ObjectManager>();
	buildMode = std::make_shared<BuildMode>(objectManager, AvailableMeshes);
	editMode = std::make_shared<EditMode>(objectManager);

	//cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
	//	LOG("Hello notifier!");
	//}, "", 0);

	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	LOG("Your hook got called and the ball went POOF");
	//});

	_globalCvarManager->registerNotifier("ringsmapeditor_buildmode_toggle", [&](std::vector<std::string> args) {
		buildMode->Toggle();
		}, "", 0);

	_globalCvarManager->registerNotifier("ringsmapeditor_editmode_toggle", [&](std::vector<std::string> args) {
		editMode->Toggle();
		}, "", 0);

	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.PostBeginPlay", std::bind(&RingsMapEditor::OnGameCreated, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&RingsMapEditor::OnGameDestroyed, this, std::placeholders::_1));

	//gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.PostBeginPlay", std::bind(&RingsMapEditor::OnCarSpawn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.OnVehicleSetup", std::bind(&RingsMapEditor::OnCarSpawn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>("Function Engine.GameViewportClient.Tick", std::bind(&RingsMapEditor::OnTick, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	gameWrapper->RegisterDrawable(bind(&RingsMapEditor::RenderCanvas, this, std::placeholders::_1));
}

void RingsMapEditor::InitPaths()
{
	DataFolderPath = gameWrapper->GetDataFolder() / "RingsMapEditor";
	if (!std::filesystem::exists(DataFolderPath))
	{
		LOG("Data folder does not exist, creating: {}", DataFolderPath.string());
		std::filesystem::create_directories(DataFolderPath);
	}

	std::string RLWin64_Path = std::filesystem::current_path().string();
	RLCookedPCConsolePath = RLWin64_Path.substr(0, RLWin64_Path.length() - 14) + "TAGame\\CookedPCConsole\\mods\\RingsMapEditor";
	LOG("CookedPCConsole Path: {}", RLCookedPCConsolePath.string());
	if (!std::filesystem::exists(RLCookedPCConsolePath))
	{
		LOG("CookedPCConsole\\mods\\RingsMapEditor does not exist, creating: {}", RLCookedPCConsolePath.string());
		std::filesystem::create_directories(RLCookedPCConsolePath);
	}

	MeshesPath = RLCookedPCConsolePath / "Meshes";
	if (!std::filesystem::exists(MeshesPath))
	{
		LOG("Meshes folder does not exist, creating: {}", MeshesPath.string());
		std::filesystem::create_directories(MeshesPath);
	}
}

bool RingsMapEditor::SaveConfig(const std::string& fileName)
{
	if (fileName.empty())
	{
		LOG("[ERROR]File name cannot be empty!");
		return false;
	}

	try
	{
		nlohmann::json objects_json = nlohmann::json::array();
		for (const std::shared_ptr<Object>& object : objectManager->GetObjects()) {
			if (object)
			{
				objects_json.push_back(object->to_json());
			}
		}

		std::filesystem::path filePath = DataFolderPath / std::string(fileName + ".json");
		std::ofstream file = std::ofstream(filePath);
		file << objects_json.dump(4);

		LOG("Saved config successfully to: {}", filePath.string());
		return true;
	}
	catch (const std::exception& e)
	{
		LOG("[ERROR]Failed to save config: {}", e.what());
	}
}

void RingsMapEditor::LoadConfig(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
	{
		LOG("[ERROR]Config file does not exist: {}", filePath.string());
		return;
	}

	std::ifstream file = std::ifstream(filePath);
	if (!file.is_open())
	{
		LOG("[ERROR]Could not open config file: {}", filePath.string());
		return;
	}

	objectManager->ClearObjects();

	try
	{
		nlohmann::json arr;
		file >> arr;
		
		for (const auto& item : arr)
		{
			std::shared_ptr<Object> loadedObject = FromJson_Object(item);
			if (!loadedObject)
			{
				LOG("[ERROR]Couldn't load object : {}", item.dump(4));
				continue;
			}

			LOG("loaded object : {}", loadedObject->name);
			objectManager->AddObject(loadedObject);
		}

		LOG("Loaded config successfully from: {}", filePath.string());
	}
	catch (const nlohmann::json::exception& e)
	{
		LOG("[ERROR]Failed to parse config: {}", e.what());
	}
}

bool RingsMapEditor::IsInEditorMode()
{
	return currentMode == Mode::Editor;
}

bool RingsMapEditor::IsInRaceMode()
{
	return currentMode == Mode::Race;
}

void RingsMapEditor::StartEditorMode()
{
	currentMode = Mode::Editor;
	isStartingRace = false;

	/*gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole\\mods\\RingsMapEditor\\Meshes\\ringsmapeditor.upk?Game=TAGame.GameInfo_Soccar_TA?GameTags=BotsNone");
		});*/

	gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start EuroStadium_P?Game=TAGame.GameInfo_Soccar_TA?GameTags=BotsNone");
		});
}

void RingsMapEditor::StartRaceMode()
{
	currentMode = Mode::Race;
	isStartingRace = true;
	raceTimer.Reset();

	/*gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole\\mods\\RingsMapEditor\\Meshes\\ringsmapeditor.upk?Game=TAGame.GameInfo_Soccar_TA?GameTags=Freeplay");
		});*/

	gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start EuroStadium_P?Game=TAGame.GameInfo_Soccar_TA?GameTags=Freeplay");
		});
}

void RingsMapEditor::OnGameCreated(std::string eventName)
{
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.GetPlayerHUDPosition", std::bind(&RingsMapEditor::OnGameFirstTick, this, std::placeholders::_1));
}

void RingsMapEditor::OnGameFirstTick(std::string eventName)
{
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.GetPlayerHUDPosition");

	gameWrapper->SetTimeout([this](GameWrapper* gw)
		{
			for (std::shared_ptr<Object>& object : objectManager->GetObjects())
			{
				if (object->objectType == ObjectType::Mesh)
				{
					std::static_pointer_cast<Mesh>(object)->SpawnInstance();
				}
				else if (object->objectType == ObjectType::Ring)
				{
					std::static_pointer_cast<Ring>(object)->mesh.SpawnInstance();
				}
			}
		}, 0.1f);
}

void RingsMapEditor::OnGameDestroyed(std::string eventName)
{
	DestroyAllMeshes();
}

void RingsMapEditor::OnCarSpawn(CarWrapper caller, void* params, std::string eventName)
{
	if (!IsInRaceMode())
		return;

	if (!isStartingRace)
		return;

	if (checkpoints.empty())
	{
		LOG("[ERROR]No checkpoints set!");
		return;
	}

	if (isStartingRace)
	{
		caller.SetLocation(checkpoints[0]->GetSpawnWorldLocation());
		isStartingRace = false;
	}
}

bool RingsMapEditor::SetCurrentCheckpoint(std::shared_ptr<Checkpoint> _checkpoint)
{
	if (currentCheckpoint != _checkpoint)
	{
		currentCheckpoint = _checkpoint;
		LOG("Current checkpoint set to: {} | {}", _checkpoint->checkpointId, _checkpoint->name);
		return true;
	}

	return false;
}

void RingsMapEditor::TeleportToCurrentCheckpoint()
{
	currentRingId = -1;

	CarWrapper localCar = gameWrapper->GetLocalCar();
	if (!localCar)
	{
		LOG("[ERROR]localCar is NULL!");
		return;
	}

	if (currentCheckpoint)
	{
		localCar.SetLocation(currentCheckpoint->GetSpawnWorldLocation());
		localCar.SetRotation(currentCheckpoint->spawnRotation);
		localCar.SetVelocity(Vector(0.f, 0.f, 0.f));
	}
}

void RingsMapEditor::SelectLastObject()
{
	std::vector<std::shared_ptr<Object>>& objects = objectManager->GetObjects();

	if (!objects.empty())
	{
		selectedObjectIndex = objects.size() - 1;
	}
	else
	{
		selectedObjectIndex = -1;
	}
}

void RingsMapEditor::CheckTriggerVolumes()
{
	if (!IsInRaceMode())
		return;

	CarWrapper localCar = gameWrapper->GetLocalCar();
	if (!localCar)
	{
		//LOG("[ERROR]localCar is NULL!");
		return;
	}

	for (std::shared_ptr<TriggerVolume>& volume : objectManager->GetTriggerVolumes())
	{
		if (volume->IsPointInside(localCar.GetLocation()))
		{
			volume->OnTouch(localCar);
		}
	}
}

void RingsMapEditor::CheckCheckpoints()
{
	if (!IsInRaceMode())
		return;

	CarWrapper localCar = gameWrapper->GetLocalCar();
	if (!localCar)
	{
		//LOG("[ERROR]localCar is NULL!");
		return;
	}

	for (std::shared_ptr<Checkpoint>& checkpoint : checkpoints)
	{
		if (checkpoint->triggerVolume.IsPointInside(localCar.GetLocation()))
		{
			if (SetCurrentCheckpoint(checkpoint))
			{
				if (checkpoint->IsStartCheckpoint())
				{
					raceTimer.Start();
					LOG("Starting timer");
				}
				else if (checkpoint->IsEndCheckpoint())
				{
					raceTimer.Stop();
					LOG("Stopping timer");
				}
			}
		}
	}
}

void RingsMapEditor::CheckRings()
{
	if (!IsInRaceMode())
		return;

	CarWrapper localCar = gameWrapper->GetLocalCar();
	if (!localCar)
	{
		//LOG("[ERROR]localCar is NULL!");
		return;
	}

	for (std::shared_ptr<Ring>& ring : objectManager->GetRings())
	{
		//Car pass through the ring
		if (ring->triggerVolumeIn.IsPointInside(localCar.GetLocation()))
		{
			currentRingId = ring->ringId;
			LOG("current ring : {}", currentRingId);
		}

		//Car pass behind the ring, checking if the car didn't pass through the ring
		if (ring->triggerVolumeOut.IsPointInside(localCar.GetLocation()))
		{
			if (currentRingId != ring->ringId)
			{
				LOG("Didn't go through the ring! teleporting back to current checkpoint");
				TeleportToCurrentCheckpoint();
			}
		}
	}
}

void RingsMapEditor::OnTick(ActorWrapper caller, void* params, std::string eventName)
{
	if (!IsInGame())
		return;

	if (IsInEditorMode())
	{
		if (buildMode->IsEnabled())
		{
			buildMode->OnTick(*reinterpret_cast<float*>(params));
		}
		else if (editMode->IsEnabled())
		{
			editMode->OnTick(*reinterpret_cast<float*>(params));
		}
	}
	else if (IsInRaceMode())
	{
		CheckTriggerVolumes();
		CheckCheckpoints();
		CheckRings();
	}
}

void RingsMapEditor::RenderTriggerVolumes(CanvasWrapper canvas)
{
	if (!IsInEditorMode())
		return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<TriggerVolume>& volume : objectManager->GetTriggerVolumes())
	{
		volume->Render(canvas, camera);
	}
}

void RingsMapEditor::RenderCheckpoints(CanvasWrapper canvas)
{
	if (!IsInEditorMode())
		return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<Checkpoint>& checkpoint : checkpoints)
	{
		checkpoint->Render(canvas, camera);
	}
}

void RingsMapEditor::RenderRings(CanvasWrapper canvas)
{
	if (!IsInEditorMode())
		return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<Ring>& ring : objectManager->GetRings())
	{
		ring->RenderTriggerVolumes(canvas, camera);
	}
}

void RingsMapEditor::RenderTimer(CanvasWrapper canvas)
{
	if (!IsInRaceMode())
		return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	if (currentCheckpoint && currentCheckpoint->IsEndCheckpoint())
		canvas.SetColor(255, 0, 0, 255); // Red color for end checkpoint
	else
		canvas.SetColor(0, 255, 0, 255); // Green color for active checkpoints

	canvas.SetPosition(Vector2{ 20, 50 });
	std::string timerText = "Time: " + std::to_string(raceTimer.GetElapsedSeconds()) + " seconds";
	canvas.DrawString(timerText, 2.f, 2.f);
}

void RingsMapEditor::RenderCanvas(CanvasWrapper canvas)
{
	if (!IsInGame())
		return;

	if (IsInEditorMode())
	{
		RenderTriggerVolumes(canvas);
		RenderCheckpoints(canvas);
		RenderRings(canvas);

		if (buildMode->IsEnabled())
		{
			buildMode->RenderCanvas(canvas);
		}
		else if (editMode->IsEnabled())
		{
			editMode->RenderCanvas(canvas);
		}
	}
	else if (IsInRaceMode())
	{
		RenderTimer(canvas);
	}
}



std::vector<MeshInfos> RingsMapEditor::GetAvailableMeshes()
{
	std::vector<MeshInfos> availableMeshes;

	for (const auto& entry : std::filesystem::directory_iterator(MeshesPath))
	{
		if (entry.path().extension() == ".json")
		{
			std::ifstream file = std::ifstream(entry.path());
			if (!file.is_open())
			{
				LOG("[ERROR]Could not open mesh config file: {}", entry.path().string());
				continue;
			}

			try
			{
				nlohmann::json meshInfos_json = nlohmann::json::parse(file);
				MeshInfos meshInfos = meshInfos_json.get<MeshInfos>();
				availableMeshes.push_back(meshInfos);
			}
			catch (const nlohmann::json::exception& e)
			{
				LOG("[ERROR]Failed to parse mesh config file: {}", e.what());
			}
		}
	}

	LOG("Found {} available meshes in {}", availableMeshes.size(), MeshesPath.string());

	return availableMeshes;
}

std::shared_ptr<Object> RingsMapEditor::FromJson_Object(const nlohmann::json& j)
{
	std::shared_ptr<Object> object = nullptr;
	ObjectType objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());

	if (objectType == ObjectType::Mesh)
		object = FromJson_Mesh(j);
	else if (objectType == ObjectType::TriggerVolume)
		object = FromJson_TriggerVolume(j);
	else if (objectType == ObjectType::Checkpoint)
		object = FromJson_Checkpoint(j);
	else if (objectType == ObjectType::Ring)
		object = FromJson_Ring(j);
	else
	{
		LOG("[ERROR]Unknown object type: {}", std::to_string(static_cast<uint8_t>(objectType)));
		return nullptr;
	}

	if (object)
	{
		object->objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());
		object->name = j.at("name").get<std::string>();
		object->location = j.at("location").get<Vector>();
		object->rotation = j.at("rotation").get<Rotator>();
		object->scale = j.at("scale").get<float>();
	}

	return object;
}

std::shared_ptr<Mesh> RingsMapEditor::FromJson_Mesh(const nlohmann::json& j)
{
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

	mesh->meshInfos = j.at("meshInfos").get<MeshInfos>();
	mesh->enableCollisions = j.at("enableCollisions").get<bool>();
	mesh->enablePhysics = j.at("enablePhysics").get<bool>();
	mesh->enableStickyWalls = j.at("enableStickyWalls").get<bool>();

	return mesh;
}

std::shared_ptr<TriggerVolume> RingsMapEditor::FromJson_TriggerVolume(const nlohmann::json& j)
{
	std::shared_ptr<TriggerVolume> triggerVolume = nullptr;
	TriggerVolumeType triggerVolumeType = static_cast<TriggerVolumeType>(j.at("triggerVolumeType").get<uint8_t>());

	if (triggerVolumeType == TriggerVolumeType::Box)
		triggerVolume = FromJson_TriggerVolume_Box(j);
	else if (triggerVolumeType == TriggerVolumeType::Cylinder)
		triggerVolume = FromJson_TriggerVolume_Cylinder(j);
	else
		throw std::runtime_error("Unknown trigger volume type: " + std::to_string(static_cast<uint8_t>(triggerVolumeType)));

	if (triggerVolume)
	{
		triggerVolume->triggerVolumeType = static_cast<TriggerVolumeType>(j.at("triggerVolumeType").get<uint8_t>());

		if (j.contains("onTouchCallback") && !j["onTouchCallback"].is_null())
		{
			if (j["onTouchCallback"].is_object())
			{
				std::string touchCallBackName = j["onTouchCallback"]["name"].get<std::string>();
				triggerVolume->SetOnTouchCallback(objectManager->GetTriggerFunctionsMap()[touchCallBackName]->CloneFromJson(j["onTouchCallback"]));
			}
		}
	}

	return triggerVolume;
}

std::shared_ptr<TriggerVolume_Box> RingsMapEditor::FromJson_TriggerVolume_Box(const nlohmann::json& j)
{
	std::shared_ptr<TriggerVolume_Box> triggerVolumeBox = std::make_shared<TriggerVolume_Box>();

	triggerVolumeBox->size = j.at("size").get<Vector>();

	return triggerVolumeBox;
}

std::shared_ptr<TriggerVolume_Cylinder> RingsMapEditor::FromJson_TriggerVolume_Cylinder(const nlohmann::json& j)
{
	std::shared_ptr<TriggerVolume_Cylinder> triggerVolumeCylinder = std::make_shared<TriggerVolume_Cylinder>();

	triggerVolumeCylinder->radius = j.at("radius").get<float>();
	triggerVolumeCylinder->height = j.at("height").get<float>();

	return triggerVolumeCylinder;
}

std::shared_ptr<Checkpoint> RingsMapEditor::FromJson_Checkpoint(const nlohmann::json& j)
{
	std::shared_ptr<Checkpoint> checkpoint = std::make_shared<Checkpoint>();

	checkpoint->checkpointId = j.at("checkpointId").get<int>();
	checkpoint->checkpointType = static_cast<CheckpointType>(j.at("checkpointType").get<uint8_t>());
	checkpoint->triggerVolume = *static_pointer_cast<TriggerVolume_Box>(FromJson_Object(j["triggerVolume"]));
	checkpoint->spawnLocation_offset = j.at("spawnLocation_offset").get<Vector>();
	checkpoint->spawnRotation = j.at("spawnRotation").get<Rotator>();

	return checkpoint;
}

std::shared_ptr<Ring> RingsMapEditor::FromJson_Ring(const nlohmann::json& j)
{
	LOG("creating ring");
	std::shared_ptr<Ring> ring = std::make_shared<Ring>();
	LOG("ring created");

	ring->ringId = j.at("ringId").get<int>();
	ring->mesh = *static_pointer_cast<Mesh>(FromJson_Object(j["mesh"]));
	ring->triggerVolumeIn = *static_pointer_cast<TriggerVolume_Cylinder>(FromJson_Object(j["triggerVolumeIn"]));
	ring->triggerVolumeOut = *static_pointer_cast<TriggerVolume_Box>(FromJson_Object(j["triggerVolumeOut"]));

	LOG("triggervolumes created");

	return ring;
}

void RingsMapEditor::DestroyAllMeshes()
{
	for (std::shared_ptr<Mesh>& mesh : objectManager->GetMeshes())
	{
		mesh->DestroyInstance();
	}
}

void RingsMapEditor::AddObject(ObjectType _objectType)
{
	objectManager->AddObject(_objectType);
	SelectLastObject();
}

void RingsMapEditor::RemoveObject(const int& _objectIndex)
{
	objectManager->RemoveObject(_objectIndex);

	if (selectedObjectIndex == _objectIndex)
		selectedObjectIndex--;

	if (selectedObjectIndex < 0 && !objectManager->GetObjects().empty())
		selectedObjectIndex = 0; // Select the first object if available
}

bool RingsMapEditor::IsInGame()
{
	return gameWrapper->IsInFreeplay() || gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();
}