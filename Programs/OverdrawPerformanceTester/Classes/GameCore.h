#pragma once

#include "DAVAEngine.h"
#include "Database/MongodbClient.h"

namespace DAVA
{
class Engine;
class Window;
}

class StartScreen;
class ViewSceneScreen;
class GameCore
{
public:
    GameCore(DAVA::Engine& e);

    static GameCore* Instance()
    {
        return instance;
    };

    void OnAppStarted();
    void OnWindowCreated(DAVA::Window* w);
    void OnAppFinished();

    void SetScenePath(const DAVA::FilePath& path)
    {
        scenePath = path;
    };

    const DAVA::FilePath& GetScenePath() const
    {
        return scenePath;
    };

protected:
    StartScreen* selectSceneScreen;
    ViewSceneScreen* viewSceneScreen;

    DAVA::FilePath scenePath;

private:
    DAVA::Engine& engine;

    static GameCore* instance;
    static const DAVA::String testerScenePath;
};
