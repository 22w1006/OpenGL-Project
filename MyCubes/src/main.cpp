#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

#include <iostream>
#include <fstream>
#include <string>

void window_focus_callback(GLFWwindow* window, int focused);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int loadTexture(const char* path);
void setShaderLight(Shader& shader, glm::vec3 pointLightPositions[]);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

#define ROOM_BOARDER 25.f
// camera
static Camera init_camera(glm::vec3(20.0f, -20.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -135.0f, 30.0f, ROOM_BOARDER);
static Camera center_camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, ROOM_BOARDER);
Camera camera = init_camera;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

bool available_dirLight = true;
bool available_spotLight = false;
bool available_pointLight[4] = { true,false,false,false };
short lightnum = 0;
glm::vec3 lightColor[4] = {
    glm::vec3(1.f, 1.f, 1.f),
    glm::vec3(1.f, 0.f, 0.f),
    glm::vec3(0.f, 1.f, 0.f),
    glm::vec3(0.f, 0.f, 1.f),
};


//direction of cube's V, E, F
static glm::vec3 V[8] = {
    glm::vec3( 1., 1., 1.),
    glm::vec3( 1., 1.,-1.),
    glm::vec3( 1.,-1., 1.),
    glm::vec3( 1.,-1.,-1.),
    glm::vec3(-1., 1., 1.),
    glm::vec3(-1., 1.,-1.),
    glm::vec3(-1.,-1., 1.),
    glm::vec3(-1.,-1.,-1.),
};
static glm::vec2 E[4] = {
    glm::vec2( 1., 1.),
    glm::vec2( 1.,-1.),
    glm::vec2(-1., 1.),
    glm::vec2(-1.,-1.),
};
static float F[2] = { 1,-1 };
#define LENGTH_OF_CUBE 22
void draw_Cube(Shader shader, int length);
void draw_Cube_with_inner(Shader shader, int length);



//theta and phi of ball's particles(all radians)
#define BALL_PARTICLE_NUM 10710 //PARTICLE_IN_RADIUS(31)
#define PARTICLE_IN_RADIUS(x)  (x)*(x+1)*(2*x+1)
static glm::vec2 ball_thph[BALL_PARTICLE_NUM];
#define RADIUS_OF_BALL 17
void generate_ball_particles();
void draw_Ball(Shader shader, int radius);
void draw_Ball_with_inner(Shader shader, int radius);



enum shapes
{
    BALL,CUBE,PYRAMID
};//pyramid temporarily unavailable
shapes past_shape = CUBE;
shapes current_shape = CUBE;
shapes should_shape = CUBE;

#define ORIGIN_TOTAL_SCALE 0.75f
#define ORIGIN_CUBE_SCALE 0.1f

float TOTAL_SCALE = 0.75f;
float CUBE_SCALE = 0.1f;

float scalechange_starttime = 0.f;

bool is_expanding = false;
bool is_shrinking = false;

void change_shape_check(float currenttime);
void expand(float currenttime);
void shrink(float currenttime);

bool is_rotating = false;
float rotatingtime = 0.f;
float rotatingoffset = 0.f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader littleCubeShader("src/colors.vs", "src/colors.fs");
    Shader lightCubeShader("src/light_cube.vs", "src/light_cube.fs");
    Shader roomShader("src/room.vs", "src/room.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // initialize point lights
    float points[12] = { 0 };
    short pointnum = 0;
    std::ifstream configfile("src/configs.txt");
    if (!configfile.is_open())
    {
        std::cout << "Failed to open configs file (configs.txt)" << std::endl;
        return -1;
    }
    for (; pointnum < 12; pointnum++)
    {
        configfile >> points[pointnum];
        if (!configfile.good())
            break;
    }
    configfile.close();
    // positions of the point lights

    glm::vec3 pointLightPositions[] = {
        glm::vec3(.0f,  .0f,  .0f),
        glm::vec3(.0f,  .0f,  .0f),
        glm::vec3(.0f,  .0f,  .0f),
        glm::vec3(.0f,  .0f,  .0f),
    };

    lightnum = pointnum / 3;
    for (int i = 0; i < lightnum; i++)
        pointLightPositions[i] = glm::vec3(points[3 * i], points[3 * i + 1], points[3 * i + 2]);

    // first, configure the cube's VAO (and VBO)
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // third, configure the room's VAO (VBO stays the same; the vertices are the same for the room object which is also a 3D cube)
    unsigned int roomCubeVAO;
    glGenVertexArrays(1, &roomCubeVAO);
    glBindVertexArray(roomCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------

    unsigned int RoomTexture = loadTexture("src/container.jpg");
    
    // shader configuration
    // --------------------
    roomShader.use();
    roomShader.setInt("material.diffuse", 0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();


        // be sure to activate shader when setting uniforms/drawing objects
        littleCubeShader.use();
        littleCubeShader.setVec3("viewPos", camera.Position);
        littleCubeShader.setVec3("material.diffuse", 1., 1., 1.);
        littleCubeShader.setVec3("material.specular", 1., 1., 1.);
        littleCubeShader.setFloat("material.shininess", 32.0f);
        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        setShaderLight(littleCubeShader, pointLightPositions);
        
        // view/projection transformations
        littleCubeShader.setMat4("projection", projection);
        littleCubeShader.setMat4("view", view);

        // render containers
        glBindVertexArray(cubeVAO);
        change_shape_check(currentFrame);
        if (current_shape == CUBE)
            draw_Cube_with_inner(littleCubeShader, LENGTH_OF_CUBE);
        if (current_shape == BALL)
            draw_Ball_with_inner(littleCubeShader, RADIUS_OF_BALL);

        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        // we now draw as many light bulbs as we have point lights.
        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < lightnum; i++)
        {
            if (available_pointLight[i])
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, pointLightPositions[i]);
                model = glm::scale(model, glm::vec3(0.1f)); // Make it a smaller cube
                lightCubeShader.setMat4("model", model);
                lightCubeShader.setVec3("lightColor", lightColor[i]);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }



        // draw the room
        roomShader.use();
        roomShader.setVec3("viewPos", camera.Position);
        roomShader.setFloat("material.shininess", 32.0f);

        /*
           Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index
           the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
           by defining light types as classes and set their values in there, or by using a more efficient uniform approach
           by using 'Uniform buffer objects', but that is something we'll discuss in the 'Advanced GLSL' tutorial.
        */
        setShaderLight(roomShader, pointLightPositions);

        // view/projection transformations
        roomShader.setMat4("projection", projection);
        roomShader.setMat4("view", view);

        // bind map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, RoomTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, RoomTexture);

        roomShader.setMat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(2 * ROOM_BOARDER)));

        // render containers
        glBindVertexArray(roomCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 30);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}
// glfw: whenever the status whether a key is pressed or released is changed this callback function executes
// ---------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        camera = init_camera;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        camera = center_camera;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        available_dirLight = !available_dirLight;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        available_spotLight = !available_spotLight;
    if (should_shape == past_shape)
    {
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            should_shape = CUBE;
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        {
            if (current_shape != BALL)
                generate_ball_particles();
            should_shape = BALL;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        available_pointLight[0] = !available_pointLight[0];
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        available_pointLight[1] = !available_pointLight[1];
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        available_pointLight[2] = !available_pointLight[2];
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        available_pointLight[3] = !available_pointLight[3];
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        is_rotating = !is_rotating;
        if (is_rotating)
            rotatingtime = glfwGetTime();
        else
            rotatingoffset = glfwGetTime() - rotatingtime + rotatingoffset;
    }

}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// glfw: whenever the focus to the window changes, this callback is called
// ----------------------------------------------------------------------
void window_focus_callback(GLFWwindow* window, int focused)
{
    if (!focused)
    {
        firstMouse = true;
    }
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
void setShaderLight(Shader& shader, glm::vec3 pointLightPositions[])
{
    // directional light
    shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    shader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

    shader.setBool("available_pointLight[0]", available_pointLight[0]);
    shader.setBool("available_pointLight[1]", available_pointLight[1]);
    shader.setBool("available_pointLight[2]", available_pointLight[2]);
    shader.setBool("available_pointLight[3]", available_pointLight[3]);
    shader.setBool("available_dirLight", available_dirLight);
    shader.setBool("available_spotLight", available_spotLight);

    std::string names[7] = {
        "pointLights[0].position",
        "pointLights[0].ambient",
        "pointLights[0].diffuse",
        "pointLights[0].specular",
        "pointLights[0].constant",
        "pointLights[0].linear",
        "pointLights[0].quadratic"
    };
    for (int i = 0; i < lightnum; i++)
    {
        // point light i
        shader.setVec3(names[0], pointLightPositions[i]);
        shader.setVec3(names[1], 0.05f * lightColor[i]);
        shader.setVec3(names[2], 0.8f * lightColor[i]);
        shader.setVec3(names[3], 1.0f * lightColor[i]);
        shader.setFloat(names[4], 1.0f);
        shader.setFloat(names[5], 0.09f);
        shader.setFloat(names[6], 0.032f);
        for (int j = 0; j < 7; j++)
        {
            names[j][12]++;
        }
    }
    // spotLight
    shader.setVec3("spotLight.position", camera.Position);
    shader.setVec3("spotLight.direction", camera.Front);
    shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    shader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
    shader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09f);
    shader.setFloat("spotLight.quadratic", 0.032f);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

}

float getRotateDegree()
{
    if (is_rotating)
        return glfwGetTime() - rotatingtime + rotatingoffset;
    else
        return rotatingoffset;
}

void draw_Cube(Shader shader, int length)
{
    // calculate the model matrix for each object and pass it to shader before drawing
    for (int i = 0; i < 8; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 place = TOTAL_SCALE * float(length - 1) / 2 * V[i];
        model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::translate(model, place);
        //float angle = 20.0f * (i % 10);
        //model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
        shader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    for (int i = 0; i < 4; i++)
    {
        for (int j = 1; j <= length - 2; j++)
        {
        glm::vec2 line = TOTAL_SCALE * float(length - 1) / 2 * E[i];

        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 place = glm::vec3(line.x, line.y, TOTAL_SCALE * (float(1 - length + 2 * j) / 2));
        model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::translate(model, place);
        model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        place = glm::vec3(line.x, TOTAL_SCALE * (float(1 - length + 2 * j) / 2), line.y);
        model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::translate(model, place);
        model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        place = glm::vec3(TOTAL_SCALE * (float(1 - length + 2 * j) / 2), line.x, line.y);
        model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::translate(model, place);
        model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    for (int i = 0; i < 2; i++)
    {
        for (int j = 1; j <= length - 2; j++)
        {
            for (int k = 1; k <= length - 2; k++)
            {
                float face = TOTAL_SCALE * float(length - 1) / 2 * F[i];

                glm::mat4 model = glm::mat4(1.0f);
                glm::vec3 place = glm::vec3(
                    TOTAL_SCALE * (float(1 - length + 2 * j) / 2),
                    TOTAL_SCALE * (float(1 - length + 2 * k) / 2),
                    face
                );
                model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
                model = glm::translate(model, place);
                model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                model = glm::mat4(1.0f);
                place = glm::vec3(
                    TOTAL_SCALE * (float(1 - length + 2 * j) / 2),
                    face,
                    TOTAL_SCALE * (float(1 - length + 2 * k) / 2)
                );
                model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
                model = glm::translate(model, place);
                model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                model = glm::mat4(1.0f);
                place = glm::vec3(
                    face,
                    TOTAL_SCALE * (float(1 - length + 2 * j) / 2),
                    TOTAL_SCALE * (float(1 - length + 2 * k) / 2)
                );
                model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
                model = glm::translate(model, place);
                model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
                shader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }

}
void draw_Cube_with_inner(Shader shader, int length)
{
    for (unsigned int i = 1; i <= length; i++)
    {
        draw_Cube(shader,i);
    }
}

void change_shape_check(float currenttime)
{
    if (should_shape == past_shape)
        return;
    else if (current_shape != should_shape && !is_shrinking)
    {
        is_shrinking = true;
        scalechange_starttime = currenttime;
        shrink(currenttime);
    }
    else if (is_shrinking)
    {
        shrink(currenttime);
    }
    else if (!is_shrinking && !is_expanding)
    {
        is_expanding = true;
        scalechange_starttime = currenttime;
        expand(currenttime);
    }
    else if (is_expanding)
    {
        expand(currenttime);
    }
}
void expand(float currenttime)//true if expand still running
{
    if (currenttime - scalechange_starttime < glm::radians(90.f))
        TOTAL_SCALE = ORIGIN_TOTAL_SCALE * sin(currenttime - scalechange_starttime) / sin(1.f);
    else if (currenttime - scalechange_starttime < glm::radians(150.f))
    {
        TOTAL_SCALE = ORIGIN_TOTAL_SCALE * (1.f + (1.f / sin(1.f) - 1.f) * (1.f + cos(3 * (currenttime - scalechange_starttime - glm::radians(90.f)))) / 2.f);
    }
    else
    {
        TOTAL_SCALE = ORIGIN_TOTAL_SCALE;
        is_expanding = false;
        past_shape = should_shape;
    }
}
void shrink(float currenttime)//true if expand still running
{
    TOTAL_SCALE = ORIGIN_TOTAL_SCALE * sin(currenttime - scalechange_starttime + 1) / sin(1.f);
    if (TOTAL_SCALE < 0)
    {
        TOTAL_SCALE = 0;
        is_shrinking = false;
        current_shape = should_shape;
    }
}

float floatrand()//return a random float in [0,1]
{
    return float(rand()) / float(RAND_MAX);
}

void generate_ball_particles()
{
    for (int i = 0; i < BALL_PARTICLE_NUM; i++)
    {
        ball_thph[i] = glm::vec2(glm::radians(360.f) * floatrand(), glm::radians(180.f) * floatrand());
    }
}
void draw_Ball(Shader shader, int radius)
{
    int startnum = PARTICLE_IN_RADIUS(radius - 1);//actual-1
    int finalnum = PARTICLE_IN_RADIUS(radius);
    for (int i = startnum; i < finalnum; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 angle = glm::vec3(
            sin(ball_thph[i].y) * sin(ball_thph[i].x),
            cos(ball_thph[i].y),
            sin(ball_thph[i].y) * cos(ball_thph[i].x)
        );
        glm::vec3 place = float(radius) * angle * 0.85f * TOTAL_SCALE;
        model = glm::rotate(model, getRotateDegree(), glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::translate(model, place);
        model = glm::rotate(model, ball_thph[i].x, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, ball_thph[i].y, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(CUBE_SCALE)); // Make it a smaller cube
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}
void draw_Ball_with_inner(Shader shader, int radius)
{
    for (unsigned int i = 1; i <= radius; i++)
    {
        draw_Ball(shader, i);
    }
}
