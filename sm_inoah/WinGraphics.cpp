#include "Graphics.hpp"
#include "sm_inoah.h"
#include <Wingdi.h>

namespace ArsLexis
{
    /*Graphics::Graphics():
        handle_(GetDC(hwndMain))
    {
        
    }*/

    Graphics::Graphics(const NativeGraphicsHandle_t& handle):
        handle_(handle)
    {
        //support_.font.setFontId(FntGetFont());
    }

    Graphics::Font_t Graphics::setFont(const Graphics::Font_t& font)
    {
        Font_t oldOne=support_.font;
        support_.font=font;
        //FntSetFont(support_.font.withEffects());
        SelectObject(handle_, font.getHandle());
        return oldOne;
    }

    Graphics::State_t Graphics::pushState()
    {
        return SaveDC(handle_);
    }

    void Graphics::popState(const Graphics::State_t& state)
    {
        RestoreDC(handle_,-1);
    }
    
    Graphics::~Graphics()
    {
        ReleaseDC(hwndMain,this->handle_);
    }

    void Graphics::drawText(const char_t* text, uint_t length, const Point& topLeft, bool inverted)
    {
        /*FontEffects fx=support_.font.effects();
        PalmUnderlineSetter setUnderline(convertUnderlineMode(fx.underline()));
        */
        
        //uint_t height=fontHeight();
        /*uint_t top=topLeft.y;
        if (fx.subscript())
            top+=(height*0.333);*/
        //WinDrawChars(text, length, topLeft.x, top);
        /*if (fx.strikeOut())
        {
            uint_t baseline=fontBaseline();
            top=topLeft.y+baseline*0.667;
            uint_t width=FntCharsWidth(text, length);
            drawLine(topLeft.x, top, topLeft.x+width, top);
        }*/
        ExtTextOut(handle_, topLeft.x, topLeft.y, 0, 
            NULL, text, length, NULL);
        //(handle_, topLeft.x , topLeft.y, text, length);
    }
    
    void Graphics::erase(const ArsLexis::Rectangle& rect)
    {
        NativeRectangle_t nr=toNative(rect);
        //TODO: Not effective at all
        HBRUSH hbr=CreateSolidBrush(GetBkColor(handle_));
        FillRect(handle_, &nr, hbr);
        DeleteObject(hbr);
    }
    
    void Graphics::copyArea(const Rectangle& sourceArea, Graphics& targetSystem, const Point& targetTopLeft)
    {
        NativeRectangle_t nr=toNative(sourceArea);
        BitBlt(handle_, targetTopLeft.x, targetTopLeft.y,
            sourceArea.width(), sourceArea.height(), 
            targetSystem.handle_,
            nr.left, nr.top, SRCCOPY);
    }
    
    void Graphics::drawLine(Coord_t x0, Coord_t y0, Coord_t x1, Coord_t y1)
    {
        POINT p[2];
        p[0].x=x0;
        p[0].y=y0;
        p[1].x=x1;
        p[1].y=y1;
        Polyline(handle_, p, 2);
        // MoveToEx(handle_, x0, y0);
        // LineTo(handle_, x1, y1); THIS DOESN'T WORK - STUPID WIN CE
        // WinDrawLine(x0, y0, x1, y1);        
    }

    NativeColor_t Graphics::setForegroundColor(NativeColor_t color)
    {
        LOGPEN pen;
        NativeColor_t old;
        //TODO: again not effective
        HGDIOBJ hgdiobj = GetCurrentObject(handle_,OBJ_PEN);
        GetObject(hgdiobj, sizeof(pen), &pen);
        old = pen.lopnColor;
        pen.lopnColor = color;
        CreatePenIndirect(&pen);
        SelectObject(handle_,hgdiobj);
        DeleteObject(hgdiobj);
        return old;
    }
    
    NativeColor_t Graphics::setBackgroundColor(NativeColor_t color)
    {
        return SetBkColor(handle_,color);
    }
    
    NativeColor_t Graphics::setTextColor(NativeColor_t color)
    {
        return SetTextColor(handle_,color ); 
    }
    
    
    uint_t Graphics::fontHeight() const
    {
        /*LOGFONT fnt;
        HGDIOBJ font=GetCurrentObject(handle_, OBJ_FONT);
        GetObject(font, sizeof(fnt), &fnt); 
        //FntLineHeight();
        //FontEffects fx=support_.font.effects();
        //if (fx.superscript() || fx.subscript())
        //    height*=1.333;*/
        TEXTMETRIC ptm;
        GetTextMetrics(handle_, &ptm);
        return ptm.tmHeight;
        //return -fnt.lfHeight;
    }
   
    uint_t Graphics::fontBaseline() const
    {
        //uint_t baseline=FntBaseLine();
        //if (support_.font.effects().superscript())
            //baseline+=(FntLineHeight()*0.333);
        TEXTMETRIC ptm;
        GetTextMetrics(handle_, &ptm);
        return ptm.tmDescent;
    }

    uint_t Graphics::wordWrap(const char_t* text, uint_t width)
    {
        //return FntWordWrap(text, width);
        int len;
        SIZE size;
        GetTextExtentExPoint(handle_, text, _tcslen(text),
            width, &len, NULL, &size);
        //length = len;
        width = size.cx;
        if(_tcslen(text)==len)
            return len;

        for(int i=len;i>0;i--)
        {
            if ((text[i]==TCHAR(' '))||
                (text[i]==TCHAR('\t'))||
                (text[i]==TCHAR('\n'))
               ) 
                return i+1;
        }
        return _tcslen(text);
    }

    uint_t Graphics::textWidth(const char_t* text, uint_t length)
    {
        //return FntCharsWidth(text, length);
        SIZE size;
        GetTextExtentPoint(handle_, text, length, &size);
        return size.cx;
    }
    
    void Graphics::charsInWidth(const char_t* text, uint_t& length, uint_t& width)
    {
        int len;
        SIZE size;
        //Boolean dontMind;
        //FntCharsInWidth(text, &w, &len, &dontMind);
        GetTextExtentExPoint(handle_, text, length, width, 
                &len, NULL, &size ); 
        length = len;
        width = size.cx;
    }

    //Graphics::Font_t& Graphics::font()
    //{return support_.font;}
    
    Graphics::Font_t Graphics::font() const
    {return support_.font;}
}