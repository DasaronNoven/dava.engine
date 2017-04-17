#include "Scene3D/Systems/SlotSystem.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/SlotComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/TransformComponent.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Logger/Logger.h"

namespace DAVA
{
void SlotSystem::ItemsCache::LoadConfigFile(const FilePath& configPath)
{
    String extension = configPath.GetExtension();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension == ".yaml")
    {
        LoadYamlConfig(configPath);
    }
    else if (extension == ".xml")
    {
        LoadXmlConfig(configPath);
    }
}

void SlotSystem::ItemsCache::LoadYamlConfig(const FilePath& configPath)
{
    YamlParser* parser = YamlParser::Create(configPath);
    if (parser == nullptr)
    {
        Logger::Error("Couldn't parse yaml file %s", configPath.GetAbsolutePathname().c_str());
        return;
    }

    YamlNode* rootNode = parser->GetRootNode();
    if (rootNode == nullptr)
    {
        Logger::Error("Configuration file %s is empty", configPath.GetAbsolutePathname().c_str());
        return;
    }

    bool errorReported = false;
    Set<Item, ItemLess>& items = cachedItems[configPath.GetAbsolutePathname()];

    const DAVA::Vector<DAVA::YamlNode*>& yamlNodes = rootNode->AsVector();
    size_t propertiesCount = yamlNodes.size();
    for (size_t i = 0; i < propertiesCount; ++i)
    {
        YamlNode* currentNode = yamlNodes[i];
        uint32 fieldsCount = currentNode->GetCount();

        Item newItem;
        for (uint32 fieldIndex = 0; fieldIndex < fieldsCount; ++fieldIndex)
        {
            const YamlNode* fieldNode = currentNode->Get(fieldIndex);
            const String& key = currentNode->GetItemKeyName(fieldIndex);
            if (fieldNode->GetType() == YamlNode::TYPE_STRING)
            {
                if (key == "Name" && fieldNode->GetType() == YamlNode::TYPE_STRING)
                {
                    newItem.itemName = FastName(fieldNode->AsString());
                }
                else if (key == "Path")
                {
                    String path = fieldNode->AsString();
                    newItem.scenePath = FilePath(path);
                }
                else if (key == "Tag")
                {
                    newItem.tag = FastName(fieldNode->AsString());
                }
                else
                {
                    if (nullptr == newItem.additionalParams)
                    {
                        newItem.additionalParams.ConstructInplace();
                    }

                    newItem.additionalParams->SetString(key, fieldNode->AsString());
                }
            }
        }

        bool isItemValid = newItem.itemName.IsValid() && newItem.tag.IsValid() && newItem.scenePath.IsEmpty() == false;
        if (isItemValid == false && errorReported == false)
        {
            Logger::Error("Yaml parsing error. Config file %s, contains incomplete items", configPath.GetAbsolutePathname().c_str());
            errorReported = true;
        }
        else
        {
            items.insert(newItem);
        }
    }
}

void SlotSystem::ItemsCache::LoadXmlConfig(const FilePath& configPath)
{
}

const SlotSystem::ItemsCache::Item* SlotSystem::ItemsCache::LookUpItem(const FilePath& configPath, const FastName& itemName, const FastName& tag)
{
    String absolutePath = configPath.GetAbsolutePathname();
    auto configIter = cachedItems.find(absolutePath);
    if (configIter == cachedItems.end())
    {
        LoadConfigFile(configPath);
        configIter = cachedItems.find(absolutePath);
    }

    if (configIter == cachedItems.end())
    {
        return nullptr;
    }

    Item key;
    key.itemName = itemName;
    key.tag = tag;

    auto itemIter = configIter->second.find(key);
    if (itemIter == configIter->second.end())
    {
        return nullptr;
    }

    return &(*itemIter);
}

Vector<SlotSystem::ItemsCache::Item> SlotSystem::ItemsCache::GetItems(const FilePath& configPath)
{
    Vector<Item> result;

    String absolutePath = configPath.GetAbsolutePathname();
    auto configIter = cachedItems.find(absolutePath);
    if (configIter == cachedItems.end())
    {
        LoadConfigFile(configPath);
        configIter = cachedItems.find(absolutePath);
    }

    if (configIter != cachedItems.end())
    {
        std::copy(configIter->second.begin(), configIter->second.end(), std::back_inserter(result));
    }

    return result;
}

bool SlotSystem::ItemsCache::ItemLess::operator()(const Item& item1, const Item& item2) const
{
    if (item1.tag != item2.tag)
    {
        return item1.tag < item2.tag;
    }

    return item1.itemName < item2.itemName;
}

void SlotSystem::ExternalEntityLoader::SetScene(Scene* scene)
{
    this->scene = scene;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              SlotSystem                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SlotSystem::SlotSystem(Scene* scene)
    : SceneSystem(scene)
    , sharedCache(new ItemsCache())
{
    deletePending.reserve(4);
}

SlotSystem::~SlotSystem()
{
}

void SlotSystem::SetSharedCache(RefPtr<ItemsCache> cache)
{
    sharedCache = cache;
}

Vector<SlotSystem::ItemsCache::Item> SlotSystem::GetItems(const FilePath& configPath)
{
    return sharedCache->GetItems(configPath);
}

void SlotSystem::SetExternalEntityLoader(RefPtr<ExternalEntityLoader> externalEntityLoader_)
{
    externalEntityLoader = externalEntityLoader_;
    if (nullptr != externalEntityLoader)
    {
        externalEntityLoader->SetScene(GetScene());
    }
}

void SlotSystem::UnregisterEntity(Entity* entity)
{
    auto iter = loadedEntityToSlot.find(entity);
    if (iter != loadedEntityToSlot.end())
    {
        slotToLoadedEntity[iter->second] = nullptr;
        loadedEntityToSlot.erase(iter);
    }
    SceneSystem::UnregisterEntity(entity);
}

void SlotSystem::AddEntity(Entity* entity)
{
    uint32 count = entity->GetComponentCount(Component::SLOT_COMPONENT);
    for (uint32 i = 0; i < count; ++i)
    {
        AddComponent(entity, entity->GetComponent(Component::SLOT_COMPONENT, i));
    }
}

void SlotSystem::RemoveEntity(Entity* entity)
{
    uint32 count = entity->GetComponentCount(Component::SLOT_COMPONENT);
    for (uint32 i = 0; i < count; ++i)
    {
        RemoveComponent(entity, entity->GetComponent(Component::SLOT_COMPONENT, i));
    }
}

void SlotSystem::AddComponent(Entity* entity, Component* component)
{
    slotToLoadedEntity[component] = nullptr;
}

void SlotSystem::RemoveComponent(Entity* entity, Component* component)
{
    auto iter = slotToLoadedEntity.find(component);
    if (iter != slotToLoadedEntity.end())
    {
        if (iter->second != nullptr)
        {
            deletePending.push_back(iter->second);
            loadedEntityToSlot.erase(iter->second);
        }
        slotToLoadedEntity.erase(iter);
    }
}

void SlotSystem::Process(float32 timeElapsed)
{
    if (deletePending.empty() == false)
    {
        for (Entity* e : deletePending)
        {
            Entity* parent = e->GetParent();
            parent->RemoveNode(e);
        }

        deletePending.clear();
    }
}

void SlotSystem::AttachItemToSlot(Entity* rootEntity, FastName slotName, FastName itemName)
{
    uint32 slotsCount = rootEntity->GetComponentCount(Component::SLOT_COMPONENT);
    for (uint32 i = 0; i < slotsCount; ++i)
    {
        SlotComponent* slotComponent = static_cast<SlotComponent*>(rootEntity->GetComponent(Component::SLOT_COMPONENT, i));
        if (slotComponent->GetSlotName() == slotName)
        {
            AttachItemToSlot(slotComponent, itemName);
        }
    }

    uint32 childCount = rootEntity->GetChildrenCount();
    for (uint32 childIndex = 0; childIndex < childCount; ++childIndex)
    {
        AttachItemToSlot(rootEntity->GetChild(childIndex), slotName, itemName);
    }
}

Entity* SlotSystem::AttachItemToSlot(SlotComponent* component, FastName itemName)
{
    UnloadItem(component);

    const FilePath& configPath = component->GetConfigFilePath();
    uint32 filtersCount = component->GetFiltersCount();

    Entity* resultEntity = nullptr;

    for (uint32 filterIndex = 0; filterIndex < filtersCount; ++filterIndex)
    {
        const ItemsCache::Item* item = sharedCache->LookUpItem(configPath, itemName, component->GetFilter(filterIndex));
        if (item != nullptr)
        {
            Entity* loadedEntity = nullptr;
            if (nullptr == externalEntityLoader)
            {
                loadedEntity = GetScene()->cache.GetClone(item->scenePath);
            }
            else
            {
                loadedEntity = externalEntityLoader->Load(item->scenePath);
            }

            if (loadedEntity == nullptr)
            {
                Logger::Error("Couldn't load item %s with path %s into slot", itemName.c_str(), item->scenePath.GetStringValue().c_str());
                continue;
            }

            resultEntity = loadedEntity;
            AttachEntityToSlot(component, loadedEntity);
            break;
        }
    }

    return resultEntity;
}

void SlotSystem::AttachEntityToSlot(SlotComponent* component, Entity* entity)
{
    UnloadItem(component);

    entity->SetName(component->GetSlotName());

    TransformComponent* transform = GetTransformComponent(entity);
    DVASSERT(transform != nullptr);
    transform->SetLocalTransform(&component->GetAttachmentTransform());

    Entity* parentEntity = component->GetEntity();
    if (nullptr == externalEntityLoader)
    {
        parentEntity->AddNode(entity);
    }
    else
    {
        externalEntityLoader->AddEntity(parentEntity, entity);
    }

    slotToLoadedEntity[component] = entity;
    loadedEntityToSlot[entity] = component;
}

Entity* SlotSystem::LookUpLoadedEntity(SlotComponent* component) const
{
    DVASSERT(component->GetEntity() != nullptr);
    auto iter = slotToLoadedEntity.find(component);
    DVASSERT(iter != slotToLoadedEntity.end());
    return iter->second;
}

SlotComponent* SlotSystem::LookUpSlot(Entity* entity) const
{
    auto iter = loadedEntityToSlot.find(entity);
    if (iter != loadedEntityToSlot.end())
    {
        DVASSERT(iter->second->GetType() == Component::SLOT_COMPONENT);
        return static_cast<SlotComponent*>(iter->second);
    }

    return nullptr;
}

void SlotSystem::SetScene(Scene* scene)
{
    SceneSystem::SetScene(scene);
    if (nullptr != externalEntityLoader)
    {
        externalEntityLoader->SetScene(scene);
    }
}

void SlotSystem::UnloadItem(SlotComponent* component)
{
    auto iter = slotToLoadedEntity.find(component);
    if (iter != slotToLoadedEntity.end() && iter->second != nullptr)
    {
        Entity* slotEntity = component->GetEntity();
        Entity* loadedEntity = iter->second;

        DVASSERT(slotEntity == loadedEntity->GetParent());
        slotEntity->RemoveNode(loadedEntity);
    }
}

} // namespace DAVA
