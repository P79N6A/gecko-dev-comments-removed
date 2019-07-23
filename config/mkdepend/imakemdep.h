




































#ifdef CCIMAKE






#ifdef hpux
#ifdef hp9000s800
#define imake_ccflags "-DSYSV"
#else
#define imake_ccflags "-Wc,-Nd4000,-Ns3000 -DSYSV"
#endif
#endif

#if defined(macII) || defined(_AUX_SOURCE)
#define imake_ccflags "-DmacII -DSYSV"
#endif

#ifdef stellar
#define imake_ccflags "-DSYSV"
#endif

#if defined(USL) || defined(Oki) || defined(NCR)
#define imake_ccflags "-Xc -DSVR4"
#endif

#ifdef sony
#if defined(SYSTYPE_SYSV) || defined(_SYSTYPE_SYSV)
#define imake_ccflags "-DSVR4"
#else
#include <sys/param.h>
#if NEWSOS < 41
#define imake_ccflags "-Dbsd43 -DNOSTDHDRS"
#else
#if NEWSOS < 42
#define imake_ccflags "-Dbsd43"
#endif
#endif
#endif
#endif

#ifdef _CRAY
#define imake_ccflags "-DSYSV -DUSG"
#endif

#if defined(_IBMR2) || defined(aix)
#define imake_ccflags "-Daix -DSYSV"
#endif

#ifdef Mips
#  if defined(SYSTYPE_BSD) || defined(BSD) || defined(BSD43)
#    define imake_ccflags "-DBSD43"
#  else 
#    define imake_ccflags "-DSYSV"
#  endif
#endif 

#ifdef is68k
#define imake_ccflags "-Dluna -Duniosb"
#endif

#ifdef SYSV386
# ifdef SVR4
#  define imake_ccflags "-Xc -DSVR4"
# else
#  define imake_ccflags "-DSYSV"
# endif
#endif

#ifdef SVR4
# ifdef i386
#  define imake_ccflags "-Xc -DSVR4"
# endif
#endif

#ifdef SYSV
# ifdef i386
#  define imake_ccflags "-DSYSV"
# endif
#endif

#ifdef __convex__
#define imake_ccflags "-fn -tm c1"
#endif

#ifdef apollo
#define imake_ccflags "-DX_NOT_POSIX"
#endif

#ifdef WIN32
#define imake_ccflags "-nologo -batch -D__STDC__"
#endif

#ifdef __uxp__
#define imake_ccflags "-DSVR4 -DANSICPP"
#endif

#ifdef __sxg__
#define imake_ccflags "-DSYSV -DUSG -DNOSTDHDRS"
#endif

#ifdef sequent
#define imake_ccflags "-DX_NOT_STDC_ENV -DX_NOT_POSIX"
#endif

#ifdef _SEQUENT_
#define imake_ccflags "-DSYSV -DUSG"
#endif

#if defined(SX) || defined(PC_UX)
#define imake_ccflags "-DSYSV"
#endif

#ifdef nec_ews_svr2
#define imake_ccflags "-DUSG"
#endif

#if defined(nec_ews_svr4) || defined(_nec_ews_svr4) || defined(_nec_up) || defined(_nec_ft)
#define imake_ccflags "-DSVR4"
#endif

#ifdef	MACH
#define imake_ccflags "-DNOSTDHDRS"
#endif


#if defined(__EMX__) 
#define imake_ccflags "-DBSD43"
#endif

#else 
#ifndef MAKEDEPEND






#if defined(SYSV) && !defined(_CRAY) && !defined(Mips) && !defined(_SEQUENT_)
#define	dup2(fd1,fd2)	((fd1 == fd2) ? fd1 : (close(fd2), \
					       fcntl(fd1, F_DUPFD, fd2)))
#endif











#if defined(sun) || defined(SYSV) || defined(SVR4) || defined(hcx) || defined(WIN32) || (defined(AMOEBA) && defined(CROSS_COMPILE))
#define FIXUP_CPP_WHITESPACE
#endif
#ifdef WIN32
#define REMOVE_CPP_LEADSPACE
#define INLINE_SYNTAX
#define MAGIC_MAKE_VARS
#endif
#ifdef __minix_vmd
#define FIXUP_CPP_WHITESPACE
#endif







#ifdef hpux
#define USE_CC_E
#endif
#ifdef WIN32
#define USE_CC_E
#define DEFAULT_CC "cl"
#endif
#ifdef apollo
#define DEFAULT_CPP "/usr/lib/cpp"
#endif
#if defined(_IBMR2) && !defined(DEFAULT_CPP)
#define DEFAULT_CPP "/usr/lpp/X11/Xamples/util/cpp/cpp"
#endif
#if defined(sun) && defined(SVR4)
#define DEFAULT_CPP "/usr/ccs/lib/cpp"
#endif
#ifdef __bsdi__
#define DEFAULT_CPP "/usr/bin/cpp"
#endif
#ifdef __uxp__
#define DEFAULT_CPP "/usr/ccs/lib/cpp"
#endif
#ifdef __sxg__
#define DEFAULT_CPP "/usr/lib/cpp"
#endif
#ifdef _CRAY
#define DEFAULT_CPP "/lib/pcpp"
#endif
#if defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#define DEFAULT_CPP "/usr/libexec/cpp"
#endif
#ifdef	MACH
#define USE_CC_E
#endif
#ifdef __minix_vmd
#define DEFAULT_CPP "/usr/lib/cpp"
#endif
#if defined(__EMX__)

#define DEFAULT_CPP "cpp"
#endif


















#define	ARGUMENTS 50	/* number of arguments in various arrays */
char *cpp_argv[ARGUMENTS] = {
	"cc",		
	"-I.",		
#ifdef unix
	"-Uunix",	
#endif
#if defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(MACH)
# ifdef __i386__
	"-D__i386__",
# endif
# ifdef __x86_64__
	"-D__x86_64__",
# endif
# ifdef __GNUC__
	"-traditional",
# endif
#endif
#ifdef M4330
	"-DM4330",	
#endif
#ifdef M4310
	"-DM4310",	
#endif
#if defined(macII) || defined(_AUX_SOURCE)
	"-DmacII",	
#endif
#ifdef USL
	"-DUSL",	
#endif
#ifdef sony
	"-Dsony",	
#if !defined(SYSTYPE_SYSV) && !defined(_SYSTYPE_SYSV) && NEWSOS < 42
	"-Dbsd43",
#endif
#endif
#ifdef _IBMR2
	"-D_IBMR2",	
#ifndef aix
#define aix		
#endif
#endif 
#ifdef aix
	"-Daix",	
#ifndef ibm
#define ibm		
#endif
#endif 
#ifdef ibm
	"-Dibm",	
#endif
#ifdef luna
	"-Dluna",	
#ifdef luna1
	"-Dluna1",
#endif
#ifdef luna88k		
	"-traditional", 
#endif			
#ifdef uniosb
	"-Duniosb",
#endif
#ifdef uniosu
	"-Duniosu",
#endif
#endif 
#ifdef _CRAY		
	"-Ucray",
#endif
#ifdef Mips
	"-DMips",	
# if defined(SYSTYPE_BSD) || defined(BSD) || defined(BSD43)
	"-DBSD43",	
# else
	"-DSYSV",	
# endif
#endif 
#ifdef MOTOROLA
	"-DMOTOROLA",    
# ifdef SYSV
	"-DSYSV", 
# endif
# ifdef SVR4
	"-DSVR4",
# endif
#endif 
#ifdef i386
	"-Di386",
# ifdef SVR4
	"-DSVR4",
# endif
# ifdef SYSV
	"-DSYSV",
#  ifdef ISC
	"-DISC",
#   ifdef ISC40
	"-DISC40",       
#   else
#    ifdef ISC202
	"-DISC202",      
#    else
#     ifdef ISC30
	"-DISC30",       
#     else
	"-DISC22",       
#     endif
#    endif
#   endif
#  endif
#  ifdef SCO
	"-DSCO",
#   ifdef SCO324
	"-DSCO324",
#   endif
#  endif
# endif
# ifdef ESIX
	"-DESIX",
# endif
# ifdef ATT
	"-DATT",
# endif
# ifdef DELL
	"-DDELL",
# endif
#endif
#ifdef SYSV386           
	"-Di386",
# ifdef SVR4
	"-DSVR4",
# endif
# ifdef ISC
	"-DISC",
#  ifdef ISC40
	"-DISC40",       
#  else
#   ifdef ISC202
	"-DISC202",      
#   else
#    ifdef ISC30
	"-DISC30",       
#    else
	"-DISC22",       
#    endif
#   endif
#  endif
# endif
# ifdef SCO
	"-DSCO",
#  ifdef SCO324
	"-DSCO324",
#  endif
# endif
# ifdef ESIX
	"-DESIX",
# endif
# ifdef ATT
	"-DATT",
# endif
# ifdef DELL
	"-DDELL",
# endif
#endif
#ifdef __osf__
	"-D__osf__",
# ifdef __mips__
	"-D__mips__",
# endif
# ifdef __alpha
	"-D__alpha",
# endif
# ifdef __i386__
	"-D__i386__",
# endif
# ifdef __GNUC__
	"-traditional",
# endif
#endif
#ifdef Oki
	"-DOki",
#endif
#ifdef sun
#ifdef SVR4
	"-DSVR4",
#endif
#endif
#ifdef WIN32
	"-DWIN32",
	"-nologo",
	"-batch",
	"-D__STDC__",
#endif
#ifdef NCR
	"-DNCR",	
#endif
#ifdef linux
        "-traditional",
        "-Dlinux",
#endif
#ifdef __uxp__
	"-D__uxp__",
#endif
#ifdef __sxg__
	"-D__sxg__",
#endif
#ifdef nec_ews_svr2
	"-Dnec_ews_svr2",
#endif
#ifdef AMOEBA
	"-DAMOEBA",
# ifdef CROSS_COMPILE
	"-DCROSS_COMPILE",
#  ifdef CROSS_i80386
	"-Di80386",
#  endif
#  ifdef CROSS_sparc
	"-Dsparc",
#  endif
#  ifdef CROSS_mc68000
	"-Dmc68000",
#  endif
# else
#  ifdef i80386
	"-Di80386",
#  endif
#  ifdef sparc
	"-Dsparc",
#  endif
#  ifdef mc68000
	"-Dmc68000",
#  endif
# endif
#endif
#ifdef __minix_vmd
        "-Dminix",
#endif

#if defined(__EMX__)
	"-traditional",
	"-Demxos2",
#endif

};
#else 






struct symtab	predefs[] = {
#ifdef apollo
	{"apollo", "1"},
#endif
#ifdef ibm032
	{"ibm032", "1"},
#endif
#ifdef ibm
	{"ibm", "1"},
#endif
#ifdef aix
	{"aix", "1"},
#endif
#ifdef sun
	{"sun", "1"},
#endif
#ifdef sun2
	{"sun2", "1"},
#endif
#ifdef sun3
	{"sun3", "1"},
#endif
#ifdef sun4
	{"sun4", "1"},
#endif
#ifdef sparc
	{"sparc", "1"},
#endif
#ifdef __sparc__
	{"__sparc__", "1"},
#endif
#ifdef hpux
	{"hpux", "1"},
#endif
#ifdef __hpux
	{"__hpux", "1"},
#endif
#ifdef __hp9000s800
	{"__hp9000s800", "1"},
#endif
#ifdef __hp9000s700
	{"__hp9000s700", "1"},
#endif
#ifdef vax
	{"vax", "1"},
#endif
#ifdef VMS
	{"VMS", "1"},
#endif
#ifdef cray
	{"cray", "1"},
#endif
#ifdef CRAY
	{"CRAY", "1"},
#endif
#ifdef _CRAY
	{"_CRAY", "1"},
#endif
#ifdef att
	{"att", "1"},
#endif
#ifdef mips
	{"mips", "1"},
#endif
#ifdef __mips__
	{"__mips__", "1"},
#endif
#ifdef ultrix
	{"ultrix", "1"},
#endif
#ifdef stellar
	{"stellar", "1"},
#endif
#ifdef mc68000
	{"mc68000", "1"},
#endif
#ifdef mc68020
	{"mc68020", "1"},
#endif
#ifdef __GNUC__
	{"__GNUC__", "1"},
#endif
#if __STDC__
	{"__STDC__", "1"},
#endif
#ifdef __HIGHC__
	{"__HIGHC__", "1"},
#endif
#ifdef CMU
	{"CMU", "1"},
#endif
#ifdef luna
	{"luna", "1"},
#ifdef luna1
	{"luna1", "1"},
#endif
#ifdef luna2
	{"luna2", "1"},
#endif
#ifdef luna88k
	{"luna88k", "1"},
#endif
#ifdef uniosb
	{"uniosb", "1"},
#endif
#ifdef uniosu
	{"uniosu", "1"},
#endif
#endif
#ifdef ieeep754
	{"ieeep754", "1"},
#endif
#ifdef is68k
	{"is68k", "1"},
#endif
#ifdef m68k
        {"m68k", "1"},
#endif
#ifdef m88k
        {"m88k", "1"},
#endif
#ifdef __m88k__
	{"__m88k__", "1"},
#endif
#ifdef bsd43
	{"bsd43", "1"},
#endif
#ifdef hcx
	{"hcx", "1"},
#endif
#ifdef sony
	{"sony", "1"},
#ifdef SYSTYPE_SYSV
	{"SYSTYPE_SYSV", "1"},
#endif
#ifdef _SYSTYPE_SYSV
	{"_SYSTYPE_SYSV", "1"},
#endif
#endif
#ifdef __OSF__
	{"__OSF__", "1"},
#endif
#ifdef __osf__
	{"__osf__", "1"},
#endif
#ifdef __alpha
	{"__alpha", "1"},
#endif
#ifdef __DECC
	{"__DECC",  "1"},
#endif
#ifdef __decc
	{"__decc",  "1"},
#endif
#ifdef __uxp__
	{"__uxp__", "1"},
#endif
#ifdef __sxg__
	{"__sxg__", "1"},
#endif
#ifdef _SEQUENT_
	{"_SEQUENT_", "1"},
	{"__STDC__", "1"},
#endif
#ifdef __bsdi__
	{"__bsdi__", "1"},
#endif
#ifdef nec_ews_svr2
	{"nec_ews_svr2", "1"},
#endif
#ifdef nec_ews_svr4
	{"nec_ews_svr4", "1"},
#endif
#ifdef _nec_ews_svr4
	{"_nec_ews_svr4", "1"},
#endif
#ifdef _nec_up
	{"_nec_up", "1"},
#endif
#ifdef SX
	{"SX", "1"},
#endif
#ifdef nec
	{"nec", "1"},
#endif
#ifdef _nec_ft
	{"_nec_ft", "1"},
#endif
#ifdef PC_UX
	{"PC_UX", "1"},
#endif
#ifdef sgi
	{"sgi", "1"},
#endif
#ifdef __sgi
	{"__sgi", "1"},
#endif
#ifdef __FreeBSD__
	{"__FreeBSD__", "1"},
#endif
#ifdef __NetBSD__
	{"__NetBSD__", "1"},
#endif
#ifdef __OpenBSD__
	{"__OpenBSD__", "1"},
#endif
#ifdef __EMX__
	{"__EMX__", "1"},
#endif
	
	{NULL, NULL}
};

#endif 
#endif 
