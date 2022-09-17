#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "vklCamera.h"
#include "vklEntity.h"
#include "vklLight.h"

namespace vkl {

enum class SCENE_UNIFORM_TYPE : uint8_t {
    UNDEFINED,
    CAMERA,
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    FLASH_LIGHT,
};

enum class SCENE_RENDER_TYPE : uint8_t {
    OPAQUE,
    TRANSPARENCY,
};

struct SceneNode {
    SceneNode *_parent;
    glm::mat4  _matrix;

    std::vector<SceneNode *> _children;

    SceneNode *createChildNode() {
        SceneNode *childNode = new SceneNode;
        _children.push_back(childNode);
        return childNode;
    }

    void setTransform(glm::mat4 matrix){
        _matrix = matrix;
    }
};

struct SceneEntityNode : SceneNode {
    vkl::Entity     *_entity;
    vkl::ShaderPass *_pass = nullptr;

    SceneEntityNode(vkl::Entity *entity, vkl::ShaderPass *pass, glm::mat4 transform)
        : _entity(entity), _pass(pass) {
        setTransform(_matrix);
    }

    void setShaderPass(vkl::ShaderPass * pass){
        _pass = pass;
    }
};

struct SceneLightNode : SceneNode {
    SCENE_UNIFORM_TYPE _type;

    vkl::Light *_object = nullptr;

    SceneLightNode(vkl::Light *object, SCENE_UNIFORM_TYPE uniformType = SCENE_UNIFORM_TYPE::UNDEFINED)
        : _type(uniformType), _object(object) {
    }
};

struct SceneCameraNode : SceneNode {
    SCENE_UNIFORM_TYPE _type = SCENE_UNIFORM_TYPE::CAMERA;

    SceneCamera *_object = nullptr;

    SceneCameraNode(vkl::SceneCamera *camera)
        : _object(camera) {
    }
};

class SceneManager {
public:
    Light*  createLight();
    Entity* createEntity(ShaderPass *pass = nullptr, glm::mat4 transform = glm::mat4(1.0f), SCENE_RENDER_TYPE renderType = SCENE_RENDER_TYPE::OPAQUE);
    SceneCamera* createCamera(float aspectRatio);

    uint32_t getRenderableCount() const;
    uint32_t getUBOCount() const;

    void destroy(){
        for (auto * node : _renderNodeList){
            node->_entity->destroy();
            delete node;
        }

        for (auto * node: _lightNodeList){
            node->_object->destroy();
            delete node;
        }

        _camera->_object->destroy();
        delete _camera;
    }

public:
    std::vector<SceneEntityNode *>  _renderNodeList;
    std::vector<SceneLightNode *> _lightNodeList;

    SceneCameraNode *_camera = nullptr;
};

} // namespace vkl

#endif // VKLSCENEMANGER_H_
