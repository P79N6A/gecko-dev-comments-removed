












#include <string.h>
#include "misc.h"
#include "process.h"

static const char nodtdopt[] = "-no-dtd-ref";
const char *progname;








static const char *const *
options(int argc, const char *const *argv)
{
    
    {
        const char *base;
        progname = argv[0];
        base = strrchr(progname, '/');
#ifdef DIRSEP
        {
            const char *base2 = strrchr(base, '\\');
            if (base2 > base)
                base = base2;
        }
#endif 
        if (base)
            progname = base + 1;
    }
    return (argc > 1 && strncmp(argv[1], nodtdopt, sizeof nodtdopt) == 0)
          ? argv + 2 : argv + 1;
}




int
main(int argc, char **argv)
{
    const char *const *parg;
    parg = options(argc, (const char *const *)argv);
    if (!*parg)
        errorexit("usage: %s [-no-dtd-ref] <interface>.widl ...", progname);
    processfiles(parg, parg == (const char *const *)argv + 1);
    return 0;
}

