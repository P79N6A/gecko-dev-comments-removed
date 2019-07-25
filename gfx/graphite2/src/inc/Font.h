

























#pragma once
#include <cassert>
#include "graphite2/Font.h"
#include "inc/Main.h"
#include "inc/Face.h"

namespace graphite2 {

#define INVALID_ADVANCE -1e38f		// can't be a static const because non-integral

class Font
{
public:
    Font(float ppm, const Face *face);
    virtual ~Font();

    float advance(unsigned short glyphid) const {
        if (m_advances[glyphid] == INVALID_ADVANCE)
            m_advances[glyphid] = computeAdvance(glyphid);
        return m_advances[glyphid];
    }


    float scale() const { return m_scale; }
    virtual bool isHinted() const { return false; }

    CLASS_NEW_DELETE
private:
    virtual float computeAdvance(unsigned short ) const { assert(false); return .0f; };
    
protected:
    float m_scale;      
    float *m_advances;  
    
private:			
    Font(const Font&);
    Font& operator=(const Font&);
};


class SimpleFont : public Font      
{
public:
    SimpleFont(float ppm, const Face *face);
private:
    virtual float computeAdvance(unsigned short glyphid) const;
private:
    const Face *m_face;   
};


class HintedFont : public Font
{
public:
    HintedFont(float ppm, const void* appFontHandle, gr_advance_fn advance, const Face *face);
    virtual bool isHinted() const { return true; }
private:
    virtual float computeAdvance(unsigned short glyphid) const;

private:
    const void* m_appFontHandle;
    gr_advance_fn m_advance;
};

} 

struct gr_font : public graphite2::Font {};
