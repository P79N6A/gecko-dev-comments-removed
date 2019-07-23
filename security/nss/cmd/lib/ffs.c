


































#if !defined(XP_UNIX) && !defined(XP_OS2)

int ffs( unsigned int i)
{
    int rv	= 1;

    if (!i) return 0;

    while (!(i & 1)) {
    	i >>= 1;
	++rv;
    }

    return rv;
}
#endif
