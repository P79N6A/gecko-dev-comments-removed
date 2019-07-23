





















#ifndef lint
#ifdef __GNUC__
static char rcsid[] __attribute__ ((unused)) = "$Id: ucgendat.c,v 1.1 1999/01/08 00:19:21 ftang%netscape.com Exp $";
#else
static char rcsid[] = "$Id: ucgendat.c,v 1.1 1999/01/08 00:19:21 ftang%netscape.com Exp $";
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

#define ishdigit(cc) (((cc) >= '0' && (cc) <= '9') ||\
                      ((cc) >= 'A' && (cc) <= 'F') ||\
                      ((cc) >= 'a' && (cc) <= 'f'))





static unsigned short hdr[2] = {0xfeff, 0};

#define NUMPROPS 49
#define NEEDPROPS (NUMPROPS + (4 - (NUMPROPS & 3)))

typedef struct {
    char *name;
    int len;
} _prop_t;















static _prop_t props[NUMPROPS] = {
    {"Mn", 2}, {"Mc", 2}, {"Me", 2}, {"Nd", 2}, {"Nl", 2}, {"No", 2},
    {"Zs", 2}, {"Zl", 2}, {"Zp", 2}, {"Cc", 2}, {"Cf", 2}, {"Cs", 2},
    {"Co", 2}, {"Cn", 2}, {"Lu", 2}, {"Ll", 2}, {"Lt", 2}, {"Lm", 2},
    {"Lo", 2}, {"Pc", 2}, {"Pd", 2}, {"Ps", 2}, {"Pe", 2}, {"Po", 2},
    {"Sm", 2}, {"Sc", 2}, {"Sk", 2}, {"So", 2}, {"L",  1}, {"R",  1},
    {"EN", 2}, {"ES", 2}, {"ET", 2}, {"AN", 2}, {"CS", 2}, {"B",  1},
    {"S",  1}, {"WS", 2}, {"ON", 2},
    {"Cm", 2}, {"Nb", 2}, {"Sy", 2}, {"Hd", 2}, {"Qm", 2}, {"Mr", 2},
    {"Ss", 2}, {"Cp", 2}, {"Pi", 2}, {"Pf", 2}
};

typedef struct {
    unsigned long *ranges;
    unsigned short used;
    unsigned short size;
} _ranges_t;

static _ranges_t proptbl[NUMPROPS];




static unsigned short propcnt[NEEDPROPS];





static unsigned long dectmp[64];
static unsigned long dectmp_size;

typedef struct {
    unsigned long code;
    unsigned short size;
    unsigned short used;
    unsigned long *decomp;
} _decomp_t;





static _decomp_t *decomps;
static unsigned long decomps_used;
static unsigned long decomps_size;




typedef struct {
    unsigned long key;
    unsigned long other1;
    unsigned long other2;
} _case_t;

static _case_t *upper;
static _case_t *lower;
static _case_t *title;
static unsigned long upper_used;
static unsigned long upper_size;
static unsigned long lower_used;
static unsigned long lower_size;
static unsigned long title_used;
static unsigned long title_size;




static unsigned long cases[3];




static unsigned long *ccl;
static unsigned long ccl_used;
static unsigned long ccl_size;




typedef struct {
    unsigned long code;
    unsigned long idx;
} _codeidx_t;

typedef struct {
    short numerator;
    short denominator;
} _num_t;




static _codeidx_t *ncodes;
static unsigned long ncodes_used;
static unsigned long ncodes_size;

static _num_t *nums;
static unsigned long nums_used;
static unsigned long nums_size;




static _num_t *nums;
static unsigned long nums_used;
static unsigned long nums_size;

static void
#ifdef __STDC__
add_range(unsigned long start, unsigned long end, char *p1, char *p2)
#else
add_range(start, end, p1, p2)
unsigned long start, end;
char *p1, *p2;
#endif
{
    int i, j, k, len;
    _ranges_t *rlp;
    char *name;

    for (k = 0; k < 2; k++) {
        if (k == 0) {
            name = p1;
            len = 2;
        } else {
            if (p2 == 0)
              break;

            name = p2;
            len = 1;
        }

        for (i = 0; i < NUMPROPS; i++) {
            if (props[i].len == len && memcmp(props[i].name, name, len) == 0)
              break;
        }

        if (i == NUMPROPS)
          continue;

        rlp = &proptbl[i];

        


        if (rlp->used == rlp->size) {
            if (rlp->size == 0)
              rlp->ranges = (unsigned long *)
                  malloc(sizeof(unsigned long) << 3);
            else
              rlp->ranges = (unsigned long *)
                  realloc((char *) rlp->ranges,
                          sizeof(unsigned long) * (rlp->size + 8));
            rlp->size += 8;
        }

        



        if (rlp->used == 0) {
            rlp->ranges[0] = start;
            rlp->ranges[1] = end;
            rlp->used += 2;
            continue;
        }

        


        j = rlp->used - 1;
        if (start > rlp->ranges[j]) {
            j = rlp->used;
            rlp->ranges[j++] = start;
            rlp->ranges[j++] = end;
            rlp->used = j;
            continue;
        }

        


        for (i = 0;
             i < rlp->used && start > rlp->ranges[i + 1] + 1; i += 2) ;

        



        if (rlp->ranges[i] <= start && start <= rlp->ranges[i + 1] + 1) {
            rlp->ranges[i + 1] = end;
            return;
        }

        


        for (j = rlp->used; j > i; j -= 2) {
            rlp->ranges[j] = rlp->ranges[j - 2];
            rlp->ranges[j + 1] = rlp->ranges[j - 1];
        }

        


        rlp->ranges[i] = start;
        rlp->ranges[i + 1] = end;
        rlp->used += 2;
    }
}

static void
#ifdef __STDC__
ordered_range_insert(unsigned long c, char *name, int len)
#else
ordered_range_insert(c, name, len)
unsigned long c;
char *name;
int len;
#endif
{
    int i, j;
    unsigned long s, e;
    _ranges_t *rlp;

    if (len == 0)
      return;

    for (i = 0; i < NUMPROPS; i++) {
        if (props[i].len == len && memcmp(props[i].name, name, len) == 0)
          break;
    }

    if (i == NUMPROPS)
      return;

    


    rlp = &proptbl[i];

    


    if (rlp->used == rlp->size) {
        if (rlp->size == 0)
          rlp->ranges = (unsigned long *)
              malloc(sizeof(unsigned long) << 3);
        else
          rlp->ranges = (unsigned long *)
              realloc((char *) rlp->ranges,
                      sizeof(unsigned long) * (rlp->size + 8));
        rlp->size += 8;
    }

    



    if (rlp->used == 0) {
        rlp->ranges[0] = rlp->ranges[1] = c;
        rlp->used += 2;
        return;
    }

    



    j = rlp->used - 1;
    e = rlp->ranges[j];
    s = rlp->ranges[j - 1];

    if (c == e + 1) {
        


        rlp->ranges[j] = c;
        return;
    }

    if (c > e + 1) {
        


        j = rlp->used;
        rlp->ranges[j] = rlp->ranges[j + 1] = c;
        rlp->used += 2;
        return;
    }

    if (c >= s)
      


      return;

    



    for (i = 0;
         i < rlp->used && c > rlp->ranges[i + 1] + 1; i += 2) ;

    s = rlp->ranges[i];
    e = rlp->ranges[i + 1];

    if (c == e + 1)
      


      rlp->ranges[i + 1] = c;
    else if (c < s) {
        



        for (j = rlp->used; j > i; j -= 2) {
            rlp->ranges[j] = rlp->ranges[j - 2];
            rlp->ranges[j + 1] = rlp->ranges[j - 1];
        }
        rlp->ranges[i] = rlp->ranges[i + 1] = c;

        rlp->used += 2;
    }
}

static void
#ifdef __STDC__
add_decomp(unsigned long code)
#else
add_decomp(code)
unsigned long code;
#endif
{
    unsigned long i, j, size;

    


    ordered_range_insert(code, "Cm", 2);

    


    for (i = 0; i < decomps_used && code > decomps[i].code; i++) ;

    


    if (decomps_used == decomps_size) {
        if (decomps_size == 0)
          decomps = (_decomp_t *) malloc(sizeof(_decomp_t) << 3);
        else
          decomps = (_decomp_t *)
              realloc((char *) decomps,
                      sizeof(_decomp_t) * (decomps_size + 8));
        (void) memset((char *) (decomps + decomps_size), 0,
                      sizeof(_decomp_t) << 3);
        decomps_size += 8;
    }

    if (i < decomps_used && code != decomps[i].code) {
        


        for (j = decomps_used; j > i; j--)
          (void) memcpy((char *) &decomps[j], (char *) &decomps[j - 1],
                        sizeof(_decomp_t));
    }

    


    size = dectmp_size + (4 - (dectmp_size & 3));
    if (decomps[i].size < size) {
        if (decomps[i].size == 0)
          decomps[i].decomp = (unsigned long *)
              malloc(sizeof(unsigned long) * size);
        else
          decomps[i].decomp = (unsigned long *)
              realloc((char *) decomps[i].decomp,
                      sizeof(unsigned long) * size);
        decomps[i].size = size;
    }

    if (decomps[i].code != code)
      decomps_used++;

    decomps[i].code = code;
    decomps[i].used = dectmp_size;
    (void) memcpy((char *) decomps[i].decomp, (char *) dectmp,
                  sizeof(unsigned long) * dectmp_size);

}

static void
#ifdef __STDC__
add_title(unsigned long code)
#else
add_title(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    


    cases[2] = code;

    if (title_used == title_size) {
        if (title_size == 0)
          title = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          title = (_case_t *) realloc((char *) title,
                                      sizeof(_case_t) * (title_size + 8));
        title_size += 8;
    }

    


    for (i = 0; i < title_used && code > title[i].key; i++) ;

    if (i < title_used) {
        


        for (j = title_used; j > i; j--)
          (void) memcpy((char *) &title[j], (char *) &title[j - 1],
                        sizeof(_case_t));
    }

    title[i].key = cases[2];    
    title[i].other1 = cases[0]; 
    title[i].other2 = cases[1]; 

    title_used++;
}

static void
#ifdef __STDC__
add_upper(unsigned long code)
#else
add_upper(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    


    cases[0] = code;

    



    if (cases[2] == 0)
      cases[2] = code;

    if (upper_used == upper_size) {
        if (upper_size == 0)
          upper = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          upper = (_case_t *) realloc((char *) upper,
                                      sizeof(_case_t) * (upper_size + 8));
        upper_size += 8;
    }

    


    for (i = 0; i < upper_used && code > upper[i].key; i++) ;

    if (i < upper_used) {
        


        for (j = upper_used; j > i; j--)
          (void) memcpy((char *) &upper[j], (char *) &upper[j - 1],
                        sizeof(_case_t));
    }

    upper[i].key = cases[0];    
    upper[i].other1 = cases[1]; 
    upper[i].other2 = cases[2]; 

    upper_used++;
}

static void
#ifdef __STDC__
add_lower(unsigned long code)
#else
add_lower(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    


    cases[1] = code;

    



    if (cases[2] == 0)
      cases[2] = cases[0];

    if (lower_used == lower_size) {
        if (lower_size == 0)
          lower = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          lower = (_case_t *) realloc((char *) lower,
                                      sizeof(_case_t) * (lower_size + 8));
        lower_size += 8;
    }

    


    for (i = 0; i < lower_used && code > lower[i].key; i++) ;

    if (i < lower_used) {
        


        for (j = lower_used; j > i; j--)
          (void) memcpy((char *) &lower[j], (char *) &lower[j - 1],
                        sizeof(_case_t));
    }

    lower[i].key = cases[1];    
    lower[i].other1 = cases[0]; 
    lower[i].other2 = cases[2]; 

    lower_used++;
}

static void
#ifdef __STDC__
ordered_ccl_insert(unsigned long c, unsigned long ccl_code)
#else
ordered_ccl_insert(c, ccl_code)
unsigned long c, ccl_code;
#endif
{
    unsigned long i, j;

    if (ccl_used == ccl_size) {
        if (ccl_size == 0)
          ccl = (unsigned long *) malloc(sizeof(unsigned long) * 24);
        else
          ccl = (unsigned long *)
              realloc((char *) ccl, sizeof(unsigned long) * (ccl_size + 24));
        ccl_size += 24;
    }

    


    if (ccl_used == 0) {
        ccl[0] = ccl[1] = c;
        ccl[2] = ccl_code;
        ccl_used += 3;
        return;
    }

    



    if (ccl_code == ccl[ccl_used - 1] && c == ccl[ccl_used - 2] + 1) {
        ccl[ccl_used - 2] = c;
        return;
    }

    


    if (c > ccl[ccl_used - 2] + 1 ||
        (c == ccl[ccl_used - 2] + 1 && ccl_code != ccl[ccl_used - 1])) {
        ccl[ccl_used++] = c;
        ccl[ccl_used++] = c;
        ccl[ccl_used++] = ccl_code;
        return;
    }

    


    for (i = 0; i < ccl_used && c > ccl[i + 1] + 1; i += 3) ;

    if (ccl_code == ccl[i + 2] && c == ccl[i + 1] + 1) {
        


        ccl[i + 1] = c;
        return;
    } else if (c < ccl[i]) {
        


        for (j = ccl_used; j > i; j -= 3) {
            ccl[j] = ccl[j - 3];
            ccl[j - 1] = ccl[j - 4];
            ccl[j - 2] = ccl[j - 5];
        }
        ccl[i] = ccl[i + 1] = c;
        ccl[i + 2] = ccl_code;
    }
}





static unsigned long
#ifdef __STDC__
make_number(short num, short denom)
#else
make_number(num, denom)
short num, denom;
#endif
{
    unsigned long n;

    


    for (n = 0; n < nums_used; n++) {
        if (nums[n].numerator == num && nums[n].denominator == denom)
          return n << 1;
    }

    if (nums_used == nums_size) {
        if (nums_size == 0)
          nums = (_num_t *) malloc(sizeof(_num_t) << 3);
        else
          nums = (_num_t *) realloc((char *) nums,
                                    sizeof(_num_t) * (nums_size + 8));
        nums_size += 8;
    }

    n = nums_used++;
    nums[n].numerator = num;
    nums[n].denominator = denom;

    return n << 1;
}

static void
#ifdef __STDC__
add_number(unsigned long code, short num, short denom)
#else
add_number(code, num, denom)
unsigned long code;
short num, denom;
#endif
{
    unsigned long i, j;

    


    for (i = 0; i < ncodes_used && code > ncodes[i].code; i++) ;

    



    if (ncodes_used > 0 && code == ncodes[i].code) {
        ncodes[i].idx = make_number(num, denom);
        return;
    }

    


    if (ncodes_used == ncodes_size) {
        if (ncodes_size == 0)
          ncodes = (_codeidx_t *) malloc(sizeof(_codeidx_t) << 3);
        else
          ncodes = (_codeidx_t *)
              realloc((char *) ncodes, sizeof(_codeidx_t) * (ncodes_size + 8));

        ncodes_size += 8;
    }

    


    if (i < ncodes_used) {
        for (j = ncodes_used; j > i; j--) {
            ncodes[j].code = ncodes[j - 1].code;
            ncodes[j].idx = ncodes[j - 1].idx;
        }
    }
    ncodes[i].code = code;
    ncodes[i].idx = make_number(num, denom);

    ncodes_used++;
}





static void
#ifdef __STDC__
read_cdata(FILE *in)
#else
read_cdata(in)
FILE *in;
#endif
{
    unsigned long i, lineno, skip, code, ccl_code;
    short wnum, neg, number[2];
    char line[512], *s, *e;

    lineno = skip = 0;
    while (fscanf(in, "%[^\n]\n", line) != EOF) {
        lineno++;

        


        if (line[0] == 0 || line[0] == '#')
          continue;

        


        if (skip) {
            skip--;
            continue;
        }

        



        for (s = line, i = code = 0; *s != ';' && i < 6; i++, s++) {
            code <<= 4;
            if (*s >= '0' && *s <= '9')
              code += *s - '0';
            else if (*s >= 'A' && *s <= 'F')
              code += (*s - 'A') + 10;
            else if (*s >= 'a' && *s <= 'f')
              code += (*s - 'a') + 10;
        }

        







        switch (code) {
          case 0x4e00:
            


            add_range(0x4e00, 0x9fff, "Lo", "L");

            


            add_range(0x4e00, 0x9fa5, "Cp", 0);

            skip = 1;
            break;
          case 0xac00:
            


            add_range(0xac00, 0xd7a3, "Lo", "L");

            


            add_range(0xac00, 0xd7a3, "Cp", 0);

            skip = 1;
            break;
          case 0xd800:
            



            add_range(0x010000, 0x10ffff, "Cs", "L");
            skip = 5;
            break;
          case 0xe000:
            


            add_range(0xe000, 0xf8ff, "Co", "L");
            skip = 1;
            break;
          case 0xf900:
            


            add_range(0xf900, 0xfaff, "Lo", "L");

            


            add_range(0xf900, 0xfaff, "Cp", 0);

            skip = 1;
        }

        if (skip)
          continue;

        


        ordered_range_insert(code, "Cp", 2);

        


        for (i = 0; *s != 0 && i < 2; s++) {
            if (*s == ';')
              i++;
        }
        for (e = s; *e && *e != ';'; e++) ;
    
        ordered_range_insert(code, s, e - s);

        


        for (s = e; *s != 0 && i < 3; s++) {
            if (*s == ';')
              i++;
        }

        


        for (ccl_code = 0, e = s; *e && *e != ';'; e++)
          ccl_code = (ccl_code * 10) + (*e - '0');

        


        if (ccl_code != 0)
          ordered_ccl_insert(code, ccl_code);

        


        for (s = e; *s != 0 && i < 4; s++) {
            if (*s == ';')
              i++;
        }
        for (e = s; *e && *e != ';'; e++) ;

        ordered_range_insert(code, s, e - s);

        


        s = ++e;
        if (*s != ';' && *s != '<') {
            


            for (dectmp_size = 0; *s != ';'; ) {
                


                while (!ishdigit(*s))
                  s++;

                for (dectmp[dectmp_size] = 0; ishdigit(*s); s++) {
                    dectmp[dectmp_size] <<= 4;
                    if (*s >= '0' && *s <= '9')
                      dectmp[dectmp_size] += *s - '0';
                    else if (*s >= 'A' && *s <= 'F')
                      dectmp[dectmp_size] += (*s - 'A') + 10;
                    else if (*s >= 'a' && *s <= 'f')
                      dectmp[dectmp_size] += (*s - 'a') + 10;
                }
                dectmp_size++;
            }

            



            if (dectmp_size > 1)
              add_decomp(code);
        }

        


        for (i = 0; i < 3 && *s; s++) {
            if (*s == ';')
              i++;
        }

        


        number[0] = number[1] = 0;
        for (e = s, neg = wnum = 0; *e && *e != ';'; e++) {
            if (*e == '-') {
                neg = 1;
                continue;
            }

            if (*e == '/') {
                


                if (neg)
                  number[wnum] *= -1;
                neg = 0;
                e++;
                wnum++;
            }
            number[wnum] = (number[wnum] * 10) + (*e - '0');
        }

        if (e > s) {
            


            if (wnum == 0)
              number[1] = number[0];

            add_number(code, number[0], number[1]);
        }

        


        for (s = e, i = 0; i < 4 && *s; s++) {
            if (*s == ';')
              i++;
        }

        


        cases[0] = cases[1] = cases[2] = 0;
        for (i = 0; i < 3; i++) {
            while (ishdigit(*s)) {
                cases[i] <<= 4;
                if (*s >= '0' && *s <= '9')
                  cases[i] += *s - '0';
                else if (*s >= 'A' && *s <= 'F')
                  cases[i] += (*s - 'A') + 10;
                else if (*s >= 'a' && *s <= 'f')
                  cases[i] += (*s - 'a') + 10;
                s++;
            }
            if (*s == ';')
              s++;
        }
        if (cases[0] && cases[1])
          


          add_title(code);
        else if (cases[1])
          



          add_upper(code);
        else if (cases[0])
          



          add_lower(code);
    }
}

static _decomp_t *
#ifdef __STDC__
find_decomp(unsigned long code)
#else
find_decomp(code)
unsigned long code;
#endif
{
    long l, r, m;

    l = 0;
    r = decomps_used - 1;
    while (l <= r) {
        m = (l + r) >> 1;
        if (code > decomps[m].code)
          l = m + 1;
        else if (code < decomps[m].code)
          r = m - 1;
        else
          return &decomps[m];
    }
    return 0;
}

static void
#ifdef __STDC__
decomp_it(_decomp_t *d)
#else
decomp_it(d)
_decomp_t *d;
#endif
{
    unsigned long i;
    _decomp_t *dp;

    for (i = 0; i < d->used; i++) {
        if ((dp = find_decomp(d->decomp[i])) != 0)
          decomp_it(dp);
        else
          dectmp[dectmp_size++] = d->decomp[i];
    }
}





static void
#ifdef __STDC__
expand_decomp(void)
#else
expand_decomp()
#endif
{
    unsigned long i;

    for (i = 0; i < decomps_used; i++) {
        dectmp_size = 0;
        decomp_it(&decomps[i]);
        if (dectmp_size > 0)
          add_decomp(decomps[i].code);
    }
}

static void
#ifdef __STDC__
write_cdata(char *opath)
#else
write_cdata(opath)
char *opath;
#endif
{
    FILE *out;
    unsigned long i, idx, bytes, nprops;
    unsigned short casecnt[2];
    char path[BUFSIZ];

    





    


    sprintf(path, "%s/ctype.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    




    for (i = idx = 0; i < NUMPROPS; i++) {
        propcnt[i] = (proptbl[i].used != 0) ? idx : 0xffff;
        idx += proptbl[i].used;
    }

    



    propcnt[i] = idx;

    




    hdr[1] = NUMPROPS;

    



    if ((bytes = sizeof(unsigned short) * (NUMPROPS + 1)) & 3)
      bytes += 4 - (bytes & 3);
    nprops = bytes / sizeof(unsigned short);
    bytes += sizeof(unsigned long) * idx;
        
    


    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    


    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    


    fwrite((char *) propcnt, sizeof(unsigned short), nprops, out);

    


    for (i = 0; i < NUMPROPS; i++) {
        if (proptbl[i].used > 0)
          fwrite((char *) proptbl[i].ranges, sizeof(unsigned long),
                 proptbl[i].used, out);
    }

    fclose(out);

    





    


    sprintf(path, "%s/case.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    


    hdr[1] = upper_used + lower_used + title_used;
    casecnt[0] = upper_used;
    casecnt[1] = lower_used;

    


    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    


    fwrite((char *) casecnt, sizeof(unsigned short), 2, out);

    if (upper_used > 0)
      


      fwrite((char *) upper, sizeof(_case_t), upper_used, out);

    if (lower_used > 0)
      


      fwrite((char *) lower, sizeof(_case_t), lower_used, out);

    if (title_used > 0)
      


      fwrite((char *) title, sizeof(_case_t), title_used, out);

    fclose(out);

    





    


    expand_decomp();

    


    sprintf(path, "%s/decomp.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    hdr[1] = decomps_used;

    


    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    



    bytes = 0;
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    if (decomps_used) {
        


        for (i = idx = 0; i < decomps_used; i++) {
            fwrite((char *) &decomps[i].code, sizeof(unsigned long), 1, out);
            fwrite((char *) &idx, sizeof(unsigned long), 1, out);
            idx += decomps[i].used;
        }

        


        fwrite((char *) &idx, sizeof(unsigned long), 1, out);

        


        for (i = 0; i < decomps_used; i++)
          fwrite((char *) decomps[i].decomp, sizeof(unsigned long),
                 decomps[i].used, out);

        


        bytes = (sizeof(unsigned long) * idx) +
            (sizeof(unsigned long) * ((hdr[1] << 1) + 1));
        fseek(out, sizeof(unsigned short) << 1, 0L);
        fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

        fclose(out);
    }

    





    


    sprintf(path, "%s/cmbcl.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    



    hdr[1] = ccl_used / 3;

    


    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    


    bytes = ccl_used * sizeof(unsigned long);
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    if (ccl_used > 0)
      


      fwrite((char *) ccl, sizeof(unsigned long), ccl_used, out);

    fclose(out);

    





    


    sprintf(path, "%s/num.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    



    hdr[1] = (unsigned short) (ncodes_used << 1);
    bytes = (ncodes_used * sizeof(_codeidx_t)) + (nums_used * sizeof(_num_t));

    


    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    


    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    


    if (ncodes_used > 0) {
        fwrite((char *) ncodes, sizeof(_codeidx_t), ncodes_used, out);
        fwrite((char *) nums, sizeof(_num_t), nums_used, out);
    }

    fclose(out);
}

void
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
    FILE *in;
    char *prog, *opath;

    if ((prog = strrchr(argv[0], '/')) != 0)
      prog++;
    else
      prog = argv[0];

    opath = 0;
    in = stdin;

    argc--;
    argv++;

    while (argc > 0) {
        if (argv[0][0] == '-' && argv[0][1] == 'o') {
            argc--;
            argv++;
            opath = argv[0];
        } else {
            if (in != stdin)
              fclose(in);
            if ((in = fopen(argv[0], "rb")) == 0)
              fprintf(stderr, "%s: unable to open ctype file %s\n",
                      prog, argv[0]);
            else {
                read_cdata(in);
                fclose(in);
                in = 0;
            }
        }
        argc--;
        argv++;
    }

    if (opath == 0)
      opath = ".";
    write_cdata(opath);

    exit(0);
}
