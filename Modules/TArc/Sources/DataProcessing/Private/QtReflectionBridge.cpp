#include "QtReflectionBridge.h"
#include "Base/Any.h"
#include "Debug/DVAssert.h"

#include "Reflection/Public/ReflectedType.h"

#include <QtCore/private/qmetaobjectbuilder_p.h>
#include <QFileSystemModel>

Q_DECLARE_METATYPE(DAVA::char16);

namespace tarc
{

namespace ReflBridgeDetails
{

template<typename T>
DAVA::Any ToAny(const QVariant& v)
{
    DAVA::Any ret;
    ret.Set(v.value<T>());
    return ret;
}

DAVA::Any QStringToAny(const QVariant& v)
{
    return DAVA::Any(v.toString().toStdString());
}

template<typename T>
QVariant ToVariant(const DAVA::Any& v)
{
    return QVariant::fromValue(v.Get<T>());
}

QVariant StringToVariant(const DAVA::Any& v)
{
    return QVariant::fromValue(QString::fromStdString(v.Get<DAVA::String>()));
}


template<typename T>
void FillConverter(DAVA::UnorderedMap<const DAVA::Type*, QVariant(*)(const DAVA::Any&)> & anyToVar,
                   DAVA::UnorderedMap<int, DAVA::Any(*)(const QVariant&)> & varToAny)
{
    anyToVar.emplace(DAVA::Type::Instance<T>(), &ToVariant<T>);
    varToAny.emplace(qMetaTypeId<T>(), &ToAny<T>);
}

}

#define FOR_ALL_BUILDIN_TYPES(F, ATV, VTA) \
    F(void*, ATV, VTA) \
    F(bool, ATV, VTA) \
    F(DAVA::int8, ATV, VTA) \
    F(DAVA::uint8, ATV, VTA) \
    F(DAVA::int16, ATV, VTA) \
    F(DAVA::uint16, ATV, VTA) \
    F(DAVA::int32, ATV, VTA) \
    F(DAVA::uint32, ATV, VTA) \
    F(DAVA::int64, ATV, VTA) \
    F(DAVA::uint64, ATV, VTA) \
    F(DAVA::char8, ATV, VTA) \
    F(DAVA::char16, ATV, VTA) \
    F(DAVA::float32, ATV, VTA) \
    F(DAVA::float64, ATV, VTA)

#define FOR_ALL_QT_SPECIFIC_TYPES(F, ATV, VTA) \
    F(QFileSystemModel*, ATV, VTA) \
    F(QModelIndex, ATV, VTA)

#define FOR_ALL_STATIC_TYPES(F, ATV, VTA) \
    FOR_ALL_BUILDIN_TYPES(F, ATV, VTA) \
    FOR_ALL_QT_SPECIFIC_TYPES(F, ATV, VTA)

#define FILL_CONVERTERS_FOR_TYPE(T, ATV, VTA) \
    ReflBridgeDetails::FillConverter<T>(ATV, VTA);

#define FILL_CONVERTES_FOR_CUSTOM_TYPE(ATV, VTA, ANY_TYPE, VAR_TYPE)\
    ATV.emplace(DAVA::Type::Instance<DAVA::ANY_TYPE>(), &ReflBridgeDetails::ANY_TYPE##ToVariant); \
    VTA.emplace(qMetaTypeId<VAR_TYPE>(), &ReflBridgeDetails::VAR_TYPE##ToAny);

QtReflected::QtReflected(QtReflectionBridge* reflectionBridge_, DataWrapper&& wrapper_, QObject* parent)
    : QObject(parent)
    , reflectionBridge(reflectionBridge_)
    , wrapper(std::move(wrapper_))
{
    DVASSERT(parent != nullptr);
}

const QMetaObject* QtReflected::metaObject() const
{
    if (qtMetaObject == nullptr)
        return &QObject::staticMetaObject;

    return qtMetaObject;
}

int QtReflected::qt_metacall(QMetaObject::Call c, int id, void **argv)
{
    id = QObject::qt_metacall(c, id, argv);

    if (id < 0)
    {
        return id;
    }

    switch (c)
    {
    case QMetaObject::InvokeMetaMethod:
    {
        CallMethod(id, argv);
        int methodCount = qtMetaObject->methodCount() - qtMetaObject->methodOffset();
        id -= methodCount;
        break;
    }
    case QMetaObject::ReadProperty:
    case QMetaObject::WriteProperty:
    {
        int propertyCount = qtMetaObject->propertyCount() - qtMetaObject->propertyOffset();
        if (wrapper.HasData())
        {
            DAVA::Reflection reflection = wrapper.GetData();
            DAVA::Vector<DAVA::Reflection::Field> fields = reflection.GetFields();
            const DAVA::Reflection::Field& field = fields[id];
            QVariant* value = reinterpret_cast<QVariant*>(argv[0]);
            DAVA::Any davaValue = field.ref.GetValue();
            if (c == QMetaObject::ReadProperty)
            {
                *value = reflectionBridge->Convert(davaValue);
            }
            else
            {
                DAVA::Any newValue = reflectionBridge->Convert(*value);
                if (newValue != davaValue)
                {
                    field.ref.SetValue(newValue);
                    wrapper.Sync(false);
                }
            }
        }

        id -= propertyCount;
    }
    break;
    default:
    break;
    }

    return id;
}

void QtReflected::Init()
{
    wrapper.AddListener(this);
    if (wrapper.HasData())
    {
        CreateMetaObject();
    }
}

void QtReflected::OnDataChanged(const DataWrapper& dataWrapper, const DAVA::Set<DAVA::String>& fields)
{
    if (qtMetaObject == nullptr)
    {
        if (reflectionBridge == nullptr)
        {
            wrapper.RemoveListener(this);
            return;
        }

        CreateMetaObject();
    }

    if (fields.empty())
    {
        for (int i = qtMetaObject->methodOffset(); i < qtMetaObject->methodCount(); ++i)
        {
            QMetaMethod method = qtMetaObject->method(i);
            if (method.methodType() == QMetaMethod::Signal)
            {
                FirePropertySignal(i);
            }
        }
    }
    else
    {
        for (const DAVA::String& fieldName : fields)
        {
            FirePropertySignal(fieldName);
        }
    }
}

void QtReflected::CreateMetaObject()
{
    DVASSERT(reflectionBridge != nullptr);
    DVASSERT(wrapper.HasData());
    DAVA::Reflection reflectionData = wrapper.GetData();

    const DAVA::ReflectedType* type = reflectionData.GetObjectType();

    SCOPE_EXIT
    {
        metaObjectCreated.Emit();
    };

    auto iter = reflectionBridge->metaObjects.find(type);
    if (iter != reflectionBridge->metaObjects.end())
    {
        qtMetaObject = iter->second;
        return;
    }

    QMetaObjectBuilder builder;

    builder.setClassName(type->GetPermanentName().c_str());
    builder.setSuperClass(&QObject::staticMetaObject);

    DAVA::Vector<DAVA::Reflection::Field> fields = reflectionData.GetFields();
    for (const DAVA::Reflection::Field& f : fields)
    {
        QByteArray propertyName = QByteArray(f.key.Cast<DAVA::String>().c_str());
        QMetaPropertyBuilder propertybuilder = builder.addProperty(propertyName, "QVariant");
        propertybuilder.setWritable(!f.ref.IsReadonly());

        QByteArray notifySignal = propertyName + "Changed";
        propertybuilder.setNotifySignal(builder.addSignal(notifySignal));
    }

    DAVA::Vector<DAVA::Reflection::Method> methods = reflectionData.GetMethods();
    for (const DAVA::Reflection::Method& method : methods)
    {
        DAVA::String signature = method.key + "(";
        const DAVA::AnyFn::InvokeParams& params = method.fn.GetInvokeParams();
        size_t paramsCount = params.argsType.size();
        for (size_t i = 0; i < paramsCount; ++i)
        {
            if (i == paramsCount - 1)
            {
                signature += "QVariant";
            }
            else
            {
                signature += "QVariant,";
            }
        }
        signature += ")";

        DAVA::String retValue = "QVariant";
        if (params.retType == DAVA::Type::Instance<void>())
        {
            retValue = "void";
        }

        builder.addMethod(signature.c_str(), retValue.c_str());
    }

    qtMetaObject = builder.toMetaObject();

    reflectionBridge->metaObjects.emplace(type, qtMetaObject);
}

void QtReflected::FirePropertySignal(const DAVA::String& propertyName)
{
    DAVA::String signalName = propertyName + "Changed";
    int id = qtMetaObject->indexOfSignal(signalName.c_str());
    FirePropertySignal(id);
}

void QtReflected::FirePropertySignal(int signalId)
{
    DVASSERT(signalId != -1);
    void* argv[] = { nullptr };
    qtMetaObject->activate(this, signalId, argv);
}

void QtReflected::CallMethod(int id, void** argv)
{
    int propertyCount = qtMetaObject->propertyCount() - qtMetaObject->propertyOffset();

    // Qt store "PropertyChanged signals" as methods at the start of table.
    // argument "id" can be translated to qt table by this operation "id + qtMetaObject->methodOffset()"
    // So to calculate method index in our table we deduct property count from id,
    // because on every property we created signal.
    int methodIndexToCall = id - propertyCount;

    DAVA::Reflection reflectedData = wrapper.GetData();
    DAVA::Vector<DAVA::Reflection::Method> methods = reflectedData.GetMethods();
    DVASSERT(methodIndexToCall < methods.size());
    DAVA::Reflection::Method method = methods[methodIndexToCall];
    const DAVA::AnyFn::InvokeParams& args = method.fn.GetInvokeParams();

    QVariant* qtResult = reinterpret_cast<QVariant*>(argv[0]);
    // first element in argv is pointer on return value
    size_t firstArgumentIndex = 1;
    size_t argumentsCount = args.argsType.size() + 1;

    DAVA::Vector<DAVA::Any> davaArguments;
    davaArguments.reserve(args.argsType.size());

    for (size_t i = firstArgumentIndex; i < argumentsCount; ++i)
    {
        davaArguments.push_back(reflectionBridge->Convert(*reinterpret_cast<QVariant*>(argv[i])));
    }

    DAVA::Any davaResult;
    switch (davaArguments.size())
    {
    case 0:
        davaResult = method.fn.Invoke();
        break;
    case 1:
        davaResult = method.fn.Invoke(davaArguments[0]);
        break;
    case 2:
        davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1]);
        break;
    case 3:
        davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2]);
        break;
    case 4:
        davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3]);
        break;
    case 5:
        davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3],
                                      davaArguments[4]);
        break;
    case 6:
        davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3],
                                      davaArguments[4], davaArguments[5]);
        break;
    //case 7:
    //    davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3],
    //                                  davaArguments[4], davaArguments[5], davaArguments[6]);
    //    break;
    //case 8:
    //    davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3],
    //                                  davaArguments[4], davaArguments[5], davaArguments[6], davaArguments[7]);
    //    break;
    //case 9:
    //    davaResult = method.fn.Invoke(davaArguments[0], davaArguments[1], davaArguments[2], davaArguments[3],
    //                                  davaArguments[4], davaArguments[5], davaArguments[6], davaArguments[7],
    //                                  davaArguments[8]);
        break;
    default:
        DVASSERT_MSG(false, "Qt Reflection bridge support only 9 arguments in methods");
        break;
    }
    
    if (qtResult)
    {
        *qtResult = reflectionBridge->Convert(davaResult);
    }
}

QtReflectionBridge::QtReflectionBridge()
{
    FOR_ALL_STATIC_TYPES(FILL_CONVERTERS_FOR_TYPE, anyToQVariant, qvariantToAny);
    FILL_CONVERTES_FOR_CUSTOM_TYPE(anyToQVariant, qvariantToAny, String, QString);
}

QtReflectionBridge::~QtReflectionBridge()
{
    for (auto node : metaObjects)
    {
        free(node.second);
    }
}

QtReflected* QtReflectionBridge::CreateQtReflected(DataWrapper&& wrapper, QObject* parent)
{
    return new QtReflected(this, std::move(wrapper), parent);
}

QVariant QtReflectionBridge::Convert(const DAVA::Any& value)
{
    auto iter = anyToQVariant.find(value.GetType());
    if (iter == anyToQVariant.end())
    {
        DVASSERT_MSG(false, DAVA::Format("Converted (Any->QVariant) has not been registered for type : %s", value.GetType()->GetName()).c_str());
        return QVariant();
    }

    return iter->second(value);
}

DAVA::Any QtReflectionBridge::Convert(const QVariant& value)
{
    auto iter = qvariantToAny.find(value.userType());
    if (iter == qvariantToAny.end())
    {
        DVASSERT_MSG(false, DAVA::Format("Converted (QVariant->Any) has not been registered for userType : %d", value.userType()).c_str());
        return DAVA::Any();
    }

    return iter->second(value);
}

}
