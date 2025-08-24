#pragma once

#include "ObjectManager.h"

class EditorSubMode
{
public:
	EditorSubMode(std::shared_ptr<ObjectManager> _objectManager) {
		m_objectManager = _objectManager;

		fname_flyCam = FName(_globalGameWrapper->GetFNameIndexByString("Fly"));
		rightShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_RightShoulder");
		leftShoulder = _globalGameWrapper->GetFNameIndexByString("XboxTypeS_LeftShoulder");
	}

	virtual ~EditorSubMode() = default;


public:
	virtual void Enable() = 0;
	virtual void Disable() = 0;
	virtual void OnTick(float _deltaTime) = 0;
	virtual void RenderObjectsCanvas(CanvasWrapper _canvas) = 0;
	virtual void RenderCanvas(CanvasWrapper _canvas) = 0;
	virtual void PlaceObject() = 0;

public:
	void Toggle() {
		if (!m_enabled)
			Enable();
		else
			Disable();
	}

	bool IsInGame() const {
		return _globalGameWrapper->IsInFreeplay() || _globalGameWrapper->IsInGame() || _globalGameWrapper->IsInOnlineGame();
	}

	bool IsSpectator() const {
		PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
		if (!pc)
			return false;

		PriWrapper pri = pc.GetPRI();
		if (!pri)
			return false;

		return pri.IsSpectator();
	}

	bool IsEnabled() {
		return m_enabled;
	}

	void HookEvents() {
		_globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode", [this](ActorWrapper caller, void* params, std::string evenName) {
			UGFxData_ReplayViewer_TA_execSetCameraMode_Params* param = reinterpret_cast<UGFxData_ReplayViewer_TA_execSetCameraMode_Params*>(params);
			param->Mode = fname_flyCam;
			});

		_globalGameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_ReplayViewer_TA.SetFOV", [this](ActorWrapper caller, void* params, std::string evenName) {
			float* param = reinterpret_cast<float*>(params);
			*param = 90.f;
			});
	}

	void UnhookEvents() {
		_globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetCameraMode");
		_globalGameWrapper->UnhookEvent("Function TAGame.GFxData_ReplayViewer_TA.SetFOV");
	}

	void Spectate() {
		PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
		if (!pc)
		{
			LOG("[ERROR]pc NULL!");
			return;
		}

		pc.Spectate();
	}

	void SwitchToFlyCam() {
		PlayerControllerWrapper pc = _globalGameWrapper->GetPlayerController();
		if (!pc)
		{
			LOG("[ERROR]pc NULL!");
			return;
		}

		SpectatorHUDWrapper spectatorHUD = pc.GetSpectatorHud();
		if (!spectatorHUD)
		{
			LOG("[ERROR]spectatorHUD NULL!");
			return;
		}

		ReplayViewerDataWrapper replayViewer = spectatorHUD.GetViewerData();
		if (!replayViewer)
		{
			LOG("[ERROR]replayViewer NULL!");
			return;
		}

		replayViewer.SetCameraMode("Fly");
	}

protected:
	bool m_enabled = false;
	std::shared_ptr<ObjectManager> m_objectManager = nullptr;

	float m_previewObjectDistance = 1400.f;

	float m_scalePerSec = 0.5f;
	float m_rotationDegreesPerSec = 90.f;
	float m_sizeUnitsPerSec = 500.f;
	float m_radiusUnitsPerSec = 500.f;
	float m_heightUnitsPerSec = 500.f;

	FName fname_flyCam;
	int rightShoulder = 0;
	int leftShoulder = 0;
};