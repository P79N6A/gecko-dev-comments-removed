


























#include "inc/Segment.h"
#include "graphite2/Font.h"
#include "inc/debug.h"
#include "inc/CharInfo.h"
#include "inc/Slot.h"
#include "inc/Main.h"
#include <math.h>

using namespace graphite2;

class JustifyTotal {
public:
    JustifyTotal() : m_numGlyphs(0), m_tStretch(0), m_tShrink(0), m_tStep(0), m_tWeight(0) {}
    void accumulate(Slot *s, Segment *seg, int level);
    int weight() const { return m_tWeight; }

    CLASS_NEW_DELETE

private:
    int m_numGlyphs;
    int m_tStretch;
    int m_tShrink;
    int m_tStep;
    int m_tWeight;
};

void JustifyTotal::accumulate(Slot *s, Segment *seg, int level)
{
    ++m_numGlyphs;
    m_tStretch += s->getJustify(seg, level, 0);
    m_tShrink += s->getJustify(seg, level, 1);
    m_tStep += s->getJustify(seg, level, 2);
    m_tWeight += s->getJustify(seg, level, 3);
}

float Segment::justify(Slot *pSlot, const Font *font, float width, GR_MAYBE_UNUSED justFlags flags, Slot *pFirst, Slot *pLast)
{
    Slot *s, *end;
    float currWidth = 0.0;
    const float scale = font ? font->scale() : 1.0f;
    Position res;

    if (width < 0 && !(silf()->flags()))
        return width;

    if (!pFirst) pFirst = pSlot;
    while (!pFirst->isBase()) pFirst = pFirst->attachedTo();
    if (!pLast) pLast = last();
    while (!pLast->isBase()) pLast = pLast->attachedTo();
    const float base = pFirst->origin().x / scale;
    width = width / scale;
    if ((flags & gr_justEndInline) == 0)
    {
        do {
            Rect bbox = theGlyphBBoxTemporary(pLast->glyph());
            if (bbox.bl.x != 0. || bbox.bl.y != 0. || bbox.tr.x != 0. || bbox.tr.y == 0.)
                break;
            pLast = pLast->prev();
        } while (pLast != pFirst);
    }

    end = pLast->nextSibling();
    pFirst = pFirst->nextSibling();

    int icount = 0;
    int numLevels = silf()->numJustLevels();
    if (!numLevels)
    {
        for (s = pSlot; s != end; s = s->next())
        {
            CharInfo *c = charinfo(s->before());
            if (isWhitespace(c->unicodeChar()))
            {
                s->setJustify(this, 0, 3, 1);
                s->setJustify(this, 0, 2, 1);
                s->setJustify(this, 0, 0, -1);
                ++icount;
            }
        }
        if (!icount)
        {
            for (s = pSlot; s != end; s = s->nextSibling())
            {
                s->setJustify(this, 0, 3, 1);
                s->setJustify(this, 0, 2, 1);
                s->setJustify(this, 0, 0, -1);
            }
        }
        ++numLevels;
    }

    JustifyTotal *stats = new JustifyTotal[numLevels];
    for (s = pFirst; s != end; s = s->nextSibling())
    {
        float w = s->origin().x / scale + s->advance() - base;
        if (w > currWidth) currWidth = w;
        for (int j = 0; j < numLevels; ++j)
            stats[j].accumulate(s, this, j);
        s->just(0);
    }

    for (int i = (width < 0.0) ? -1 : numLevels - 1; i >= 0; --i)
    {
        float diff;
        float error = 0.;
        float diffpw;
        int tWeight = stats[i].weight();

        do {
            error = 0.;
            diff = width - currWidth;
            diffpw = diff / tWeight;
            tWeight = 0;
            for (s = pFirst; s != end; s = s->nextSibling()) 
            {
                int w = s->getJustify(this, i, 3);
                float pref = diffpw * w + error;
                int step = s->getJustify(this, i, 2);
                if (!step) step = 1;        
                if (pref > 0)
                {
                    float max = uint16(s->getJustify(this, i, 0));
                    if (i == 0) max -= s->just();
                    if (pref > max) pref = max;
                    else tWeight += w;
                }
                else
                {
                    float max = uint16(s->getJustify(this, i, 1));
                    if (i == 0) max += s->just();
                    if (-pref > max) pref = -max;
                    else tWeight += w;
                }
                int actual = step ? int(pref / step) * step : int(pref);

                if (actual)
                {
                    error += diffpw * w - actual;
                    if (i == 0)
                        s->just(s->just() + actual);
                    else
                        s->setJustify(this, i, 4, actual);
                }
            }
            currWidth += diff - error;
        } while (i == 0 && int(abs(error)) > 0 && tWeight);
    }

    Slot *oldFirst = m_first;
    Slot *oldLast = m_last;
    if (silf()->flags() & 1)
    {
        m_first = pSlot = addLineEnd(pSlot);
        m_last = pLast = addLineEnd(end);
    }
    else
    {
        m_first = pSlot;
        m_last = pLast;
    }

    
#if !defined GRAPHITE2_NTRACING
    json * const dbgout = m_face->logger();
    if (dbgout)
        *dbgout << json::object
                    << "justifies"	<< objectid(this)
                    << "passes"     << json::array;
#endif

    if (m_silf->justificationPass() != m_silf->positionPass() && (width >= 0. || (silf()->flags() & 1)))
        m_silf->runGraphite(this, m_silf->justificationPass(), m_silf->positionPass());

#if !defined GRAPHITE2_NTRACING
    if (dbgout)
    {
        *dbgout     << json::item << json::close; 
        positionSlots(NULL, pSlot, pLast);
        Slot *lEnd = pLast->nextSibling();
        *dbgout << "output" << json::array;
        for(Slot * t = pSlot; t != lEnd; t = t->next())
            *dbgout		<< dslot(this, t);
        *dbgout			<< json::close << json::close;
    }
#endif

    res = positionSlots(font, pSlot, pLast);

    if (silf()->flags() & 1)
    {
        delLineEnd(m_first);
        delLineEnd(m_last);
    }
    m_first = oldFirst;
    m_last = oldLast;
    return res.x;
}

Slot *Segment::addLineEnd(Slot *nSlot)
{
    Slot *eSlot = newSlot();
    const uint16 gid = silf()->endLineGlyphid();
    const GlyphFace * theGlyph = m_face->glyphs().glyphSafe(gid);
    eSlot->setGlyph(this, gid, theGlyph);
    if (nSlot)
    {
        eSlot->next(nSlot);
        eSlot->prev(nSlot->prev());
        nSlot->prev(eSlot);
        eSlot->before(nSlot->before());
        if (eSlot->prev())
            eSlot->after(eSlot->prev()->after());
        else
            eSlot->after(nSlot->before());
    }
    else
    {
        nSlot = m_last;
        eSlot->prev(nSlot);
        nSlot->next(eSlot);
        eSlot->after(eSlot->prev()->after());
        eSlot->before(nSlot->after());
    }
    return eSlot;
}

void Segment::delLineEnd(Slot *s)
{
    Slot *nSlot = s->next();
    if (nSlot)
    {
        nSlot->prev(s->prev());
        if (s->prev())
            s->prev()->next(nSlot);
    }
    else
        s->prev()->next(NULL);
    freeSlot(s);
}

