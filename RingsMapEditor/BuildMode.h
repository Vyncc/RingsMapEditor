#pragma once

#include "ObjectManager.h"

class BuildMode
{
public:
    BuildMode(std::shared_ptr<ObjectManager> _objectManager, std::vector<MeshInfos> _availableMeshes) {
        m_enabled = false;
        m_objectManager = _objectManager;
        m_meshes = _availableMeshes;
    }
    ~BuildMode() {}

    void ToggleBuildMode() {
        if (m_enabled)
            DisableBuildMode();
        else
            EnableBuildMode();
    }

    void EnableBuildMode() {
        m_previewActor = SpawnPreviewActor();
        if (m_previewActor)
            LOG("Spawned preview actor successfully!");
        else
            LOG("[ERROR]Couldn't spawn preview actor!");

        m_enabled = true;
        LOG("Build mode activated");
    }

    void DisableBuildMode() {
        m_enabled = false;
        LOG("Build mode disabled");
    }

    AKActor* SpawnPreviewActor() {
        AKActor* KActorDefault = GetDefaultInstanceOf<AKActorSpawnable>();
        if (!KActorDefault)
        {
            LOG("[ERROR]AKActorSpawnable default NULL");
            return nullptr;
        }

        AKActorSpawnable* spawnedKActor = reinterpret_cast<AKActorSpawnable*>(KActorDefault->SpawnInstance(NULL, FName(0), FVector{ 0.f, 0.f, 0.f }, FRotator{ 0, 0, 0 }, 0));
        if (!spawnedKActor)
        {
            LOG("[ERROR]spawnedKActor NULL");
            return nullptr;
        }

        MarkInvincible(spawnedKActor);
        return spawnedKActor;
    }

    bool IsInGame()
    {
        return _globalGameWrapper->IsInFreeplay() || _globalGameWrapper->IsInGame() || _globalGameWrapper->IsInOnlineGame();
    }

    void OnTick() {
        if (!IsInGame()) return;
        if (!m_previewActor) return;

        //set location in front of camera
    }

    bool m_enabled = false;
    std::shared_ptr<ObjectManager> m_objectManager = nullptr;
    std::vector<MeshInfos> m_meshes;
    int m_meshesIndex = 0;
    AKActor* m_previewActor = nullptr;
    Vector previewActorLocation;
    Rotator previewActorRotation;

private:
};