#include "TexturesCollection.h"
using namespace collections;

TexturesCollection *TexturesCollection::Instance()
{
    if (!instance) {
        // Create Unique Shared Static Instance
        instance = new TexturesCollection();
    }

    return instance;
}

TexturesCollection::TexturesCollection() : preventDuplicates(true)
{
}

TexturesCollection::~TexturesCollection()
{
    deleteAllTextures();
}

types::Texture *TexturesCollection::addTexture(const std::string &sFilename, types::Texture::TextureType textureType)
{
    if (preventDuplicates) {
        for (auto it = this->textures.begin(); it != this->textures.end(); it++) {
            if ((*it).second != nullptr && ((*it).second->getFilename() == sFilename)) {
                std::cout << "Textures(" << this << "): " << "File " << std::string(sFilename) << " already loaded, "
                          << (*it).second->getTextureTypeString() << " texture " << "oglId(" << (*it).second->getOGLTexId() << ") "
                          << "texId(" << (*it).second->geTexId() << ") " << "associated successfully" << std::endl;
                return (*it).second;
            }
        }
    }

    types::Texture *newTex = new types::Texture(sFilename, idCounter, textureType);
    bool loadingResult = newTex->loadTexture();

    if (!loadingResult) {
        std::cout << "Textures(" << this << "): " << "Error loading " << sFilename << " texture" << std::endl;
        return nullptr;
    }

    //if this texture ID is in use, unload the current texture
    std::map<unsigned int, types::Texture *>::iterator it = textures.find(idCounter);

    if (it != textures.end()) {
        std::cout << "Textures(" << this << "): " << "Warning texture: " << textures[idCounter]->getFilename() << " replaced" << std::endl;
        it->second->unload();
        textures.erase(it);
    }

    // Store new texID and return success
    textures[idCounter++] = newTex;
    std::cout << "Textures(" << this << "): " << newTex->getTextureTypeString() << " texture, oglId(" << newTex->getOGLTexId() << ") "
              << "texId(" << newTex->geTexId() << ") " << std::string(sFilename) << " loaded successfully" << std::endl;
    return newTex;
}

bool TexturesCollection::unloadTexture(const unsigned int &texID)
{
    // don't delete default texture
    if (texID == core::EngineData::Commoms::DEFAULT_TEXTURE_ID) { return false; }

    bool result = true;
    // if this texture ID mapped, unload it's texture, and remove it from the map
    std::map<unsigned int, types::Texture *>::iterator it = textures.find(texID);

    if (it != textures.end()) {
        // unload from GPU
        it->second->unload();
        // erase map reference
        textures.erase(texID);
    } else {
        result = false;
    }

    return result;
}

bool collections::TexturesCollection::deteleTexture(const unsigned int &texID)
{
    // don't delete default texture
    if (texID == core::EngineData::Commoms::DEFAULT_TEXTURE_ID) { return false; }

    bool result = true;
    // if this texture ID mapped, unload it's texture, and remove it from the map
    std::map<unsigned int, types::Texture *>::iterator it = textures.find(texID);

    if (it != textures.end()) {
        // unload from GPU
        it->second->unload();
        // free memory reserved
        delete it->second;
        // erase map reference
        textures.erase(texID);
    } else {
        result = false;
    }

    return result;
}


bool TexturesCollection::bindTexture(const unsigned int &texID)
{
    bool result(true);
    // If this texture ID mapped, bind it's texture as current
    std::map<unsigned int, types::Texture *>::iterator it = textures.find(texID);

    if (it != textures.end()) {
        it->second->bind();
    } else {
        // Binding Failed
        result = false;
    }

    return result;
}

void TexturesCollection::unloadAllTextures()
{
    //start at the begginning of the texture map
    std::map<unsigned int, types::Texture *>::iterator it = textures.begin();

    // Collection already empty
    if (it == textures.end()) { return; }

    //Unload the textures untill the end of the texture map is found
    do { it->second->unload(); } while (++it != textures.end());

    //clear the texture map
    textures.clear();
}

void collections::TexturesCollection::deleteAllTextures()
{
    //start at the begginning of the texture map
    std::map<unsigned int, types::Texture *>::iterator it = textures.begin();

    // Collection already empty
    if (it == textures.end()) { return; }

    //Unload the textures untill the end of the texture map is found
    do { it->second->unload(); delete it->second; } while (++it != textures.end());

    //clear the texture map
    textures.clear();
}

unsigned int TexturesCollection::textureCount(void) const
{
    return this->textures.size();
}

types::Texture *collections::TexturesCollection::getTexture(const unsigned &texID)
{
    return this->textures.at(texID);
}

void collections::TexturesCollection::loadDefaultTexture()
{
    std::map<unsigned int, types::Texture *>::iterator it = textures.find(core::EngineData::Commoms::DEFAULT_TEXTURE_ID);

    // delete previous loaded default texture if it exists
    if (it != textures.end()) {
        // unload from GPU
        it->second->unload();
        // free memory reserved
        delete it->second;
        // erase map reference
        textures.erase(core::EngineData::Commoms::DEFAULT_TEXTURE_ID);
    }

    std::string sFilename = core::ExecutionInfo::EXEC_DIR + core::ShadersData::Samplers::DEFAULT_TEX_FILENAME;
    types::Texture *newTex = new types::Texture(sFilename, core::EngineData::Commoms::DEFAULT_TEXTURE_ID, types::Texture::Diffuse);
    bool loadingResult = newTex->loadTexture();

    if (!loadingResult) {
        std::cout << "Textures(" << this << "): " << "Error loading " << sFilename << " texture" << std::endl;
    }

    // Store default texture to default position
    textures[core::EngineData::Commoms::DEFAULT_TEXTURE_ID] = newTex;
    std::cout << "Textures(" << this << "): " << newTex->getTextureTypeString() << " texture, oglId(" << newTex->getOGLTexId() << ") "
              << "texId(" << newTex->geTexId() << ") " << sFilename << " loaded successfully" << std::endl;
}

// zero is reserved ID for default texture
int collections::TexturesCollection::idCounter = 1;

TexturesCollection *collections::TexturesCollection::instance = nullptr;
