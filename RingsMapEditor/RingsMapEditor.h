#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


#include "Mesh.h"
#include "TriggerFunctions.h"


class RingsMapEditor: public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

    std::filesystem::path DataFolderPath;
    std::filesystem::path RLCookedPCConsolePath;
    std::filesystem::path MeshesPath;

    std::vector<std::shared_ptr<Object>> objects;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<TriggerVolume>> triggerVolumes;
    std::vector<std::shared_ptr<TriggerFunction>> triggerFunctions = {
        std::make_shared<SetLocation>(),
        std::make_shared<SetRotation>(),
        std::make_shared<Destroy>(),
        std::make_shared<TeleportToCheckpoint>()
    };
    int selectedObjectIndex = -1;

    void SetCurrentCheckpoint(std::shared_ptr<Checkpoint> _checkpoint);

    std::shared_ptr<Object> AddObject(ObjectType _objectType);
	void SelectLastObject();

    void InitPaths();

    std::vector<MeshInfos> GetAvailableMeshes();


	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

    void CheckTriggerVolumes();
    void OnTick(std::string eventName);
    void RenderTriggerVolumes(CanvasWrapper canvas);
    void RenderCheckpoints(CanvasWrapper canvas);
    void RenderCanvas(CanvasWrapper canvas);


    void EnableCollisions(AKActor* _kActor);
    void DisableCollisions(AKActor* _kActor);
    void EnablePhysics(AKActor* _kActor);
    void DisablePhysics(AKActor* _kActor);
    void SetActorLocation(AActor* _actor, const FVector& _newLocation);
    void SetActorRotation(AActor* _actor, const FRotator& _newRotation);
    void SetActorScale3D(AActor* _actor, const FVector& _newScale3D);
    void SpawnMesh(Mesh& _mesh);
    void DestroyMesh(Mesh& _mesh);
    void RemoveObject(int objectIndex);

    bool IsInGame();

    void RenderObjectProperties(std::shared_ptr<Object>& _object);
	void RenderMeshProperties(Mesh& _mesh);
    void RenderTriggerVolumeProperties(TriggerVolume& _volume);
    void RenderCheckpointProperties(Checkpoint& _checkpoint);
    void RenderInputText(std::string _label, std::string* _value, ImGuiInputTextFlags _flags = 0);
    std::shared_ptr<Object> CopyObject(Object& _object);
    void RenderAddObjectPopup();

public:
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
