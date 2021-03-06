#include "ProgressiveMeshes.h"
#include "..\core\Data.h"
#include "glm\gtc\quaternion.hpp"
#include "glm\gtc\constants.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
using namespace utils;

bool utils::ProgressiveMesh::Face::replaceVertex(std::unordered_map<unsigned int, utils::ProgressiveMesh::Face *>::iterator &it, Vertex *oldVertex, Vertex *newVertex,
        const bool calcNormal /* = true */)
{
    if (!(oldVertex && newVertex) || !this->hasVertex(oldVertex) || this->hasVertex(newVertex)) { return false; }

    if (oldVertex == vertices[0]) {
        vertices[0] = newVertex;
    } else if (oldVertex == vertices[1]) {
        vertices[1] = newVertex;
    } else if (oldVertex == vertices[2]) {
        vertices[2] = newVertex;
    }

    // erase in the old vertex this face association
    it = oldVertex->faces.erase(oldVertex->faces.find(this->mappingId));

    // add to the new vertex this face as an associate
    if (newVertex->faces.find(this->mappingId) != newVertex->faces.end()) { return false; }

    newVertex->faces[this->mappingId] = this;
    // remove non-neighbors
    std::for_each(this->vertices.begin(), this->vertices.end(), [oldVertex](Vertex * vertex) {
        oldVertex->removeNonNeighbor(vertex);
        vertex->removeNonNeighbor(oldVertex);
    });

    // update face vertices neighbors
    for (unsigned int i = 0; i < 3; i++) {
        if (this->vertices[i]->faces.find(this->mappingId) == this->vertices[i]->faces.end()) { return true; }

        for (unsigned int j = 0; j < 3; j++) {
            if (j != i) {
                if (this->vertices[i]->neightbors.find(this->vertices[j]->mappingId) == this->vertices[i]->neightbors.end()) {
                    this->vertices[i]->neightbors[this->vertices[j]->mappingId] = this->vertices[j];
                }
            }
        }
    }

    // recalculate face normal
    if (calcNormal) { this->calculateNormal(); }

    return true;
}

utils::ProgressiveMesh::Face::Face(const unsigned int id, Vertex *v1, Vertex *v2, Vertex *v3, const types::Face &nonProgFace) : types::Face(nonProgFace)
{
    this->mappingId = id;
    this->vertices[0] = v1;
    this->vertices[1] = v2;
    this->vertices[2] = v3;
    // calculate face normal
    this->calculateNormal();

    // add associate face vertices with this face
    for (int i = 0; i < 3; i++) {
        this->vertices[i]->faces[this->mappingId] = this;

        // associate vertices neighbors
        for (int j = 0; j < 3; j++) {
            if (i != j) { this->vertices[i]->neightbors[this->vertices[j]->mappingId] = this->vertices[j]; }
        }
    }
}

utils::ProgressiveMesh::Vertex::Vertex(const unsigned int id, const types::Vertex &nonProgVert) : types::Vertex(nonProgVert)
{
    this->mappingId = id;
    this->originalPlace = id;
}

void utils::ProgressiveMesh::Face::calculateNormal()
{
    glm::vec3 v0 = this->vertices[0]->position;
    glm::vec3 v1 = this->vertices[1]->position;
    glm::vec3 v2 = this->vertices[2]->position;
    normal = (v1 - v0) * (v2 - v1);

    if (glm::length(normal) == 0) { return; }

    normal = glm::normalize(normal);
}

bool utils::ProgressiveMesh::Face::hasVertex(types::Vertex *v)
{
    return (v == vertices[0] || v == vertices[1] || v == vertices[2]);
}

void utils::ProgressiveMesh::Vertex::removeNonNeighbor(Vertex *v)
{
    // this vertex isn't stored
    if (this->neightbors.find(v->mappingId) == this->neightbors.end()) { return; }

    for (auto it = this->faces.begin(); it != this->faces.end(); ++it) {
        if ((*it).second->hasVertex(v)) { return; }
    }

    this->neightbors.erase(v->mappingId);
}

utils::ProgressiveMesh::Vertex::~Vertex()
{
    if (this->faces.empty()) { return; }

    while (!this->neightbors.empty()) {
        (*this->neightbors.begin()).second->neightbors.erase(this->mappingId);
        this->neightbors.erase(this->neightbors.begin());
    }
}

utils::ProgressiveMesh::Face::~Face()
{
    this->vertices[0] = nullptr; this->vertices[1] = nullptr; this->vertices[2] = nullptr;
}

float utils::ProgressiveMesh::edgeCollapseCost(Vertex *u, Vertex *v)
{
    float edgeLength, curvature = 0.f;
    edgeLength = glm::length(v->position - u->position);
    // find faces associated with uv edge
    std::unordered_map<unsigned int, Face *> sides;

    for (auto it = u->faces.begin(); it != u->faces.end(); ++it) {
        if ((*it).second->hasVertex(v)) { sides[(*it).first] = (*it).second; }
    }

    // use face facing most away from associated faces
    for (auto it = u->faces.begin(); it != u->faces.end(); ++it) {
        float minCurvature = 1.f;

        // determine the minimum normal curvature between faces
        for (auto itSides = sides.begin(); itSides != sides.end(); itSides++) {
            float dotProduct = glm::dot((*it).second->normal, (*itSides).second->normal);
            minCurvature = glm::min(minCurvature, (1.f - dotProduct) / 2.f);
        }

        curvature = glm::max(curvature, minCurvature);
    }

    return edgeLength * curvature;
}

void utils::ProgressiveMesh::edgeCostAtVertex(Vertex *v)
{
    if (v->neightbors.empty()) {
        v->collapseCandidate = nullptr;
        v->collapseCost = 0.01f;
        return;
    }

    v->collapseCost = std::numeric_limits<float>::infinity();
    v->collapseCandidate = nullptr;

    // search for the least cost edge
    for (auto it = v->neightbors.begin(); it != v->neightbors.end(); ++it) {
        float currentCost = edgeCollapseCost(v, (*it).second);

        if (currentCost < v->collapseCost) {
            v->collapseCandidate = (*it).second;
            v->collapseCost = currentCost;
        }
    }
}

bool  utils::ProgressiveMesh::removeProgFace(std::unordered_map<unsigned int, utils::ProgressiveMesh::Face *>::iterator &it, Vertex *src)
{
    Face *ptr = (*it).second;
    bool iteratorInvalidated = false;

    // remove association with this face at face vertices
    for (int i = 0; i < 3; i++) {
        // if we found this vertice is the src we need to replace
        // the iterator, erase operation invalidates all class iterators
        if (!ptr->vertices[i]) { continue; }

        auto faceIt  = ptr->vertices[i]->faces.find(ptr->mappingId);

        if (faceIt == ptr->vertices[i]->faces.end()) { continue; }

        if (ptr->vertices[i] && ptr->vertices[i] == src) {
            it = ptr->vertices[i]->faces.erase(faceIt);
            iteratorInvalidated = true;
        } else if (ptr->vertices[i]) {
            ptr->vertices[i]->faces.erase(faceIt); // else just erase normally
        }
    }

    // update vertices neighbors
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        if (ptr->vertices[i] && ptr->vertices[j]) {
            ptr->vertices[i]->removeNonNeighbor(ptr->vertices[j]);
            ptr->vertices[j]->removeNonNeighbor(ptr->vertices[i]);
        }
    }

    // delete face reserved memory
    delete ptr;
    // delete face reference from
    // progressivemeshes class collection
    this->progFaces.erase(ptr->mappingId);
    // return iterator status
    return iteratorInvalidated;
}

void utils::ProgressiveMesh::collapse(Vertex *u, Vertex *v)
{
    if (!v) {
        this->progVertices.erase(u->mappingId);
        delete u;;
        return;
    }

    std::unordered_map<unsigned int, Vertex *> tmpNeighbors = u->neightbors;
    // deletes the face triangle from the progmesh
    // and updates vertices and neighbors accordly
    auto it = u->faces.begin();

    while (it != u->faces.end()) {
        if ((*it).second->hasVertex(v)) {
            removeProgFace(it, u) ? it : ++it;
        } else { ++it; }
    }

    // update rest of the triangles and vertices
    // accordily to deleted triangle
    it = u->faces.begin();

    while (it != u->faces.end()) {
        (*it).second->replaceVertex(it, u, v) ?  it : ++it;
    }

    this->progVertices.erase(u->mappingId);
    delete u;

    // recalculate edge cost with new neighbors
    for (auto it = tmpNeighbors.begin(); it != tmpNeighbors.end(); ++it) {
        edgeCostAtVertex((*it).second);
    }
}

utils::ProgressiveMesh::Vertex *utils::ProgressiveMesh::minimumCostEdge()
{
    Vertex *minCostVertex = (*(progVertices.begin())).second;
    // the find lowest cost vertex to be deleted meaning
    // this vertex will affect less the final model
    auto it = std::find_if(progVertices.begin(), progVertices.end(),
    [&minCostVertex](const std::pair<unsigned int, Vertex *> &index) {
        // found smaller cost so swap vertex candidate
        if (index.second->collapseCost < minCostVertex->collapseCost) {
            minCostVertex = index.second;

            // early loop break if the collapse cost is zero or really close to zero
            if (minCostVertex->collapseCost > -glm::epsilon<float>() && minCostVertex->collapseCost < glm::epsilon<float>()) {
                return true;
            }
        }

        return false;
    });
    return minCostVertex;
}

void utils::ProgressiveMesh::copyVertices(const std::vector<types::Vertex> &vertices)
{
    int index = 0;

    for (std::vector<types::Vertex>::const_iterator it = vertices.begin(); it < vertices.end(); ++it) {
        this->progVertices[index++] = new Vertex(index, *it); // starts from 1
    }
}

void utils::ProgressiveMesh::copyFaces(const std::vector<types::Face> &faces)
{
    int index = 0;

    for (std::vector<types::Face>::const_iterator it = faces.begin(); it < faces.end(); ++it) {
        this->progFaces[index++] = new Face(index, progVertices[(*it).indices[0]], progVertices[(*it).indices[1]], progVertices[(*it).indices[2]], *it);;
    }
}

void utils::ProgressiveMesh::generateProgressiveMesh(const std::vector<types::Vertex> &vertices, const std::vector<types::Face> &faces)
{
    vertexCount = vertices.size();
    polyCount = faces.size();
    // copy regular mesh values to prog mesh structures
    copyVertices(vertices); copyFaces(faces);
    // allocate space for output
    this->candidatesMap.resize(this->progVertices.size());
    this->permutations.resize(this->progVertices.size());

    for (auto it = this->progVertices.begin(); it != this->progVertices.end(); ++it) {
        edgeCostAtVertex((*it).second);
    }

    while (!this->progVertices.empty()) {
        Vertex *mnimum = minimumCostEdge();
        // track of the collapse vertices order
        this->permutations[mnimum->mappingId] = this->progVertices.size() - 1;
        // track of the collapse candidate
        this->candidatesMap[this->progVertices.size() - 1] = mnimum->collapseCandidate != nullptr ? mnimum->collapseCandidate->mappingId : -1;
        // collapse with candidate and reorganize progressivemesh
        this->collapse(mnimum, mnimum->collapseCandidate);
    }

    for (auto it = this->candidatesMap.begin(); it != this->candidatesMap.end(); ++it) {
        (*it) = (*it) == -1 ? 0 : this->permutations[(*it)];
    }
}

void utils::ProgressiveMesh::generateProgressiveMesh(const scene::Mesh::SubMesh *input)
{
    generateProgressiveMesh(input->vertices, input->faces);
}


utils::ProgressiveMesh::ProgressiveMesh(const std::vector<types::Vertex> &vertices, const std::vector<types::Face> &faces) : levelOfDetailBase(0.5f), morphingFactor(1.0f), vertexCount(-1)
{
    generateProgressiveMesh(vertices, faces);
}

utils::ProgressiveMesh::ProgressiveMesh(const scene::Mesh::SubMesh *input) : levelOfDetailBase(0.5f), morphingFactor(1.0f), vertexCount(-1)
{
    generateProgressiveMesh(input);
}

void utils::ProgressiveMesh::permuteVertices(std::vector<types::Vertex> &vertices, std::vector<unsigned int> &indices, std::vector<types::Face> &faces)
{
    if (permutations.empty()) { return; }

    std::vector<types::Vertex> tmpVerts = vertices;

    // reorder vertices by permutation
    for (unsigned int i = 0; i < vertices.size(); i++) {
        vertices[permutations[i]] = tmpVerts[i];
    }

    // update face indices respectively
    for (unsigned int i = 0; i < faces.size(); i++) {
        for (int j = 0; j < 3; j++) {
            faces[i].indices[j] = permutations[faces[i].indices[j]];
            faces[i].vertices[j] = &vertices[faces[i].indices[j]];
            indices[i * 3 + j] = faces[i].indices[j];
        }
    }
}

void utils::ProgressiveMesh::permuteVertices(scene::Mesh::SubMesh *input)
{
    this->permuteVertices(input->vertices, input->indices, input->faces);
}

unsigned int utils::ProgressiveMesh::mapVertexCollapse(const unsigned int a, const unsigned int b)
{
    if (b <= 0) { return 0; }

    unsigned int result = a;

    while (result >= b) {
        result = candidatesMap[result];
    }

    return result;
}

utils::ProgressiveMesh::ReducedMesh *utils::ProgressiveMesh::reduceVerticesCount(const std::vector<types::Vertex> &vertices,
        const std::vector<unsigned int> &indices,
        const std::vector<types::Face> &faces,
        const int vertexCount)
{
    if (vertexCount <= 2 || vertices.empty() || indices.empty() || faces.empty()) { return nullptr; }

    ReducedMesh *result = new ReducedMesh();
    result->vertices.resize(vertexCount);
    types::Vertex permutedVertex[3];
    unsigned int permutePosition[3], permutePositionLoD[3];

    for (unsigned int i = 0; i < faces.size(); i++) {
        for (int j = 0; j < 3; j++) {
            permutePosition[j] = this->mapVertexCollapse(faces[i].indices[j], vertexCount);
        }

        if (permutePosition[0] == permutePosition[1]
                || permutePosition[1] == permutePosition[2]
                || permutePosition[2] == permutePosition[0]) {
            continue;
        }

        result->faces.push_back(faces[i]);

        for (int j = 0; j < 3; j++) {
            permutePositionLoD[j] = this->mapVertexCollapse(permutePosition[j], (int)(vertexCount * levelOfDetailBase));
        }

        for (int j = 0; j < 3; j++) {
            permutedVertex[j] = vertices[permutePosition[j]];
            permutedVertex[j].position = vertices[permutePosition[j]].position * morphingFactor +
                                         vertices[permutePositionLoD[j]].position * (1.f - morphingFactor);
            // write to result structure
            result->indices.push_back(permutePosition[j]);
            result->vertices[permutePosition[j]] = permutedVertex[j];
        }
    }

    // rewite statistic data
    this->vertexCount = result->vertices.size();
    this->polyCount = result->faces.size();
    // return whole structure
    return result;
}

utils::ProgressiveMesh::ReducedMesh *utils::ProgressiveMesh::reduceVerticesCount(const scene::Mesh::SubMesh *input, const int vertexCount)
{
    return reduceVerticesCount(input->vertices, input->indices, input->faces, vertexCount);
}

void utils::ProgressiveMesh::reduceAndSetBufferData(scene::Mesh::SubMesh *input, const int vertexCount)
{
    ReducedMesh *rslt = reduceVerticesCount(input->vertices, input->indices, input->faces, vertexCount);

    if (rslt) {
        input->active = true;
        input->setBuffersData(rslt->vertices, rslt->indices);
        delete rslt;
    } else {
        input->active = false;
        input->setBuffersData(std::vector<types::Vertex>(), std::vector<unsigned int>());
    }
}

void utils::MeshReductor::load(scene::Mesh *baseMesh)
{
    this->baseMesh = baseMesh;
    originalPolyCount = originalVertexCount = 0;
    const std::vector<scene::Mesh::SubMesh *> &baseSubmeshes = baseMesh->getMeshEntries();

    for (auto it = baseSubmeshes.begin(); it != baseSubmeshes.end(); ++it) {
        this->reducedMeshEntries.push_back(ProgressiveMesh(*it));
        this->reducedMeshEntries.back().permuteVertices(*it);
        originalVertexCount += this->reducedMeshEntries.back().vertexCount;
        originalPolyCount += this->reducedMeshEntries.back().polyCount;
        std::cout << "MeshReductor(" << this << ") " << "generated progressive mesh for Submesh(" << *it << ") with polycount (" << this->reducedMeshEntries.back().polyCount << ") ";
        std::cout << "and vertexcount (" << this->reducedMeshEntries.back().vertexCount << ")" << std::endl;
    }

    this->actualPolyCount = originalPolyCount;
    this->actualVertexCount = originalVertexCount;
}

void utils::MeshReductor::reduce(const float prcentil /* 0.0 - 1.0 */)
{
    unsigned int finalVertexCount = (unsigned int)((float)originalVertexCount * prcentil);
    unsigned int meshIndex; actualPolyCount = meshIndex = actualVertexCount = 0;
    reduce(finalVertexCount);
}

void utils::MeshReductor::reduce(const unsigned int vertexCount)
{
    unsigned int meshIndex; actualPolyCount = meshIndex = actualVertexCount = 0;
    unsigned int reductionTotalAcum = 0;
    unsigned int reductionPerMesh = 0;
    float percentilReduction = (float)(originalVertexCount - vertexCount) / originalVertexCount;
    const std::vector<scene::Mesh::SubMesh *> &baseSubmeshes = baseMesh->getMeshEntries();

    for (auto it = this->reducedMeshEntries.begin(); it != this->reducedMeshEntries.end(); ++it, ++meshIndex) {
        reductionPerMesh = (unsigned int)std::ceil(baseSubmeshes[meshIndex]->vertices.size() * percentilReduction);

        if (reductionTotalAcum <= originalVertexCount - vertexCount) {
            (*it).reduceAndSetBufferData(baseSubmeshes[meshIndex], baseSubmeshes[meshIndex]->vertices.size() - reductionPerMesh);
        }

        reductionTotalAcum += reductionPerMesh;
        actualPolyCount += (*it).polyCount;
        actualVertexCount += (*it).vertexCount;
    }
}

