






























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

typedef std::pair<const Segment * const, const Slot * const>	dslot;
struct objectid
{
	char name[16];
	objectid(const dslot &) throw();
	objectid(const Segment * const p) throw();
};

json & operator << (json & j, const Position &) throw();
json & operator << (json & j, const CharInfo &) throw();
json & operator << (json & j, const dslot &) throw();
json & operator << (json & j, const objectid &) throw();


inline
json & operator << (json & j, const Position & p) throw()
{
	return j << json::flat << json::array << p.x << p.y << json::close;
}


inline
json & operator << (json & j, const objectid & sid) throw()
{
	return j << sid.name;
}


} 

#endif 

