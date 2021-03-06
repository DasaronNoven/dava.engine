#pragma once

#include "TArc/Controls/PropertyPanel/BaseComponentValue.h"

namespace DAVA
{
class MatrixComponentValue : public BaseComponentValue
{
public:
    MatrixComponentValue() = default;

protected:
    Any GetMultipleValue() const override;
    bool IsValidValueToSet(const Any& newValue, const Any& currentValue) const override;
    ControlProxy* CreateEditorWidget(QWidget* parent, const Reflection& model, DataWrappersProcessor* wrappersProcessor) override;

    String GetTextValue() const;

    DAVA_VIRTUAL_REFLECTION(MatrixComponentValue, BaseComponentValue);
};
} // namespace DAVA
