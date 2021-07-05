//
// Created by npchitman on 6/27/21.
//

#ifndef Aphrodite_ENGINE_COMPONENTS_H
#define Aphrodite_ENGINE_COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Aphrodite/Renderer/Texture.h"
#include "Aphrodite/Scene/SceneCamera.h"
#include "Aphrodite/Scene/ScriptableEntity.h"

namespace Aph {
    struct IDComponent {
        uint32_t ID = 0;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        IDComponent(const uint32_t id)
            : ID(id) {}
        operator uint32_t() { return ID; }
    };

    struct TagComponent {
        std::string Tag;
        bool renaming = false;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        explicit TagComponent(std::string tag) : Tag(std::move(tag)) {}
    };

    struct TransformComponent {
        glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        explicit TransformComponent(const glm::vec3& translation) : Translation(translation) {}

        glm::mat4 GetTransform() const {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

            return translation * rotation * scale;
        }
    };

    struct SpriteRendererComponent {
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        Ref<Texture2D> Texture = nullptr;
        float TilingFactor = 1.0f;
        std::string TextureFilepath;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        explicit SpriteRendererComponent(const glm::vec4& color)
            : Color(color) {}
        void SetTexture(std::string& filepath) {
            Texture = Texture2D::Create(filepath);
            TextureFilepath = filepath;
        }
        void RemoveTexture() { Texture = nullptr; }
    };


    struct CameraComponent {
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;
    };

    struct NativeScriptComponent {
        ScriptableEntity* Instance = nullptr;

        ScriptableEntity* (*InstantiateScript)(){};
        void (*DestroyScript)(NativeScriptComponent*){};

        template<typename T>
        void Bind() {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
        }
    };

}// namespace Aph


#endif//Aphrodite_ENGINE_COMPONENTS_H
