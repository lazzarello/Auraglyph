//
//  AGRenderModel.cpp
//  Auraglyph
//
//  Created by Spencer Salazar on 12/29/19.
//  Copyright © 2019 Spencer Salazar. All rights reserved.
//

#include "AGRenderModel.h"

GLvertex3f AGRenderModel::screenToWorld(CGPoint p)
{
    int viewport[] = {
        (int)m_screenBounds.origin.x, (int)(m_screenBounds.origin.y),
        (int)m_screenBounds.size.width, (int)m_screenBounds.size.height
    };
    bool success;
    
    // get window-z coordinate at (0, 0, 0)
    GLKVector3 probe = GLKMathProject(GLKVector3Make(0, 0, 0), modelView, projection, viewport);
    
    GLKVector3 vec = GLKMathUnproject(GLKVector3Make(p.x, m_screenBounds.size.height-p.y, probe.z),
                                      modelView, projection, viewport, &success);
    
    return GLvertex3f(vec.x, vec.y, 0);
}

GLvertex3f AGRenderModel::screenToFixed(CGPoint p)
{
    int viewport[] = { (int)m_screenBounds.origin.x, (int)(m_screenBounds.origin.y),
        (int)m_screenBounds.size.width, (int)m_screenBounds.size.height };
    bool success;
    GLKVector3 vec = GLKMathUnproject(GLKVector3Make(p.x, m_screenBounds.size.height-p.y, 0.0f),
                                      fixedModelView, projection, viewport, &success);
    
    return GLvertex3f(vec.x, vec.y, 0);
}

void AGRenderModel::setScreenBounds(CGRect bounds)
{
    m_screenBounds = bounds;
    updateMatrices();
    
    modalOverlay.setScreenSize(GLvertex2f(m_screenBounds.size.width,
                                          m_screenBounds.size.height));
}

void AGRenderModel::update(float dt)
{
    t += dt;
    
    cameraZ.interp();
    
    updateMatrices();
}

void AGRenderModel::updateMatrices()
{
    /* PROJECTION MATRIX */
    projection = Matrix4::makeFrustum(-m_screenBounds.size.width/2, m_screenBounds.size.width/2,
                                      -m_screenBounds.size.height/2, m_screenBounds.size.height/2,
                                      10.0f, 10000.0f);
    
    /* FIXED MODEL/VIEW MATRIX (e.g does not move with camera) */
    fixedModelView = Matrix4::makeTranslation(0, 0, -10.1f);
        
    // camera
    dbgprint_off("cameraZ: %f\n", (float) cameraZ);
    
    float cameraScale = 1.0;
    if(cameraZ > 0)
        cameraZ.reset(0);
    if(cameraZ < -160)
        cameraZ.reset(-160);
    if(cameraZ <= 0)
        camera.z = -0.1-(-1+powf(2, -cameraZ*0.045));
    
    /* MODEL/VIEW MATRIX */
    modelView = fixedModelView.translate(camera.x, camera.y, camera.z);
    if(cameraScale > 1.0f)
        modelView.scaleInPlace(cameraScale, cameraScale, 1.0f);
    
    // update render object shared variables
    AGRenderObject::setProjectionMatrix(projection);
    AGRenderObject::setGlobalModelViewMatrix(modelView);
    AGRenderObject::setFixedModelViewMatrix(fixedModelView);
    AGRenderObject::setCameraMatrix(Matrix4::makeTranslation(camera.x, camera.y, camera.z));
}

