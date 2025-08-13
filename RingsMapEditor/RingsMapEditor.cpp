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

	//cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
	//	LOG("Hello notifier!");
	//}, "", 0);

	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	LOG("Your hook got called and the ball went POOF");
	//});

	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.PostBeginPlay", std::bind(&RingsMapEditor::OnGameCreated, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&RingsMapEditor::OnGameDestroyed, this, std::placeholders::_1));

	//gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.PostBeginPlay", std::bind(&RingsMapEditor::OnCarSpawn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.OnVehicleSetup", std::bind(&RingsMapEditor::OnCarSpawn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEvent("Function Engine.GameViewportClient.Tick", std::bind(&RingsMapEditor::OnTick, this, std::placeholders::_1));
	gameWrapper->RegisterDrawable(bind(&RingsMapEditor::RenderCanvas, this, std::placeholders::_1));
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
		for (const std::shared_ptr<Object>& object : objects) {
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

	objects.clear();

	try
	{
		nlohmann::json arr;
		file >> arr;
		
		for (const auto& item : arr)
		{
			std::shared_ptr<Object> loadedObject = FromJson_Object(item);
			objects.push_back(loadedObject);

			if (loadedObject->objectType == ObjectType::Mesh)
				meshes.push_back(std::static_pointer_cast<Mesh>(loadedObject));
			else if(loadedObject->objectType == ObjectType::TriggerVolume)
				triggerVolumes.push_back(std::static_pointer_cast<TriggerVolume>(loadedObject));
			else if(loadedObject->objectType == ObjectType::Checkpoint)
				checkpoints.push_back(std::static_pointer_cast<Checkpoint>(loadedObject));
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

	gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole\\mods\\RingsMapEditor\\Meshes\\ringsmapeditor.upk?Game=TAGame.GameInfo_Soccar_TA?GameTags=Freeplay");
		});
}

void RingsMapEditor::StartRaceMode()
{
	currentMode = Mode::Race;
	isStartingRace = true;
	raceTimer.Reset();

	gameWrapper->Execute([this](GameWrapper* gw) {
		gw->ExecuteUnrealCommand("start ringsmapeditor.upk?Game=TAGame.GameInfo_Soccar_TA?GameTags=Freeplay");
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
			for (std::shared_ptr<Object>& object : objects)
			{
				if (object->objectType == ObjectType::Mesh)
				{
					std::shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(object);
					SpawnMesh(*mesh);
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
		LOG("Current checkpoint set to: {} | {}", _checkpoint->id, _checkpoint->name);
		return true;
	}

	return false;
}

std::shared_ptr<Object> RingsMapEditor::AddObject(ObjectType _objectType)
{
	if (_objectType == ObjectType::Mesh)
	{
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>("New Mesh");
		objects.emplace_back(newMesh);
		meshes.emplace_back(newMesh);
		SelectLastObject();
		return newMesh;
	}
	else if (_objectType == ObjectType::TriggerVolume)
	{
		std::shared_ptr<TriggerVolume> newTriggerVolume = std::make_shared<TriggerVolume>();
		objects.emplace_back(newTriggerVolume);
		triggerVolumes.emplace_back(newTriggerVolume);
		SelectLastObject();
		return newTriggerVolume;
	}
	else if (_objectType == ObjectType::Checkpoint)
	{
		std::shared_ptr<Checkpoint> newCheckpoint = std::make_shared<Checkpoint>();
		newCheckpoint->id = checkpoints.size() + 1; // Assign a new ID based on the current size
		objects.emplace_back(newCheckpoint);
		checkpoints.emplace_back(newCheckpoint);
		SelectLastObject();
		return newCheckpoint;
	}
	else
	{
		LOG("[ERROR] Unsupported object type: {}", static_cast<int>(_objectType));
		return nullptr;
	}
}

void RingsMapEditor::SelectLastObject()
{
	if (!objects.empty())
	{
		selectedObjectIndex = objects.size() - 1;
	}
	else
	{
		selectedObjectIndex = -1;
	}
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

void RingsMapEditor::CheckTriggerVolumes()
{
	if (!IsInRaceMode())
		return;

	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return;

	ArrayWrapper<CarWrapper> cars = server.GetCars();

	for (std::shared_ptr<TriggerVolume>& volume : triggerVolumes)
	{
		for (int i = 0; i < cars.Count(); i++)
		{
			CarWrapper car = cars.Get(i);
			if (!car) continue;

			if (volume->IsPointInside(car.GetLocation()))
			{
				volume->OnTouch(car);
			}
		}
	}

}

void RingsMapEditor::CheckCheckpoints()
{
	if (!IsInRaceMode())
		return;

	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return;

	ArrayWrapper<CarWrapper> cars = server.GetCars();

	for (std::shared_ptr<Checkpoint>& checkpoint : checkpoints)
	{
		for (int i = 0; i < cars.Count(); i++)
		{
			CarWrapper car = cars.Get(i);
			if (!car) continue;

			if (checkpoint->IsPointInside(car.GetLocation()))
			{
				if (SetCurrentCheckpoint(checkpoint))
				{
					if (checkpoint->IsStartCheckpoint())
					{
						raceTimer.Start();
						LOG("starting timer");
					}
					else if (checkpoint->IsEndCheckpoint())
					{
						raceTimer.Stop();
					}
				}
			}
		}
	}
}

void RingsMapEditor::OnTick(std::string eventName)
{
	if (!IsInRaceMode())
		return;

	CheckTriggerVolumes();
	CheckCheckpoints();
}

void RingsMapEditor::RenderTriggerVolumes(CanvasWrapper canvas)
{
	if (!IsInEditorMode())
		return;

	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<TriggerVolume>& volume : triggerVolumes)
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
	ObjectType objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());

	if (objectType == ObjectType::Mesh)
		return FromJson_Mesh(j);
	else if(objectType == ObjectType::TriggerVolume)
		return FromJson_TriggerVolume(j);
	else if(objectType == ObjectType::Checkpoint)
		return FromJson_Checkpoint(j);

	throw std::runtime_error("Unknown object type: " + std::to_string(static_cast<uint8_t>(objectType)));
}

std::shared_ptr<Mesh> RingsMapEditor::FromJson_Mesh(const nlohmann::json& j)
{
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

	mesh->objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());
	mesh->name = j.at("name").get<std::string>();
	mesh->location = j.at("location").get<FVector>();
	mesh->rotation = j.at("rotation").get<FRotator>();
	mesh->scale = j.at("scale").get<float>();
	mesh->meshInfos = j.at("meshInfos").get<MeshInfos>();
	mesh->enableCollisions = j.at("enableCollisions").get<bool>();
	mesh->enablePhysics = j.at("enablePhysics").get<bool>();
	mesh->enableStickyWalls = j.at("enableStickyWalls").get<bool>();

	return mesh;
}

std::shared_ptr<TriggerVolume> RingsMapEditor::FromJson_TriggerVolume(const nlohmann::json& j)
{
	std::shared_ptr<TriggerVolume> triggerVolume = std::make_shared<TriggerVolume>();

	triggerVolume->objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());
	triggerVolume->name = j.at("name").get<std::string>();
	triggerVolume->location = j.at("location").get<FVector>();
	triggerVolume->rotation = j.at("rotation").get<FRotator>();
	triggerVolume->scale = j.at("scale").get<float>();
	triggerVolume->size = j.at("size").get<FVector>();
	triggerVolume->vertices = j.at("vertices").get<std::array<FVector, 8>>();

	if (j.contains("onTouchCallback") && !j["onTouchCallback"].is_null())
	{
		if (j["onTouchCallback"].is_object())
		{
			std::string touchCallBackName = j["onTouchCallback"]["name"].get<std::string>();
			triggerVolume->SetOnTouchCallback(triggerFunctionsMap[touchCallBackName]->CloneFromJson(j["onTouchCallback"]));
		}
	}

	return triggerVolume;
}

std::shared_ptr<Checkpoint> RingsMapEditor::FromJson_Checkpoint(const nlohmann::json& j)
{
	std::shared_ptr<Checkpoint> checkpoint = std::make_shared<Checkpoint>();

	checkpoint->objectType = static_cast<ObjectType>(j.at("objectType").get<uint8_t>());
	checkpoint->name = j.at("name").get<std::string>();
	checkpoint->location = j.at("location").get<FVector>();
	checkpoint->rotation = j.at("rotation").get<FRotator>();
	checkpoint->scale = j.at("scale").get<float>();
	checkpoint->size = j.at("size").get<FVector>();
	checkpoint->vertices = j.at("vertices").get<std::array<FVector, 8>>();

	if (j.contains("onTouchCallback") && !j["onTouchCallback"].is_null())
	{
		if (j["onTouchCallback"].is_object())
		{
			std::string touchCallBackName = j["onTouchCallback"]["name"].get<std::string>();
			checkpoint->SetOnTouchCallback(triggerFunctionsMap[touchCallBackName]->CloneFromJson(j["onTouchCallback"]));
		}
	}

	checkpoint->id = j.at("id").get<int>();
	checkpoint->type = static_cast<CheckpointType>(j.at("type").get<uint8_t>());
	checkpoint->spawnLocation = j.at("spawnLocation").get<Vector>();
	checkpoint->spawnRotation = j.at("spawnRotation").get<Rotator>();

	return checkpoint;
}

void RingsMapEditor::EnableCollisions(AKActor* _kActor)
{
	if (!_kActor)
	{
		LOG("[ERROR]AKActor is null!");
		return;
	}

	UStaticMeshComponent* collisionComp = _kActor->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	collisionComp->SetRBChannel(ERBCollisionChannel::RBCC_Pawn);
	collisionComp->SetBlockRigidBody(true);
}

void RingsMapEditor::DisableCollisions(AKActor* _kActor)
{
	if (!_kActor)
	{
		LOG("[ERROR]AKActor is null!");
		return;
	}

	UStaticMeshComponent* collisionComp = _kActor->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	collisionComp->SetRBChannel(ERBCollisionChannel::RBCC_Nothing);
	collisionComp->SetBlockRigidBody(false);
}

void RingsMapEditor::EnablePhysics(AKActor* _kActor)
{
	if (_kActor)
		_kActor->SetPhysics(EPhysics::PHYS_RigidBody);
}

void RingsMapEditor::DisablePhysics(AKActor* _kActor)
{
	if (_kActor)
		_kActor->SetPhysics(EPhysics::PHYS_SoftBody);
}

void RingsMapEditor::SetActorLocation(AActor* _actor, const FVector& _newLocation)
{
	if (!_actor)
	{
		LOG("[ERROR]AKActor is null!");
		return;
	}

	_actor->SetLocation(_newLocation);

	UPrimitiveComponent* collisionComp = _actor->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetRBPosition(_newLocation, FName());
	}
}

void RingsMapEditor::SetActorRotation(AActor* _actor, const FRotator& _newRotation)
{
	if (!_actor)
	{
		LOG("[ERROR]AKActor is null!");
		return;
	}

	_actor->SetRotation(_newRotation);

	UPrimitiveComponent* collisionComp = _actor->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetRBRotation(_newRotation, FName());
	}
}

//Not working for the collision component
void RingsMapEditor::SetActorScale3D(AActor* _actor, const FVector& _newScale3D)
{
	if (!_actor)
	{
		LOG("[ERROR]AKActor is null!");
		return;
	}

	_actor->SetDrawScale3D(_newScale3D);

	UPrimitiveComponent* collisionComp = _actor->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetScale3D(_newScale3D);
	}
}

void RingsMapEditor::SpawnMesh(Mesh& _mesh)
{
	if (!IsInGame())
	{
		LOG("[ERROR]You must be in a game to spawn obejcts!");
		return;
	}

	if (_mesh.IsMeshPathEmpty())
	{
		LOG("[ERROR]{} : Mesh path is empty!", _mesh.name);
		return;
	}

	UStaticMesh* loadedObject = reinterpret_cast<UStaticMesh*>(UObject::DynamicLoadObject(StringToFString(_mesh.meshInfos.meshPath), UStaticMesh::StaticClass(), 1));
	if (!loadedObject)
	{
		LOG("[ERROR]Couldn't load object : {}", _mesh.meshInfos.meshPath);
		return;
	}

	LOG("Loaded object successfully : {}", loadedObject->GetFullName());

	AKActor* KActorDefault = GetDefaultInstanceOf<AKActorSpawnable>();
	if (!KActorDefault)
	{
		LOG("[ERROR]KActor default NULL");
		return;
	}

	AKActorSpawnable* spawnedKActor = reinterpret_cast<AKActorSpawnable*>(KActorDefault->SpawnInstance(NULL, FName(0), _mesh.location, _mesh.rotation, 0));
	if (!spawnedKActor)
	{
		LOG("[ERROR]spawnedKActor NULL");
		return;
	}

	spawnedKActor->SetStaticMesh(loadedObject, FVector{ 0.f, 0.f, 0.f }, FRotator{ 0, 0, 0 }, FVector{ 1.f, 1.f, 1.f });

	_mesh.instance = spawnedKActor;

	_mesh.SetLocation(_mesh.location);
	_mesh.SetRotation(_mesh.rotation);
	_mesh.SetScale3D(FVector{ _mesh.scale, _mesh.scale, _mesh.scale });


	if (_mesh.enableCollisions)
		_mesh.EnableCollisions();
	else
		_mesh.DisableCollisions();

	if (_mesh.enablePhysics)
		_mesh.EnablePhysics();
	else
		_mesh.DisablePhysics();

	if (_mesh.enableStickyWalls)
		_mesh.EnableStickyWalls();
	else
		_mesh.DisableStickyWalls();

	LOG("Spawned object successfully : {}", _mesh.name);
}

void RingsMapEditor::DestroyMesh(Mesh& _mesh)
{
	if (_mesh.instance)
	{
		_mesh.instance->Destroy();
		_mesh.instance = nullptr;
	}
}

void RingsMapEditor::DestroyAllMeshes()
{
	for (std::shared_ptr<Mesh>& mesh : meshes)
	{
		DestroyMesh(*mesh);
	}
}

void RingsMapEditor::RemoveObject(int objectIndex)
{
	std::shared_ptr<Object> selectedObject = objects[objectIndex];

	if (selectedObject->objectType == ObjectType::Mesh)
	{
		std::shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(selectedObject);
		DestroyMesh(*mesh);
		meshes.erase(std::remove(meshes.begin(), meshes.end(), mesh), meshes.end());
		LOG("Removed mesh: {}", mesh->name);
	}
	else if (selectedObject->objectType == ObjectType::TriggerVolume)
	{
		std::shared_ptr<TriggerVolume> triggerVolume = std::static_pointer_cast<TriggerVolume>(selectedObject);
		triggerVolume->SetOnTouchCallback(nullptr); // Clear the callback to avoid dangling pointers
		triggerVolumes.erase(std::remove(triggerVolumes.begin(), triggerVolumes.end(), triggerVolume), triggerVolumes.end());
		LOG("Removed trigger volume: {}", triggerVolume->name);
	}
	else if (selectedObject->objectType == ObjectType::Checkpoint)
	{
		std::shared_ptr<Checkpoint> chekpoint = std::static_pointer_cast<Checkpoint>(selectedObject);
		chekpoint->SetOnTouchCallback(nullptr); // Clear the callback to avoid dangling pointers
		checkpoints.erase(std::remove(checkpoints.begin(), checkpoints.end(), chekpoint), checkpoints.end());
		LOG("Removed checkpoint: {}", chekpoint->name);
	}

	objects.erase(objects.begin() + objectIndex);

	if (selectedObjectIndex == objectIndex)
		selectedObjectIndex--;

	if (selectedObjectIndex < 0 && !objects.empty())
		selectedObjectIndex = 0; // Select the first object if available
}

bool RingsMapEditor::IsInGame()
{
	return gameWrapper->IsInFreeplay() || gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame();
}