








































typedef unsigned char BYTE;
typedef unsigned int  HALF;

#define DES_ENCRYPT 0
#define DES_DECRYPT 1


static HALF C0, D0;

static HALF L0, R0;


static BYTE KS [8] [16];











static const BYTE PC2[64] = {
    14, 17, 11, 24,  1,  5,  0,  0,	
     3, 28, 15,  6, 21, 10,  0,  0,	
    23, 19, 12,  4, 26,  8,  0,  0,	
    16,  7, 27, 20, 13,  2,  0,  0,	

    41, 52, 31, 37, 47, 55,  0,  0,	
    30, 40, 51, 45, 33, 48,  0,  0,	
    44, 49, 39, 56, 34, 53,  0,  0,	
    46, 42, 50, 36, 29, 32,  0,  0	
};







static       signed char PC2a[64] = {

    14, 11, 17,  4, 27, 23, -1, -1,	
    25,  0, 13, 22,  7, 18, -1, -1,	
     5,  9, 16, 24,  2, 20, -1, -1,	
    12, 21,  1,  8, 15, 26, -1, -1,	

    15,  4, 25, 19,  9,  1, -1, -1,	
    26, 16,  5, 11, 23,  8, -1, -1,	
    12,  7, 17,  0, 22,  3, -1, -1,	
    10, 14,  6, 20, 27, 24, -1, -1 	
};






static const signed char PC2b[64] = {

    14, 11, 17,  4, 27, 23, -1, -1,	
     5,  9, 16, 24,  2, 20, -1, -1,	
    25,  0, 13, 22,  7, 18, -1, -1,	
    12, 21,  1,  8, 15, 26, -1, -1,	

    26, 16,  5, 11, 23,  8, -1, -1,	
    10, 14,  6, 20, 27, 24, -1, -1,	
    15,  4, 25, 19,  9,  1, -1, -1,	
    12,  7, 17,  0, 22,  3, -1, -1 	
};









static BYTE NDX[48] = {

    27, 26, 25, 24, 23, 22,	
    18, 17, 16, 15, 14, 13,	
     9,  8,  7,  2,  1,  0,	
     5,  4, 21, 20, 12, 11,	

    27, 26, 25, 24, 23, 22,	
    20, 19, 17, 16, 15, 14,	
    12, 11, 10,  9,  8,  7,	
     6,  5,  4,  3,  1,  0	
};













void
make_pc2a( void )
{

    int i, j;

    for ( i = 0; i < 64; ++i ) {
	j = PC2[i];
	if (j == 0)
	    j = -1;
	else if ( j < 29 )
	    j = 28 - j ;
	else
	    j = 56 - j;
	PC2a[i] = j;
    }
    for ( i = 0; i < 64; i += 8 ) {
	printf("%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,\n",
		PC2a[i+0],PC2a[i+1],PC2a[i+2],PC2a[i+3],
		PC2a[i+4],PC2a[i+5],PC2a[i+6],PC2a[i+7] );
    }
}

HALF PC2cd0[64];

HALF PC_2H[8][64];

void
mktable( )
{
    int i;
    int table;
    const BYTE * ndx   = NDX;
    HALF         mask;

    mask  = 0x80000000;
    for (i = 0; i < 32; ++i, mask >>= 1) {
	int bit = PC2b[i];
	if (bit < 0)
	    continue;
	PC2cd0[bit + 32] = mask;
    }

    mask  = 0x80000000;
    for (i = 32; i < 64; ++i, mask >>= 1) {
	int bit = PC2b[i];
	if (bit < 0)
	    continue;
	PC2cd0[bit] = mask;
    }

#if DEBUG
    for (i = 0; i < 64; ++i) {
    	printf("0x%08x,\n", PC2cd0[i]);
    }
#endif
    for (i = 0; i < 24; ++i) {
    	NDX[i] += 32;	
    }

    for (table = 0; table < 8; ++table) {
	HALF bitvals[6];
    	for (i = 0; i < 6; ++i) {
	    bitvals[5-i] = PC2cd0[*ndx++];
	}
	for (i = 0; i < 64; ++i) {
	    int  j;
	    int  k     = 0;
	    HALF value = 0;

	    for (j = i; j; j >>= 1, ++k) {
	    	if (j & 1) {
		    value |= bitvals[k];
		}
	    }
	    PC_2H[table][i] = value;
	}
	printf("/* table %d */ {\n", table );
	for (i = 0; i < 64; i += 4) {
	    printf("    0x%08x, 0x%08x, 0x%08x, 0x%08x, \n",
		    PC_2H[table][i],   PC_2H[table][i+1],
		    PC_2H[table][i+2], PC_2H[table][i+3]);
	}
	printf("  },\n");
    }
}


int
main(void)
{

   mktable();
   return 0;
}
