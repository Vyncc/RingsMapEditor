#pragma once
#include "EditorSubMode.h"

struct RayCastHitResult
{
	bool hit = false;
	Vector extent = Vector(0, 0, 0);
	Vector hitLocation = Vector(0, 0, 0);
	Vector hitNormal = Vector(0, 0, 0);
	FTraceHitInfo traceInfo;
	AActor* hitActor = nullptr;
};

class EditMode : public EditorSubMode
{
public:
	EditMode(std::shared_ptr<ObjectManager> _objectManager);
    ~EditMode() = default;

    void Enable() override;
    void Disable() override;
    void RegisterCommands() override;
    void UnregisterCommands() override;
    void OnTick(float _deltaTime) override;
    void RenderCanvas(CanvasWrapper _canvas) override;
    void PlaceObject() override;

	void RenderCrosshair(CanvasWrapper _canvas);

    static float CalculateDistance(const Vector& pos1, const Vector& pos2);
	float CalculateDistanceToObject(const std::shared_ptr<Object>& _object);
	void SelectObject();

	RayCastHitResult RayCastActorsFromCamera();
	std::shared_ptr<Object> RayCastFromCamera();
	std::shared_ptr<Object> CheckForObjectUnderCursor();

private:
	float m_rayCast_distance = 5000.f;
	std::shared_ptr<Object> m_objectUnderCursor;
};