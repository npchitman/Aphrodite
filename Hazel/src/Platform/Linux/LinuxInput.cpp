//
// Created by npchitman on 6/1/21.
//

#include <GLFW/glfw3.h>

#include "Hazel/Core/Application.h"
#include "hzpch.h"

namespace Hazel {
    bool Input::IsKeyPressed(KeyCode keycode) {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetKey(window, static_cast<int32_t>(keycode));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(MouseCode button) {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }

    std::pair<float, float> Input::GetMousePosition() {
        auto window = static_cast<GLFWwindow *>(
                Application::Get().GetWindow().GetNativeWindow());
        double xPos, yPos;
        glfwGetCursorPos(window, &xPos, &yPos);

        return {static_cast<float>(xPos), static_cast<float>(yPos)};
    }

    float Input::GetMouseX() {
        auto [x, y] = GetMousePosition();
        return x;
    }

    float Input::GetMouseY() {
        auto [x, y] = GetMousePosition();
        return y;
    }
}
