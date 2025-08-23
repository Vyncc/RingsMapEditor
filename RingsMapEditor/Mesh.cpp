#include "pch.h"
#include "Mesh.h"


bool Mesh::IsSpawned() const
{
	return instance != nullptr;
}

bool Mesh::HasCollisionMesh() const
{
	if (IsSpawned())
	{
		UStaticMeshComponent* meshComp = instance->StaticMeshComponent;
		if (meshComp)
		{
			return meshComp->BodyInstance != nullptr;
		}
	}
}

bool Mesh::IsMeshPathEmpty() const
{
	return meshInfos.meshPath.empty();
}

bool Mesh::IsInGame() const
{
	return _globalGameWrapper->IsInFreeplay() || _globalGameWrapper->IsInGame() || _globalGameWrapper->IsInOnlineGame();
}

void Mesh::SpawnInstance()
{
	if (!IsInGame())
	{
		LOG("[ERROR]You must be in a game to spawn objects!");
		return;
	}

	if (IsMeshPathEmpty())
	{
		LOG("[ERROR]{} : Mesh path is empty!", name);
		return;
	}

	UStaticMesh* loadedMesh = reinterpret_cast<UStaticMesh*>(UObject::DynamicLoadObject(StringToFString(meshInfos.meshPath), UStaticMesh::StaticClass(), 1));
	if (!loadedMesh)
	{
		LOG("[ERROR]Couldn't load mesh : {} | {}", meshInfos.name, meshInfos.meshPath);
		return;
	}

	LOG("Loaded mesh successfully : {}", loadedMesh->GetFullName());

	AKActor* KActorDefault = GetDefaultInstanceOf<AKActorSpawnable>();
	if (!KActorDefault)
	{
		LOG("[ERROR]AKActorSpawnable default NULL");
		return;
	}

	AKActorSpawnable* spawnedKActor = reinterpret_cast<AKActorSpawnable*>(KActorDefault->SpawnInstance(NULL, FName(0), GetFVectorLocation(), GetFRotatorRotation(), 0));
	if (!spawnedKActor)
	{
		LOG("[ERROR]spawnedKActor NULL");
		return;
	}

	instance = spawnedKActor;
	SetStaticMesh(loadedMesh);
	SetLocation(location);
	SetRotation(rotation);
	SetScale3D(FVector{ scale, scale, scale });

	if (enableCollisions)
		EnableCollisions();
	else
		DisableCollisions();

	if (enablePhysics)
		EnablePhysics();
	else
		DisablePhysics();

	if (enableStickyWalls)
		EnableStickyWalls();

	LOG("Spawned object successfully : {}", name);
}

void Mesh::DestroyInstance()
{
	if (instance)
	{
		instance->Destroy();
		instance = nullptr;
	}
}

void Mesh::EnableCollisions()
{
	enableCollisions = true;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	UStaticMeshComponent* collisionComp = instance->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	collisionComp->SetRBChannel(ERBCollisionChannel::RBCC_Pawn);
	collisionComp->SetBlockRigidBody(true);
}

void Mesh::DisableCollisions()
{
	enableCollisions = false;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	UStaticMeshComponent* collisionComp = instance->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	collisionComp->SetRBChannel(ERBCollisionChannel::RBCC_Nothing);
	collisionComp->SetBlockRigidBody(false);
}

void Mesh::EnablePhysics()
{
	enablePhysics = true;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetPhysics(EPhysics::PHYS_RigidBody);
}

void Mesh::DisablePhysics()
{
	enablePhysics = false;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetPhysics(EPhysics::PHYS_SoftBody);
}

void Mesh::EnableStickyWalls()
{
	enableStickyWalls = true;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	UStaticMeshComponent* collisionComp = instance->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	UPhysicalMaterial* stickyWalls = GetStickyWallsPhysMaterial();
	if (!stickyWalls)
	{
		LOG("[ERROR]Couldn't find sticky walls!");
		return;
	}

	collisionComp->SetPhysMaterialOverride(stickyWalls);
}

void Mesh::DisableStickyWalls()
{
	enableStickyWalls = false;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	UStaticMeshComponent* collisionComp = instance->StaticMeshComponent;
	if (!collisionComp)
	{
		LOG("[ERROR]Collision component is null!");
		return;
	}

	collisionComp->SetPhysMaterialOverride(nullptr);
}

void Mesh::SetLocation(const FVector& _newLocation)
{
	location = Object::FVectorToVector(_newLocation);

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetLocation(_newLocation);

	UPrimitiveComponent* collisionComp = instance->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetRBPosition(_newLocation, FName());
	}
}

void Mesh::SetLocation(const Vector& _newLocation)
{
	SetLocation(VectorToFVector(_newLocation));
}

void Mesh::SetRotation(const FRotator& _newRotation)
{
	rotation = Object::FRotatorToRotator(_newRotation);

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetRotation(_newRotation);

	UPrimitiveComponent* collisionComp = instance->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetRBRotation(_newRotation, FName());
	}
}

void Mesh::SetRotation(const Rotator& _newRotation)
{
	SetRotation(RotatorToFRotator(_newRotation));
}

//Not working for the collision component
void Mesh::SetScale3D(const FVector& _newScale3D)
{
	scale = _newScale3D.X;

	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetDrawScale3D(_newScale3D);

	UPrimitiveComponent* collisionComp = instance->CollisionComponent;
	if (collisionComp)
	{
		collisionComp->SetScale3D(_newScale3D);
	}
}

void Mesh::SetScale3D(const Vector& _newScale3D)
{
	SetScale3D(VectorToFVector(_newScale3D));
}

void Mesh::SetStaticMesh(UStaticMesh* _staticMesh)
{
	if (!IsSpawned())
	{
		LOG("[ERROR]{} instance is null!", name);
		return;
	}

	instance->SetStaticMesh(_staticMesh, FVector{ 0.f, 0.f, 0.f }, FRotator{ 0, 0, 0 }, FVector{ 1.f, 1.f, 1.f });
	LOG("Set mesh : {}", _staticMesh->GetFullName());
}

void Mesh::SetStaticMesh(const MeshInfos& _meshInfos)
{
	if (!IsInGame())
	{
		LOG("[ERROR]You must be in a game to spawn objects!");
		return;
	}

	if (IsMeshPathEmpty())
	{
		LOG("[ERROR]{} : Mesh path is empty!", _meshInfos.name);
		return;
	}

	UStaticMesh* loadedMesh = reinterpret_cast<UStaticMesh*>(UObject::DynamicLoadObject(StringToFString(_meshInfos.meshPath), UStaticMesh::StaticClass(), 1));
	if (!loadedMesh)
	{
		LOG("[ERROR]Couldn't load mesh : {} | {}", _meshInfos.name, _meshInfos.meshPath);
		return;
	}

	LOG("Loaded mesh successfully : {}", loadedMesh->GetFullName());

	instance->SetStaticMesh(loadedMesh, FVector{ 0.f, 0.f, 0.f }, FRotator{ 0, 0, 0 }, FVector{ 1.f, 1.f, 1.f });
	LOG("Set mesh : {}", loadedMesh->GetFullName());

	meshInfos = _meshInfos;
}

UPhysicalMaterial* Mesh::GetStickyWallsPhysMaterial()
{
	return GetInstanceOfByFullName<UPhysicalMaterial>("PhysicalMaterial PhysicalMaterials.Collision_Sticky");
}