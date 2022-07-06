#define _CRT_SECURE_NO_WARNINGS

//#define DIFFUSE_TEXTURE_MAPPING

#include "gl_env.h"
#include <conio.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif

#include <iostream>

#include "skeletal_mesh.h"

#include <glm\gtc\matrix_transform.hpp>

namespace SkeletalAnimation
{
	const char * vertex_shader_450 =
		"#version 450\n"
		"const int MAX_BONES = 100;\n"
		"uniform mat4 u_bone_transf[MAX_BONES];\n"
		"uniform mat4 u_mvp;\n"
		"layout(location = 0) in vec3 in_position;\n"
		"layout(location = 1) in vec2 in_texcoord;\n"
		"layout(location = 2) in vec3 in_normal;\n"
		"layout(location = 3) in ivec4 in_bone_index;\n"
		"layout(location = 4) in vec4 in_bone_weight;\n"
		"out vec2 pass_texcoord;\n"
		"void main() {\n"
		"    float adjust_factor = 0.0;\n"
		"    for (int i = 0; i < 4; i++) adjust_factor += in_bone_weight[i] * 0.25;\n"
		"    mat4 bone_transform = mat4(1.0);\n"
		"    if (adjust_factor > 1e-3) {\n"
		"        bone_transform -= bone_transform;\n"
		"        for (int i = 0; i < 4; i++)\n"
		"            bone_transform += u_bone_transf[in_bone_index[i]] * in_bone_weight[i] / adjust_factor;\n"
		"	 }\n"
		"    gl_Position = u_mvp * bone_transform * vec4(in_position, 1.0);\n"
		"    pass_texcoord = in_texcoord;\n"
		"}\n";

	const char* fragment_shader_450 =
		"#version 450\n"
		"uniform sampler2D u_diffuse;\n"
		"in vec2 pass_texcoord;\n"
		"out vec4 out_color;\n"
		"void main() {\n"
#ifdef DIFFUSE_TEXTURE_MAPPING
		"    out_color = vec4(texture(u_diffuse, pass_texcoord).xyz, 1.0);\n"
#else
		"    out_color = vec4(pass_texcoord, 0.0, 1.0);\n"
#endif
		"}\n";
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

#define pause 0
#define victory 1
#define fist 2
#define thumb_up 3
#define wave 4
#define switch_look_up 5
#define n_place 6
#define m_place 7
#define initial_place 10

float delta_time = 0.0f;
float last_time = glfwGetTime();

typedef glm::vec3 v3;
v3 camera_pos = v3(0.0f, 0.0f, 3.0f);
v3 camera_front = v3(0.0f, 0.0f, -1.0f);
v3 camera_up = v3(0.0f, 1.0f, 0.0f);
v3 camera_right = v3(1.0f, 0.0f, 0.0f);

v3 camera_target = v3(0.0f, 0.0f, 0.0f);
v3 camera_direction = glm::normalize(camera_pos - camera_target);


int motion = 0, last_motion = 0;
int look_up = initial_place;
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	else if (key == GLFW_KEY_P && action == GLFW_PRESS) //P:victory
	{
		motion = victory; last_motion = motion;
	}
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) //Q:thumb_up
	{
		motion = thumb_up; last_motion = motion;
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) //R:fist
	{
		motion = fist; last_motion = motion;
	}
	else if (key == GLFW_KEY_E && action == GLFW_PRESS) //E:wave
	{
		motion = wave; last_motion = motion;
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) //Space:pause
	{
		if (motion != pause) motion = pause;
		else motion = last_motion;
	}

}

void camera_move(GLFWwindow* window) {
	glfwSetKeyCallback(window, key_callback);

	float move_speed = (float) 3 * delta_time;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		camera_pos -= camera_front * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		camera_pos += camera_front * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera_pos += camera_up * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera_pos -= camera_up * move_speed;
	}
}


int main(int argc, char *argv[])
{
	GLFWwindow* window;
	GLuint vertex_shader, fragment_shader, program;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(800, 800, "OpenGL output", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);


	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	if (glewInit() != GLEW_OK)
		exit(EXIT_FAILURE);

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &SkeletalAnimation::vertex_shader_450, NULL);
	glCompileShader(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &SkeletalAnimation::fragment_shader_450, NULL);
	glCompileShader(fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	int linkStatus;
	if (glGetProgramiv(program, GL_LINK_STATUS, &linkStatus), linkStatus == GL_FALSE)
		std::cout << "Error occured in glLinkProgram()" << std::endl;

	SkeletalMesh::Scene & sr = SkeletalMesh::Scene::loadScene("Hand", "Hand.fbx");
	if (&sr == &SkeletalMesh::Scene::error)
		std::cout << "Error occured in loadMesh()" << std::endl;

	sr.setShaderInput(program, "in_position", "in_texcoord", "in_normal", "in_bone_index", "in_bone_weight");

	float passed_time;
	SkeletalMesh::SkeletonModifier modifier;

	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		passed_time = clock() / double(CLOCKS_PER_SEC);
		camera_move(window);
		delta_time = glfwGetTime() - last_time;
		last_time = glfwGetTime();


		// * turn around every 4 seconds
		float metacarpals_angle = passed_time * (M_PI / 2.0);
		// * target = metacarpals
		// * rotation axis = (1, 0, 0)
		modifier["metacarpals"] = glm::rotate(glm::fmat4(), metacarpals_angle, glm::fvec3(1.0, 0.2, 0.1));

		/**********************************************************************************\
		*
		* To animate fingers, modify modifier["HAND_SECTION"] each frame,
		* where HAND_SECTION can only be one of the bone names in the Hand's Hierarchy.
		* 
		* A virtual hand's structure is like this: (slightly DIFFERENT from the real world)
		*    5432 1
		*    ....        1 = thumb           . = fingertip
		*    |||| .      2 = index finger    | = distal phalange
		*    $$$$ |      3 = middle finger   $ = intermediate phalange
		*    #### $      4 = ring finger     # = proximal phalange
		*    OOOO#       5 = pinky           O = metacarpals
		*     OOO
		* (Hand in the real world -> https://en.wikipedia.org/wiki/Hand)
		* 
		* From the structure we can infer the Hand's Hierarchy:
		*	- metacarpals
		*		- thumb_proximal_phalange
		*			- thumb_intermediate_phalange
		*				- thumb_distal_phalange
		*					- thumb_fingertip
		*		- index_proximal_phalange
		*			- index_intermediate_phalange
		*				- index_distal_phalange
		*					- index_fingertip
		*		- middle_proximal_phalange
		*			- middle_intermediate_phalange
		*				- middle_distal_phalange
		*					- middle_fingertip
		*		- ring_proximal_phalange
		*			- ring_intermediate_phalange
		*				- ring_distal_phalange
		*					- ring_fingertip
		*		- pinky_proximal_phalange
		*			- pinky_intermediate_phalange
		*				- pinky_distal_phalange
		*					- pinky_fingertip
		* 
		* Notice that modifier["HAND_SECTION"] is a local transformation matrix,
		* where (1, 0, 0) is the bone's direction, and apparently (0, 1, 0) / (0, 0, 1)
		* is perpendicular to the bone.
		* Particularly, (0, 0, 1) is the rotation axis of the nearer joint.
		*
		\**********************************************************************************/

		// Example: Animate the index finger
		// * period = 2.4 seconds
		float time_in_period = 0;
		float period = 1;
		if (motion == fist) {
			period = 1.5f;
			time_in_period = fmod(passed_time, period);
			// * angle: 0 -> PI/3 -> 0
			float thumb_angle = 1.3 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			// * target = proximal phalange of the index
			// * rotation axis = (0, 0, 1)

			//拇指
			modifier["thumb_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(-0.7, 0.07, 1.0));
			modifier["thumb_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/2, glm::fvec3(0.0, 0.0, 0.5));
			modifier["thumb_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 0.1));
			modifier["thumb_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 0.05));
			//食指
			modifier["index_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.05, 1.0));
			modifier["index_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["index_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["index_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
			//中指
			modifier["middle_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
			modifier["middle_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 2.0));
			modifier["middle_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["middle_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 4.0));
			//无名指
			modifier["ring_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.05, 1.0));
			modifier["ring_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["ring_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["ring_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
			//小指
			modifier["pinky_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.07, 1.0));
			modifier["pinky_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["pinky_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["pinky_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
		}
		else if (motion == thumb_up) {
			// * turn around every 4 seconds
			//printf("%.5f\n", metacarpals_angle);
			// * target = metacarpals
			// * rotation axis = (1, 0, 0)
			period = 2.5f;
			time_in_period = fmod(passed_time, period);
			float metacarpals_angle = 1.3 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			modifier["metacarpals"] = glm::rotate(glm::fmat4(), metacarpals_angle, glm::fvec3(0.0, 1.0, 0.0));
			// * angle: 0 -> PI/3 -> 0
			float thumb_angle = 1.3 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			// * target = proximal phalange of the index
			// * rotation axis = (0, 0, 1)

			//拇指
			modifier["thumb_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/2, glm::fvec3(0.0, 0.0, -1.0));
			modifier["thumb_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/300, glm::fvec3(1.0, 0.0, -1.0));
			modifier["thumb_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/3, glm::fvec3(0.0, 0.0, -1.0));
			modifier["thumb_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle/1000, glm::fvec3(0.0, 0.0,1.0));
			//食指
			modifier["index_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.05, 1.0));
			modifier["index_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["index_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["index_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
			//中指
			modifier["middle_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
			modifier["middle_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 2.0));
			modifier["middle_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["middle_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 4.0));
			//无名指
			modifier["ring_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.05, 1.0));
			modifier["ring_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["ring_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["ring_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
			//小指
			modifier["pinky_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.07, 1.0));
			modifier["pinky_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["pinky_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["pinky_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
		}
		else if (motion == wave) {
			// * turn around every 4 seconds
			//printf("%.5f\n", metacarpals_angle);
			// * target = metacarpals
			// * rotation axis = (1, 0, 0)
			period = 1.2f;
			time_in_period = fmod(passed_time, period);
			float metacarpals_angle = 1.5 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			//printf("%.3f %.3f\n", time_in_period, period);
			modifier["metacarpals"] = glm::rotate(glm::fmat4(), metacarpals_angle, glm::fvec3(0.0, 1.0, 0.0));
			// * angle: 0 -> PI/3 -> 0
			float thumb_angle = 1.3 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			// * target = proximal phalange of the index
			// * rotation axis = (0, 0, 1)
		}
		else if (motion == victory) {

			period = 1.5f;
			time_in_period = fmod(passed_time, period);
			// * angle: 0 -> PI/3 -> 0
			float thumb_angle = 1.3 * abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
			// * target = proximal phalange of the index
			// * rotation axis = (0, 0, 1)


			//拇指
			modifier["thumb_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(-0.7, 0.07, 1.0));
			modifier["thumb_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 0.5));
			modifier["thumb_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 0.1));
			modifier["thumb_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 0.05));
			//食指
			modifier["index_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/4, glm::fvec3(-0.5, -1.0, -0.5));
			modifier["index_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/6, glm::fvec3(0.0, 0.0, -1.0));
			modifier["index_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/6, glm::fvec3(0.0, 0.0, -1.0));
			modifier["index_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle/1000, glm::fvec3(0.0, 0.0, 1.0));
			//中指
			modifier["middle_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/4, glm::fvec3(0.0, 1.0, -0.1));
			modifier["middle_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/6, glm::fvec3(0.0, 0.0, -1.0));
			modifier["middle_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle/6, glm::fvec3(0.0, 0.0, -1.0));
			modifier["middle_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle/1000, glm::fvec3(0.0, 0.0, 1.0));
			//无名指
			modifier["ring_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.05, 1.0));
			modifier["ring_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["ring_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["ring_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
			//小指
			modifier["pinky_intermediate_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, -0.07, 1.0));
			modifier["pinky_proximal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 3.0));
			modifier["pinky_distal_phalange"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 9.0));
			modifier["pinky_fingertip"] = glm::rotate(glm::fmat4(), thumb_angle, glm::fvec3(0.0, 0.0, 27.0));
		}
		float ratio;
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		glClearColor(0.5, 0.5, 0.5, 1.0);

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//控制相机视角变化
		glUseProgram(program);
		/*
		time_in_period = fmod(passed_time, period * 5);
		printf("%.4f\n", look_at);
		*/
		glm::fmat4 mvp;
		mvp = glm::ortho(-12.5f * ratio, 12.5f * ratio, -5.f, 20.f, -20.f, 20.f);
				//* glm::lookAt(glm::fvec3(0.0f, 0.0f, -1.0f), glm::fvec3(0.0f, 0.0f, 0.0f), glm::fvec3(0.0f, 1.0f, 0.0f));

		mvp *= glm::lookAt(camera_pos, camera_target, camera_up);

		glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, (const GLfloat*)&mvp);
		glUniform1i(glGetUniformLocation(program, "u_diffuse"), SCENE_RESOURCE_SHADER_DIFFUSE_CHANNEL);
		SkeletalMesh::Scene::SkeletonTransf bonesTransf;
		sr.getSkeletonTransform(bonesTransf, modifier);
		if (!bonesTransf.empty())
			glUniformMatrix4fv(glGetUniformLocation(program, "u_bone_transf"), bonesTransf.size(), GL_FALSE, (float *)bonesTransf.data());
		sr.render();

		glfwSwapBuffers(window);
		glfwPollEvents();


	}

	SkeletalMesh::Scene::unloadScene("Hand");

	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);
}