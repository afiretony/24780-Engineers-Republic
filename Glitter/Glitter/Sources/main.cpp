// OpenCity Simulator
// Group project of 21fall 24780B Engineering Computation
// Carnegie Mellon University
// main function
// more info: https://github.com/afiretony/24780-Engineers-Republic

// include basic library
#include <string>
#include <iostream>
#include <filesystem>

// include graphics and sound library(others)
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <shader_m.h>
#include <model.h>
#include "ysglfontdata.h"
#include "StringPlus.h"
#include "yssimplesound.h"
#include "yspng.h"
#include <limits.h>

// test multi thread
#include <thread>

// include our own library
#include "flightcontrol.h"
#include <camera.h>
#include "SimObject.h"
#include "obstacleavoid.h"
#include "map.h"
#include "detector.h"

// include gui library
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
// #include "imgui_impl_opengl3_loader.h"
//#define STB_IMAGE_IMPLEMENTATION

#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, uav* UAV_fc);
void GameWindow(string Path_to_Project);
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);
void StartInterface(const char* glsl_version, string Path_to_Project);
void EndInterface(const char* glsl_version, string Path_to_Project);

GLFWwindow* NewGUIWindow();
unsigned int loadCubemap(vector<std::string> faces);

// settings
unsigned int SCR_WIDTH = 1024;
unsigned int SCR_HEIGHT = 768;

// camera
Camera camera(glm::vec3(0.f, 10.f, 10.f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// interface control
int model_selection = 0;
bool GameTerminate = false;

string getCurrentDir() {
    char buff[MAX_PATH];
    GetModuleFileName(NULL, buff, MAX_PATH);
    string::size_type position = string(buff).find_last_of("\\/");
    return string(buff).substr(0, position);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main()
{
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // find path to project for different user
    string Path_to_Project = getCurrentDir();
    cout << "Current working directory : " << Path_to_Project << endl;
    // when compiling using visual studio, current directory is usually .../[Path to your project]/Glitter/Build/Debug
    // we can use feature to cd to shader files

    string t = "Glitter";
    string::size_type i = Path_to_Project.find(t);
    // delete everything after(including) "Glitter"
    if (i != string::npos)
        Path_to_Project.erase(i, Path_to_Project.length() - i);
    cout << "Path to Project is: " << Path_to_Project << endl;
    std::replace(Path_to_Project.begin(), Path_to_Project.end(), '\\', '/');
    
    while (!GameTerminate) {
        // load user interface
        StartInterface(glsl_version, Path_to_Project);

        // load Game
        GameWindow(Path_to_Project);

        // load user interface
        EndInterface(glsl_version, Path_to_Project);
    }
    
    
    return 0;
}

GLFWwindow* NewGUIWindow() {
    // Create window with graphics context
    // GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        exit(-1);
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    SCR_WIDTH = mode->width;
    SCR_HEIGHT = mode->height;
    // real full screen, but not good for Zoom share screen
    // glfwSetWindowMonitor(window, primary, 0, 0, SCR_WIDTH, SCR_HEIGHT, mode->refreshRate);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Start GUI", NULL, NULL);

    if (window == NULL)
        exit(-1);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    glfwMakeContextCurrent(window);


    //// tell GLFW to capture our mouse
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //// glad: load all OpenGL function pointers
    //// ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    //// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    return window;
}

void StartInterface(const char* glsl_version, string Path_to_Project)
{

    GLFWwindow* window = NewGUIWindow();


    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    // Our state
    int page = 0;
    bool show_demo_window = false;
    bool show_another_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    /****** Load Images for the start-up menu ***************/
    //Load our texture for background
    int my_image_width = 0;
    int my_image_height = 0;
    GLuint bg_img_texture = 0;
    GLuint title_img_texture = 0;
    GLuint start_btn_img_texture = 0;
    GLuint selection_img_texture = 0;


    GLuint model1_img_texture = 0;
    GLuint model2_img_texture = 0;
    GLuint model3_img_texture = 0;
    GLuint model4_img_texture = 0;

    GLuint model1btn_img_texture = 0;
    GLuint model2btn_img_texture = 0;
    GLuint model3btn_img_texture = 0;
    GLuint model4btn_img_texture = 0;
    GLuint modelconfirmbtn_img_texture = 0;

    GLuint instruction_img_texture = 0;

    bool ret = LoadTextureFromFile((Path_to_Project + string("/figures/city_nobg.png")).c_str(), &bg_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/start_interface-removebg.png")).c_str(), &title_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/start_button_nobg.png")).c_str(), &start_btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/MyImage01.jpg")).c_str(), &selection_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    // four model textures
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model1.png")).c_str(), &model1_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model2.png")).c_str(), &model2_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model3.png")).c_str(), &model3_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model4.png")).c_str(), &model4_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    // button textures for four models
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model1_btn.png")).c_str(), &model1btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model2_btn.png")).c_str(), &model2btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model3_btn.png")).c_str(), &model3btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/model4_btn.png")).c_str(), &model4btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/confirm_btn.png")).c_str(), &modelconfirmbtn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    // instruction texture
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/instruction.png")).c_str(), &instruction_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    bool show_dropdown_menu = false;


    // Main loop
    while (!glfwWindowShouldClose(window))
    {


        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        {
            static int counter = 0;

            ImVec2 pos_offset(SCR_WIDTH / 2, SCR_HEIGHT / 2); // center location of the window

            ImVec2 titleWin_size(SCR_WIDTH * 0.8, SCR_HEIGHT * 0.8);  // size of the window
            ImVec2 titleWin_pos(pos_offset.x - titleWin_size.x / 2, pos_offset.y - titleWin_size.y / 2);

            ImVec2 modelWin_size(SCR_WIDTH * 0.8, SCR_HEIGHT * 0.6);
            ImVec2 modelWin_pos(pos_offset.x - modelWin_size.x / 2 - 500, pos_offset.y - modelWin_size.y / 2 - 200);

            ImVec2 titleImgSize(titleWin_size.x, titleWin_size.y * 0.8);
            ImVec2 START_btnSize(titleWin_size.x * 0.15, titleWin_size.y * 0.15);

            ImVec2 modelImgSize(titleWin_size.x * 0.6, titleWin_size.y * 0.6);


            switch (page) {
            case 0:
                // =============================================================== START Interface =================================================================
                   // title window
                ImGui::SetNextWindowPos(titleWin_pos);
                ImGui::SetNextWindowSize(titleWin_size);
                ImGui::Begin("Title", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
                ImGui::Image((void*)(intptr_t)title_img_texture, titleImgSize);
                ImGui::SetCursorPosX(pos_offset.x - START_btnSize.x);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - START_btnSize.y);
                if (ImGui::ImageButton((ImTextureID)start_btn_img_texture, START_btnSize)) {
                    cout << "I Love 24780 soooooo much!!!! Start selecting model!";
                    page++;
                }
                ImGui::End();

                //Load the bg into ImGui
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("OpenGL Texture Text", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
                ImGui::Image((void*)(intptr_t)bg_img_texture, ImVec2(SCR_WIDTH, SCR_HEIGHT));
                ImGui::End();
                break;
            case 1:
                // =============================================================== Selection Interface ============================================================================

                    // Load models
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("UAV selection", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

                // buttom 1
                // check model 1
                ImGui::SetCursorPosX(pos_offset.x + 1.5 * START_btnSize.x);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + START_btnSize.y);
                //float button1_y = ImGui::GetCursorPosY();
                if (ImGui::ImageButton((ImTextureID)model1btn_img_texture, START_btnSize)) {
                    model_selection = 1;
                }
                // button 2
                // check model 2
                ImGui::SetCursorPosX(pos_offset.x + 1.5 * START_btnSize.x);
                if (ImGui::ImageButton((ImTextureID)model2btn_img_texture, START_btnSize)) {
                    cout << "button 2 pushed" << endl;
                    model_selection = 2;
                }
                // button 3
                ImGui::SetCursorPosX(pos_offset.x + 1.5 * START_btnSize.x);
                if (ImGui::ImageButton((ImTextureID)model3btn_img_texture, START_btnSize)) {
                    model_selection = 3;
                    //    counter++;
                    //    imgui::sameline();
                    //    imgui::text("counter = %d", counter);
                }
                // button 4
                ImGui::SetCursorPosX(pos_offset.x + 1.5 * START_btnSize.x);
                if (ImGui::ImageButton((ImTextureID)model4btn_img_texture, START_btnSize)) {
                    model_selection = 4;
                }

                // start game btn
                ImGui::SetCursorPosX(pos_offset.x - 0.5 * START_btnSize.x);
                ImGui::SetCursorPosY(SCR_HEIGHT * 0.7);
                if (ImGui::ImageButton((ImTextureID)modelconfirmbtn_img_texture, START_btnSize)) {

                    cout << "I Love 24780 soooooo much!!!! Model confirmed!";
                    page++;
                }

                switch (model_selection) {
                case 1:
                    ImGui::SetCursorPos(ImVec2(SCR_WIDTH * 0.1, SCR_HEIGHT * 0.1));
                    ImGui::Image((void*)(intptr_t)model1_img_texture, modelImgSize);
                    break;
                case 2:
                    cout << "bottom 2 image show" << endl;
                    ImGui::SetCursorPos(ImVec2(SCR_WIDTH * 0.1, SCR_HEIGHT * 0.1));
                    ImGui::Image((void*)(intptr_t)model2_img_texture, modelImgSize);
                    break;
                case 3:
                    ImGui::SetCursorPos(ImVec2(SCR_WIDTH * 0.1, SCR_HEIGHT * 0.1));
                    ImGui::Image((void*)(intptr_t)model3_img_texture, modelImgSize);
                    break;

                case 4:
                    ImGui::SetCursorPos(ImVec2(SCR_WIDTH * 0.1, SCR_HEIGHT * 0.1));
                    ImGui::Image((void*)(intptr_t)model4_img_texture, modelImgSize);
                    break;
                }

                ImGui::End();
                break;

            case 2:
                // show instruction

                ImGui::StyleColorsLight();
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                //ImGui::SetNextWindowSize(ImVec2(0.9*SCR_WIDTH, 0.9* SCR_HEIGHT));
                ImGui::Begin("Title", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
                ImGui::Image((void*)(intptr_t)instruction_img_texture, ImVec2(SCR_WIDTH, 0.9 * SCR_HEIGHT));
                // ImGui::Image((void*)(intptr_t)instruction_img_texture, titleImgSize);
                ImGui::SetCursorPosX(pos_offset.x - 0.5 * START_btnSize.x);
                ImGui::SetCursorPosY(pos_offset.y + 2 * START_btnSize.y);
                ImGui::StyleColorsClassic();
                // close the gui window and start the game window
                if (ImGui::ImageButton((ImTextureID)start_btn_img_texture, START_btnSize)) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::End();
                break;
            }


        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        // glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        // glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }



    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
void EndInterface(const char* glsl_version, string Path_to_Project)
{

    GLFWwindow* window = NewGUIWindow();


    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    /****** Load Images for the start-up menu ***************/
    //Load our texture for background
    int my_image_width = 0;
    int my_image_height = 0;
    GLuint bg_img_texture = 0;
    GLuint title_img_texture = 0;
    GLuint start_btn_img_texture = 0;
    GLuint selection_img_texture = 0;
    GLuint restart_btn_img_texture = 0;
    GLuint exit_btn_img_texture = 0;


    GLuint instruction_img_texture = 0;

    bool ret = LoadTextureFromFile((Path_to_Project + string("/figures/city_nobg.png")).c_str(), &bg_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/start_interface-removebg.png")).c_str(), &title_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/start_button_nobg.png")).c_str(), &start_btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/restart.png")).c_str(), &restart_btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile((Path_to_Project + string("/figures/exit.png")).c_str(), &exit_btn_img_texture, &my_image_width, &my_image_height);
    IM_ASSERT(ret);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {


        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        {
            static int counter = 0;

            ImVec2 pos_offset(SCR_WIDTH / 2, SCR_HEIGHT / 2); // center location of the window

            ImVec2 titleWin_size(SCR_WIDTH * 0.8, SCR_HEIGHT * 0.8);  // size of the window
            ImVec2 titleWin_pos(pos_offset.x - titleWin_size.x / 2, pos_offset.y - titleWin_size.y / 2);

            ImVec2 modelWin_size(SCR_WIDTH * 0.8, SCR_HEIGHT * 0.6);
            ImVec2 modelWin_pos(pos_offset.x - modelWin_size.x / 2 - 500, pos_offset.y - modelWin_size.y / 2 - 200);

            ImVec2 titleImgSize(titleWin_size.x, titleWin_size.y * 0.8);
            ImVec2 START_btnSize(titleWin_size.x * 0.15, titleWin_size.y * 0.15);

            ImVec2 modelImgSize(titleWin_size.x * 0.6, titleWin_size.y * 0.6);

            // end window
            ImGui::SetNextWindowPos(titleWin_pos);
            ImGui::SetNextWindowSize(titleWin_size);
            ImGui::Begin("END", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::Image((void*)(intptr_t)title_img_texture, titleImgSize);
            ImGui::SetCursorPosX(SCR_WIDTH*0.2 - START_btnSize.x*0.5);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - START_btnSize.y);
            if (ImGui::ImageButton((ImTextureID)exit_btn_img_texture, START_btnSize)) {
                glfwSetWindowShouldClose(window, true);
                GameTerminate = true;
            }
            ImGui::SetCursorPosX(SCR_WIDTH * 0.6 - START_btnSize.x * 0.5);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - START_btnSize.y);
            if (ImGui::ImageButton((ImTextureID)restart_btn_img_texture, START_btnSize)) {
                glfwSetWindowShouldClose(window, true);
                GameTerminate = false;
            }
            ImGui::End();

            //Load the bg into ImGui
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("OpenGL Texture Text", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::Image((void*)(intptr_t)bg_img_texture, ImVec2(SCR_WIDTH, SCR_HEIGHT));
            ImGui::End();

        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        // glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        // glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }



    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    //int tmp = GL_TEXTURE_2D;
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

void GameWindow(string Path_to_Project)
{

    string Path_to_Shader1 = Path_to_Project + "Glitter/Glitter/Shaders/modelvs.vs";
    const char* path1 = Path_to_Shader1.c_str(); // convert string to char, because Shader class input has to be char
    string Path_to_Shader2 = Path_to_Project + "Glitter/Glitter/Shaders/modelfs.fs";
    const char* path2 = Path_to_Shader2.c_str();

    //------------------------------skybox------------------------------
    string Path_to_Shader3 = Path_to_Project + "Glitter/Glitter/Shaders/skyboxvs.vs";
    const char* path3 = Path_to_Shader3.c_str(); // convert string to char, because Shader class input has to be char
    string Path_to_Shader4 = Path_to_Project + "Glitter/Glitter/Shaders/skyboxfs.fs";
    const char* path4 = Path_to_Shader4.c_str();

    string Path_to_skybox_right = Path_to_Project + "Glitter/Glitter/Model/Skybox/right.png";
    const char* skypath1 = Path_to_skybox_right.c_str();
    string Path_to_skybox_left = Path_to_Project + "Glitter/Glitter/Model/Skybox/left.png";
    const char* skypath2 = Path_to_skybox_left.c_str();
    string Path_to_skybox_top = Path_to_Project + "Glitter/Glitter/Model/Skybox/top.png";
    const char* skypath3 = Path_to_skybox_top.c_str();
    string Path_to_skybox_bottom = Path_to_Project + "Glitter/Glitter/Model/Skybox/bottom.png";
    const char* skypath4 = Path_to_skybox_bottom.c_str();
    string Path_to_skybox_front = Path_to_Project + "Glitter/Glitter/Model/Skybox/front.png";
    const char* skypath5 = Path_to_skybox_front.c_str();
    string Path_to_skybox_back = Path_to_Project + "Glitter/Glitter/Model/Skybox/back.png";
    const char* skypath6 = Path_to_skybox_back.c_str();
    //------------------------------skybox------------------------------


    // load uav model
    // Declear UAV Model
    string Path_to_Model1 = Path_to_Project + "Glitter/Glitter/Model/UAV2/uploads_files_893841_drone.obj";
    string Path_to_Model2 = Path_to_Project + "Glitter/Glitter/Model/UAV2/warship.obj";
    string Path_to_Model3 = Path_to_Project + "Glitter/Glitter/Model/UAV2/tie_UV.obj";
    string Path_to_Model4 = Path_to_Project + "Glitter/Glitter/Model/Gloden_snitch/Golden_Snitch.obj";

    string Path_to_Sound1 = Path_to_Project + "Glitter/Glitter/Sounds/UAV1.wav";
    string Path_to_Sound2 = Path_to_Project + "Glitter/Glitter/Sounds/Kylo.wav";
    string Path_to_Sound3 = Path_to_Project + "Glitter/Glitter/Sounds/tie.wav";
    string Path_to_Sound4 = Path_to_Project + "Glitter/Glitter/Sounds/golden2.wav";

    glfwInit();
    // Use OpenGL version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);  // Enables sRGB framebuffer capability
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Engineering Republic", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    // tell GLFW to make the context of our window the main context on the current thread
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);  // Actually enables sRGB framebuffer

    // load UAV sound effect
    YsSoundPlayer player1;
    YsSoundPlayer::SoundData myWav1;

    // store the filename of music
    string fileNames[] = { Path_to_Sound1, Path_to_Sound2, Path_to_Sound3, Path_to_Sound4 };

    if (true) {
        // build and compile shaders
        // -------------------------

        Shader ourShader(path1, path2);
        cout << "shader loaded" << endl;

        // load flight control and dynamics model
        // SimObject init: file path, scalar, position
        string Path_to_Model, Path_to_Sound;
        switch (model_selection) {
        case 1:
            Path_to_Model = Path_to_Model1;
            Path_to_Sound = Path_to_Sound1;
            break;
        case 2:
            Path_to_Model = Path_to_Model2;
            Path_to_Sound = Path_to_Sound2;
            break;
        case 3:
            Path_to_Model = Path_to_Model3;
            Path_to_Sound = Path_to_Sound3;
            break;
        case 4:
            Path_to_Model = Path_to_Model4;
            Path_to_Sound = Path_to_Sound4;
            break;
        }
        auto UAV_fc = new uav(Path_to_Model, glm::vec3(0.005f, 0.005f, 0.005f), glm::vec3(-10.0f, 0.1f, -0.0f));  //second glm change initial landed location of UAV

        //------------------------------skybox------------------------------
        Shader skyboxShader(path3, path4);

        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        // skybox VAO
        unsigned int skyboxVAO, skyboxVBO;
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        string test = getCurrentDir();

        vector<std::string> faces{skypath1, skypath2, skypath3, skypath4, skypath5, skypath6};
        unsigned int cubemapTexture = loadCubemap(faces);

        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);

        //------------------------------skybox------------------------------


        // initialize UAV sound effects
        //set initial volume at first display of UAV
        float volume;
        // load user choice, note use of .c_str()
        if (YSOK == myWav1.LoadWav(Path_to_Sound.c_str())) {
            cout << "sounds here" << endl;
            player1.Start();
            player1.SetVolume(myWav1, 0.5);
            player1.PlayBackground(myWav1);
        }
        else {
            cout << "Failed to read " << "UAV1.wav" << endl;
        }

        // City model
        auto cityMap = new Map(14, 14, Path_to_Project);

        // Obstacle detector
        detector avoidControl(UAV_fc, cityMap);

        //intialize point of view status
        bool firstPOV = true;
        bool thirdPOV = true;
        bool freePOV = true;

        // camera parameters
        float distance = 0.5;
        glm::vec3 cameraPosition;
        float cameraYaw, cameraPitch, cameraRoll;
        // render loop
        // -----------
        int last_frame = 0;
        int current_frame = 0;

        while (!glfwWindowShouldClose(window))
        {

            // program dynamic UAV sound effect volume
            if (UAV_fc->GetNormVel() > 0)
                volume = UAV_fc->GetNormVel() / 2 + 0.5;
            else
                volume = 0.5;
            //cout << UAV_fc.GetNormVel() << endl;
            player1.SetVolume(myWav1, volume);
            // per-frame time logic
            // --------------------

            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            UAV_fc->getdeltatime(deltaTime);
            // cout << currentFrame << endl;

            // input
            // -----
            processInput(window, UAV_fc);

            // obstacle avoid
            // -----
            avoidControl.uav_control();

            // render
            // ------
            //glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
            glClearColor(0.8f, 0.5f, 0.f, 1.0f);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            // enable shader before setting uniforms
            ourShader.use();

            // camera/view transformation
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();

            // view/projection transformations
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);


            // specificy Z and X key to switch between first and third POV
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                firstPOV = true;
                thirdPOV = false;
                freePOV = false;
            }
            else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
            {
                thirdPOV = true;
                firstPOV = false;
                freePOV = false;
            }
            else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                thirdPOV = false;
                firstPOV = false;
                freePOV = true;
                camera.Pitch = 0.f;
            }

            //distance from camera to UAV
            cameraPosition = UAV_fc->getUavPos();
            cameraYaw = UAV_fc->getUavTwist().y;
            cameraPitch = UAV_fc->getUavTwist().x;
            cameraRoll = UAV_fc->getUavTwist().z;

            last_frame = currentFrame;

            if (thirdPOV) {
                // draw UAV (physical update is integrated in class)
                
                camera.Position.x = cameraPosition.x - distance * cosf(cameraYaw);//cos
                camera.Position.y = cameraPosition.y + distance;//maybe add tilt with UAV
                camera.Position.z = cameraPosition.z + distance * sinf(cameraYaw); //sin
                camera.rotateWithUAV(-cameraYaw);
                camera.Pitch = -30.f;
                currentFrame++;
                //camera.tiltHorizontalWithUAV(cameraPitch - 45.f);
               
            }
            else if(firstPOV) {
                // or perhaps write a unity function with verying distance (firstPOV distance=0, else distance=0.5)
                camera.Position = UAV_fc->getUavPos();
                camera.rotateWithUAV(-cameraYaw);
                camera.Pitch = 0.f;
                currentFrame++;
                //camera.tiltHorizontalWithUAV(cameraPitch);
                //camera.tiltVerticalWithUAV(-cameraRoll);
            }
            else if (freePOV) {
                camera.Position = UAV_fc->getUavPos();
                camera.Position.y -= 0.15;
                currentFrame++;
            }
            while (last_frame == currentFrame);

            // use shader again to project using updated camera position 
               // enable shader before setting uniforms
            ourShader.use();

            // camera/view transformation
            projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            view = camera.GetViewMatrix();

            // view/projection transformations
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            if(!firstPOV)
                UAV_fc->Draw(ourShader);


            // draw city
            for (auto& grid : cityMap->grids_map) {
                grid.block->Draw(ourShader);
            }

            //------------------------------skybox------------------------------
            // draw skybox as last
            glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
            skyboxShader.use();
            view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);
            // skybox cube
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS); // set depth function back to default
            //------------------------------skybox------------------------------


            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(window);
            glfwPollEvents();

            /*cout << UAV_fc.getUavPos().x << "  " << UAV_fc.getUavPos().y << "  " << UAV_fc.getUavPos().z << endl;*/
        }
    }




    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, uav* UAV_fc)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        UAV_fc->forward();
    }
        
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        UAV_fc->backward();
    }
        
         
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        UAV_fc->left();
    }
        
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        UAV_fc->right();
    }
        
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        UAV_fc->up();
    }
        
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        UAV_fc->down();
    }
        
        
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        UAV_fc->yawleft();
    }
        
        
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        UAV_fc->yawright();
    }
        
    if (glfwGetKey(window, GLFW_KEY_UP) != GLFW_PRESS 
        && glfwGetKey(window, GLFW_KEY_DOWN) != GLFW_PRESS 
        && glfwGetKey(window, GLFW_KEY_LEFT) != GLFW_PRESS 
        && glfwGetKey(window, GLFW_KEY_RIGHT) != GLFW_PRESS
        && glfwGetKey(window, GLFW_KEY_W) != GLFW_PRESS
        && glfwGetKey(window, GLFW_KEY_S) != GLFW_PRESS
        && glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS
        && glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS)
        UAV_fc->hold();

    //if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    //    UAV_fc->hold();

    //if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    //    UAV_fc->hold();

    //if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    //    UAV_fc->hold();

    //if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    //    UAV_fc->hold();

    UAV_fc->dynamics();
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
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    stbi_set_flip_vertically_on_load(false);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
