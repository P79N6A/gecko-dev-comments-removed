








































#include "rctime.h"

RCTime::~RCTime() { }

RCTime::RCTime(PRTime time): RCBase() { gmt = time; }
RCTime::RCTime(const RCTime& his): RCBase() { gmt = his.gmt; }
RCTime::RCTime(RCTime::Current): RCBase() { gmt = PR_Now(); }
RCTime::RCTime(const PRExplodedTime& time): RCBase()
{ gmt = PR_ImplodeTime(&time); }

void RCTime::operator=(const PRExplodedTime& time)
{ gmt = PR_ImplodeTime(&time); }

RCTime RCTime::operator+(const RCTime& his)
{ RCTime sum(gmt + his.gmt); return sum; }

RCTime RCTime::operator-(const RCTime& his)
{ RCTime difference(gmt - his.gmt); return difference; }

RCTime RCTime::operator/(PRUint64 his)
{ RCTime quotient(gmt / gmt); return quotient; }

RCTime RCTime::operator*(PRUint64 his)
{ RCTime product(gmt * his); return product; }

