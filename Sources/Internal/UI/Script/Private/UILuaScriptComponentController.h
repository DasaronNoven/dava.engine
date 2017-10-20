#pragma once

#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Script/UIScriptComponentController.h"

namespace DAVA
{
class UIContext;
class UIControl;
class LuaScript;

class UILuaScriptComponentController : public UIScriptComponentController
{
    DAVA_VIRTUAL_REFLECTION(UILuaScriptComponentController, UIScriptComponentController);

public:
    UILuaScriptComponentController(const FilePath& scriptPath);
    ~UILuaScriptComponentController() override;

    void Init(UIScriptComponent* component) override;
    void Release(UIScriptComponent* component) override;
    void ParametersChanged(UIScriptComponent* component) override;
    void Process(UIScriptComponent* component, float32 elapsedTime) override;
    bool ProcessEvent(UIScriptComponent* component, const FastName& eventName, const Vector<Any>& params = Vector<Any>()) override;

private:
    bool loaded = false;
    bool hasProcess = false;
    bool hasProcessEvent = false;
    std::unique_ptr<LuaScript> script;
};
}