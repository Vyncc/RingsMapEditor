#pragma once
#include "Object.h"

struct MeshInfos
{
    MeshInfos() {}
    MeshInfos(std::string _name, std::string _meshPath) : name(_name), meshPath(_meshPath) {}

    std::string name;
    std::string meshPath;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshInfos, name, meshPath)

inline std::vector<MeshInfos> AvailableMeshes;

class Mesh : public Object
{
public:
    Mesh() {
        objectType = ObjectType::Mesh;
        name = "Mesh";
        location = Vector(0);
        rotation = Rotator(0);
        scale = 1.f;
		meshInfos = MeshInfos();
    }

    Mesh(std::string _name, MeshInfos _meshInfos = MeshInfos(), Vector _location = { 0.f, 0.f, 0.f }, Rotator _rotation = { 0, 0, 0 }, float _scale = 1.f) {
        objectType = ObjectType::Mesh;
        name = _name;
        location = _location;
        rotation = _rotation;
        scale = _scale;
		meshInfos = _meshInfos;
    }
    ~Mesh() {}

	bool IsSpawned() const;
	bool HasCollisionMesh() const;
	bool IsMeshPathEmpty() const;

	void EnableCollisions();
	void DisableCollisions();
	void EnablePhysics();
	void DisablePhysics();
    void EnableStickyWalls();
    void DisableStickyWalls();
	void SetLocation(const FVector& _newLocation);
	void SetLocation(const Vector& _newLocation) override;
	void SetRotation(const FRotator& _newRotation);
	void SetRotation(const Rotator& _newRotation) override;
	void SetScale3D(const FVector& _newScale3D);
	void SetScale3D(const Vector& _newScale3D);

    static UPhysicalMaterial* GetStickyWallsPhysMaterial();

    nlohmann::json to_json() const override {
        return nlohmann::json {
            {"objectType", static_cast<uint8_t>(objectType)},
            {"name", name},
            {"location", location},
            {"rotation", rotation},
            {"scale", scale},
            {"meshInfos", meshInfos},
            {"enableCollisions", enableCollisions},
            {"enablePhysics", enablePhysics},
            {"enableStickyWalls", enableStickyWalls},
        };
    }

	std::shared_ptr<Object> Clone() override {
		std::shared_ptr<Mesh> clonedMesh = std::make_shared<Mesh>(*this);
		clonedMesh->instance = nullptr;
		return clonedMesh;
	}

    MeshInfos meshInfos;
    AKActor* instance = nullptr;
	bool enableCollisions = false;
	bool enablePhysics = false;
    bool enableStickyWalls = false;
};