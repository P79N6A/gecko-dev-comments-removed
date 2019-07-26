

























#include "inc/GlyphFace.h"


using namespace graphite2;

uint16 GlyphFace::getMetric(uint8 metric) const
{
    switch (metrics(metric))
    {
        case kgmetLsb       : return static_cast<uint16>(m_bbox.bl.x);
        case kgmetRsb       : return static_cast<uint16>(m_advance.x - m_bbox.tr.x);
        case kgmetBbTop     : return static_cast<uint16>(m_bbox.tr.y);
        case kgmetBbBottom  : return static_cast<uint16>(m_bbox.bl.y);
        case kgmetBbLeft    : return static_cast<uint16>(m_bbox.bl.x);
        case kgmetBbRight   : return static_cast<uint16>(m_bbox.tr.x);
        case kgmetBbHeight  : return static_cast<uint16>(m_bbox.tr.y - m_bbox.bl.y);
        case kgmetBbWidth   : return static_cast<uint16>(m_bbox.tr.x - m_bbox.bl.x);
        case kgmetAdvWidth  : return static_cast<uint16>(m_advance.x);
        case kgmetAdvHeight : return static_cast<uint16>(m_advance.y);
        default : return 0;
    }
}
