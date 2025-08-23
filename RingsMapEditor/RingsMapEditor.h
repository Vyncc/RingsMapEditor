#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


#include "ObjectManager.h"
#include "Timer.h"
#include "BuildMode.h"

enum Mode : uint8_t
{
    Editor = 0,
    Race = 1
};

class RingsMapEditor: public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	,public PluginWindowBase // Uncomment if you want to render your own plugin window
{
    std::filesystem::path DataFolderPath;
    std::filesystem::path RLCookedPCConsolePath;
    std::filesystem::path MeshesPath;
    void InitPaths();

    bool SaveConfig(const std::string& fileName);
    void LoadConfig(const std::filesystem::path& filePath);

	Mode currentMode = Mode::Editor;
    bool IsInEditorMode();
	bool IsInRaceMode();
    void StartEditorMode();
    void StartRaceMode();

    std::shared_ptr<ObjectManager> objectManager = nullptr;
    std::shared_ptr<BuildMode> buildMode = nullptr;

    Timer raceTimer;
    bool isStartingRace = false;

    int currentRingId = -1;
    int selectedObjectIndex = -1;

    void OnGameCreated(std::string eventName);
    void OnGameFirstTick(std::string eventName);
    void OnGameDestroyed(std::string eventName);
    void OnCarSpawn(CarWrapper caller, void* params, std::string eventName);

    bool SetCurrentCheckpoint(std::shared_ptr<Checkpoint> _checkpoint);
    void TeleportToCurrentCheckpoint();

	void SelectLastObject();

    std::vector<MeshInfos> AvailableMeshes;
    std::vector<MeshInfos> GetAvailableMeshes();

    std::shared_ptr<Object> FromJson_Object(const nlohmann::json& j);
    std::shared_ptr<Mesh> FromJson_Mesh(const nlohmann::json& j);
    std::shared_ptr<TriggerVolume> FromJson_TriggerVolume(const nlohmann::json& j);
    std::shared_ptr<TriggerVolume_Box> FromJson_TriggerVolume_Box(const nlohmann::json& j);
    std::shared_ptr<TriggerVolume_Cylinder> FromJson_TriggerVolume_Cylinder(const nlohmann::json& j);
    std::shared_ptr<Checkpoint> FromJson_Checkpoint(const nlohmann::json& j);
    std::shared_ptr<Ring> FromJson_Ring(const nlohmann::json& j);


	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

    void CheckTriggerVolumes();
    void CheckCheckpoints();
    void CheckRings();
    void OnTick(ActorWrapper caller, void* params, std::string eventName);
    void RenderTriggerVolumes(CanvasWrapper canvas);
    void RenderCheckpoints(CanvasWrapper canvas);
    void RenderRings(CanvasWrapper canvas);
	void RenderTimer(CanvasWrapper canvas);
    void RenderCanvas(CanvasWrapper canvas);


    void DestroyAllMeshes();
    void AddObject(ObjectType _objectType);
    void RemoveObject(const int& _objectIndex);

    bool IsInGame();

    void RenderProperties_Object(std::shared_ptr<Object>& _object);
	void RenderProperties_Mesh(Mesh& _mesh);
    void RenderProperties_TriggerVolume(std::shared_ptr<TriggerVolume>& _volume);
    void RenderProperties_TriggerVolume_Box(TriggerVolume_Box& _volume);
    void RenderProperties_TriggerVolume_Cylinder(TriggerVolume_Cylinder& _volume);
    void RenderProperties_Checkpoint(Checkpoint& _checkpoint);
    void RenderProperties_Ring(std::shared_ptr<Ring>& _ring);
    void RenderInputText(std::string _label, std::string* _value, ImGuiInputTextFlags _flags = 0);
    void CopyObject(Object& _object);
    void RenderAddObjectPopup();
    void RenderSaveConfigPopup();
    void RenderLoadConfigPopup();

public:
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
