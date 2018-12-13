//
//  AGUILoadDialog.cpp
//  Auraglyph
//
//  Created by Spencer Salazar on 4/17/18.
//  Copyright © 2018 Spencer Salazar. All rights reserved.
//

#include "AGUILoadDialog.h"
#include "AGDocumentManager.h"
#include "AGGenericShader.h"
#include "AGStyle.h"

//------------------------------------------------------------------------------
// ### AGUIConcreteLoadDialog ###
//------------------------------------------------------------------------------
#pragma mark - AGUIConcreteLoadDialog

class AGUIConcreteLoadDialog : public AGUILoadDialog
{
private:
    GLvertex2f m_size;
    
    float m_itemStart;
    float m_itemHeight;
    //clampf m_verticalScrollPos;
    momentum<float, clampf> m_verticalScrollPos;
    slew<float, clampf> m_horizontalSlidePos;
    
    AGSqueezeAnimation m_squeeze;
    
    AGUIButton *m_cancelButton;
    
    GLvertex3f m_touchStart;
    GLvertex3f m_lastTouch;
    int m_selection = -1;
    int m_utilitySelection = -1;
    bool m_utilityHit = false;
    bool m_utilityHitOnTouchDown = false;

    bool m_scrollingVertical = false;
    bool m_slidingHorizontal = false;
    
    float m_utilityButtonWidth;
    float m_marginFraction = 0.9;
    
    std::vector<AGDocumentManager::DocumentListing> m_documentList;
    
public:
    AGUIConcreteLoadDialog(const GLvertex3f &pos, const std::vector<AGDocumentManager::DocumentListing> &list) :
    m_documentList(list)
    {
        setPosition(pos);
        
        m_selection = -1;
        m_utilitySelection = -1;
        
        m_onLoad = [](const AGFile &, AGDocument &){};
        m_onUtility = [](const AGFile &){};
        
        m_size = GLvertex2f(500, 2*500/AGStyle::aspect16_9);
        m_itemStart = m_size.y/3.0f;
        m_itemHeight = m_size.y/3.0f;
        
        m_utilityButtonWidth = m_size.x/5;
        
        m_verticalScrollPos.raw().clampTo(0, max(0.0f, (m_documentList.size()-3.0f)*m_itemHeight));
        
        m_horizontalSlidePos.rate = 0.6;
        m_horizontalSlidePos.target.clampTo(-m_utilityButtonWidth, 0);
        m_horizontalSlidePos.value.clampTo(-m_utilityButtonWidth, 0);
        m_horizontalSlidePos.reset(0);

        float buttonWidth = 100;
        float buttonHeight = 25;
        float buttonMargin = 10;
        
        m_cancelButton = new AGUIButton("Cancel",
                                        GLvertex3f(-m_size.x/2+buttonMargin,
                                                   -m_size.y/2+buttonMargin, 0),
                                        GLvertex2f(buttonWidth, buttonHeight));
        m_cancelButton->init();
        m_cancelButton->setRenderFixed(false);
        m_cancelButton->setAction(^{
            removeFromTopLevel();
        });
        addChild(m_cancelButton);
    }
    
    virtual ~AGUIConcreteLoadDialog() { }
    
    virtual GLKMatrix4 localTransform() override
    {
        GLKMatrix4 local = GLKMatrix4MakeTranslation(m_pos.x, m_pos.y, m_pos.z);
        local = m_squeeze.apply(local);
        return local;
    }
    
    virtual void update(float t, float dt) override
    {
        m_squeeze.update(t, dt);
        m_verticalScrollPos.update(t, dt);
        m_horizontalSlidePos.interp();

        m_renderState.projection = projectionMatrix();
        m_renderState.modelview = GLKMatrix4Multiply(fixedModelViewMatrix(), localTransform());
        
        updateChildren(t, dt);
    }
    
    void _renderFrame()
    {
        // draw inner box
        AGStyle::frameBackgroundColor().set();
        drawTriangleFan((GLvertex3f[]){
            { -m_size.x/2, -m_size.y/2, 0 },
            {  m_size.x/2, -m_size.y/2, 0 },
            {  m_size.x/2,  m_size.y/2, 0 },
            { -m_size.x/2,  m_size.y/2, 0 },
        }, 4);
        
        AGStyle::foregroundColor().set();
        glLineWidth(4.0f);
        drawLineLoop((GLvertex3f[]){
            { -m_size.x/2, -m_size.y/2, 0 },
            {  m_size.x/2, -m_size.y/2, 0 },
            {  m_size.x/2,  m_size.y/2, 0 },
            { -m_size.x/2,  m_size.y/2, 0 },
        }, 4);
    }
    
    void _renderUtilityButton(AGClipShader &shader, int itemNum)
    {
        float xPos = m_horizontalSlidePos.value;
        float yPos = m_itemStart + m_verticalScrollPos - m_itemHeight*itemNum;
        float deleteButtonLeftMargin = m_utilityButtonWidth*0.1;
        
        // translate to right edge of button position
        GLKMatrix4 xform = GLKMatrix4MakeTranslation(m_size.x/2*m_marginFraction, yPos, 0);
        shader.setLocalMatrix(xform);
        
        xform = GLKMatrix4Scale(xform, fabsf(xPos)/m_utilityButtonWidth, 1, 1);
        
        // draw box
        AGStyle::foregroundColor().set();
        GLvertex3f geo[] = {
            { -m_utilityButtonWidth+deleteButtonLeftMargin,  m_itemHeight/2*m_marginFraction, 0 },
            { -m_utilityButtonWidth+deleteButtonLeftMargin, -m_itemHeight/2*m_marginFraction, 0 },
            {  0, -m_itemHeight/2*m_marginFraction, 0 },
            {  0,  m_itemHeight/2*m_marginFraction, 0 }
        };
        
        if(m_utilityHit)
        {
            drawLineLoop(shader, geo, 4, xform);
        }
        else
        {
            drawTriangleFan(shader, geo, 4, xform);
        }
        
        // draw x
        // radius of X figure (both x and y dimensions)
        float xRadius = m_utilityButtonWidth/3/2;
        if(m_utilityHit)
            AGStyle::foregroundColor().set();
        else
            AGStyle::backgroundColor().set();
        
        float leftEdge = (-m_utilityButtonWidth+deleteButtonLeftMargin)/2;
        drawLineStrip(shader, (GLvertex2f[]){
            { leftEdge-xRadius,  xRadius },
            { leftEdge+xRadius, -xRadius },
        }, 2, xform);
        drawLineStrip(shader, (GLvertex2f[]){
            { leftEdge+xRadius,  xRadius },
            { leftEdge-xRadius, -xRadius },
        }, 2, xform);
    }
    
    void _renderDocuments()
    {
        GLcolor4f whiteA = AGStyle::foregroundColor().withAlpha(0.75);
        float yPos = m_itemStart + m_verticalScrollPos;
        int i = 0;
        int len = (int) m_documentList.size();
        
        glLineWidth(4.0f);
        
        AGClipShader &shader = AGClipShader::instance();
        shader.useProgram();
        shader.setClip(m_pos.xy()-m_size/2, m_size);
        
        for(const AGDocumentManager::DocumentListing &document : m_documentList)
        {
            float xPos = 0;
            if(i == m_utilitySelection)
                xPos = m_horizontalSlidePos.value;
            
            GLKMatrix4 xform = GLKMatrix4MakeTranslation(xPos, yPos, 0);
            shader.setLocalMatrix(xform);
            
            float margin = m_marginFraction;
            
            if(i == m_selection)
            {
                // draw selection box
                glVertexAttrib4fv(AGVertexAttribColor, (const GLfloat *) &AGStyle::foregroundColor());
                
                drawTriangleFan(shader, (GLvertex3f[]){
                    { -m_size.x/2*margin,  m_itemHeight/2*margin, 0 },
                    { -m_size.x/2*margin, -m_itemHeight/2*margin, 0 },
                    {  m_size.x/2*margin, -m_itemHeight/2*margin, 0 },
                    {  m_size.x/2*margin,  m_itemHeight/2*margin, 0 },
                }, 4, xform);
                
                glVertexAttrib4fv(AGVertexAttribColor, (const GLfloat *) &AGStyle::frameBackgroundColor());
            }
            else
            {
                glVertexAttrib4fv(AGVertexAttribColor, (const GLfloat *) &AGStyle::foregroundColor());
            }
            
            // draw each "figure" in the "name"
            for(auto figure : document.name)
                drawLineStrip(shader, figure.data(), figure.size(), xform);
            
            // draw delete button (if it exists)
            if(i == m_utilitySelection && fabsf(m_horizontalSlidePos/m_utilityButtonWidth) > 0.01)
                _renderUtilityButton(shader, i);
            
            // draw separating line between rows
            if(i != len-1 || len == 1)
            {
                xform = GLKMatrix4MakeTranslation(0, yPos, 0);
                shader.setLocalMatrix(xform);
                
                glVertexAttrib4fv(AGVertexAttribColor, (const GLfloat *) &whiteA);
                drawLineStrip(shader, (GLvertex2f[]){
                    { -m_size.x/2*margin, -m_itemHeight/2 }, { m_size.x/2*margin, -m_itemHeight/2 },
                }, 2, xform);
            }
            
            yPos -= m_itemHeight;
            i++;
        }
    }
    
    void _renderScrollbar()
    {
        /* draw scroll bar */
        int nRows = (int) m_documentList.size();
        if(nRows > 3)
        {
            float scroll_bar_margin = 0.95;
            // maximum distance that can be scrolled
            float scroll_max_scroll = (nRows-3)*m_itemHeight;
            // height of the scroll bar tray area
            float scroll_bar_tray_height = m_size.y*scroll_bar_margin;
            // percent of the total scroll area that is visible * tray height
            float scroll_bar_height = scroll_bar_tray_height/ceilf(nRows-2);
            // percent of scroll position * (tray height - bar height)
            float scroll_bar_y = m_verticalScrollPos/scroll_max_scroll*(scroll_bar_tray_height-scroll_bar_height);
            
            // load it up and draw
            glVertexAttrib4fv(AGVertexAttribColor, (const float *) &AGStyle::foregroundColor());
            glLineWidth(1.0);
            drawLineStrip((GLvertex2f[]) {
                { m_size.x/2*scroll_bar_margin, m_size.y/2*scroll_bar_margin-scroll_bar_y },
                { m_size.x/2*scroll_bar_margin, m_size.y/2*scroll_bar_margin-(scroll_bar_y+scroll_bar_height) },
            }, 2);
        }
    }
    
    virtual void render() override
    {
        _renderFrame();
        
        _renderDocuments();
        
        _renderScrollbar();
        
        // restore color
        glVertexAttrib4fv(AGVertexAttribColor, (const GLfloat *) &AGStyle::foregroundColor());
        
        AGInteractiveObject::render();
    }
    
    GLvrectf effectiveBounds() override
    {
        return GLvrectf(m_pos-m_size/2, m_pos+m_size/2);
    }
    
    bool renderFixed() override { return true; }
    
    int _itemForPosition(const GLvertex3f &position)
    {
        // TODO: should be possible to do this closed form
        GLvertex3f relPos = position-m_pos;
        float yPos = m_itemStart+m_verticalScrollPos;
        
        for(int i = 0; i < m_documentList.size(); i++)
        {
            if(relPos.y < yPos+m_itemHeight/2.0f && relPos.y > yPos-m_itemHeight/2.0f &&
               relPos.x > -m_size.x/2 && relPos.x < m_size.x/2)
                return i;
            
            yPos -= m_itemStart;
        }
        
        return -1;
    }
    
    bool _hitTestDeleteButton(const GLvertex3f &position)
    {
        if(m_utilitySelection == -1)
            return false;
        
        // check if in area of right item
        int item = _itemForPosition(position);
        if (item != m_utilitySelection)
            return false;
        
        float margin = m_marginFraction;
        GLvertex3f relPos = position-m_pos;
        // check if touch x is in delete button area
        if(relPos.x < m_size.x/2*margin+m_horizontalSlidePos.value ||
           relPos.x > m_size.x/2*margin)
            return false;
        
        return true;
    }
    
    virtual void touchDown(const AGTouchInfo &t) override
    {
        m_scrollingVertical = false;
        m_slidingHorizontal = false;
        m_utilityHitOnTouchDown = false;
        m_utilityHit = false;
        
        if(m_utilitySelection != -1 && _hitTestDeleteButton(t.position))
        {
            m_utilityHitOnTouchDown = true;
            m_utilityHit = true;
        }
        else
        {
            //m_utilitySelection = -1;
            m_horizontalSlidePos = 0.0f;
            
            m_selection = _itemForPosition(t.position);
            
            m_verticalScrollPos.on();
        }
        
        m_touchStart = t.position;
        m_lastTouch = t.position;
    }
    
    virtual void touchMove(const AGTouchInfo &t) override
    {
        if(m_utilityHitOnTouchDown)
        {
            m_utilityHit = _hitTestDeleteButton(t.position);
        }
        else if(m_slidingHorizontal)
        {
            // continue to scroll sideways
            m_horizontalSlidePos.reset(m_horizontalSlidePos+(t.position.x - m_lastTouch.x));
        }
        else if(m_scrollingVertical)
        {
            m_verticalScrollPos += (t.position.y - m_lastTouch.y);
        }
        else if(fabsf(m_touchStart.y-t.position.y) > AGStyle::maxTravel)
        {
            // start scrolling
            m_selection = -1;
            m_verticalScrollPos += (t.position.y - m_lastTouch.y);
            m_scrollingVertical = true;
        }
        else if(fabsf(m_touchStart.x-t.position.x) > AGStyle::maxTravel)
        {
            // start to show delete button
            m_selection = -1;
            m_utilitySelection = _itemForPosition(m_touchStart);
            m_slidingHorizontal = true;
        }

        m_lastTouch = t.position;
    }
    
    virtual void touchUp(const AGTouchInfo &t) override
    {
        if(fabsf(m_touchStart.y-t.position.y) > AGStyle::maxTravel)
        {
            m_selection = -1;
        }
        
        if(m_selection >= 0)
        {
            const AGFile &file = m_documentList[m_selection].filename;
            AGDocument doc = AGDocumentManager::instance().load(file);
            m_onLoad(file, doc);
            removeFromTopLevel();
        }
        
        bool forceUtilitySlideOut = false;
        if(m_utilityHitOnTouchDown && _hitTestDeleteButton(t.position))
        {
            m_onUtility(m_documentList[m_utilitySelection].filename);
            forceUtilitySlideOut = true;
        }
        
        if(forceUtilitySlideOut || fabsf(m_horizontalSlidePos) < m_utilityButtonWidth/2)
            m_horizontalSlidePos = 0;
        else
            m_horizontalSlidePos = -m_utilityButtonWidth;
        
        m_utilityHit = false;
        m_verticalScrollPos.off();
    }
    
    virtual void renderOut() override
    {
        m_squeeze.close();
    }
    
    virtual bool finishedRenderingOut() override
    {
        return m_squeeze.finishedClosing();
    }
};

AGUILoadDialog *AGUILoadDialog::load(const GLvertex3f &pos)
{
    std::vector<AGDocumentManager::DocumentListing> list = AGDocumentManager::instance().list();
    std::sort(list.begin(), list.end(), [](const AGDocumentManager::DocumentListing &a,
                                           const AGDocumentManager::DocumentListing &b) {
        return a.filename.m_creationTime > b.filename.m_creationTime;
    });
    AGUILoadDialog *loadDialog = new AGUIConcreteLoadDialog(pos, list);
    loadDialog->init();
    return loadDialog;
}

AGUILoadDialog *AGUILoadDialog::loadExample(const GLvertex3f &pos)
{
    AGUILoadDialog *loadDialog = new AGUIConcreteLoadDialog(pos, AGDocumentManager::instance().examplesList());
    loadDialog->init();
    return loadDialog;
}

