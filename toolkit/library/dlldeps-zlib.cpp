







































#include "zlib.h"

void xxxNeverCalledZLib()
{
    deflate(0, 0);
    deflateInit(0, 0);
	deflateInit2(0, 0, 0, 0, 0, 0);
    deflateEnd(0);
    inflate(0, 0);
    inflateInit(0);
    inflateInit2(0, 0);
    inflateEnd(0);
    inflateReset(0);
}
