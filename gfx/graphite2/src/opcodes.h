

























#pragma once




































     


#define NOT_IMPLEMENTED

#ifdef ENABLE_DEEP_TRACING
#define TRACEPARAM(n)       XmlTraceLog::get().addArrayElement(ElementParams, dp-n, n)
#define TRACEPUSH(n)        XmlTraceLog::get().addSingleElement(ElementPush, n)
#else
#define TRACEPARAM(n)
#define TRACEPUSH(n)
#endif

#define binop(op)           const int32 a = pop(); *sp = int32(*sp) op a; TRACEPUSH(*sp)
#define use_params(n)       dp += n; TRACEPARAM(n)

#define declare_params(n)   const byte * param = dp; \
                            use_params(n);

#define push(n)             { *++sp = n; TRACEPUSH(n); }
#define pop()               (*sp--)
#define slotat(x)           (map[(x)])
#define DIE                 { is=seg.last(); EXIT(1); }
#define POSITIONED          1

STARTOP(nop)
    do {} while (0);
ENDOP

STARTOP(push_byte)
    declare_params(1);
    push(int8(*param));
ENDOP

STARTOP(push_byte_u)
    declare_params(1);
    push(uint8(*param));
ENDOP

STARTOP(push_short)
    declare_params(2);
    const int16 r   = int16(param[0]) << 8 
                    | uint8(param[1]);
    push(r);
ENDOP

STARTOP(push_short_u)
    declare_params(2);
    const uint16 r  = uint16(param[0]) << 8
                    | uint8(param[1]);
    push(r);
ENDOP

STARTOP(push_long)
    declare_params(4);
    const  int32 r  = int32(param[0]) << 24
                    | uint32(param[1]) << 16
                    | uint32(param[2]) << 8
                    | uint8(param[3]);
    push(r);
ENDOP

STARTOP(add)
    binop(+);
ENDOP

STARTOP(sub)
    binop(-);
ENDOP

STARTOP(mul)
    binop(*);
ENDOP

STARTOP(div_)
    if (*sp == 0) DIE;
    binop(/);
ENDOP

STARTOP(min)
    const int32 a = pop(), b = *sp;
    if (a < b) *sp = a;
ENDOP

STARTOP(max)
    const int32 a = pop(), b = *sp;
    if (a > b) *sp = a;
ENDOP

STARTOP(neg)
    *sp = uint32(-int32(*sp));
ENDOP

STARTOP(trunc8)
    *sp = uint8(*sp);
ENDOP

STARTOP(trunc16)
    *sp = uint16(*sp);
ENDOP

STARTOP(cond)
    const uint32 f = pop(), t = pop(), c = pop();
    push(c ? t : f);
ENDOP

STARTOP(and_)
    binop(&&);
ENDOP

STARTOP(or_)
    binop(||);
ENDOP

STARTOP(not_)
    *sp = !*sp;
ENDOP

STARTOP(equal)
    binop(==);
ENDOP

STARTOP(not_eq_)
    binop(!=);
ENDOP

STARTOP(less)
    binop(<);
ENDOP

STARTOP(gtr)
    binop(>);
ENDOP

STARTOP(less_eq)
    binop(<=);
ENDOP

STARTOP(gtr_eq)
    binop(>=);
ENDOP

STARTOP(next)
    if (map - &smap[0] >= int(smap.size())) DIE
    if (is)
    {
    	if (is == smap.highwater())
    		smap.highpassed(true);
    	is = is->next();
    }
    ++map;
ENDOP

STARTOP(next_n)
    use_params(1);
    NOT_IMPLEMENTED;
    
    
ENDOP






STARTOP(put_glyph_8bit_obs)
    declare_params(1);
    const unsigned int output_class = uint8(*param);
    is->setGlyph(&seg, seg.getClassGlyph(output_class, 0));
ENDOP

STARTOP(put_subs_8bit_obs)
    declare_params(3);
    const int           slot_ref     = int8(param[0]);
    const unsigned int  input_class  = uint8(param[1]),
                        output_class = uint8(param[2]);
    uint16 index;
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        index = seg.findClassIndex(input_class, slot->gid());
        is->setGlyph(&seg, seg.getClassGlyph(output_class, index));
    }
ENDOP

STARTOP(put_copy)
    declare_params(1);
    const int  slot_ref = int8(*param);
    if (is && (slot_ref ||is != *map))
    {
        uint16 *tempUserAttrs = is->userAttrs();
        slotref ref = slotat(slot_ref);
        if (ref)
        {
            memcpy(tempUserAttrs, ref->userAttrs(), seg.numAttrs() * sizeof(uint16));
            Slot *prev = is->prev();
            Slot *next = is->next();
            memcpy(is, slotat(slot_ref), sizeof(Slot));
            is->userAttrs(tempUserAttrs);
            is->next(next);
            is->prev(prev);
            is->sibling(NULL);
        }
        is->markCopied(false);
        is->markDeleted(false);
    }
ENDOP

STARTOP(insert)
    Slot *newSlot = seg.newSlot();
    Slot *iss = is;
    while (iss && iss->isDeleted()) iss = iss->next();
    if (!iss)
    {
        if (seg.last())
        {
            seg.last()->next(newSlot);
            newSlot->prev(seg.last());
            newSlot->before(seg.last()->before());
            seg.last(newSlot);
        }
        else
        {
            seg.first(newSlot);
            seg.last(newSlot);
        }
    }
    else if (iss->prev())
    {
        iss->prev()->next(newSlot);
        newSlot->prev(iss->prev());
	newSlot->before(iss->prev()->after());
    }
    else
    {
        newSlot->prev(NULL);
        newSlot->before(iss->before());
        seg.first(newSlot);
    }
    newSlot->next(iss);
    if (iss)
    {
        iss->prev(newSlot);
        newSlot->originate(iss->original());
	newSlot->after(iss->before());
    }
    else if (newSlot->prev())
    {
        newSlot->originate(newSlot->prev()->original());
	newSlot->after(newSlot->prev()->after());
    }
    else
    {
        newSlot->originate(seg.defaultOriginal());
    }
    is = newSlot;
    seg.extendLength(1);
    if (map != &smap[-1]) 
        --map;
ENDOP

STARTOP(delete_)
    if (!is) DIE
    is->markDeleted(true);
    if (is->prev())
        is->prev()->next(is->next());
    else
        seg.first(is->next());
    
    if (is->next())
        is->next()->prev(is->prev());
    else
        seg.last(is->prev());
    
    if (is->prev())
        is = is->prev();
    seg.extendLength(-1);
ENDOP

STARTOP(assoc)
    declare_params(1);
    unsigned int  num = uint8(*param);
    const int8 *  assocs = reinterpret_cast<const int8 *>(param+1);
    use_params(num);
    int max = -1;
    int min = -1;

    while (num-- > 0)
    {
        int sr = *assocs++;
        slotref ts = slotat(sr);
        if (ts && (min == -1 || ts->before() < min)) min = ts->before();
        if (ts && ts->after() > max) max = ts->after();
    }
    if (min > -1)   
    {
        is->before(min);
        is->after(max);
    }
ENDOP

STARTOP(cntxt_item)
    
    declare_params(3);    
    const int       is_arg = int8(param[0]);
    const size_t    iskip  = uint8(param[1]),
                    dskip  = uint8(param[2]);

    if (mapb + is_arg != map)
    {
        ip += iskip;
        dp += dskip;
        push(true);
    }
ENDOP

STARTOP(attr_set)
    declare_params(1);
    const attrCode  	slat = attrCode(uint8(*param));
    const          int  val  = int(pop());
    is->setAttr(&seg, slat, 0, val, smap);
ENDOP

STARTOP(attr_add)
    declare_params(1);
    const attrCode  	slat = attrCode(uint8(*param));
    const          int  val  = int(pop());
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    int res = is->getAttr(&seg, slat, 0);
    is->setAttr(&seg, slat, 0, val + res, smap);
ENDOP

STARTOP(attr_sub)
    declare_params(1);
    const attrCode  	slat = attrCode(uint8(*param));
    const          int  val  = int(pop());
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    int res = is->getAttr(&seg, slat, 0);
    is->setAttr(&seg, slat, 0, val - res, smap);
ENDOP

STARTOP(attr_set_slot)
    declare_params(1);
    const attrCode  	slat = attrCode(uint8(*param));
    const          int  val  = int(pop())  + (map - smap.begin())*int(slat == gr_slatAttTo);
    is->setAttr(&seg, slat, 0, val, smap);
ENDOP

STARTOP(iattr_set_slot)
    declare_params(2);
    const attrCode  	slat = attrCode(uint8(param[0]));
    const size_t        idx  = uint8(param[1]);
    const          int  val  = int(pop())  + (map - smap.begin())*int(slat == gr_slatAttTo);
    is->setAttr(&seg, slat, idx, val, smap);
ENDOP

STARTOP(push_slot_attr)
    declare_params(2);
    const attrCode  	slat     = attrCode(uint8(param[0]));
    const int           slot_ref = int8(param[1]);
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        int res = slot->getAttr(&seg, slat, 0);
        push(res);
    }
ENDOP

STARTOP(push_glyph_attr_obs)
    declare_params(2);
    const unsigned int  glyph_attr = uint8(param[0]);
    const int           slot_ref   = int8(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot)
        push(seg.glyphAttr(slot->gid(), glyph_attr));
ENDOP

STARTOP(push_glyph_metric)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8(param[0]);
    const int           slot_ref    = int8(param[1]);
    const signed int    attr_level  = uint8(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot)
        push(seg.getGlyphMetric(slot, glyph_attr, attr_level));
ENDOP

STARTOP(push_feat)
    declare_params(2);
    const unsigned int  feat        = uint8(param[0]);
    const int           slot_ref    = int8(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        uint8 fid = seg.charinfo(slot->original())->fid();
        push(seg.getFeature(fid, feat));
    }
ENDOP

STARTOP(push_att_to_gattr_obs)
    declare_params(2);
    const unsigned int  glyph_attr  = uint8(param[0]);
    const int           slot_ref    = int8(param[1]);
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        slotref att = slot->attachTo();
        if (att) slot = att;
        push(seg.glyphAttr(slot->gid(), glyph_attr));
    }
ENDOP

STARTOP(push_att_to_glyph_metric)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8(param[0]);
    const int           slot_ref    = int8(param[1]);
    const signed int    attr_level  = uint8(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        slotref att = slot->attachTo();
        if (att) slot = att;
        push(seg.getGlyphMetric(slot, glyph_attr, attr_level));
    }
ENDOP

STARTOP(push_islot_attr)
    declare_params(3);
    const attrCode	slat     = attrCode(uint8(param[0]));
    const int           slot_ref = int8(param[1]),
                        idx      = uint8(param[2]);
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        int res = slot->getAttr(&seg, slat, idx);
        push(res);
    }
ENDOP

#if 0
STARTOP(push_iglyph_attr) 
    NOT_IMPLEMENTED;
ENDOP
#endif
      
STARTOP(pop_ret)
    const uint32 ret = pop();
    EXIT(ret);
ENDOP

STARTOP(ret_zero)
    EXIT(0);
ENDOP

STARTOP(ret_true)
    EXIT(1);
ENDOP

STARTOP(iattr_set)
    declare_params(2);
    const attrCode  	slat = attrCode(uint8(param[0]));
    const size_t        idx  = uint8(param[1]);
    const          int  val  = int(pop());
    is->setAttr(&seg, slat, idx, val, smap);
ENDOP

STARTOP(iattr_add)
    declare_params(2);
    const attrCode  	slat = attrCode(uint8(param[0]));
    const size_t        idx  = uint8(param[1]);
    const          int  val  = int(pop());
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    int res = is->getAttr(&seg, slat, idx);
    is->setAttr(&seg, slat, idx, val + res, smap);
ENDOP

STARTOP(iattr_sub)
    declare_params(2);
    const attrCode  	slat = attrCode(uint8(param[0]));
    const size_t        idx  = uint8(param[1]);
    const          int  val  = int(pop());
    if ((slat == gr_slatPosX || slat == gr_slatPosY) && (flags & POSITIONED) == 0)
    {
        seg.positionSlots(0, *smap.begin(), *(smap.end()-1));
        flags |= POSITIONED;
    }
    int res = is->getAttr(&seg, slat, idx);
    is->setAttr(&seg, slat, idx, val - res, smap);
ENDOP

STARTOP(push_proc_state)



                    
    
    use_params(1);
    push(1);
ENDOP

STARTOP(push_version)
    push(0x00030000);
ENDOP

STARTOP(put_subs)
    declare_params(5);
    const int        slot_ref     = int8(param[0]);
    const unsigned int  input_class  = uint8(param[1]) << 8
                                     | uint8(param[2]);
    const unsigned int  output_class = uint8(param[3]) << 8
                                     | uint8(param[4]);
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        int index = seg.findClassIndex(input_class, slot->gid());
        is->setGlyph(&seg, seg.getClassGlyph(output_class, index));
    }
ENDOP

#if 0
STARTOP(put_subs2) 
    NOT_IMPLEMENTED;
ENDOP

STARTOP(put_subs3) 
    NOT_IMPLEMENTED;
ENDOP
#endif

STARTOP(put_glyph)
    declare_params(2);
    const unsigned int output_class  = uint8(param[0]) << 8
                                     | uint8(param[1]);
    is->setGlyph(&seg, seg.getClassGlyph(output_class, 0));
ENDOP

STARTOP(push_glyph_attr)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8(param[0]) << 8
                                    | uint8(param[1]);
    const int           slot_ref    = int8(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot)
        push(seg.glyphAttr(slot->gid(), glyph_attr));
ENDOP

STARTOP(push_att_to_glyph_attr)
    declare_params(3);
    const unsigned int  glyph_attr  = uint8(param[0]) << 8
                                    | uint8(param[1]);
    const int           slot_ref    = int8(param[2]);
    slotref slot = slotat(slot_ref);
    if (slot)
    {
        slotref att = slot->attachTo();
        if (att) slot = att;
        push(seg.glyphAttr(slot->gid(), glyph_attr));
    }
ENDOP

STARTOP(temp_copy)
    slotref newSlot = seg.newSlot();
    uint16 *tempUserAttrs = newSlot->userAttrs();
    memcpy(newSlot, is, sizeof(Slot));
    newSlot->userAttrs(tempUserAttrs);
    memcpy(tempUserAttrs, is->userAttrs(), seg.numAttrs() * sizeof(uint16));
    newSlot->markCopied(true);
    *map = newSlot;
ENDOP
