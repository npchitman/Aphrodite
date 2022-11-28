#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "sceneNode.h"

namespace vkl {
enum class ShadingModel {
    UNLIT,
    DEFAULTLIT,
    PBR,
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

using CameraMapList = std::unordered_map<IdType, std::shared_ptr<Camera>>;
using EntityMapList = std::unordered_map<IdType, std::shared_ptr<Entity>>;
using LightMapList  = std::unordered_map<IdType, std::shared_ptr<Light>>;

enum class SceneManagerType {
    DEFAULT,
};

class Scene {
public:
    static std::unique_ptr<Scene> Create(SceneManagerType type);

    Scene();
    ~Scene();

public:
    std::shared_ptr<Light>      createLight();
    std::shared_ptr<Entity>     createEntity();
    std::shared_ptr<Entity>     createEntityFromGLTF(const std::string &path);
    std::shared_ptr<Camera>     createCamera(float aspectRatio);
    std::shared_ptr<SceneNode>  getRootNode();

    std::shared_ptr<Entity> getEntityWithId(IdType id);
    std::shared_ptr<Camera> getCameraWithId(IdType id);
    std::shared_ptr<Light>  getLightWithId(IdType id);

public:
    void      setMainCamera(const std::shared_ptr<Camera> &camera);
    void      setAmbient(glm::vec4 value);
    glm::vec4 getAmbient();

private:
    AABB      aabb;
    glm::vec4 _ambient;

    std::shared_ptr<SceneNode> _rootNode;
    std::shared_ptr<Camera>    _camera = nullptr;

    CameraMapList _cameraMapList;
    EntityMapList _entityMapList;
    LightMapList  _lightMapList;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
