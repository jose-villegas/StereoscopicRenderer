#pragma once
#include "../types/Vertex.h"
#include "../bases/BaseComponent.h"
#include "../collections/TexturesCollection.h"
#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include "Assimp/postprocess.h"
#include "GLM/glm.hpp"
#include <vector>
#include <iostream>
#include "../Types/Material.h"
#define INVALID_MATERIAL 0xFFFFFFFF
#define INVALID_VALUE 0xFFFFFFFF
namespace scene {

    class Mesh : public bases::BaseComponent {
        public:
            Mesh(void);
            ~Mesh(void);
            bool loadMesh(const std::string &sFileName);
            void render() const;
            std::vector<types::Material *> materials() const { return _materials; };

        private:
            Mesh(const Mesh &mesh);

            bool initFromScene(const aiScene *paiScene, const std::string &sFilename);
            void initMesh(unsigned int index, const aiMesh *paiMesh);
            bool initMaterials(const aiScene *paiScene, const std::string &sFilename);
            void clear();

            struct MeshEntry {
                GLuint VB;
                GLuint IB;
                unsigned int numIndices;
                unsigned int materialIndex;
                MeshEntry();
                ~MeshEntry();
                void init(const std::vector<types::Vertex> &vertices, const std::vector<unsigned int> &indices);
            };

            std::vector<MeshEntry> _meshEntries;
            std::vector<types::Material *> _materials;
            // Engine Textures Collection
            collections::TexturesCollection *_texCollection;
    };
}