













































#include <unistd.h>

void
__cyg_profile_func_enter(void *this_fn, void *call_site)
{
    write(STDOUT_FILENO, &this_fn, sizeof this_fn);
}

void
__cyg_profile_func_exit(void *this_fn, void *call_site)
{
    write(STDOUT_FILENO, &call_site, sizeof call_site);
}
