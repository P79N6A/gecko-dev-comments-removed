

























#include "graphite2/Log.h"
#include "inc/debug.h"
#include "inc/CharInfo.h"
#include "inc/Slot.h"
#include "inc/Segment.h"

using namespace graphite2;

extern "C" {


bool graphite_start_logging(FILE * logFile, GrLogMask mask)
{
	if (!logFile || !mask)	return false;

#if !defined GRAPHITE2_NTRACING
	dbgout = new json(logFile);
	return dbgout;
#else
	return false;
#endif
}

void graphite_stop_logging()
{
#if !defined GRAPHITE2_NTRACING
	delete dbgout;
#endif
}


} 

#if !defined GRAPHITE2_NTRACING

json *graphite2::dbgout = 0;


json & graphite2::operator << (json & j, const CharInfo & ci) throw()
{
	return j << json::object
				<< "offset"			<< ci.base()
				<< "unicode"		<< ci.unicodeChar()
				<< "break"			<< ci.breakWeight()
				<< "slot" << json::flat << json::object
					<< "before"	<< ci.before()
					<< "after"	<< ci.after()
					<< json::close
				<< json::close;
}


json & graphite2::operator << (json & j, const dslot & ds) throw()
{
	assert(ds.first);
	assert(ds.second);
	Segment & seg = *ds.first;
	Slot & s = *ds.second;

	j << json::object
		<< "id"				<< slotid(&s)
		<< "gid"			<< s.gid()
		<< "charinfo" << json::flat << json::object
			<< "original"		<< s.original()
			<< "before"			<< s.before()
			<< "after" 			<< s.after()
			<< json::close
		<< "origin"			<< s.origin()
		<< "shift"			<< Position(s.getAttr(0, gr_slatShiftX, 0), s.getAttr(0, gr_slatShiftY, 0))
		<< "advance"		<< s.advancePos()
		<< "insert"			<< s.isInsertBefore()
		<< "break"			<< s.getAttr(&seg, gr_slatBreak, 0);
	if (s.just() > 0)
		j << "justification"	<< s.just();
	if (s.getBidiLevel() > 0)
		j << "bidi"		<< s.getBidiLevel();
	if (!s.isBase())
		j << "parent" << json::flat << json::object
			<< "id"				<< slotid(s.attachedTo())
			<< "level"			<< s.getAttr(0, gr_slatAttLevel, 0)
			<< "offset"			<< s.attachOffset()
			<< json::close;
	j << "user" << json::flat << json::array;
	for (int n = 0; n!= seg.numAttrs(); ++n)
		j	<< s.userAttrs()[n];
		j 	<< json::close;
	if (s.firstChild())
	{
		j	<< "children" << json::flat << json::array;
		for (const Slot *c = s.firstChild(); c; c = c->nextSibling())  j << slotid(c);
		j		<< json::close;
	}
	return j << json::close;
}


graphite2::slotid::slotid(const Slot * const p) throw()
{
	uint32 s = reinterpret_cast<size_t>(p);
	sprintf(name, "%.4x-%.2x-%.4hx", uint16(s >> 16), uint16(p ? p->index() : 0), uint16(s));
	name[sizeof name-1] = 0;
}

#endif
