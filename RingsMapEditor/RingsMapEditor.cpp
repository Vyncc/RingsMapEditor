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

	gameWrapper->HookEvent("Function Engine.GameViewportClient.Tick", std::bind(&RingsMapEditor::OnTick, this, std::placeholders::_1));
	gameWrapper->RegisterDrawable(bind(&RingsMapEditor::RenderCanvas, this, std::placeholders::_1));
}

void RingsMapEditor::SetCurrentCheckpoint(std::shared_ptr<Checkpoint> _checkpoint)
{
	currentCheckpoint = _checkpoint;
	LOG("Current checkpoint set to: {} | {}", _checkpoint->id, _checkpoint->name);
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
	DataFolderPath = gameWrapper->GetDataFolder() / "Ads3D";
	if (!std::filesystem::exists(DataFolderPath))
	{
		LOG("Data folder does not exist, creating: {}", DataFolderPath.string());
		std::filesystem::create_directories(DataFolderPath);
	}

	std::string RLWin64_Path = std::filesystem::current_path().string();
	RLCookedPCConsolePath = RLWin64_Path.substr(0, RLWin64_Path.length() - 14) + "TAGame\\CookedPCConsole\\mods\\Ads3D";
	LOG("CookedPCConsole Path: {}", RLCookedPCConsolePath.string());
	if (!std::filesystem::exists(RLCookedPCConsolePath))
	{
		LOG("CookedPCConsole\\mods\\Ads3D does not exist, creating: {}", RLCookedPCConsolePath.string());
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

	for (std::shared_ptr<Checkpoint>& checkpoint : checkpoints)
	{
		for (int i = 0; i < cars.Count(); i++)
		{
			CarWrapper car = cars.Get(i);
			if (!car) continue;

			if (checkpoint->IsPointInside(car.GetLocation()))
			{
				SetCurrentCheckpoint(checkpoint);
			}
		}
	}
}

void RingsMapEditor::OnTick(std::string eventName)
{
	CheckTriggerVolumes();
}

void RingsMapEditor::RenderTriggerVolumes(CanvasWrapper canvas)
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<TriggerVolume>& volume : triggerVolumes)
	{
		volume->Render(canvas, camera);
	}
}

void RingsMapEditor::RenderCheckpoints(CanvasWrapper canvas)
{
	CameraWrapper camera = gameWrapper->GetCamera();
	if (!camera) return;

	for (std::shared_ptr<Checkpoint>& checkpoint : checkpoints)
	{
		checkpoint->Render(canvas, camera);
	}
}

void RingsMapEditor::RenderCanvas(CanvasWrapper canvas)
{
	RenderTriggerVolumes(canvas);
	RenderCheckpoints(canvas);

	CarWrapper localCar = gameWrapper->GetLocalCar();
	if (!localCar) return;

	Vector carLocation = localCar.GetLocation();
	canvas.SetPosition(canvas.ProjectF(carLocation));
	canvas.DrawBox(Vector2{ 4, 4 });
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
	SetActorLocation(spawnedKActor, _mesh.location);
	SetActorRotation(spawnedKActor, _mesh.rotation);
	SetActorScale3D(spawnedKActor, FVector{ _mesh.scale, _mesh.scale, _mesh.scale });

	_mesh.instance = spawnedKActor;

	if (_mesh.enableCollisions)
		EnableCollisions(spawnedKActor);
	else
		DisableCollisions(spawnedKActor);

	if (_mesh.enablePhysics)
		EnablePhysics(spawnedKActor);
	else
		DisablePhysics(spawnedKActor);

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

void RingsMapEditor::RemoveObject(int objectIndex)
{
	std::shared_ptr<Object> selectedObject = objects[objectIndex];
	//DestroyMesh(objects[objectIndex]);

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