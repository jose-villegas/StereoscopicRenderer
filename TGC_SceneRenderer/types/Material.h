#pragma once
#include "../collections/stored/StoredShaders.h"
#include "../bases/ShaderLinks.h"
#include "../core/Data.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include <set>
namespace types {

    class Material : public bases::ShaderLink {
        private:
            struct TexturePtrComp {
                bool operator()(const Texture *lhs, const Texture *rhs) const  { return lhs->getType() < rhs->getType(); }
            };
            // Ordered Textures by type check Texture.h
            std::set<Texture *, TexturePtrComp> textures;
            ShaderProgram *matShader;
            Material(const Material &mat);

        public:
            Material(void);
            ~Material(void);
            glm::vec3 ambient;
            glm::vec3 diffuse;
            glm::vec3 specular;
            float shininess;

            /*
             * Assigns a guessed ShaderProgram from the stored shaders
             * based on the stored textures and their type for example
             * a material with diffuse and normal maps would be assigned
             * a BumpedDiffuse ShaderProgram
             */
            void guessMaterialShader();
            // Adds textures, either a single texture or many textures pass in an array
            void addTexture(Texture *tex);
            void addTexture(Texture *tex[], const unsigned int texCount);
            void setShaderProgram(ShaderProgram *shp);
            // matShader needs to be set
            void useMaterialShader();
            void bindTextures() const;
            // Pass and binds the textures ID to the ShaderProgram
            void setTexturesUniforms(types::ShaderProgram *shp);
            // Uses the class member stored ShaderProgram _matShader
            void setTexturesUniforms();
            // Implemented pure virtual method, check ShaderLink class
            // sets the shaderprogram uniform data and
            // sets the stored textures uniforms
            void setUniforms(types::ShaderProgram *shp);
            // Uses the class member stored ShaderProgram _matShader
            // sets the shaderprogram uniform data and
            // sets the stored textures uniforms
            void setUniforms();

            unsigned int textureCount() const { return textures.size(); };
    };
}

