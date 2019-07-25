

























#pragma once
#include "inc/Main.h"


namespace graphite2 {

class CharInfo
{

public:
    CharInfo() : m_before(-1), m_after(0) {}
    void init(int cid) { m_char = cid; }
    unsigned int unicodeChar() const { return m_char; }
    void feats(int offset) { m_featureid = offset; }
    int fid() const { return m_featureid; }
    int breakWeight() const { return m_break; }
    void breakWeight(int val) { m_break = val; }
    int after() const { return m_after; }
    void after(int val) { m_after = val; }
    int before() const { return m_before; }
    void before(int val) { m_before = val; }
    size_t base() const { return m_base; }
    void base(size_t offset) { m_base = offset; }

    CLASS_NEW_DELETE
private:
    int m_char;     
    int m_before;   
    int m_after;    
    size_t  m_base; 
    uint8 m_featureid;	
    int8 m_break;	
};

} 

struct gr_char_info : public graphite2::CharInfo {};

