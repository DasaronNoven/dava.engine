#ifndef __DAVAENGINE_BULLET_COMPONENT_H__
#define __DAVAENGINE_BULLET_COMPONENT_H__

#include "Base/BaseTypes.h"
#include "Entity/Component.h"

namespace DAVA
{

class BaseObject;
class BulletComponent : public Component
{
public:
	IMPLEMENT_COMPONENT_TYPE(BULLET_COMPONENT);

	BulletComponent();
	virtual ~BulletComponent();
	virtual Component * Clone();

	void SetBulletObject(BaseObject * bulletObject);
	BaseObject * GetBulletObject();

private:
	BaseObject * bulletObject;
    
public:
    INTROSPECTION_EXTEND(BulletComponent, Component,
        MEMBER(bulletObject, "Bullet Object", INTROSPECTION_SERIALIZABLE | INTROSPECTION_EDITOR)
    );
};

}

#endif //__DAVAENGINE_BULLET_COMPONENT_H__
