#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/vec4.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
struct GPUVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;
};

glm::vec3 anyPerpendicular(const glm::vec3& n)
{
    const glm::vec3 axis = std::abs(n.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    return glm::normalize(glm::cross(axis, n));
}

std::vector<glm::vec4> computeTangents(const Mesh& mesh)
{
    const size_t count = mesh.vertices.size();
    std::vector<glm::vec3> tangentSums(count, glm::vec3(0.0f));
    std::vector<glm::vec3> bitangentSums(count, glm::vec3(0.0f));

    // tangent per triangle, summed per vertex
    for (const glm::uvec3& triangle : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[triangle.x];
        const Vertex& v1 = mesh.vertices[triangle.y];
        const Vertex& v2 = mesh.vertices[triangle.z];

        const glm::vec3 e1 = v1.position - v0.position;
        const glm::vec3 e2 = v2.position - v0.position;
        const glm::vec2 d1 = v1.texCoord - v0.texCoord;
        const glm::vec2 d2 = v2.texCoord - v0.texCoord;
        const float det = d1.x * d2.y - d2.x * d1.y;
        if (std::abs(det) < 1e-12f)
            continue;
        const float inverseDet = 1.0f / det;
        const glm::vec3 tangent = (d2.y * e1 - d1.y * e2) * inverseDet;
        const glm::vec3 bitangent = (d1.x * e2 - d2.x * e1) * inverseDet;

        for (const uint32_t index : { triangle.x, triangle.y, triangle.z }) {
            tangentSums[index] += tangent;
            bitangentSums[index] += bitangent;
        }
    }

    // orthogonalize against the normal, store handedness in w
    std::vector<glm::vec4> result(count);
    for (size_t i = 0; i < count; ++i) {
        const glm::vec3 normal = mesh.vertices[i].normal;
        const float normalLength = glm::length(normal);
        const glm::vec3 n = normalLength > 0.0f ? normal / normalLength : glm::vec3(0, 1, 0);

        glm::vec3 t = tangentSums[i] - n * glm::dot(n, tangentSums[i]);
        const float tangentLengthSquared = glm::dot(t, t);
        t = tangentLengthSquared > 1e-12f ? t / std::sqrt(tangentLengthSquared) : anyPerpendicular(n);

        const float handedness = glm::dot(glm::cross(n, t), bitangentSums[i]) < 0.0f ? -1.0f : 1.0f;
        result[i] = glm::vec4(t, handedness);
    }
    return result;
}
}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
{
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);

    // create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // add tangents to vertices
    const std::vector<glm::vec4> tangents = computeTangents(cpuMesh);
    std::vector<GPUVertex> gpuVertices;
    gpuVertices.reserve(cpuMesh.vertices.size());
    for (size_t i = 0; i < cpuMesh.vertices.size(); ++i) {
        const Vertex& v = cpuMesh.vertices[i];
        gpuVertices.push_back(GPUVertex { v.position, v.normal, v.texCoord, tangents[i] });
    }

    // upload vertices (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gpuVertices.size() * sizeof(GPUVertex)), gpuVertices.data(), GL_STATIC_DRAW);

    // upload triangles (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // vertex layout: position, normal, texCoord, tangent
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, texCoord));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, tangent));

    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());
}

GPUMesh::GPUMesh(GPUMesh&& other)
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other)
{
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize)
{
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    std::vector<Mesh> subMeshes = loadMesh(filePath, { .normalizeVertexPositions = normalize });
    std::vector<GPUMesh> gpuMeshes;
    for (const Mesh& mesh : subMeshes)
        gpuMeshes.emplace_back(mesh);
    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const
{
    return m_hasTextureCoords;
}

void GPUMesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

void GPUMesh::moveInto(GPUMesh&& other)
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;

    other.m_numIndices = 0;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
}
