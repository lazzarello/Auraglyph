//
//  AGTouchHandler.m
//  Auragraph
//
//  Created by Spencer Salazar on 2/2/14.
//  Copyright (c) 2014 Spencer Salazar. All rights reserved.
//

#import "AGMoveNodeTouchHandler.h"

#import "AGViewController.h"
#import "Geometry.h"
#import "ShaderHelper.h"
#import "hsv.h"
#import "ES2Render.h"
#import "AGHandwritingRecognizer.h"
#import "AGNode.h"
#import "AGFreeDraw.h"
#import "AGCompositeNode.h"
#import "AGAudioCapturer.h"
#import "AGAudioManager.h"
#import "AGUserInterface.h"
#import "TexFont.h"
#import "AGDef.h"
#import "AGTrainerViewController.h"
#import "AGNodeSelector.h"
#import "AGUINodeEditor.h"
#import "AGGenericShader.h"
#include "AGActivityManager.h"
#include "AGActivity.h"
#import "AGAnalytics.h"

#import "GeoGenerator.h"
#import "spMath.h"

#include "AGStyle.h"

#import <set>


//------------------------------------------------------------------------------
// ### AGMoveNodeTouchHandler ###
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark AGMoveNodeTouchHandler

@implementation AGMoveNodeTouchHandler

- (id)initWithViewController:(AGViewController *)viewController node:(AGNode *)node
{
    if(self = [super initWithViewController:viewController])
    {
        _moveNode = node;
        _initialPos = node->position();
    }
    
    return self;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    CGPoint p = [[touches anyObject] locationInView:_viewController.view];
    GLvertex3f pos = [_viewController worldCoordinateForScreenCoordinate:p];
    
    _anchorOffset = pos - _moveNode->position();
    _moveNode->activate(1);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    CGPoint p = [[touches anyObject] locationInView:_viewController.view];
    GLvertex3f pos = [_viewController worldCoordinateForScreenCoordinate:p];
    
    float travel = _firstPoint.distanceSquaredTo(GLvertex2f(p.x, p.y));
    if(travel > _maxTouchTravel)
        _maxTouchTravel = travel;
    
    if(_maxTouchTravel >= 2*2) // TODO: #define constant for touch travel limit
    {
        _moveNode->setPosition(pos - _anchorOffset);
        _moveNode->activate(0);
    }
    
    AGUITrash &trash = AGUITrash::instance();
    GLvertex3f fixedPos = GLKMatrix4MultiplyVector4(AGNode::cameraMatrix(), pos.asGLKVector4());
    if(trash.hitTest(fixedPos))
        trash.activate();
    else
        trash.deactivate();
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    CGPoint p = [[touches anyObject] locationInView:_viewController.view];
    GLvertex3f pos = [_viewController worldCoordinateForScreenCoordinate:p];
    
    AGUITrash &trash = AGUITrash::instance();
    trash.deactivate();
    
    if(_moveNode && _maxTouchTravel < 2*2)
    {
        AGAnalytics::instance().eventOpenNodeEditor(_moveNode->type());
        
        _moveNode->activate(0);
        // _nextHandler = [[AGEditTouchHandler alloc] initWithViewController:_viewController node:_moveNode];
        
        AGUINodeEditor *nodeEditor = _moveNode->createCustomEditor();
        if(nodeEditor == NULL)
        {
            nodeEditor = new AGUIStandardNodeEditor(_moveNode);
            nodeEditor->init();
        }
        
        [_viewController addTopLevelObject:nodeEditor over:NULL];
    }
    else
    {
        AGAnalytics::instance().eventMoveNode(_moveNode->type());
        
        GLvertex3f fixedPos = GLKMatrix4MultiplyVector4(AGNode::cameraMatrix(), pos.asGLKVector4());
        if(trash.hitTest(fixedPos))
        {
            AGAnalytics::instance().eventDeleteNode(_moveNode->type());
            
            AGActivity *action = AGActivity::deleteNodeActivity(_moveNode);
            AGActivityManager::instance().addActivity(action);
            
            _moveNode->removeFromTopLevel();
        }
        else
        {
            AGActivity *action = AGActivity::moveNodeActivity(_moveNode, _initialPos, _moveNode->position());
            AGActivityManager::instance().addActivity(action);
        }
    }
}


@end

