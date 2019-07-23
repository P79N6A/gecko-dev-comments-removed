






#include "prnetdb.h"



































































#define XX 127
static const unsigned char index_hex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

static PRBool _isdigit(char c) { return c >= '0' && c <= '9'; }
static PRBool _isxdigit(char c) { return index_hex[(unsigned char) c] != XX; }
static PRBool _isspace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
#undef XX

int
pr_inet_aton(const char *cp, PRUint32 *addr)
{
    PRUint32 val;
    int base, n;
    char c;
    PRUint8 parts[4];
    PRUint8 *pp = parts;
    int digit;

    c = *cp;
    for (;;) {
        




        if (!_isdigit(c))
            return (0);
        val = 0; base = 10; digit = 0;
        if (c == '0') {
            c = *++cp;
            if (c == 'x' || c == 'X')
                base = 16, c = *++cp;
            else {
                base = 8;
                digit = 1;
            }
        }
        for (;;) {
            if (_isdigit(c)) {
                if (base == 8 && (c == '8' || c == '9'))
                    return (0);
                val = (val * base) + (c - '0');
                c = *++cp;
                digit = 1;
            } else if (base == 16 && _isxdigit(c)) {
                val = (val << 4) + index_hex[(unsigned char) c];
                c = *++cp;
                digit = 1;
            } else
                break;
        }
        if (c == '.') {
            





            if (pp >= parts + 3 || val > 0xffU)
                return (0);
            *pp++ = val;
            c = *++cp;
        } else
            break;
    }
    


    if (c != '\0' && !_isspace(c))
        return (0);
    


    if (!digit)
        return (0);
    



    n = pp - parts + 1;
    switch (n) {
    case 1:                
        break;

    case 2:                
        if (val > 0xffffffU)
            return (0);
        val |= parts[0] << 24;
        break;

    case 3:                
        if (val > 0xffffU)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4:                
        if (val > 0xffU)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }
    *addr = PR_htonl(val);
    return (1);
}

