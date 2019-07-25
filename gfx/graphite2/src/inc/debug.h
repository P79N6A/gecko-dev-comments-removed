






























#pragma once

#if !defined GRAPHITE2_NTRACING

#include <utility>
#include "inc/json.h"
#include "inc/Position.h"

namespace graphite2
{

class CharInfo;
class Segment;
class Slot;

typedef std::pair<Segment * const, Slot * const>	dslot;
struct slotid
{
	char name[16];
	slotid(const Slot * const p) throw();
};

extern json * dbgout;

json & operator << (json & j, const Position &) throw();
json & operator << (json & j, const CharInfo &) throw();
json & operator << (json & j, const dslot &) throw();
json & operator << (json & j, const slotid &) throw();


inline
json & operator << (json & j, const Position & p) throw()
{
	return j << json::flat << json::array << p.x << p.y << json::close;
}


inline
json & operator << (json & j, const slotid & sid) throw()
{
	return j << sid.name;
}


} 

#endif 

