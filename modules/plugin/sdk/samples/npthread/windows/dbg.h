




































#ifndef __DBG_HPP_
#define __DBG_HPP_

#ifdef NTRACE

#define dbgOut1(x)        ((void)0)
#define dbgOut2(x,y)      ((void)0)
#define dbgOut3(x,y,z)      ((void)0)
#define dbgOut4(x,y,z,t)    ((void)0)
#define dbgOut5(x,y,z,t,u)    ((void)0)
#define dbgOut6(x,y,z,t,u,v)  ((void)0)
#define dbgOut7(x,y,z,t,u,v,a)  ((void)0)
#define dbgOut8(x,y,z,t,u,v,a,b)  ((void)0)

#else

void __cdecl dbgOut(LPSTR format, ...);
#define dbgOut1(x)        dbgOut(x)
#define dbgOut2(x,y)      dbgOut(x, y)
#define dbgOut3(x,y,z)      dbgOut(x, y, z)
#define dbgOut4(x,y,z,t)    dbgOut(x, y, z, t)
#define dbgOut5(x,y,z,t,u)    dbgOut(x, y, z, t, u)
#define dbgOut6(x,y,z,t,u,v)  dbgOut(x, y, z, t, u, v)
#define dbgOut7(x,y,z,t,u,v, a)  dbgOut(x, y, z, t, u, v, a)
#define dbgOut8(x,y,z,t,u,v, a, b)  dbgOut(x, y, z, t, u, v, a, b)

#endif
#endif
