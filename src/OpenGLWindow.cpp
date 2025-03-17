#include "OpenGLWindow.hpp"

#include "Model/TextureFactory.hpp"
#include "OpenGL/OpenGLException.hpp"
#include "Utils/Compilers.hpp"
#include "Utils/Global.hpp"
#include "Utils/StringFormat/StringFormat.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "tiny_obj_loader.h"

#include <iostream>
#include <utility>
#include <vector>

namespace Detail
{

void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);

bool compileShaders(OpenGL::OpenGLShaderProgram &program,
                    const char *vertexShaderFile,
                    const char *fragmentShaderFile = nullptr,
                    const char *geometryShaderFile = nullptr);

bool compileShaders(OpenGL::OpenGLShaderProgram &program,
                    const char *vertexShaderFile,
                    const char *fragmentShaderFile,
                    const char *geometryShaderFile)
{
    if (!program.addShaderFromFile(OpenGL::OpenGLShader::Type::Vertex,
                                   vertexShaderFile))
    {
        return false;
    }

    if (fragmentShaderFile &&

        !program.addShaderFromFile(OpenGL::OpenGLShader::Type::Fragment,
                                   fragmentShaderFile))
    {
        return false;
    }

    if (geometryShaderFile &&
        !program.addShaderFromFile(OpenGL::OpenGLShader::Type::Geometry,
                                   geometryShaderFile))
    {
        return false;
    }

    program.link();
    if (!program.linkStatus())
    {
        return false;
    }

    return true;
}

void frameBufferSizeCallback(GLFWwindow *window, int width, int height)
{

    auto *windowPtr =
        static_cast<OpenGLWindow *>(glfwGetWindowUserPointer(window));
    if (windowPtr)
    {
        windowPtr->frameBufferSizeCallbackImpl(window, width, height);
    }

    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    auto *windowPtr =
        static_cast<OpenGLWindow *>(glfwGetWindowUserPointer(window));
    if (windowPtr)
    {
        windowPtr->mouseCallbackImpl(window, xpos, ypos);
        windowPtr->wasFreeCamera_ = false;
    }
}

} // namespace Detail

OpenGLWindow::OpenGLWindow(glm::ivec2 windowSize, std::string title,
                           glm::ivec2 openglVersion)
    : window_{nullptr}, size_{windowSize}, title_{title},
      version_{openglVersion}, models_{}, renderMode_{RenderMode::Fill},
      backgroundColor_{0}, lookAt_{0}, cameraPosition_{lookAt_ + glm::vec3{8}}
{
    create();
}

OpenGLWindow::~OpenGLWindow() { destroy(); }

bool OpenGLWindow::addModel(const char *modelSource, const char *textureSource,
                            OpenGL::OpenGLShaderProgram &program)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string errorMessage;

    auto success =
        tinyobj::LoadObj(shapes, materials, errorMessage, modelSource);

    if (!success)
    {
        std::cerr << "[Error]" << errorMessage.c_str();

        return false;
    }

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> textureCoordinates;
    std::vector<unsigned int> indices;

    for (auto &shape : shapes)
    {
        positions.insert(positions.end(), shape.mesh.positions.begin(),
                         shape.mesh.positions.end());
        if (!shape.mesh.normals.empty())
        {
            normals.insert(normals.end(), shape.mesh.normals.begin(),
                           shape.mesh.normals.end());
        }

        if (!shape.mesh.texcoords.empty())
        {
            textureCoordinates.insert(textureCoordinates.end(),
                                      shape.mesh.texcoords.begin(),
                                      shape.mesh.texcoords.end());
        }

        indices.insert(indices.end(), shape.mesh.indices.begin(),
                       shape.mesh.indices.end());
    }

    std::unique_ptr<OpenGL::OpenGLTexture> texture;
    std::unique_ptr<Model::Mesh> mesh;

    if (textureSource)
    {
        texture = Model::TextureFactory::loadFromFile(textureSource);
        mesh.reset(new Model::Mesh{positions, normals, textureCoordinates,
                                   indices, program, texture.get()});

        textures.push_back(std::move(texture));
    }
    else
    {
        mesh.reset(new Model::Mesh{positions, normals, textureCoordinates,
                                   indices, program});
    }

    models_.push_back(std::move(mesh));

    return true;
}

OpenGL::OpenGLShaderProgram *
OpenGLWindow::addShader(const char *vertexShaderSource,
                        const char *fragmentShaderSource,
                        const char *geometryShaderSource)
{
    std::unique_ptr<OpenGL::OpenGLShaderProgram> program{
        new OpenGL::OpenGLShaderProgram{}};
    if (!Detail::compileShaders(*program, vertexShaderSource,
                                fragmentShaderSource, geometryShaderSource))
    {
        return nullptr;
    }
    shaders_.push_back(std::move(program));

    return shaders_.back().get();
}

void OpenGLWindow::setMouseLastPosition(float x, float y) noexcept
{
    mouse_lastX_ = x;
    mouse_lastY_ = y;
}

void OpenGLWindow::frameBufferSizeCallbackImpl(GLFWwindow *window, int width,
                                               int height)
{
    size_.x = width;
    size_.y = height;
}

void OpenGLWindow::firstEnterMouseCallback(GLFWwindow *window, double xpos,
                                           double ypos, bool &control_flag)
{
    if (control_flag)
    {
        mouse_lastX_ = xpos;
        mouse_lastY_ = ypos;

        cameraFront_ = glm::normalize(lookAt_ - cameraPosition_);

        mouse_yaw_ = glm::degrees(atan2(cameraFront_.z, cameraFront_.x));
        mouse_pitch_ = glm::degrees(asin(cameraFront_.y));
        control_flag = false;
    }
}

void OpenGLWindow::cameraDirectionLoop(GLFWwindow *window, double xpos,
                                       double ypos)
{
    float xoffset = xpos - mouse_lastX_;
    float yoffset = mouse_lastY_ - ypos;
    mouse_lastX_ = xpos;
    mouse_lastY_ = ypos;

    xoffset *= sensitivity_;
    yoffset *= sensitivity_;

    mouse_yaw_ += xoffset;
    mouse_pitch_ += yoffset;

    // Add constraints to prevent camera flipping
    if (mouse_pitch_ > 89.0f)
        mouse_pitch_ = 89.0f;
    if (mouse_pitch_ < -89.0f)
        mouse_pitch_ = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(mouse_pitch_)) * cos(glm::radians(mouse_yaw_));
    front.y = sin(glm::radians(mouse_pitch_));
    front.z = cos(glm::radians(mouse_pitch_)) * sin(glm::radians(mouse_yaw_));
    cameraFront_ = glm::normalize(front);

    lookAt_ = cameraPosition_ + cameraFront_;
}

void OpenGLWindow::mouseCallbackImpl(GLFWwindow *window, double xpos,
                                     double ypos)
{
    firstEnterMouseCallback(window, xpos, ypos, wasFreeCamera_);

    cameraDirectionLoop(window, xpos, ypos);
}

float OpenGLWindow::aspectRatio() const noexcept
{
    return static_cast<float>(width()) / static_cast<float>(height());
}

void OpenGLWindow::create()
{
    if (!initializeOpenGL())
    {
        throw OpenGL::OpenGLException{"Failed to initialize OpenGL"};
    }

    if (!createWindow())
    {
        glfwTerminate();
        throw OpenGL::OpenGLException{"Failed to Create GLFW window"};
    }

    if (!initializeGLAD())
    {
        glfwTerminate();
        throw OpenGL::OpenGLException{"Failed to initialize GLAD"};
    }

    initializeImgui();

    glEnable(GL_DEPTH_TEST);
}

void OpenGLWindow::clearColor()
{
    glClearColor(backgroundColor_.x, backgroundColor_.y, backgroundColor_.z,
                 backgroundColor_.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool OpenGLWindow::createWindow()
{
    window_ =
        glfwCreateWindow(width(), height(), title_.c_str(), nullptr, nullptr);
    if (!window_)
    {
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, Detail::frameBufferSizeCallback);

    return true;
}

void OpenGLWindow::destroy()
{
    for (auto &model : models_)
    {
        model.reset(nullptr);
    }
    models_.clear();

    for (auto &texture : textures)
    {
        texture.reset(nullptr);
    }
    textures.clear();

    shaders_.clear();

    destroyImgui();
    destroyOpenGL();
}

void OpenGLWindow::destroyImgui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void OpenGLWindow::destroyOpenGL()
{
    glfwDestroyWindow(window_);
    glfwTerminate();
}

int OpenGLWindow::height() const noexcept { return size_.y; }

bool OpenGLWindow::initializeGLAD()
{
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void OpenGLWindow::initializeImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    PROGRAM_MAYBE_UNUSED(io)

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(
        StringFormat::StringFormat("#version %d%d0", version_.x, version_.y)
            .c_str());
}

bool OpenGLWindow::initializeOpenGL()
{
    if (!glfwInit())
    {
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version_[0]);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version_[1]);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

void OpenGLWindow::processInput()
{
    cameraMovement();
    shouldExit();
}

void OpenGLWindow::captureMouse()
{
    static bool mouseCaptured = false;

    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS)
    {
        if (!mouseCaptured)
        {
            mouseCaptured = true;

            if (glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            {
                glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwSetCursorPosCallback(window_, nullptr);
                wasFreeCamera_ = true;
            }
            else
            {
                glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwSetCursorPosCallback(window_, Detail::mouseCallback);
            }
        }
    }
    else
    {
        mouseCaptured = false;
    }

    firstEnterMouseCallback(window_, mouse_lastX_, mouse_lastY_,
                            isFirstFreeCamera_);

    cameraDirectionLoop(window_, mouse_lastX_, mouse_lastY_);
}

void OpenGLWindow::cameraMovement()
{
    if (isFreeCamera_)
    {
        float cameraSpeed = cameraSpeedFactor_ * deltaTime_;
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
            cameraPosition_ += cameraSpeed * cameraFront_;
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
            cameraPosition_ -= cameraSpeed * cameraFront_;
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
            cameraPosition_ -=
                glm::normalize(glm::cross(cameraFront_, cameraUp_)) *
                cameraSpeed;
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
            cameraPosition_ +=
                glm::normalize(glm::cross(cameraFront_, cameraUp_)) *
                cameraSpeed;

        captureMouse();
    }
    else
    {
        if (!isFirstFreeCamera_)
        {
            isFirstFreeCamera_ = true;
        }
    }
}

void OpenGLWindow::shouldExit()
{
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window_, true);
    }
}

void OpenGLWindow::startRender() { windowRenderLoop(); }

int OpenGLWindow::width() const noexcept { return size_.x; }

void OpenGLWindow::windowImguiMain()
{
    ImGui::Begin("Settings & Models");

    windowImguiGeneralSetting();
    ImGui::Separator();
    windowImguiModelSetting();

    ImGui::End();
}

void OpenGLWindow::windowImguiGeneralSetting()
{
    ImGui::ColorEdit4("Background color", glm::value_ptr(backgroundColor_));
    ImGui::Checkbox("Free Camera", &isFreeCamera_);
    ImGui::SliderFloat("Camera Speed", &cameraSpeedFactor_, 0.1f, 20.0f);
    ImGui::SliderFloat3("Camera Position", glm::value_ptr(cameraPosition_),
                        -20.0f, 20.0f);
    ImGui::SliderFloat3("Look At", glm::value_ptr(lookAt_), -20.0f, 20.0f);

    const char *items[] = {"Line", "Fill"};
    int current_item = renderMode_;
    ImGui::Combo("combo", &current_item, items, IM_ARRAYSIZE(items));

    if (current_item != renderMode_)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE + current_item);
        renderMode_ = static_cast<RenderMode>(current_item);
    }
}

void OpenGLWindow::windowImguiModelSetting()
{
    for (auto &model : models_)
    {
        ImGui::Text("Model");
        glm::vec3 position = model->getPosition();

        glm::vec3 rotation = model->getRotationEuler();

        glm::vec3 scale = model->getScale();

        if (ImGui::SliderFloat3("Position", glm::value_ptr(position), -20.0f,
                                20.0f))
        {
            model->setPosition(position);
        }

        if (ImGui::SliderFloat3("Rotation", glm::value_ptr(rotation), -180.0f,
                                180.0f))
        {
            model->setRotationDeg(rotation);
        }

        if (ImGui::SliderFloat3("Scale", glm::value_ptr(scale), 0.1f, 10.0f))
        {
            model->setScale(scale);
        }
    }
}

void OpenGLWindow::windowImguiModelLoader() {}

void OpenGLWindow::windowRenderLateUpdate() {}

void OpenGLWindow::windowRenderImguiUpdate()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    windowImguiGeneralSetting();

    windowImguiModelSetting();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OpenGLWindow::windowRenderLoop()
{
    while (!(glfwWindowShouldClose(window_)))
    {
        float currentFrame = glfwGetTime();
        deltaTime_ = currentFrame - lastFrame_;
        lastFrame_ = currentFrame;

        processInput();
        clearColor();

        windowRenderUpdate();
        windowRenderLateUpdate();

        windowRenderImguiUpdate();

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
}

void OpenGLWindow::windowRenderUpdate()
{
    PRAGMA_WARNING_PUSH
    PRAGMA_WARNING_DISABLE_CONSTANTCONDITIONAL
    glm::mat4 view = glm::lookAt(cameraPosition_, lookAt_, glm::vec3{0, 1, 0}) *
                     glm::mat4(1);
    PRAGMA_WARNING_POP

    glm::mat4 projection{
        glm::perspective(glm::radians(45.0f), aspectRatio(), 0.1f, 100.0f)};

    for (auto &model : models_)
    {
        model->draw(view, projection);
    }
}
