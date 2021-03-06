#pragma once

#include "REPlatform/DataNodes/Selectable.h"

#include <TArc/Qt/QtString.h>

#include <Functional/Signal.h>
#include <Reflection/Reflection.h>

namespace DAVA
{
class ContextAccessor;
class SceneTreeFilterBase : public DAVA::ReflectionBase
{
public:
    virtual ~SceneTreeFilterBase() = default;

    // Is matched should implement inly forward logic.
    // Filtration controller will invert filtration subset if needed and ignore filter if it disabled.
    virtual bool IsMatched(const Selectable& object, ContextAccessor* accessor) const = 0;
    virtual QString GetTitle() const = 0;

    bool IsEnabled() const;
    void SetEnabled(bool isEnabled);

    bool IsInverted() const;
    void SetInverted(bool isInverted);

    Signal<> changed;

    static const char* titleFieldName;
    static const char* enabledFieldName;
    static const char* inversedFieldName;

private:
    bool isEnabled = true;
    bool isInverted = false;

    DAVA_VIRTUAL_REFLECTION(SceneTreeFilterBase, ReflectionBase);
};

void RegisterPredefinedFilters();

} // namespace DAVA