










































#include "secasn1.h"

typedef enum {
    beforeHeader,
    duringContents,
    duringGroup,
    duringSequence,
    afterContents,
    afterImplicit,
    afterInline,
    afterPointer,
    afterChoice,
    notInUse
} sec_asn1e_parse_place;

typedef enum {
    allDone,
    encodeError,
    keepGoing,
    needBytes
} sec_asn1e_parse_status;

typedef enum {
    hdr_normal      = 0,  
    hdr_any         = 1,  
    hdr_decoder     = 2,  
    hdr_optional    = 3,  
    hdr_placeholder = 4   
} sec_asn1e_hdr_encoding;

typedef struct sec_asn1e_state_struct {
    SEC_ASN1EncoderContext *top;
    const SEC_ASN1Template *theTemplate;
    void *src;

    struct sec_asn1e_state_struct *parent;	
    struct sec_asn1e_state_struct *child;	

    sec_asn1e_parse_place place;	

    


    unsigned char tag_modifiers;
    unsigned char tag_number;
    unsigned long underlying_kind;

    int depth;

    PRBool isExplicit,		
	   indefinite,		
	   is_string,		
	   may_stream,		
	   optional,		
	   disallowStreaming;		
} sec_asn1e_state;







struct sec_EncoderContext_struct {
    PRArenaPool *our_pool;		

    sec_asn1e_state *current;
    sec_asn1e_parse_status status;

    PRBool streaming;
    PRBool from_buf;

    SEC_ASN1NotifyProc notify_proc;	
    void *notify_arg;			
    PRBool during_notify;		

    SEC_ASN1WriteProc output_proc;	
    void *output_arg;			
};


static sec_asn1e_state *
sec_asn1e_push_state (SEC_ASN1EncoderContext *cx,
		      const SEC_ASN1Template *theTemplate,
		      const void *src, PRBool new_depth)
{
    sec_asn1e_state *state, *new_state;

    state = cx->current;

    new_state = (sec_asn1e_state*)PORT_ArenaZAlloc (cx->our_pool, 
						    sizeof(*new_state));
    if (new_state == NULL) {
	cx->status = encodeError;
	return NULL;
    }

    new_state->top = cx;
    new_state->parent = state;
    new_state->theTemplate = theTemplate;
    new_state->place = notInUse;
    if (src != NULL)
	new_state->src = (char *)src + theTemplate->offset;

    if (state != NULL) {
	new_state->depth = state->depth;
	if (new_depth)
	    new_state->depth++;
	state->child = new_state;
    }

    cx->current = new_state;
    return new_state;
}


static void
sec_asn1e_scrub_state (sec_asn1e_state *state)
{
    



    state->place = beforeHeader;
    state->indefinite = PR_FALSE;
}


static void
sec_asn1e_notify_before (SEC_ASN1EncoderContext *cx, void *src, int depth)
{
    if (cx->notify_proc == NULL)
	return;

    cx->during_notify = PR_TRUE;
    (* cx->notify_proc) (cx->notify_arg, PR_TRUE, src, depth);
    cx->during_notify = PR_FALSE;
}


static void
sec_asn1e_notify_after (SEC_ASN1EncoderContext *cx, void *src, int depth)
{
    if (cx->notify_proc == NULL)
	return;

    cx->during_notify = PR_TRUE;
    (* cx->notify_proc) (cx->notify_arg, PR_FALSE, src, depth);
    cx->during_notify = PR_FALSE;
}


static sec_asn1e_state *
sec_asn1e_init_state_based_on_template (sec_asn1e_state *state)
{
    PRBool isExplicit, is_string, may_stream, optional, universal; 
    PRBool disallowStreaming;
    unsigned char tag_modifiers;
    unsigned long encode_kind, under_kind;
    unsigned long tag_number;
    PRBool isInline = PR_FALSE;


    encode_kind = state->theTemplate->kind;

    universal = ((encode_kind & SEC_ASN1_CLASS_MASK) == SEC_ASN1_UNIVERSAL)
		? PR_TRUE : PR_FALSE;

    isExplicit = (encode_kind & SEC_ASN1_EXPLICIT) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_EXPLICIT;

    optional = (encode_kind & SEC_ASN1_OPTIONAL) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_OPTIONAL;

    PORT_Assert (!(isExplicit && universal));	

    may_stream = (encode_kind & SEC_ASN1_MAY_STREAM) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_MAY_STREAM;

    disallowStreaming = (encode_kind & SEC_ASN1_NO_STREAM) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_NO_STREAM;

    
    encode_kind &= ~SEC_ASN1_DYNAMIC;

    if( encode_kind & SEC_ASN1_CHOICE ) {
      under_kind = SEC_ASN1_CHOICE;
    } else if ((encode_kind & (SEC_ASN1_POINTER | SEC_ASN1_INLINE)) || 
        (!universal && !isExplicit)) {
	const SEC_ASN1Template *subt;
	void *src = NULL;

	PORT_Assert ((encode_kind & (SEC_ASN1_ANY | SEC_ASN1_SKIP)) == 0);

	sec_asn1e_scrub_state (state);

	if (encode_kind & SEC_ASN1_POINTER) {
	    src = *(void **)state->src;
	    state->place = afterPointer;

	    if (src == NULL) {
		




		if (optional)
		    return state;

		



	    }
	} else {
	    src = state->src;
	    if (encode_kind & SEC_ASN1_INLINE) {
		
		
		state->place = afterInline;
		isInline = PR_TRUE;
	    } else {
		




		state->tag_modifiers = (unsigned char)
		    (encode_kind & (SEC_ASN1_TAG_MASK & ~SEC_ASN1_TAGNUM_MASK));
		state->tag_number = (unsigned char)
		    (encode_kind & SEC_ASN1_TAGNUM_MASK);
		
		state->place = afterImplicit;
		state->optional = optional;
	    }
	}

	subt = SEC_ASN1GetSubtemplate (state->theTemplate, state->src, PR_TRUE);
	if (isInline && optional) {
	    

	    if (PR_FALSE != SEC_ASN1IsTemplateSimple(subt)) {
		

		SECItem* target = (SECItem*)state->src;
		if (!target || !target->data || !target->len) {
		    
		    return state;
		}
	    } else {
		PORT_Assert(0); 

	    }
	}
	state = sec_asn1e_push_state (state->top, subt, src, PR_FALSE);
	if (state == NULL)
	    return state;

	if (universal) {
	    



	    return sec_asn1e_init_state_based_on_template (state);
	}

	











	under_kind = state->theTemplate->kind;
	if ((under_kind & SEC_ASN1_MAY_STREAM) && !disallowStreaming) {
	    may_stream = PR_TRUE;
	}
	under_kind &= ~(SEC_ASN1_MAY_STREAM | SEC_ASN1_DYNAMIC);
    } else {
	under_kind = encode_kind;
    }

    






#define UNEXPECTED_FLAGS \
 (SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_SKIP | SEC_ASN1_INNER | \
  SEC_ASN1_DYNAMIC | SEC_ASN1_MAY_STREAM | SEC_ASN1_INLINE | SEC_ASN1_POINTER)

    PORT_Assert ((under_kind & UNEXPECTED_FLAGS) == 0);
    under_kind &= ~UNEXPECTED_FLAGS;
#undef UNEXPECTED_FLAGS

    if (encode_kind & SEC_ASN1_ANY) {
	PORT_Assert (encode_kind == under_kind);
	tag_modifiers = 0;
	tag_number = 0;
	is_string = PR_TRUE;
    } else {
	tag_modifiers = (unsigned char)
		(encode_kind & (SEC_ASN1_TAG_MASK & ~SEC_ASN1_TAGNUM_MASK));
	





	tag_number = encode_kind & SEC_ASN1_TAGNUM_MASK;

	is_string = PR_FALSE;
	switch (under_kind & SEC_ASN1_TAGNUM_MASK) {
	  case SEC_ASN1_SET:
	    



	    PORT_Assert ((under_kind & SEC_ASN1_GROUP) != 0);
	    
	  case SEC_ASN1_SEQUENCE:
	    tag_modifiers |= SEC_ASN1_CONSTRUCTED;
	    break;
	  case SEC_ASN1_BIT_STRING:
	  case SEC_ASN1_BMP_STRING: 
	  case SEC_ASN1_GENERALIZED_TIME:
	  case SEC_ASN1_IA5_STRING:
	  case SEC_ASN1_OCTET_STRING:
	  case SEC_ASN1_PRINTABLE_STRING:
	  case SEC_ASN1_T61_STRING:
	  case SEC_ASN1_UNIVERSAL_STRING: 
	  case SEC_ASN1_UTC_TIME:
	  case SEC_ASN1_UTF8_STRING:
	  case SEC_ASN1_VISIBLE_STRING: 
	    



	    is_string = PR_TRUE;
	    break;
	}
    }

    state->tag_modifiers = tag_modifiers;
    state->tag_number = (unsigned char)tag_number;
    state->underlying_kind = under_kind;
    state->isExplicit = isExplicit;
    state->may_stream = may_stream;
    state->is_string = is_string;
    state->optional = optional;
    state->disallowStreaming = disallowStreaming;

    sec_asn1e_scrub_state (state);

    return state;
}


static void
sec_asn1e_write_part (sec_asn1e_state *state,
		      const char *buf, unsigned long len,
		      SEC_ASN1EncodingPart part)
{
    SEC_ASN1EncoderContext *cx;

    cx = state->top;
    (* cx->output_proc) (cx->output_arg, buf, len, state->depth, part);
}







static void
sec_asn1e_write_identifier_bytes (sec_asn1e_state *state, unsigned char value)
{
    char byte;

    byte = (char) value;
    sec_asn1e_write_part (state, &byte, 1, SEC_ASN1_Identifier);
}

int
SEC_ASN1EncodeLength(unsigned char *buf,int value) {
    int lenlen;

    lenlen = SEC_ASN1LengthLength (value);
    if (lenlen == 1) {
	buf[0] = value;
    } else {
	int i;

	i = lenlen - 1;
	buf[0] = 0x80 | i;
	while (i) {
	    buf[i--] = value;
	    value >>= 8;
	}
        PORT_Assert (value == 0);
    }
    return lenlen;
}

static void
sec_asn1e_write_length_bytes (sec_asn1e_state *state, unsigned long value,
			      PRBool indefinite)
{
    int lenlen;
    unsigned char buf[sizeof(unsigned long) + 1];

    if (indefinite) {
	PORT_Assert (value == 0);
	buf[0] = 0x80;
	lenlen = 1;
    } else {
	lenlen = SEC_ASN1EncodeLength(buf,value);
    }

    sec_asn1e_write_part (state, (char *) buf, lenlen, SEC_ASN1_Length);
}


static void
sec_asn1e_write_contents_bytes (sec_asn1e_state *state,
				const char *buf, unsigned long len)
{
    sec_asn1e_write_part (state, buf, len, SEC_ASN1_Contents);
}


static void
sec_asn1e_write_end_of_contents_bytes (sec_asn1e_state *state)
{
    const char eoc[2] = {0, 0};

    sec_asn1e_write_part (state, eoc, 2, SEC_ASN1_EndOfContents);
}

static int
sec_asn1e_which_choice
(
  void *src,
  const SEC_ASN1Template *theTemplate
)
{
  int rv;
  unsigned int which = *(unsigned int *)src;

  for( rv = 1, theTemplate++; theTemplate->kind != 0; rv++, theTemplate++ ) {
    if( which == theTemplate->size ) {
      return rv;
    }
  }

  return 0;
}

static unsigned long
sec_asn1e_contents_length (const SEC_ASN1Template *theTemplate, void *src,
			   PRBool disallowStreaming, PRBool insideIndefinite,
			   sec_asn1e_hdr_encoding *pHdrException)
{
    unsigned long encode_kind, underlying_kind;
    PRBool isExplicit, optional, universal, may_stream;
    unsigned long len;

    














    encode_kind = theTemplate->kind;

    universal = ((encode_kind & SEC_ASN1_CLASS_MASK) == SEC_ASN1_UNIVERSAL)
		? PR_TRUE : PR_FALSE;

    isExplicit = (encode_kind & SEC_ASN1_EXPLICIT) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_EXPLICIT;

    optional = (encode_kind & SEC_ASN1_OPTIONAL) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_OPTIONAL;

    PORT_Assert (!(isExplicit && universal));	

    may_stream = (encode_kind & SEC_ASN1_MAY_STREAM) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_MAY_STREAM;

    
    encode_kind &= ~SEC_ASN1_DYNAMIC;

    if (encode_kind & SEC_ASN1_NO_STREAM) {
	disallowStreaming = PR_TRUE;
    }
    encode_kind &= ~SEC_ASN1_NO_STREAM;

    if (encode_kind & SEC_ASN1_CHOICE) {
	void *src2;
	int indx = sec_asn1e_which_choice(src, theTemplate);
	if (0 == indx) {
	    
	    
	    return 0;
	}

        src2 = (void *)
	        ((char *)src - theTemplate->offset + theTemplate[indx].offset);

        return sec_asn1e_contents_length(&theTemplate[indx], src2, 
					 disallowStreaming, insideIndefinite,
					 pHdrException);
    }

    if ((encode_kind & (SEC_ASN1_POINTER | SEC_ASN1_INLINE)) || !universal) {
	
	theTemplate = SEC_ASN1GetSubtemplate (theTemplate, src, PR_TRUE);
	if (encode_kind & SEC_ASN1_POINTER) {
	    src = *(void **)src;
	    if (src == NULL) {
		*pHdrException = optional ? hdr_optional : hdr_normal;
		return 0;
	    }
	} else if (encode_kind & SEC_ASN1_INLINE) {
	    
	    if (optional) {
		if (PR_FALSE != SEC_ASN1IsTemplateSimple(theTemplate)) {
		    

		    SECItem* target = (SECItem*)src;
		    if (!target || !target->data || !target->len) {
			
			*pHdrException = hdr_optional;
			return 0;
		    }
		} else {
		    PORT_Assert(0); 

		}
	    }
	}

	src = (char *)src + theTemplate->offset;

	
	len = sec_asn1e_contents_length (theTemplate, src, disallowStreaming, 
	                                 insideIndefinite, pHdrException);
	if (len == 0 && optional) {
	    *pHdrException = hdr_optional;
	} else if (isExplicit) {
	    if (*pHdrException == hdr_any) {
		


		*pHdrException = hdr_normal;
	    } else if (*pHdrException == hdr_normal) {
		




		len += 1 + SEC_ASN1LengthLength (len);
	    }
	}
	return len;
    }
    underlying_kind = encode_kind;

    
    if (underlying_kind & SEC_ASN1_SAVE) {
	
	PORT_Assert (underlying_kind == SEC_ASN1_SAVE);
	*pHdrException = hdr_decoder;
	return 0;
    }

#define UNEXPECTED_FLAGS \
 (SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_INLINE | SEC_ASN1_POINTER |\
  SEC_ASN1_DYNAMIC | SEC_ASN1_MAY_STREAM | SEC_ASN1_SAVE | SEC_ASN1_SKIP)

    
    PORT_Assert ((underlying_kind & UNEXPECTED_FLAGS) == 0);
    underlying_kind &= ~UNEXPECTED_FLAGS;
#undef UNEXPECTED_FLAGS

    if (underlying_kind & SEC_ASN1_CHOICE) {
	void *src2;
	int indx = sec_asn1e_which_choice(src, theTemplate);
	if (0 == indx) {
	    
	    
	    return 0;
	}

        src2 = (void *)
		((char *)src - theTemplate->offset + theTemplate[indx].offset);
        len = sec_asn1e_contents_length(&theTemplate[indx], src2, 
	                                disallowStreaming, insideIndefinite, 
					pHdrException);
    } else {
      switch (underlying_kind) {
      case SEC_ASN1_SEQUENCE_OF:
      case SEC_ASN1_SET_OF:
	{
	    const SEC_ASN1Template *tmpt;
	    void *sub_src;
	    unsigned long sub_len;
	    void **group;

	    len = 0;

	    group = *(void ***)src;
	    if (group == NULL)
		break;

	    tmpt = SEC_ASN1GetSubtemplate (theTemplate, src, PR_TRUE);

	    for (; *group != NULL; group++) {
		sub_src = (char *)(*group) + tmpt->offset;
		sub_len = sec_asn1e_contents_length (tmpt, sub_src, 
		                                     disallowStreaming,
						     insideIndefinite,
                                                     pHdrException);
		len += sub_len;
		



		if (*pHdrException == hdr_normal)
		    len += 1 + SEC_ASN1LengthLength (sub_len);
	    }
	}
	break;

      case SEC_ASN1_SEQUENCE:
      case SEC_ASN1_SET:
	{
	    const SEC_ASN1Template *tmpt;
	    void *sub_src;
	    unsigned long sub_len;

	    len = 0;
	    for (tmpt = theTemplate + 1; tmpt->kind; tmpt++) {
		sub_src = (char *)src + tmpt->offset;
		sub_len = sec_asn1e_contents_length (tmpt, sub_src, 
		                                     disallowStreaming,
						     insideIndefinite,
                                                     pHdrException);
		len += sub_len;
		



		if (*pHdrException == hdr_normal)
		    len += 1 + SEC_ASN1LengthLength (sub_len);
	    }
	}
	break;

      case SEC_ASN1_BIT_STRING:
	
	len = (((SECItem *)src)->len + 7) >> 3;
	
	if (len)
	    len++;
	break;

      case SEC_ASN1_INTEGER:
	



	{
	    unsigned char *buf = ((SECItem *)src)->data;
	    SECItemType integerType = ((SECItem *)src)->type;
	    len = ((SECItem *)src)->len;
	    while (len > 0) {
		if (*buf != 0) {
		    if (*buf & 0x80 && integerType == siUnsignedInteger) {
			len++; 
		    }
		    break; 
		}
		if (len == 1) {
		    break; 
		}
		if (buf[1] & 0x80) {
		    break; 
		} 
		
		buf++;
		len--;
	    }
	}
	break;

      default:
	len = ((SECItem *)src)->len;
	break;
      }  

#ifndef WHAT_PROBLEM_DOES_THIS_SOLVE
      
      if (!len && insideIndefinite && may_stream && !disallowStreaming) {
	  len = 1;
      }
#endif
    }    

    if (len == 0 && optional)
	*pHdrException = hdr_optional;
    else if (underlying_kind == SEC_ASN1_ANY)
	*pHdrException = hdr_any;
    else 
	*pHdrException = hdr_normal;

    return len;
}


static void
sec_asn1e_write_header (sec_asn1e_state *state)
{
    unsigned long contents_length;
    unsigned char tag_number, tag_modifiers;
    sec_asn1e_hdr_encoding hdrException = hdr_normal;
    PRBool indefinite = PR_FALSE;

    PORT_Assert (state->place == beforeHeader);

    tag_number = state->tag_number;
    tag_modifiers = state->tag_modifiers;

    if (state->underlying_kind == SEC_ASN1_ANY) {
	state->place = duringContents;
	return;
    }

    if (state->underlying_kind & SEC_ASN1_CHOICE) {
	int indx = sec_asn1e_which_choice(state->src, state->theTemplate);
	if( 0 == indx ) {
	    
	    state->top->status = encodeError;
	    return;
	}
	state->place = afterChoice;
	state = sec_asn1e_push_state(state->top, &state->theTemplate[indx],
			       (char *)state->src - state->theTemplate->offset, 
			       PR_TRUE);
	if (state) {
	    


	    sec_asn1e_notify_before (state->top, state->src, state->depth);
	    state = sec_asn1e_init_state_based_on_template (state);
	}
	return;
    }

    


   indefinite = (PRBool) 
	(state->top->streaming && state->may_stream && 
	 (state->top->from_buf || !state->is_string));

    









    contents_length = sec_asn1e_contents_length (state->theTemplate,
						 state->src, 
                                                 state->disallowStreaming,
						 indefinite,
                                                 &hdrException);
    





    if (hdrException != hdr_normal || 
	(contents_length == 0 && state->optional)) {
	state->place = afterContents;
	if (state->top->streaming && 
	    state->may_stream && 
	    state->top->from_buf) {
	    




	    state->top->status = needBytes;
	}
	return;
    }

    if (indefinite) {
	





	state->indefinite = PR_TRUE;
	PORT_Assert ((tag_number == SEC_ASN1_SET)
		     || (tag_number == SEC_ASN1_SEQUENCE)
		     || ((tag_modifiers & SEC_ASN1_CLASS_MASK) != 0)
		     || state->is_string);
	tag_modifiers |= SEC_ASN1_CONSTRUCTED;
	contents_length = 0;
    }

    sec_asn1e_write_identifier_bytes (state, 
                                (unsigned char)(tag_number | tag_modifiers));
    sec_asn1e_write_length_bytes (state, contents_length, state->indefinite);

    if (contents_length == 0 && !state->indefinite) {
	


	state->place = afterContents;
	return;
    }

    



    if (state->isExplicit) {
	const SEC_ASN1Template *subt =
	      SEC_ASN1GetSubtemplate(state->theTemplate, state->src, PR_TRUE);
	state->place = afterContents;
	state = sec_asn1e_push_state (state->top, subt, state->src, PR_TRUE);
	if (state != NULL)
	    state = sec_asn1e_init_state_based_on_template (state);
	return;
    }

    switch (state->underlying_kind) {
      case SEC_ASN1_SET_OF:
      case SEC_ASN1_SEQUENCE_OF:
	


	{
	    void **group;
	    const SEC_ASN1Template *subt;

	    group = *(void ***)state->src;
	    if (group == NULL || *group == NULL) {
		


		state->place = afterContents;
		return;
	    }
	    state->place = duringGroup;
	    subt = SEC_ASN1GetSubtemplate (state->theTemplate, state->src,
					   PR_TRUE);
	    state = sec_asn1e_push_state (state->top, subt, *group, PR_TRUE);
	    if (state != NULL)
		state = sec_asn1e_init_state_based_on_template (state);
	}
	break;

      case SEC_ASN1_SEQUENCE:
      case SEC_ASN1_SET:
	


	state->place = duringSequence;
	state = sec_asn1e_push_state (state->top, state->theTemplate + 1,
				      state->src, PR_TRUE);
	if (state != NULL) {
	    


	    sec_asn1e_notify_before (state->top, state->src, state->depth);
	    state = sec_asn1e_init_state_based_on_template (state);
	}
	break;

      default:
	



	state->place = duringContents;
	break;
    }
}


static void
sec_asn1e_write_contents_from_buf (sec_asn1e_state *state,
			  const char *buf, unsigned long len)
{
    PORT_Assert (state->place == duringContents);
    PORT_Assert (state->top->from_buf);
    PORT_Assert (state->may_stream && !state->disallowStreaming);

    




    if (buf == NULL || len == 0) {
	state->top->status = needBytes;
	return;
    }
    







    PORT_Assert (state->is_string);		
    if (state->underlying_kind != SEC_ASN1_ANY) {
	unsigned char identifier;

	





	identifier = (unsigned char)
	                    (state->underlying_kind & SEC_ASN1_TAG_MASK);
	



	PORT_Assert ((identifier & SEC_ASN1_TAGNUM_MASK) == identifier);
	


	sec_asn1e_write_identifier_bytes (state, identifier);
	if (state->underlying_kind == SEC_ASN1_BIT_STRING) {
	    char byte;
	    











	    sec_asn1e_write_length_bytes (state, len + 1, PR_FALSE);
	    byte = 0;
	    sec_asn1e_write_contents_bytes (state, &byte, 1);
	} else {
	    sec_asn1e_write_length_bytes (state, len, PR_FALSE);
	}
    }
    sec_asn1e_write_contents_bytes (state, buf, len);
    state->top->status = needBytes;
}

static void
sec_asn1e_write_contents (sec_asn1e_state *state)
{
    unsigned long len = 0;

    PORT_Assert (state->place == duringContents);

    switch (state->underlying_kind) {
      case SEC_ASN1_SET:
      case SEC_ASN1_SEQUENCE:
	PORT_Assert (0);
	break;

      case SEC_ASN1_BIT_STRING:
	{
	    SECItem *item;
	    char rem;

	    item = (SECItem *)state->src;
	    len = (item->len + 7) >> 3;
	    rem = (unsigned char)((len << 3) - item->len); 
	    sec_asn1e_write_contents_bytes (state, &rem, 1);
	    sec_asn1e_write_contents_bytes (state, (char *) item->data, len);
	}
	break;

      case SEC_ASN1_BMP_STRING:
	
	if ((((SECItem *)state->src)->len) % 2) {
	    SEC_ASN1EncoderContext *cx;

	    cx = state->top;
	    cx->status = encodeError;
	    break;
	}
	
	goto process_string;

      case SEC_ASN1_UNIVERSAL_STRING:
	
	if ((((SECItem *)state->src)->len) % 4) {
	    SEC_ASN1EncoderContext *cx;

	    cx = state->top;
	    cx->status = encodeError;
	    break;
	}
	
	goto process_string;

      case SEC_ASN1_INTEGER:
       


	{
	    unsigned int blen;
	    unsigned char *buf;
	    SECItemType integerType;
	    blen = ((SECItem *)state->src)->len;
	    buf = ((SECItem *)state->src)->data;
	    integerType = ((SECItem *)state->src)->type;
	    while (blen > 0) {
		if (*buf & 0x80 && integerType == siUnsignedInteger) {
		    char zero = 0; 
		    sec_asn1e_write_contents_bytes(state, &zero, 1);
		    
		    sec_asn1e_write_contents_bytes(state, 
						   (char *)buf, blen); 
		    break;
		} 
		





		if (*buf != 0 || 
		     blen == 1 || 
		     (buf[1] & 0x80 && integerType != siUnsignedInteger) ) 
		{
		    sec_asn1e_write_contents_bytes(state, 
						   (char *)buf, blen); 
		    break;
		}
		
		buf++;
		blen--;
	    }
	}
	
	break;
			
process_string:			
      default:
	{
	    SECItem *item;

	    item = (SECItem *)state->src;
	    sec_asn1e_write_contents_bytes (state, (char *) item->data,
					    item->len);
	}
	break;
    }
    state->place = afterContents;
}




static void
sec_asn1e_next_in_group (sec_asn1e_state *state)
{
    sec_asn1e_state *child;
    void **group;
    void *member;

    PORT_Assert (state->place == duringGroup);
    PORT_Assert (state->child != NULL);

    child = state->child;

    group = *(void ***)state->src;

    


    member = (char *)(state->child->src) - child->theTemplate->offset;
    while (*group != member)
	group++;

    


    group++;
    if (*group == NULL) {
	


	child->place = notInUse;
	state->place = afterContents;
	return;
    }
    child->src = (char *)(*group) + child->theTemplate->offset;

    


    sec_asn1e_scrub_state (child);
    state->top->current = child;
}






static void
sec_asn1e_next_in_sequence (sec_asn1e_state *state)
{
    sec_asn1e_state *child;

    PORT_Assert (state->place == duringSequence);
    PORT_Assert (state->child != NULL);

    child = state->child;

    


    sec_asn1e_notify_after (state->top, child->src, child->depth);

    


    child->theTemplate++;
    if (child->theTemplate->kind == 0) {
	


	child->place = notInUse;
	state->place = afterContents;
	return;
    }

    



    child->src = (char *)state->src + child->theTemplate->offset;

    


    sec_asn1e_notify_before (state->top, child->src, child->depth);

    state->top->current = child;
    (void) sec_asn1e_init_state_based_on_template (child);
}


static void
sec_asn1e_after_contents (sec_asn1e_state *state)
{
    PORT_Assert (state->place == afterContents);

    if (state->indefinite)
	sec_asn1e_write_end_of_contents_bytes (state);

    



    state->top->current = state->parent;
}













SECStatus
SEC_ASN1EncoderUpdate (SEC_ASN1EncoderContext *cx,
		       const char *buf, unsigned long len)
{
    sec_asn1e_state *state;

    if (cx->status == needBytes) {
	cx->status = keepGoing;
    }

    while (cx->status == keepGoing) {
	state = cx->current;
	switch (state->place) {
	  case beforeHeader:
	    sec_asn1e_write_header (state);
	    break;
	  case duringContents:
	    if (cx->from_buf)
		sec_asn1e_write_contents_from_buf (state, buf, len);
	    else
		sec_asn1e_write_contents (state);
	    break;
	  case duringGroup:
	    sec_asn1e_next_in_group (state);
	    break;
	  case duringSequence:
	    sec_asn1e_next_in_sequence (state);
	    break;
	  case afterContents:
	    sec_asn1e_after_contents (state);
	    break;
	  case afterImplicit:
	  case afterInline:
	  case afterPointer:
	  case afterChoice:
	    



	    PORT_Assert (!state->indefinite);
	    state->place = afterContents;
	    break;
	  case notInUse:
	  default:
	    
	    PORT_Assert (0);
	    cx->status = encodeError;
	    break;
	}

	if (cx->status == encodeError)
	    break;

	
	state = cx->current;

	
	if (state == NULL) {
	    cx->status = allDone;
	    break;
	}
    }

    if (cx->status == encodeError) {
	return SECFailure;
    }

    return SECSuccess;
}


void
SEC_ASN1EncoderFinish (SEC_ASN1EncoderContext *cx)
{
    



    PORT_FreeArena (cx->our_pool, PR_FALSE);
}


SEC_ASN1EncoderContext *
SEC_ASN1EncoderStart (const void *src, const SEC_ASN1Template *theTemplate,
		      SEC_ASN1WriteProc output_proc, void *output_arg)
{
    PRArenaPool *our_pool;
    SEC_ASN1EncoderContext *cx;

    our_pool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
    if (our_pool == NULL)
	return NULL;

    cx = (SEC_ASN1EncoderContext*)PORT_ArenaZAlloc (our_pool, sizeof(*cx));
    if (cx == NULL) {
	PORT_FreeArena (our_pool, PR_FALSE);
	return NULL;
    }

    cx->our_pool = our_pool;
    cx->output_proc = output_proc;
    cx->output_arg = output_arg;

    cx->status = keepGoing;

    if (sec_asn1e_push_state(cx, theTemplate, src, PR_FALSE) == NULL
	|| sec_asn1e_init_state_based_on_template (cx->current) == NULL) {
	



	PORT_FreeArena (our_pool, PR_FALSE);
	return NULL;
    }

    return cx;
}







void
SEC_ASN1EncoderSetNotifyProc (SEC_ASN1EncoderContext *cx,
			      SEC_ASN1NotifyProc fn, void *arg)
{
    cx->notify_proc = fn;
    cx->notify_arg = arg;
}


void
SEC_ASN1EncoderClearNotifyProc (SEC_ASN1EncoderContext *cx)
{
    cx->notify_proc = NULL;
    cx->notify_arg = NULL;	
}

void
SEC_ASN1EncoderAbort(SEC_ASN1EncoderContext *cx, int error)
{
    PORT_Assert(cx);
    PORT_SetError(error);
    cx->status = encodeError;
}

void
SEC_ASN1EncoderSetStreaming (SEC_ASN1EncoderContext *cx)
{
    

    cx->streaming = PR_TRUE;
}


void
SEC_ASN1EncoderClearStreaming (SEC_ASN1EncoderContext *cx)
{
    

    cx->streaming = PR_FALSE;
}


void
SEC_ASN1EncoderSetTakeFromBuf (SEC_ASN1EncoderContext *cx)
{
    




    PORT_Assert (cx->streaming);

    cx->from_buf = PR_TRUE;
}


void
SEC_ASN1EncoderClearTakeFromBuf (SEC_ASN1EncoderContext *cx)
{
    
    PORT_Assert (cx->from_buf);
    if (! cx->from_buf)		
	return;

    cx->from_buf = PR_FALSE;

    if (cx->status == needBytes) {
	cx->status = keepGoing;
	cx->current->place = afterContents;
    }
}


SECStatus
SEC_ASN1Encode (const void *src, const SEC_ASN1Template *theTemplate,
		SEC_ASN1WriteProc output_proc, void *output_arg)
{
    SEC_ASN1EncoderContext *ecx;
    SECStatus rv;

    ecx = SEC_ASN1EncoderStart (src, theTemplate, output_proc, output_arg);
    if (ecx == NULL)
	return SECFailure;

    rv = SEC_ASN1EncoderUpdate (ecx, NULL, 0);

    SEC_ASN1EncoderFinish (ecx);
    return rv;
}






static void
sec_asn1e_encode_item_count (void *arg, const char *buf, unsigned long len,
			     int depth, SEC_ASN1EncodingPart data_kind)
{
    unsigned long *count;

    count = (unsigned long*)arg;
    PORT_Assert (count != NULL);

    *count += len;
}



static void
sec_asn1e_encode_item_store (void *arg, const char *buf, unsigned long len,
			     int depth, SEC_ASN1EncodingPart data_kind)
{
    SECItem *dest;

    dest = (SECItem*)arg;
    PORT_Assert (dest != NULL);

    PORT_Memcpy (dest->data + dest->len, buf, len);
    dest->len += len;
}









static SECItem *
sec_asn1e_allocate_item (PRArenaPool *poolp, SECItem *dest, unsigned long len)
{
    if (poolp != NULL) {
	void *release;

	release = PORT_ArenaMark (poolp);
	if (dest == NULL)
	    dest = (SECItem*)PORT_ArenaAlloc (poolp, sizeof(SECItem));
	if (dest != NULL) {
	    dest->data = (unsigned char*)PORT_ArenaAlloc (poolp, len);
	    if (dest->data == NULL) {
		dest = NULL;
	    }
	}
	if (dest == NULL) {
	    
	    PORT_ArenaRelease (poolp, release);
	} else {
	    
	    PORT_ArenaUnmark (poolp, release);
	}
    } else {
	SECItem *indest;

	indest = dest;
	if (dest == NULL)
	    dest = (SECItem*)PORT_Alloc (sizeof(SECItem));
	if (dest != NULL) {
	    dest->type = siBuffer;
	    dest->data = (unsigned char*)PORT_Alloc (len);
	    if (dest->data == NULL) {
		if (indest == NULL)
		    PORT_Free (dest);
		dest = NULL;
	    }
	}
    }

    return dest;
}


SECItem *
SEC_ASN1EncodeItem (PRArenaPool *poolp, SECItem *dest, const void *src,
		    const SEC_ASN1Template *theTemplate)
{
    unsigned long encoding_length;
    SECStatus rv;

    PORT_Assert (dest == NULL || dest->data == NULL);

    encoding_length = 0;
    rv = SEC_ASN1Encode (src, theTemplate,
			 sec_asn1e_encode_item_count, &encoding_length);
    if (rv != SECSuccess)
	return NULL;

    dest = sec_asn1e_allocate_item (poolp, dest, encoding_length);
    if (dest == NULL)
	return NULL;

    
    PORT_Assert (dest->data != NULL);
    if (dest->data == NULL)
	return NULL;

    dest->len = 0;
    (void) SEC_ASN1Encode (src, theTemplate, sec_asn1e_encode_item_store, dest);

    PORT_Assert (encoding_length == dest->len);
    return dest;
}


static SECItem *
sec_asn1e_integer(PRArenaPool *poolp, SECItem *dest, unsigned long value,
		  PRBool is_unsigned)
{
    unsigned long copy;
    unsigned char sign;
    int len = 0;

    


    copy = value;
    do {
	len++;
	sign = (unsigned char)(copy & 0x80);
	copy >>= 8;
    } while (copy);

    




    if (sign && (is_unsigned || (long)value >= 0))
	len++;

    


    dest = sec_asn1e_allocate_item (poolp, dest, len);
    if (dest == NULL)
	return NULL;

    


    dest->len = len;
    while (len) {
	dest->data[--len] = (unsigned char)value;
	value >>= 8;
    }
    PORT_Assert (value == 0);

    return dest;
}


SECItem *
SEC_ASN1EncodeInteger(PRArenaPool *poolp, SECItem *dest, long value)
{
    return sec_asn1e_integer (poolp, dest, (unsigned long) value, PR_FALSE);
}


SECItem *
SEC_ASN1EncodeUnsignedInteger(PRArenaPool *poolp,
			      SECItem *dest, unsigned long value)
{
    return sec_asn1e_integer (poolp, dest, value, PR_TRUE);
}
