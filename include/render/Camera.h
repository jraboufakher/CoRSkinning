#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
	Camera();
	~Camera() = default;

	glm::mat4 getViewMatrix() const;

	void processMouseButton(int button, int action, double x, double y);
	void processCursorPos(double xpos, double ypos);
	void processScroll(double yoffset);
	
private:
	// Spherical Coords
	float yaw_, pitch_, radius_;

	// Orbit Vector
	glm::vec3 target_;

	// Inputs
	bool rotating_, dragging_;
	double lastX_, lastY_;

	// Sensitivity
	const float yawSpeed = 0.005f;
	const float pitchSpeed = 0.005f;
	const float zoomSpeed = 1.0f;
	const float dragSpeed = 0.005f;
};

