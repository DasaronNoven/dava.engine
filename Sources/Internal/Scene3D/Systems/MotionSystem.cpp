#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/MotionComponent.h"
#include "Scene3D/Components/SingleComponents/MotionSingleComponent.h"
#include "Scene3D/SkeletonAnimation/SkeletonAnimation.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Systems/MotionSystem.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"

namespace DAVA
{
MotionSystem::MotionSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

MotionSystem::~MotionSystem()
{
    GetScene()->GetEventSystem()->UnregisterSystemForEvent(this, EventSystem::SKELETON_CONFIG_CHANGED);
}

void MotionSystem::AddEntity(Entity* entity)
{
    MotionComponent* motionComponent = GetMotionComponent(entity);

    motions.push_back(motionComponent);
    GetScene()->motionSingleComponent->rebindAnimation.push_back(motionComponent);
}

void MotionSystem::RemoveEntity(Entity* entity)
{
    FindAndRemoveExchangingWithLast(motions, GetMotionComponent(entity));
}

void MotionSystem::ImmediateEvent(Component* component, uint32 event)
{
    if (event == EventSystem::SKELETON_CONFIG_CHANGED)
    {
        MotionComponent* motionComponent = GetMotionComponent(component->GetEntity());
        GetScene()->motionSingleComponent->rebindAnimation.push_back(motionComponent);
    }
}

void MotionSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_MOTION_SYSTEM);

    Vector<std::pair<MotionComponent*, FastName>> triggeredEvents;

    MotionSingleComponent* msc = GetScene()->motionSingleComponent;
    for (MotionComponent* motionComponent : msc->rebindAnimation)
    {
        SkeletonComponent* skeleton = GetSkeletonComponent(motionComponent->GetEntity());
        MotionComponent::SimpleMotion* motion = motionComponent->simpleMotion;

        motion->BindSkeleton(skeleton);
        skeleton->ApplyPose(motion->GetAnimation()->GetSkeletonPose());
    }

    for (MotionComponent* motionComponent : msc->stopAnimation)
    {
        SkeletonComponent* skeleton = GetSkeletonComponent(motionComponent->GetEntity());
        MotionComponent::SimpleMotion* motion = motionComponent->simpleMotion;

        motion->Stop();
        skeleton->ApplyPose(motion->GetAnimation()->GetSkeletonPose());
    }

    for (MotionComponent* motionComponent : msc->startAnimation)
    {
        motionComponent->simpleMotion->Start();
        triggeredEvents.emplace_back(motionComponent, MotionComponent::EVENT_SINGLE_ANIMATION_STARTED);
    }

    for (MotionComponent* motionComponent : motions)
    {
        MotionComponent::SimpleMotion* motion = motionComponent->simpleMotion;

        if (motion->IsPlaying())
        {
            motion->Update(timeElapsed);

            if (motion->IsFinished())
            {
                motion->Stop();
                triggeredEvents.emplace_back(motionComponent, MotionComponent::EVENT_SINGLE_ANIMATION_ENDED);
            }

            SkeletonComponent* skeleton = GetSkeletonComponent(motionComponent->GetEntity());
            skeleton->ApplyPose(motion->GetAnimation()->GetSkeletonPose());
        }
    }

    msc->Clear();
    msc->eventTriggered = triggeredEvents;
}
}