#ifndef SCENENODE_H_
#define SCENENODE_H_

#include <functional>

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"

namespace aph
{
template <typename TNode>
class Node : public Object
{
public:
    Node(TNode* parent, IdType id, ObjectType type, glm::mat4 transform = glm::mat4(1.0f), std::string name = "") :
        Object{id, type},
        name{std::move(name)},
        parent{parent},
        matrix{transform}
    {
        if constexpr(std::is_same_v<TNode, Node>)
        {
            if(parent->parent)
            {
                name = parent->name + "-" + std::to_string(id);
            }
            else
            {
                name = std::to_string(id);
            }
        }
    }

    TNode* createChildNode(glm::mat4 transform = glm::mat4(1.0f), std::string name = "")
    {
        auto childNode = std::unique_ptr<TNode>(new TNode(static_cast<TNode*>(this), transform, std::move(name)));
        children.push_back(std::move(childNode));
        return children.back().get();
    }

    glm::mat4 getTransform()
    {
        glm::mat4 res         = matrix;
        auto      currentNode = parent;
        while(currentNode)
        {
            res         = currentNode->matrix * res;
            currentNode = currentNode->parent;
        }
        return res;
    }

    void addChild(std::unique_ptr<TNode>&& childNode) { children.push_back(std::move(childNode)); }

    std::string_view getName() const { return name; }

    void rotate(float angle, glm::vec3 axis) { matrix = glm::rotate(matrix, angle, axis); }
    void translate(glm::vec3 value) { matrix = glm::translate(matrix, value); }
    void scale(glm::vec3 value) { matrix = glm::scale(matrix, value); }

protected:
    std::string                         name     = {};
    std::vector<std::unique_ptr<TNode>> children = {};
    TNode*                              parent   = {};
    glm::mat4                           matrix   = {glm::mat4(1.0f)};
};

class SceneNode : public Node<SceneNode>
{
public:
    SceneNode(SceneNode* parent, glm::mat4 matrix = glm::mat4(1.0f), std::string name = "");
    ObjectType getAttachType() const { return m_object ? m_object->getType() : ObjectType::UNATTACHED; };
    IdType     getAttachObjectId() { return m_object->getId(); }

    template <typename TObject>
    void attachObject(TObject* object)
    {
        if constexpr(isObjectTypeValid<TObject>())
        {
            m_object = object;
        }
        else
        {
            CM_LOG_ERR("Invalid type of the object.");
            APH_ASSERT(false);
        }
    }

    template <typename TObject>
    TObject* getObject()
    {
        if constexpr(isObjectTypeValid<TObject>())
        {
            return static_cast<TObject*>(m_object);
        }
        else
        {
            CM_LOG_ERR("Invalid type of the object.");
            APH_ASSERT(false);
        }
    }

    template <typename TObject>
    constexpr static bool isObjectTypeValid()
    {
        return std::is_same_v<TObject, Camera> || std::is_same_v<TObject, Light> || std::is_same_v<TObject, Mesh>;
    }

    void traversalChildren(std::function<void(SceneNode* node)>&& func)
    {
        std::queue<SceneNode*> q;
        q.push(this);

        while(!q.empty())
        {
            auto* node = q.front();
            q.pop();

            func(node);

            for(const auto& subNode : node->children)
            {
                q.push(subNode.get());
            }
        }
    }

private:
    Object* m_object{};
};
}  // namespace aph

#endif  // SCENENODE_H_
