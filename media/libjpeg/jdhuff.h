














#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_make_d_derived_tbl	jMkDDerived
#define jpeg_fill_bit_buffer	jFilBitBuf
#define jpeg_huff_decode	jHufDecode
#endif 




#define HUFF_LOOKAHEAD	8	/* # of bits of lookahead */

typedef struct {
  
  INT32 maxcode[18];		
  
  INT32 valoffset[18];		
  




  
  JHUFF_TBL *pub;

  









  int lookup[1<<HUFF_LOOKAHEAD];
} d_derived_tbl;


EXTERN(void) jpeg_make_d_derived_tbl
	JPP((j_decompress_ptr cinfo, boolean isDC, int tblno,
	     d_derived_tbl ** pdtbl));




















#if __WORDSIZE == 64 || defined(_WIN64)

typedef size_t bit_buf_type;	
#define BIT_BUF_SIZE  64		/* size of buffer in bits */

#else

typedef INT32 bit_buf_type;	
#define BIT_BUF_SIZE  32		/* size of buffer in bits */

#endif








typedef struct {		
  bit_buf_type get_buffer;	
  int bits_left;		
} bitread_perm_state;

typedef struct {		
  
  
  const JOCTET * next_input_byte; 
  size_t bytes_in_buffer;	
  


  bit_buf_type get_buffer;	
  int bits_left;		
  
  j_decompress_ptr cinfo;	
} bitread_working_state;


#define BITREAD_STATE_VARS  \
	register bit_buf_type get_buffer;  \
	register int bits_left;  \
	bitread_working_state br_state

#define BITREAD_LOAD_STATE(cinfop,permstate)  \
	br_state.cinfo = cinfop; \
	br_state.next_input_byte = cinfop->src->next_input_byte; \
	br_state.bytes_in_buffer = cinfop->src->bytes_in_buffer; \
	get_buffer = permstate.get_buffer; \
	bits_left = permstate.bits_left;

#define BITREAD_SAVE_STATE(cinfop,permstate)  \
	cinfop->src->next_input_byte = br_state.next_input_byte; \
	cinfop->src->bytes_in_buffer = br_state.bytes_in_buffer; \
	permstate.get_buffer = get_buffer; \
	permstate.bits_left = bits_left



















#define CHECK_BIT_BUFFER(state,nbits,action) \
	{ if (bits_left < (nbits)) {  \
	    if (! jpeg_fill_bit_buffer(&(state),get_buffer,bits_left,nbits))  \
	      { action; }  \
	    get_buffer = (state).get_buffer; bits_left = (state).bits_left; } }

#define GET_BITS(nbits) \
	(((int) (get_buffer >> (bits_left -= (nbits)))) & ((1<<(nbits))-1))

#define PEEK_BITS(nbits) \
	(((int) (get_buffer >> (bits_left -  (nbits)))) & ((1<<(nbits))-1))

#define DROP_BITS(nbits) \
	(bits_left -= (nbits))


EXTERN(boolean) jpeg_fill_bit_buffer
	JPP((bitread_working_state * state, register bit_buf_type get_buffer,
	     register int bits_left, int nbits));



















#define HUFF_DECODE(result,state,htbl,failaction,slowlabel) \
{ register int nb, look; \
  if (bits_left < HUFF_LOOKAHEAD) { \
    if (! jpeg_fill_bit_buffer(&state,get_buffer,bits_left, 0)) {failaction;} \
    get_buffer = state.get_buffer; bits_left = state.bits_left; \
    if (bits_left < HUFF_LOOKAHEAD) { \
      nb = 1; goto slowlabel; \
    } \
  } \
  look = PEEK_BITS(HUFF_LOOKAHEAD); \
  if ((nb = (htbl->lookup[look] >> HUFF_LOOKAHEAD)) <= HUFF_LOOKAHEAD) { \
    DROP_BITS(nb); \
    result = htbl->lookup[look] & ((1 << HUFF_LOOKAHEAD) - 1); \
  } else { \
slowlabel: \
    if ((result=jpeg_huff_decode(&state,get_buffer,bits_left,htbl,nb)) < 0) \
	{ failaction; } \
    get_buffer = state.get_buffer; bits_left = state.bits_left; \
  } \
}

#define HUFF_DECODE_FAST(s,nb,htbl) \
  FILL_BIT_BUFFER_FAST; \
  s = PEEK_BITS(HUFF_LOOKAHEAD); \
  s = htbl->lookup[s]; \
  nb = s >> HUFF_LOOKAHEAD; \
  /* Pre-execute the common case of nb <= HUFF_LOOKAHEAD */ \
  DROP_BITS(nb); \
  s = s & ((1 << HUFF_LOOKAHEAD) - 1); \
  if (nb > HUFF_LOOKAHEAD) { \
    /* Equivalent of jpeg_huff_decode() */ \
    /* Don't use GET_BITS() here because we don't want to modify bits_left */ \
    s = (get_buffer >> bits_left) & ((1 << (nb)) - 1); \
    while (s > htbl->maxcode[nb]) { \
      s <<= 1; \
      s |= GET_BITS(1); \
      nb++; \
    } \
    s = htbl->pub->huffval[ (int) (s + htbl->valoffset[nb]) & 0xFF ]; \
  }


EXTERN(int) jpeg_huff_decode
	JPP((bitread_working_state * state, register bit_buf_type get_buffer,
	     register int bits_left, d_derived_tbl * htbl, int min_bits));
