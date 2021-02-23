//
// Created by Npchitman on 2021/1/20.
//

#ifndef HAZELENGINE_WINDOW_H
#define HAZELENGINE_WINDOW_H

#include "hzpch.h"
#include "Hazel/Core.h"
#include "Hazel/Events/Event.h"

namespace Hazel {

    // 窗口参数
    struct WindowProps {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        explicit WindowProps(std::string title = "Hazel Engine",
                             unsigned int width = 1280,
                             unsigned int height = 720)
                             : Title(std::move(title)), Width(width), Height(height) {}
    };

    // 基础窗口类型接口
    class HAZEL_API Window {
    public:
        // 窗口回调函数
        using EventCallbackFn = std::function<void(Event & )>;

        virtual ~Window() = default;

        // 窗口更新事件
        virtual void OnUpdate() = 0;

        // 获得窗口宽高
        virtual unsigned int GetWidth() const = 0;
        virtual unsigned int GetHeight() const = 0;

        // 函数回调
        virtual void SetEventCallback(const EventCallbackFn &callback) = 0;

        // 垂直同步
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void* GetNativeWindow() const = 0;

        // 创建窗口
        static Window *Create(const WindowProps &props = WindowProps());
    };
}

#endif //HAZELENGINE_WINDOW_H
