#ifndef HOMEWORK01_MODEL_MESH_HPP_
#define HOMEWORK01_MODEL_MESH_HPP_

#include "OpenGL/OpenGLBufferObject.hpp"
#include "OpenGL/OpenGLShaderProgram.hpp"
#include "OpenGL/OpenGLTexture.hpp"
#include "OpenGL/OpenGLVertexArrayObject.hpp"

#include "glm/mat4x4.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Model
{

class Mesh
{
public:
    using IndexType = unsigned int;
    using TextureType = OpenGL::OpenGLTexture;
    using ShaderProgramType = OpenGL::OpenGLShaderProgram;

    explicit Mesh() noexcept;
    explicit Mesh(const std::vector<float> &positions,
                  const std::vector<float> &normals,
                  const std::vector<float> &textureCoordinates,
                  const std::vector<IndexType> &indices,
                  ShaderProgramType &shaderProgram,
                  TextureType *texture = nullptr);

    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;
    ~Mesh();

    Mesh(const Mesh &other) = delete;
    Mesh &operator=(const Mesh &other) = delete;

    void draw(glm::mat4 &view, glm::mat4 &projection);

    glm::mat4 model();
    void setModel(glm::mat4 &model);

    glm::vec3 getPosition();

    glm::quat getRotation();

    glm::vec3 getRotationEuler();

    glm::vec3 getScale();

    void setPosition(glm::vec3 &position);

    void setRotation(const glm::quat &rot);
    // euler
    void setRotation(const glm::vec3 &rot);

    void setRotationDeg(const glm::vec3 &rot);

    void setScale(glm::vec3 &scale);

    void translate(const glm::vec3 &offset);

    void rotate(float angleDegrees, const glm::vec3 &axis);

private:
    using VertexArrayObjectType = OpenGL::OpenGLVertexArrayObject;
    using BufferObjectType = OpenGL::OpenGLBufferObject;

    void create(const std::vector<float> &positions,
                const std::vector<float> &normals,
                const std::vector<float> &textureCoordinates,
                const std::vector<IndexType> &indices);
    void tidy() noexcept;

    static void vertexBufferObjectSetup(BufferObjectType &object,
                                        const std::vector<float> &data,
                                        ShaderProgramType &program,
                                        GLuint index, GLint size, GLenum type,
                                        GLboolean normalized, GLsizei stride,
                                        int offset);

    ShaderProgramType *shaderProgram_;
    TextureType *texture_;

    std::unique_ptr<VertexArrayObjectType> vertexArrayObject_;
    std::array<std::unique_ptr<BufferObjectType>, 3> vertexBufferObject_;
    std::unique_ptr<BufferObjectType> elementBufferObject_;

    GLsizei indicesCount_;

    glm::mat4 model_;

    glm::vec3 position_ = glm::vec3(0.0f);
    glm::quat rotation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale_ = glm::vec3(1.0f);

    void updateModelMatrix();
};

} // namespace Model

#endif // HOMEWORK01_MODEL_MESH_HPP_
