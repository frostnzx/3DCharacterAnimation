#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 2.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Character properties
glm::vec3 characterPosition(0.0f, 0.0f, 0.0f);
glm::vec3 characterVelocity(0.0f);
float characterYaw = -90.0f;
float characterSpeed = 3.0f;
bool isJumping = false;
const float gravity = -15.0f; 
const float jumpStrength = 7.0f;
glm::vec3 moveDir(0.0f);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Playable Character", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	stbi_set_flip_vertically_on_load(true);
	glEnable(GL_DEPTH_TEST);

	camera.Zoom = 90.0f; // 0.5x zoom (doubled FOV for wider view)

	Shader ourShader("anim_model.vs", "anim_model.fs");

	// Load character model and animations
	// Pointing to a generic folder as requested, you can add your models here later
	Model characterModel(FileSystem::getPath("resources/objects/character/model.dae"));
	Animation idleAnim(FileSystem::getPath("resources/objects/character/idle.dae"), &characterModel);
	Animation walkAnim(FileSystem::getPath("resources/objects/character/walk.dae"), &characterModel);
	Animation jumpAnim(FileSystem::getPath("resources/objects/character/jump.dae"), &characterModel);
	Animator animator(&idleAnim);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// Set animation based on state
		static Animation* currentAnim = &idleAnim;
		if (isJumping) {
			if (currentAnim != &jumpAnim) {
				animator.PlayAnimation(&jumpAnim);
				currentAnim = &jumpAnim;
			}
		}
		else {
			if (glm::length(moveDir) > 0.0f) {
				if (currentAnim != &walkAnim) {
					animator.PlayAnimation(&walkAnim);
					currentAnim = &walkAnim;
				}
			}
			else {
				if (currentAnim != &idleAnim) {
					animator.PlayAnimation(&idleAnim);
					currentAnim = &idleAnim;
				}
			}
		}

		// Update physics and position
		characterPosition.x += moveDir.x * characterSpeed * deltaTime;
		characterPosition.z += moveDir.z * characterSpeed * deltaTime;

		characterVelocity.y += gravity * deltaTime; // Apply gravity
		characterPosition.y += characterVelocity.y * deltaTime; // Apply vertical movement

		// Ground check
		if (characterPosition.y < 0.0f)
		{
			characterPosition.y = 0.0f;
			characterVelocity.y = 0.0f;
			isJumping = false;
		}

		animator.UpdateAnimation(deltaTime);

		// Update camera to follow character
		glm::vec3 cameraOffset = glm::vec3(
			-cos(glm::radians(characterYaw)) * 4.0f,
			2.0f,
			-sin(glm::radians(characterYaw)) * 4.0f
		);
		camera.Position = characterPosition + cameraOffset;

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(camera.Position, characterPosition + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		auto transforms = animator.GetFinalBoneMatrices();
		for (int i = 0; i < transforms.size(); ++i)
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, characterPosition);
		model = glm::rotate(model, glm::radians(-characterYaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		ourShader.setMat4("model", model);
		characterModel.Draw(ourShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	moveDir = glm::vec3(0.0f);

	float adjustedYaw = -characterYaw + 90.0f;
	glm::vec3 forward(cos(glm::radians(adjustedYaw)), 0.0f, sin(glm::radians(adjustedYaw)));
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += forward;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= forward;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += right;

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping)
	{
		characterVelocity.y = jumpStrength;
		isJumping = true;
	}

	if (glm::length(moveDir) > 0.0f) {
		moveDir = glm::normalize(moveDir);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	lastX = xpos;

	characterYaw += xoffset * 0.1f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}