//
//  AGInteractiveObject.h
//  Auragraph
//
//  Created by Spencer Salazar on 9/16/16.
//  Copyright © 2016 Spencer Salazar. All rights reserved.
//

#ifndef AGInteractiveObject_h
#define AGInteractiveObject_h


#include "AGRenderObject.h"
#include "gfx.h"

#include <list>


#ifdef __LP64__ // arm64
typedef uint64_t TouchID;
#else // arm32
typedef uint32_t TouchID;
#endif

#if __APPLE__ && TARGET_OS_IPHONE

#ifdef __OBJC__
typedef UITouch *AGPlatformTouchInfo;
#else
typedef void *AGPlatformTouchInfo;
#endif // __OBJC__

#else //
typedef void *AGPlatformTouchInfo;
#endif

class AGTouchOutsideListener;

//------------------------------------------------------------------------------
// ### AGTouchInfo ###
// Class representing a single touch.
//------------------------------------------------------------------------------
struct AGTouchInfo
{
    AGTouchInfo() { }
    AGTouchInfo(const GLvertex3f &_position, const CGPoint &_screenPosition, TouchID _touchId, AGPlatformTouchInfo _platformTouchInfo) :
    position(_position), screenPosition(_screenPosition), touchId(_touchId), platformTouchInfo(_platformTouchInfo)
    { }
    
    GLvertex3f position;
    CGPoint screenPosition;
    TouchID touchId;
    AGPlatformTouchInfo platformTouchInfo;
};


class AGInteractiveObject;


//------------------------------------------------------------------------------
// ### AGInteractive ###
// Basic pure virtual class for interactivity.
//------------------------------------------------------------------------------
class AGInteractive
{
public:
    virtual ~AGInteractive() { }
    
    virtual void touchDown(const AGTouchInfo &t) = 0;
    virtual void touchMove(const AGTouchInfo &t) = 0;
    virtual void touchUp(const AGTouchInfo &t) = 0;

    virtual AGInteractiveObject *hitTest(const GLvertex3f &t) = 0;
};


//------------------------------------------------------------------------------
// ### AGInteractiveObject ###
// Base class for objects that support interaction in addition to rendering.
//------------------------------------------------------------------------------
class AGInteractiveObject : public AGRenderObject, public AGInteractive
{
public:
    AGInteractiveObject();
    virtual ~AGInteractiveObject();
    
    // DEPRECATED
    virtual void touchDown(const GLvertex3f &t);
    virtual void touchMove(const GLvertex3f &t);
    virtual void touchUp(const GLvertex3f &t);
    
    // new version
    // subclasses generally should override these
    virtual void touchDown(const AGTouchInfo &t) override;
    virtual void touchMove(const AGTouchInfo &t) override;
    virtual void touchUp(const AGTouchInfo &t) override;
    
    // default implementation checks if touch is within this->effectiveBounds()
    virtual AGInteractiveObject *hitTest(const GLvertex3f &t) override;
    
    virtual AGInteractiveObject *userInterface() { return NULL; }
    
    void removeFromTopLevel();
    
    static void addTouchOutsideListener(AGTouchOutsideListener *);
    static void removeTouchOutsideListener(AGTouchOutsideListener *);
};


typedef std::list<AGInteractiveObject*> AGInteractiveObjectList;


#endif /* AGInteractiveObject_hpp */
