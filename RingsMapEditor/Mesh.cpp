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

void Mesh::DestroyInstance()
{
	if (instance)
	{
		instance->Destroy();
		instance = nullptr;
	}
}

UPhysicalMaterial* Mesh::GetStickyWallsPhysMaterial()
{
	return GetInstanceOfByFullName<UPhysicalMaterial>("PhysicalMaterial PhysicalMaterials.Collision_Sticky");
}