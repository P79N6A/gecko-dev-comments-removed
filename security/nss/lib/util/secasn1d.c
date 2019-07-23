












































#ifdef DEBUG_ASN1D_STATES
#include <stdio.h>
#define PR_Assert sec_asn1d_Assert
#endif

#include "secasn1.h"
#include "secerr.h"

typedef enum {
    beforeIdentifier,
    duringIdentifier,
    afterIdentifier,
    beforeLength,
    duringLength,
    afterLength,
    beforeBitString,
    duringBitString,
    duringConstructedString,
    duringGroup,
    duringLeaf,
    duringSaveEncoding,
    duringSequence,
    afterConstructedString,
    afterGroup,
    afterExplicit,
    afterImplicit,
    afterInline,
    afterPointer,
    afterSaveEncoding,
    beforeEndOfContents,
    duringEndOfContents,
    afterEndOfContents,
    beforeChoice,
    duringChoice,
    afterChoice,
    notInUse
} sec_asn1d_parse_place;

#ifdef DEBUG_ASN1D_STATES
static const char * const place_names[] = {
    "beforeIdentifier",
    "duringIdentifier",
    "afterIdentifier",
    "beforeLength",
    "duringLength",
    "afterLength",
    "beforeBitString",
    "duringBitString",
    "duringConstructedString",
    "duringGroup",
    "duringLeaf",
    "duringSaveEncoding",
    "duringSequence",
    "afterConstructedString",
    "afterGroup",
    "afterExplicit",
    "afterImplicit",
    "afterInline",
    "afterPointer",
    "afterSaveEncoding",
    "beforeEndOfContents",
    "duringEndOfContents",
    "afterEndOfContents",
    "beforeChoice",
    "duringChoice",
    "afterChoice",
    "notInUse"
};

static const char * const class_names[] = {
    "UNIVERSAL",
    "APPLICATION",
    "CONTEXT_SPECIFIC",
    "PRIVATE"
};

static const char * const method_names[] = { "PRIMITIVE", "CONSTRUCTED" };

static const char * const type_names[] = {
    "END_OF_CONTENTS",
    "BOOLEAN",
    "INTEGER",
    "BIT_STRING",
    "OCTET_STRING",
    "NULL",
    "OBJECT_ID",
    "OBJECT_DESCRIPTOR",
    "(type 08)",
    "REAL",
    "ENUMERATED",
    "EMBEDDED",
    "UTF8_STRING",
    "(type 0d)",
    "(type 0e)",
    "(type 0f)",
    "SEQUENCE",
    "SET",
    "NUMERIC_STRING",
    "PRINTABLE_STRING",
    "T61_STRING",
    "VIDEOTEXT_STRING",
    "IA5_STRING",
    "UTC_TIME",
    "GENERALIZED_TIME",
    "GRAPHIC_STRING",
    "VISIBLE_STRING",
    "GENERAL_STRING",
    "UNIVERSAL_STRING",
    "(type 1d)",
    "BMP_STRING",
    "HIGH_TAG_VALUE"
};

static const char * const flag_names[] = { 
    "OPTIONAL",
    "EXPLICIT",
    "ANY",
    "INLINE",
    "POINTER",
    "GROUP",
    "DYNAMIC",
    "SKIP",
    "INNER",
    "SAVE",
    "",            
    "SKIP_REST",
    "CHOICE",
    "NO_STREAM",
    "DEBUG_BREAK",
    "unknown 08",
    "unknown 10",
    "unknown 20",
    "unknown 40",
    "unknown 80"
};

static int 
formatKind(unsigned long kind, char * buf)
{
    int i;
    unsigned long k = kind & SEC_ASN1_TAGNUM_MASK;
    unsigned long notag = kind & (SEC_ASN1_CHOICE | SEC_ASN1_POINTER |
        SEC_ASN1_INLINE | SEC_ASN1_ANY | SEC_ASN1_SAVE);

    buf[0] = 0;
    if ((kind & SEC_ASN1_CLASS_MASK) != SEC_ASN1_UNIVERSAL) {
        sprintf(buf, " %s", class_names[(kind & SEC_ASN1_CLASS_MASK) >> 6] );
        buf += strlen(buf);
    }
    if (kind & SEC_ASN1_METHOD_MASK) {
        sprintf(buf, " %s", method_names[1]);
        buf += strlen(buf);
    }
    if ((kind & SEC_ASN1_CLASS_MASK) == SEC_ASN1_UNIVERSAL) {
        if (k || !notag) {
            sprintf(buf, " %s", type_names[k] );
            if ((k == SEC_ASN1_SET || k == SEC_ASN1_SEQUENCE) &&
                (kind & SEC_ASN1_GROUP)) {
                buf += strlen(buf);
                sprintf(buf, "_OF");
            }
        }
    } else {
        sprintf(buf, " [%d]", k);
    }
    buf += strlen(buf);

    for (k = kind >> 8, i = 0; k; k >>= 1, ++i) {
        if (k & 1) {
            sprintf(buf, " %s", flag_names[i]);
            buf += strlen(buf);
        }
    }
    return notag != 0;
}

#endif 

typedef enum {
    allDone,
    decodeError,
    keepGoing,
    needBytes
} sec_asn1d_parse_status;

struct subitem {
    const void *data;
    unsigned long len;		
    struct subitem *next;
};

typedef struct sec_asn1d_state_struct {
    SEC_ASN1DecoderContext *top;
    const SEC_ASN1Template *theTemplate;
    void *dest;

    void *our_mark;	

    struct sec_asn1d_state_struct *parent;	
    struct sec_asn1d_state_struct *child;	

    sec_asn1d_parse_place place;

    


    unsigned char found_tag_modifiers;
    unsigned char expect_tag_modifiers;
    unsigned long check_tag_mask;
    unsigned long found_tag_number;
    unsigned long expect_tag_number;
    unsigned long underlying_kind;

    unsigned long contents_length;
    unsigned long pending;
    unsigned long consumed;

    int depth;

    






    unsigned int bit_string_unused_bits;

    


    struct subitem *subitems_head;
    struct subitem *subitems_tail;

    PRPackedBool
	allocate,	
	endofcontents,	
	explicit,	
	indefinite,	
	missing,	
	optional,	
	substring;	

} sec_asn1d_state;

#define IS_HIGH_TAG_NUMBER(n)	((n) == SEC_ASN1_HIGH_TAG_NUMBER)
#define LAST_TAG_NUMBER_BYTE(b)	(((b) & 0x80) == 0)
#define TAG_NUMBER_BITS		7
#define TAG_NUMBER_MASK		0x7f

#define LENGTH_IS_SHORT_FORM(b)	(((b) & 0x80) == 0)
#define LONG_FORM_LENGTH(b)	((b) & 0x7f)

#define HIGH_BITS(field,cnt)	((field) >> ((sizeof(field) * 8) - (cnt)))








struct sec_DecoderContext_struct {
    PRArenaPool *our_pool;		
    PRArenaPool *their_pool;		
#ifdef SEC_ASN1D_FREE_ON_ERROR		





    



    void *their_mark;			
#endif

    sec_asn1d_state *current;
    sec_asn1d_parse_status status;

    SEC_ASN1NotifyProc notify_proc;	
    void *notify_arg;			
    PRBool during_notify;		

    SEC_ASN1WriteProc filter_proc;	
    void *filter_arg;			
    PRBool filter_only;			
};





static void *
sec_asn1d_alloc (PRArenaPool *poolp, unsigned long len)
{
    void *thing;

    if (poolp != NULL) {
	


	thing = PORT_ArenaAlloc (poolp, len);
    } else {
	


	thing = PORT_Alloc (len);
    }

    return thing;
}





static void *
sec_asn1d_zalloc (PRArenaPool *poolp, unsigned long len)
{
    void *thing;

    thing = sec_asn1d_alloc (poolp, len);
    if (thing != NULL)
	PORT_Memset (thing, 0, len);
    return thing;
}


static sec_asn1d_state *
sec_asn1d_push_state (SEC_ASN1DecoderContext *cx,
		      const SEC_ASN1Template *theTemplate,
		      void *dest, PRBool new_depth)
{
    sec_asn1d_state *state, *new_state;

    state = cx->current;

    PORT_Assert (state == NULL || state->child == NULL);

    if (state != NULL) {
	PORT_Assert (state->our_mark == NULL);
	state->our_mark = PORT_ArenaMark (cx->our_pool);
    }

    new_state = (sec_asn1d_state*)sec_asn1d_zalloc (cx->our_pool, 
						    sizeof(*new_state));
    if (new_state == NULL) {
	goto loser;
    }

    new_state->top         = cx;
    new_state->parent      = state;
    new_state->theTemplate = theTemplate;
    new_state->place       = notInUse;
    if (dest != NULL)
	new_state->dest = (char *)dest + theTemplate->offset;

    if (state != NULL) {
	new_state->depth = state->depth;
	if (new_depth) {
	    if (++new_state->depth > SEC_ASN1D_MAX_DEPTH) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		goto loser;
	    }
	}
	state->child = new_state;
    }

    cx->current = new_state;
    return new_state;

loser:
    cx->status = decodeError;
    if (state != NULL) {
	PORT_ArenaRelease(cx->our_pool, state->our_mark);
	state->our_mark = NULL;
    }
    return NULL;
}


static void
sec_asn1d_scrub_state (sec_asn1d_state *state)
{
    



    state->place = beforeIdentifier;
    state->endofcontents = PR_FALSE;
    state->indefinite = PR_FALSE;
    state->missing = PR_FALSE;
    PORT_Assert (state->consumed == 0);
}


static void
sec_asn1d_notify_before (SEC_ASN1DecoderContext *cx, void *dest, int depth)
{
    if (cx->notify_proc == NULL)
	return;

    cx->during_notify = PR_TRUE;
    (* cx->notify_proc) (cx->notify_arg, PR_TRUE, dest, depth);
    cx->during_notify = PR_FALSE;
}


static void
sec_asn1d_notify_after (SEC_ASN1DecoderContext *cx, void *dest, int depth)
{
    if (cx->notify_proc == NULL)
	return;

    cx->during_notify = PR_TRUE;
    (* cx->notify_proc) (cx->notify_arg, PR_FALSE, dest, depth);
    cx->during_notify = PR_FALSE;
}


static sec_asn1d_state *
sec_asn1d_init_state_based_on_template (sec_asn1d_state *state)
{
    PRBool explicit, optional, universal;
    unsigned char expect_tag_modifiers;
    unsigned long encode_kind, under_kind;
    unsigned long check_tag_mask, expect_tag_number;


    
    if (state == NULL || state->top->status == decodeError)
	return state;

    encode_kind = state->theTemplate->kind;

    if (encode_kind & SEC_ASN1_SAVE) {
	




	
	PORT_Assert (encode_kind == SEC_ASN1_SAVE);
	if (state->top->filter_only) {
	    




	    sec_asn1d_notify_after (state->top, state->dest, state->depth);
	    





	    if (state->dest == NULL)
		state->dest = state->parent->dest;
	    else
		state->dest = (char *)state->dest - state->theTemplate->offset;
	    state->theTemplate++;
	    if (state->dest != NULL)
		state->dest = (char *)state->dest + state->theTemplate->offset;
	    sec_asn1d_notify_before (state->top, state->dest, state->depth);
	    encode_kind = state->theTemplate->kind;
	    PORT_Assert ((encode_kind & SEC_ASN1_SAVE) == 0);
	} else {
	    sec_asn1d_scrub_state (state);
	    state->place = duringSaveEncoding;
	    state = sec_asn1d_push_state (state->top, SEC_AnyTemplate,
					  state->dest, PR_FALSE);
	    if (state != NULL)
		state = sec_asn1d_init_state_based_on_template (state);
	    return state;
	}
    }


    universal = ((encode_kind & SEC_ASN1_CLASS_MASK) == SEC_ASN1_UNIVERSAL)
		? PR_TRUE : PR_FALSE;

    explicit = (encode_kind & SEC_ASN1_EXPLICIT) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_EXPLICIT;

    optional = (encode_kind & SEC_ASN1_OPTIONAL) ? PR_TRUE : PR_FALSE;
    encode_kind &= ~SEC_ASN1_OPTIONAL;

    PORT_Assert (!(explicit && universal));	

    encode_kind &= ~SEC_ASN1_DYNAMIC;
    encode_kind &= ~SEC_ASN1_MAY_STREAM;

    if (encode_kind & SEC_ASN1_CHOICE) {
#if 0	
      sec_asn1d_state *child = sec_asn1d_push_state(state->top, state->theTemplate, state->dest, PR_FALSE);
      if ((sec_asn1d_state *)NULL == child) {
        return (sec_asn1d_state *)NULL;
      }

      child->allocate = state->allocate;
      child->place = beforeChoice;
      return child;
#else
      state->place = beforeChoice;
      return state;
#endif
    }

    if ((encode_kind & (SEC_ASN1_POINTER | SEC_ASN1_INLINE)) || (!universal
							      && !explicit)) {
	const SEC_ASN1Template *subt;
	void *dest;
	PRBool child_allocate;

	PORT_Assert ((encode_kind & (SEC_ASN1_ANY | SEC_ASN1_SKIP)) == 0);

	sec_asn1d_scrub_state (state);
	child_allocate = PR_FALSE;

	if (encode_kind & SEC_ASN1_POINTER) {
	    














	    if (universal) {
		



		PORT_Assert (encode_kind == SEC_ASN1_POINTER);
	    } else {
		








		PORT_Assert ((encode_kind & ~SEC_ASN1_TAG_MASK)
			     == SEC_ASN1_POINTER);
	    }
	    if (!state->top->filter_only)
		child_allocate = PR_TRUE;
	    dest = NULL;
	    state->place = afterPointer;
	} else {
	    dest = state->dest;
	    if (encode_kind & SEC_ASN1_INLINE) {
		
		PORT_Assert (encode_kind == SEC_ASN1_INLINE && !optional);
		state->place = afterInline;
	    } else {
		state->place = afterImplicit;
	    }
	}

	state->optional = optional;
	subt = SEC_ASN1GetSubtemplate (state->theTemplate, state->dest, PR_FALSE);
	state = sec_asn1d_push_state (state->top, subt, dest, PR_FALSE);
	if (state == NULL)
	    return NULL;

	state->allocate = child_allocate;

	if (universal) {
	    state = sec_asn1d_init_state_based_on_template (state);
	    if (state != NULL) {
		







		PORT_Assert (!state->optional);
		state->optional = optional;
	    }
	    return state;
	}

	under_kind = state->theTemplate->kind;
	under_kind &= ~SEC_ASN1_MAY_STREAM;
    } else if (explicit) {
	







	under_kind = 0;
    } else {
	



	under_kind = encode_kind;
    }

    
    PORT_Assert ((under_kind & (SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL
				| SEC_ASN1_MAY_STREAM
				| SEC_ASN1_INLINE | SEC_ASN1_POINTER)) == 0);

    if (encode_kind & (SEC_ASN1_ANY | SEC_ASN1_SKIP)) {
	PORT_Assert (encode_kind == under_kind);
	if (encode_kind & SEC_ASN1_SKIP) {
	    PORT_Assert (!optional);
	    PORT_Assert (encode_kind == SEC_ASN1_SKIP);
	    state->dest = NULL;
	}
	check_tag_mask = 0;
	expect_tag_modifiers = 0;
	expect_tag_number = 0;
    } else {
	check_tag_mask = SEC_ASN1_TAG_MASK;
	expect_tag_modifiers = (unsigned char)encode_kind & SEC_ASN1_TAG_MASK
				& ~SEC_ASN1_TAGNUM_MASK;
	





	expect_tag_number = encode_kind & SEC_ASN1_TAGNUM_MASK;

	switch (under_kind & SEC_ASN1_TAGNUM_MASK) {
	  case SEC_ASN1_SET:
	    



	    PORT_Assert ((under_kind & SEC_ASN1_GROUP) != 0);
	    
	  case SEC_ASN1_SEQUENCE:
	    expect_tag_modifiers |= SEC_ASN1_CONSTRUCTED;
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
	    check_tag_mask &= ~SEC_ASN1_CONSTRUCTED;
	    break;
	}
    }

    state->check_tag_mask = check_tag_mask;
    state->expect_tag_modifiers = expect_tag_modifiers;
    state->expect_tag_number = expect_tag_number;
    state->underlying_kind = under_kind;
    state->explicit = explicit;
    state->optional = optional;

    sec_asn1d_scrub_state (state);

    return state;
}

static sec_asn1d_state *
sec_asn1d_get_enclosing_construct(sec_asn1d_state *state)
{
    for (state = state->parent; state; state = state->parent) {
	sec_asn1d_parse_place place = state->place;
	if (place != afterImplicit      &&
	    place != afterPointer       &&
	    place != afterInline        &&
	    place != afterSaveEncoding  &&
	    place != duringSaveEncoding &&
	    place != duringChoice) {

            


            break;
	}
    }
    return state;
}

static PRBool
sec_asn1d_parent_allows_EOC(sec_asn1d_state *state)
{
    
    state = sec_asn1d_get_enclosing_construct(state);
    if (state) {
	sec_asn1d_parse_place place = state->place;
        
	int eoc_permitted = 
	    (place == duringGroup ||
	     place == duringConstructedString ||
	     state->child->optional);
	return (state->indefinite && eoc_permitted) ? PR_TRUE : PR_FALSE;
    }
    return PR_FALSE;
}

static unsigned long
sec_asn1d_parse_identifier (sec_asn1d_state *state,
			    const char *buf, unsigned long len)
{
    unsigned char byte;
    unsigned char tag_number;

    PORT_Assert (state->place == beforeIdentifier);

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    byte = (unsigned char) *buf;
#ifdef DEBUG_ASN1D_STATES
    {
        char kindBuf[256];
        formatKind(byte, kindBuf);
        printf("Found tag %02x %s\n", byte, kindBuf);
    }
#endif
    tag_number = byte & SEC_ASN1_TAGNUM_MASK;

    if (IS_HIGH_TAG_NUMBER (tag_number)) {
	state->place = duringIdentifier;
	state->found_tag_number = 0;
	




	state->pending = 1;
    } else {
	if (byte == 0 && sec_asn1d_parent_allows_EOC(state)) {
	    








	    state->place = duringEndOfContents;
	    state->pending = 2;
	    state->found_tag_number = 0;
	    state->found_tag_modifiers = 0;
	    



	    if (state->optional)
		state->missing = PR_TRUE;
	    return 0;
	}
	state->place = afterIdentifier;
	state->found_tag_number = tag_number;
    }
    state->found_tag_modifiers = byte & ~SEC_ASN1_TAGNUM_MASK;

    return 1;
}


static unsigned long
sec_asn1d_parse_more_identifier (sec_asn1d_state *state,
				 const char *buf, unsigned long len)
{
    unsigned char byte;
    int count;

    PORT_Assert (state->pending == 1);
    PORT_Assert (state->place == duringIdentifier);

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    count = 0;

    while (len && state->pending) {
	if (HIGH_BITS (state->found_tag_number, TAG_NUMBER_BITS) != 0) {
	    



	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return 0;
	}

	state->found_tag_number <<= TAG_NUMBER_BITS;

	byte = (unsigned char) buf[count++];
	state->found_tag_number |= (byte & TAG_NUMBER_MASK);

	len--;
	if (LAST_TAG_NUMBER_BYTE (byte))
	    state->pending = 0;
    }

    if (state->pending == 0)
	state->place = afterIdentifier;

    return count;
}


static void
sec_asn1d_confirm_identifier (sec_asn1d_state *state)
{
    PRBool match;

    PORT_Assert (state->place == afterIdentifier);

    match = (PRBool)(((state->found_tag_modifiers & state->check_tag_mask)
	     == state->expect_tag_modifiers)
	    && ((state->found_tag_number & state->check_tag_mask)
		== state->expect_tag_number));
    if (match) {
	state->place = beforeLength;
    } else {
	if (state->optional) {
	    state->missing = PR_TRUE;
	    state->place = afterEndOfContents;
	} else {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	}
    }
}


static unsigned long
sec_asn1d_parse_length (sec_asn1d_state *state,
			const char *buf, unsigned long len)
{
    unsigned char byte;

    PORT_Assert (state->place == beforeLength);

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    


    state->place = afterLength;

    byte = (unsigned char) *buf;

    if (LENGTH_IS_SHORT_FORM (byte)) {
	state->contents_length = byte;
    } else {
	state->contents_length = 0;
	state->pending = LONG_FORM_LENGTH (byte);
	if (state->pending == 0) {
	    state->indefinite = PR_TRUE;
	} else {
	    state->place = duringLength;
	}
    }

    





    if (!state->indefinite &&
	(state->underlying_kind & (SEC_ASN1_ANY | SEC_ASN1_SKIP))) {
	state->found_tag_modifiers &= ~SEC_ASN1_CONSTRUCTED;
    }

    return 1;
}


static unsigned long
sec_asn1d_parse_more_length (sec_asn1d_state *state,
			     const char *buf, unsigned long len)
{
    int count;

    PORT_Assert (state->pending > 0);
    PORT_Assert (state->place == duringLength);

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    count = 0;

    while (len && state->pending) {
	if (HIGH_BITS (state->contents_length, 9) != 0) {
	    



	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return 0;
	}

	state->contents_length <<= 8;
	state->contents_length |= (unsigned char) buf[count++];

	len--;
	state->pending--;
    }

    if (state->pending == 0)
	state->place = afterLength;

    return count;
}


static void
sec_asn1d_prepare_for_contents (sec_asn1d_state *state)
{
    SECItem *item;
    PRArenaPool *poolp;
    unsigned long alloc_len;

#ifdef DEBUG_ASN1D_STATES
    {
        printf("Found Length %d %s\n", state->contents_length,
               state->indefinite ? "indefinite" : "");
    }
#endif

    



    if (state->allocate) {
	void *dest;

	PORT_Assert (state->dest == NULL);
	



	dest = sec_asn1d_zalloc (state->top->their_pool,
				 state->theTemplate->size);
	if (dest == NULL) {
	    state->top->status = decodeError;
	    return;
	}
	state->dest = (char *)dest + state->theTemplate->offset;

	






	if (state->parent->place == afterPointer) {
	    void **placep;

	    placep = state->parent->dest;
	    *placep = dest;
	}
    }

    



    state->pending = state->contents_length;

    




    if (state->contents_length > 0) {
	sec_asn1d_state *parent = sec_asn1d_get_enclosing_construct(state);
	if (parent && !parent->indefinite && 
	    state->consumed + state->contents_length > parent->pending) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return;
	}
    }

    




    if (state->explicit) {
	state->place = afterExplicit;
	state = sec_asn1d_push_state (state->top,
				      SEC_ASN1GetSubtemplate(state->theTemplate,
							     state->dest,
							     PR_FALSE),
				      state->dest, PR_TRUE);
	if (state != NULL)
	    state = sec_asn1d_init_state_based_on_template (state);
	return;
    }

    






    if (state->underlying_kind & SEC_ASN1_GROUP) {
	



	PORT_Assert (state->underlying_kind == SEC_ASN1_SET_OF
	   || state->underlying_kind == SEC_ASN1_SEQUENCE_OF
	   || state->underlying_kind == (SEC_ASN1_SEQUENCE_OF|SEC_ASN1_DYNAMIC)
	   || state->underlying_kind == (SEC_ASN1_SEQUENCE_OF|SEC_ASN1_DYNAMIC)
		     );
	if (state->contents_length != 0 || state->indefinite) {
	    const SEC_ASN1Template *subt;

	    state->place = duringGroup;
	    subt = SEC_ASN1GetSubtemplate (state->theTemplate, state->dest,
					   PR_FALSE);
	    state = sec_asn1d_push_state (state->top, subt, NULL, PR_TRUE);
	    if (state != NULL) {
		if (!state->top->filter_only)
		    state->allocate = PR_TRUE;	
		


		sec_asn1d_notify_before (state->top, state->dest, state->depth);
		state = sec_asn1d_init_state_based_on_template (state);
	    }
	} else {
	    



	    state->place = afterGroup;
	}
	return;
    }

    switch (state->underlying_kind) {
      case SEC_ASN1_SEQUENCE:
	


	state->place = duringSequence;
	state = sec_asn1d_push_state (state->top, state->theTemplate + 1,
				      state->dest, PR_TRUE);
	if (state != NULL) {
	    


	    sec_asn1d_notify_before (state->top, state->dest, state->depth);
	    state = sec_asn1d_init_state_based_on_template (state);
	}
	break;

      case SEC_ASN1_SET:	
	






	PORT_Assert(0);
	PORT_SetError (SEC_ERROR_BAD_DER); 
	state->top->status = decodeError;
	break;

      case SEC_ASN1_NULL:
	



	if (state->contents_length || state->indefinite) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    break;
	}
	if (state->dest != NULL) {
	    item = (SECItem *)(state->dest);
	    item->data = NULL;
	    item->len = 0;
	}
	state->place = afterEndOfContents;
	break;

      case SEC_ASN1_BMP_STRING:
	
	if (state->contents_length % 2) {
	   PORT_SetError (SEC_ERROR_BAD_DER);
	   state->top->status = decodeError;
	   break;
	}   
	
	goto regular_string_type;

      case SEC_ASN1_UNIVERSAL_STRING:
	
	if (state->contents_length % 4) {
	   PORT_SetError (SEC_ERROR_BAD_DER);
	   state->top->status = decodeError;
	   break;
	}   
	
	goto regular_string_type;

      case SEC_ASN1_SKIP:
      case SEC_ASN1_ANY:
      case SEC_ASN1_ANY_CONTENTS:
	




	
regular_string_type:
      case SEC_ASN1_BIT_STRING:
      case SEC_ASN1_IA5_STRING:
      case SEC_ASN1_OCTET_STRING:
      case SEC_ASN1_PRINTABLE_STRING:
      case SEC_ASN1_T61_STRING:
      case SEC_ASN1_UTC_TIME:
      case SEC_ASN1_UTF8_STRING:
      case SEC_ASN1_VISIBLE_STRING:
	






	item = (SECItem *)(state->dest);

	








	alloc_len = state->contents_length;
	poolp = NULL;	

	if (item == NULL || state->top->filter_only) {
	    if (item != NULL) {
		item->data = NULL;
		item->len = 0;
	    }
	    alloc_len = 0;
	} else if (state->substring) {
	    








	    if (item->data == NULL) {
		PORT_Assert (item->len == 0);
		poolp = state->top->our_pool;
	    } else {
		alloc_len = 0;
	    }
	} else {
	    item->len = 0;
	    item->data = NULL;
	    poolp = state->top->their_pool;
	}

	if (alloc_len || ((! state->indefinite)
			  && (state->subitems_head != NULL))) {
	    struct subitem *subitem;
	    int len;

	    PORT_Assert (item);
	    if (!item) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return;
	    }
	    PORT_Assert (item->len == 0 && item->data == NULL);
	    




	    if (state->subitems_head != NULL) {
		PORT_Assert (state->underlying_kind == SEC_ASN1_ANY);
		for (subitem = state->subitems_head;
		     subitem != NULL; subitem = subitem->next)
		    alloc_len += subitem->len;
	    }

	    item->data = (unsigned char*)sec_asn1d_zalloc (poolp, alloc_len);
	    if (item->data == NULL) {
		state->top->status = decodeError;
		break;
	    }

	    len = 0;
	    for (subitem = state->subitems_head;
		 subitem != NULL; subitem = subitem->next) {
		PORT_Memcpy (item->data + len, subitem->data, subitem->len);
		len += subitem->len;
	    }
	    item->len = len;

	    




	    state->subitems_head = state->subitems_tail = NULL;
	}

	if (state->contents_length == 0 && (! state->indefinite)) {
	    


	    state->place = afterEndOfContents;
	} else if (state->found_tag_modifiers & SEC_ASN1_CONSTRUCTED) {
	    const SEC_ASN1Template *sub;

	    switch (state->underlying_kind) {
	      case SEC_ASN1_ANY:
	      case SEC_ASN1_ANY_CONTENTS:
		sub = SEC_AnyTemplate;
		break;
	      case SEC_ASN1_BIT_STRING:
		sub = SEC_BitStringTemplate;
		break;
	      case SEC_ASN1_BMP_STRING:
		sub = SEC_BMPStringTemplate;
		break;
	      case SEC_ASN1_GENERALIZED_TIME:
		sub = SEC_GeneralizedTimeTemplate;
		break;
	      case SEC_ASN1_IA5_STRING:
		sub = SEC_IA5StringTemplate;
		break;
	      case SEC_ASN1_OCTET_STRING:
		sub = SEC_OctetStringTemplate;
		break;
	      case SEC_ASN1_PRINTABLE_STRING:
		sub = SEC_PrintableStringTemplate;
		break;
	      case SEC_ASN1_T61_STRING:
		sub = SEC_T61StringTemplate;
		break;
	      case SEC_ASN1_UNIVERSAL_STRING:
		sub = SEC_UniversalStringTemplate;
		break;
	      case SEC_ASN1_UTC_TIME:
		sub = SEC_UTCTimeTemplate;
		break;
	      case SEC_ASN1_UTF8_STRING:
		sub = SEC_UTF8StringTemplate;
		break;
	      case SEC_ASN1_VISIBLE_STRING:
		sub = SEC_VisibleStringTemplate;
		break;
	      case SEC_ASN1_SKIP:
		sub = SEC_SkipTemplate;
		break;
	      default:		
		PORT_Assert(0);	
		sub = NULL;	
		break;
	    }

	    state->place = duringConstructedString;
	    state = sec_asn1d_push_state (state->top, sub, item, PR_TRUE);
	    if (state != NULL) {
		state->substring = PR_TRUE;	
		state = sec_asn1d_init_state_based_on_template (state);
	    }
	} else if (state->indefinite) {
	    


	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	} else {
	    


	    if (state->underlying_kind == SEC_ASN1_BIT_STRING)
		state->place = beforeBitString;
	    else
		state->place = duringLeaf;
	}
	break;

      default:
	


	if (state->contents_length) {
	    if (state->dest != NULL) {
		item = (SECItem *)(state->dest);
		item->len = 0;
		if (state->top->filter_only) {
		    item->data = NULL;
		} else {
		    item->data = (unsigned char*)
		                  sec_asn1d_zalloc (state->top->their_pool,
						   state->contents_length);
		    if (item->data == NULL) {
			state->top->status = decodeError;
			return;
		    }
		}
	    }
	    state->place = duringLeaf;
	} else {
	    



	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	}
    }
}


static void
sec_asn1d_free_child (sec_asn1d_state *state, PRBool error)
{
    if (state->child != NULL) {
	PORT_Assert (error || state->child->consumed == 0);
	PORT_Assert (state->our_mark != NULL);
	PORT_ArenaZRelease (state->top->our_pool, state->our_mark);
	if (error && state->top->their_pool == NULL) {
	    







	}
	state->child = NULL;
	state->our_mark = NULL;
    } else {
	






	PORT_Assert (state->our_mark == NULL);
    }
    state->place = beforeEndOfContents;
}












static void
sec_asn1d_reuse_encoding (sec_asn1d_state *state)
{
    sec_asn1d_state *child;
    unsigned long consumed;
    SECItem *item;
    void *dest;


    child = state->child;
    PORT_Assert (child != NULL);

    consumed = child->consumed;
    child->consumed = 0;

    item = (SECItem *)(state->dest);
    PORT_Assert (item != NULL);

    PORT_Assert (item->len == consumed);

    


    sec_asn1d_free_child (child, PR_FALSE);

    


    sec_asn1d_notify_after (state->top, state->dest, state->depth);

    


    dest = (char *)state->dest - state->theTemplate->offset;
    state->theTemplate++;
    child->dest = (char *)dest + state->theTemplate->offset;
    child->theTemplate = state->theTemplate;

    


    PORT_Assert (state->depth == child->depth);
    sec_asn1d_notify_before (state->top, child->dest, child->depth);

    


    state->place = afterSaveEncoding;

    


    state->top->current = child;

    


    (void) sec_asn1d_init_state_based_on_template(child);

    


    if (SEC_ASN1DecoderUpdate (state->top,
			       (char *) item->data, item->len) != SECSuccess)
	return;
    if (state->top->status == needBytes) {
	return;
    }

    PORT_Assert (state->top->current == state);
    PORT_Assert (state->child == child);

    


    PORT_Assert (consumed == child->consumed);
    child->consumed = 0;

    


    state->consumed += consumed;
    child->place = notInUse;
    state->place = afterEndOfContents;
}


static unsigned long
sec_asn1d_parse_leaf (sec_asn1d_state *state,
		      const char *buf, unsigned long len)
{
    SECItem *item;
    unsigned long bufLen;

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    if (state->pending < len)
	len = state->pending;

    bufLen = len;

    item = (SECItem *)(state->dest);
    if (item != NULL && item->data != NULL) {
	
	if (state->underlying_kind == SEC_ASN1_INTEGER && 
	    item->len == 0 &&                             
	    item->type == siUnsignedInteger)              
	{
	    while (len > 1 && buf[0] == 0) {              
		buf++;
		len--;
	    }
	}
	PORT_Memcpy (item->data + item->len, buf, len);
	item->len += len;
    }
    state->pending -= bufLen;
    if (state->pending == 0)
	state->place = beforeEndOfContents;

    return bufLen;
}


static unsigned long
sec_asn1d_parse_bit_string (sec_asn1d_state *state,
			    const char *buf, unsigned long len)
{
    unsigned char byte;

    
    PORT_Assert (state->place == beforeBitString);

    if (state->pending == 0) {
	if (state->dest != NULL) {
	    SECItem *item = (SECItem *)(state->dest);
	    item->data = NULL;
	    item->len = 0;
	    state->place = beforeEndOfContents;
	    return 0;
	}
    }

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    byte = (unsigned char) *buf;
    if (byte > 7) {
	PORT_SetError (SEC_ERROR_BAD_DER);
	state->top->status = decodeError;
	return 0;
    }

    state->bit_string_unused_bits = byte;
    state->place = duringBitString;
    state->pending -= 1;

    return 1;
}


static unsigned long
sec_asn1d_parse_more_bit_string (sec_asn1d_state *state,
				 const char *buf, unsigned long len)
{
    PORT_Assert (state->place == duringBitString);
    if (state->pending == 0) {
	
	if (state->bit_string_unused_bits) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	} else {
	    
	    state->place = beforeEndOfContents;
	}
	return 0;
    }

    len = sec_asn1d_parse_leaf (state, buf, len);
    if (state->place == beforeEndOfContents && state->dest != NULL) {
	SECItem *item;

	item = (SECItem *)(state->dest);
	if (item->len)
	    item->len = (item->len << 3) - state->bit_string_unused_bits;
    }

    return len;
}






static struct subitem *
sec_asn1d_add_to_subitems (sec_asn1d_state *state,
			   const void *data, unsigned long len,
			   PRBool copy_data)
{
    struct subitem *thing;

    thing = (struct subitem*)sec_asn1d_zalloc (state->top->our_pool,
				sizeof (struct subitem));
    if (thing == NULL) {
	state->top->status = decodeError;
	return NULL;
    }

    if (copy_data) {
	void *copy;
	copy = sec_asn1d_alloc (state->top->our_pool, len);
	if (copy == NULL) {
	    state->top->status = decodeError;
	    if (!state->top->our_pool)
	    	PORT_Free(thing);
	    return NULL;
	}
	PORT_Memcpy (copy, data, len);
	thing->data = copy;
    } else {
	thing->data = data;
    }
    thing->len = len;
    thing->next = NULL;

    if (state->subitems_head == NULL) {
	PORT_Assert (state->subitems_tail == NULL);
	state->subitems_head = state->subitems_tail = thing;
    } else {
	state->subitems_tail->next = thing;
	state->subitems_tail = thing;
    }

    return thing;
}


static void
sec_asn1d_record_any_header (sec_asn1d_state *state,
			     const char *buf,
			     unsigned long len)
{
    SECItem *item;

    item = (SECItem *)(state->dest);
    if (item != NULL && item->data != NULL) {
	PORT_Assert (state->substring);
	PORT_Memcpy (item->data + item->len, buf, len);
	item->len += len;
    } else {
	sec_asn1d_add_to_subitems (state, buf, len, PR_TRUE);
    }
}













static void
sec_asn1d_next_substring (sec_asn1d_state *state)
{
    sec_asn1d_state *child;
    SECItem *item;
    unsigned long child_consumed;
    PRBool done;

    PORT_Assert (state->place == duringConstructedString);
    PORT_Assert (state->child != NULL);

    child = state->child;

    child_consumed = child->consumed;
    child->consumed = 0;
    state->consumed += child_consumed;

    done = PR_FALSE;

    if (state->pending) {
	PORT_Assert (!state->indefinite);
	if (child_consumed > state->pending) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return;
	}

	state->pending -= child_consumed;
	if (state->pending == 0)
	    done = PR_TRUE;
    } else {
	PORT_Assert (state->indefinite);

	item = (SECItem *)(child->dest);
	if (item != NULL && item->data != NULL) {
	    


	    PORT_Assert (item->data != NULL);
	    sec_asn1d_add_to_subitems (state, item->data, item->len, PR_FALSE);
	    


	    item->data = NULL;
	    item->len = 0;
	}

	


	if (child->endofcontents)
	    done = PR_TRUE;
    }

    


    if (done) {
	child->place = notInUse;
	state->place = afterConstructedString;
    } else {
	sec_asn1d_scrub_state (child);
	state->top->current = child;
    }
}





static void
sec_asn1d_next_in_group (sec_asn1d_state *state)
{
    sec_asn1d_state *child;
    unsigned long child_consumed;

    PORT_Assert (state->place == duringGroup);
    PORT_Assert (state->child != NULL);

    child = state->child;

    child_consumed = child->consumed;
    child->consumed = 0;
    state->consumed += child_consumed;

    


    if (child->endofcontents) {
	





	










	PORT_Assert (state->indefinite);
	PORT_Assert (state->pending == 0);
	if(child->dest && !state->subitems_head) {
	    sec_asn1d_add_to_subitems (state, child->dest, 0, PR_FALSE);
	    child->dest = NULL;
	}

	child->place = notInUse;
	state->place = afterGroup;
	return;
    }

    


    sec_asn1d_notify_after (state->top, child->dest, child->depth);

    


    if (child->dest != NULL) {
	void *dest;

	dest = child->dest;
	dest = (char *)dest - child->theTemplate->offset;
	sec_asn1d_add_to_subitems (state, dest, 0, PR_FALSE);
	child->dest = NULL;
    }

    


    if (state->pending) {
	PORT_Assert (!state->indefinite);
	if (child_consumed > state->pending) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return;
	}

	state->pending -= child_consumed;
	if (state->pending == 0) {
	    child->place = notInUse;
	    state->place = afterGroup;
	    return;
	}
    }

    


    sec_asn1d_notify_before (state->top, child->dest, child->depth);

    


    sec_asn1d_scrub_state (child);

    
    sec_asn1d_init_state_based_on_template(child);

    state->top->current = child;
}







static void
sec_asn1d_next_in_sequence (sec_asn1d_state *state)
{
    sec_asn1d_state *child;
    unsigned long child_consumed;
    PRBool child_missing;

    PORT_Assert (state->place == duringSequence);
    PORT_Assert (state->child != NULL);

    child = state->child;

    


    sec_asn1d_notify_after (state->top, child->dest, child->depth);

    child_missing = (PRBool) child->missing;
    child_consumed = child->consumed;
    child->consumed = 0;

    


    if (child_missing) {
	PORT_Assert (child->optional);
    } else {
	state->consumed += child_consumed;
	


	sec_asn1d_free_child (child, PR_FALSE);
	if (state->pending) {
	    PORT_Assert (!state->indefinite);
	    if (child_consumed > state->pending) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return;
	    }
	    state->pending -= child_consumed;
	    if (state->pending == 0) {
		child->theTemplate++;
		while (child->theTemplate->kind != 0) {
		    if ((child->theTemplate->kind & SEC_ASN1_OPTIONAL) == 0) {
			PORT_SetError (SEC_ERROR_BAD_DER);
			state->top->status = decodeError;
			return;
		    }
		    child->theTemplate++;
		}
		child->place = notInUse;
		state->place = afterEndOfContents;
		return;
	    }
	}
    }

    


    child->theTemplate++;
    if (child->theTemplate->kind == 0) {
	


	child->place = notInUse;
	if (state->pending) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	} else if (child_missing) {
	    










	    if (state->indefinite && child->endofcontents) {
		PORT_Assert (child_consumed == 2);
		if (child_consumed != 2) {
		    PORT_SetError (SEC_ERROR_BAD_DER);
		    state->top->status = decodeError;
		} else {
		    state->consumed += child_consumed;
		    state->place = afterEndOfContents;
		}
	    } else {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
	    }
	} else {
	    



	    state->place = beforeEndOfContents;
	}
    } else {
	unsigned char child_found_tag_modifiers = 0;
	unsigned long child_found_tag_number = 0;

	


	if (state->dest != NULL)
	    child->dest = (char *)state->dest + child->theTemplate->offset;

	


	sec_asn1d_notify_before (state->top, child->dest, child->depth);

	if (child_missing) { 
	    child_found_tag_modifiers = child->found_tag_modifiers;
	    child_found_tag_number = child->found_tag_number;
	}
	state->top->current = child;
	child = sec_asn1d_init_state_based_on_template (child);
	if (child_missing && child) {
	    child->place = afterIdentifier;
	    child->found_tag_modifiers = child_found_tag_modifiers;
	    child->found_tag_number = child_found_tag_number;
	    child->consumed = child_consumed;
	    if (child->underlying_kind == SEC_ASN1_ANY
		&& !child->top->filter_only) {
		






		unsigned char identifier;

		






		PORT_Assert (child_found_tag_number < SEC_ASN1_HIGH_TAG_NUMBER);
		identifier = (unsigned char)(child_found_tag_modifiers | child_found_tag_number);
		sec_asn1d_record_any_header (child, (char *) &identifier, 1);
	    }
	}
    }
}


static void
sec_asn1d_concat_substrings (sec_asn1d_state *state)
{
    PORT_Assert (state->place == afterConstructedString);

    if (state->subitems_head != NULL) {
	struct subitem *substring;
	unsigned long alloc_len, item_len;
	unsigned char *where;
	SECItem *item;
	PRBool is_bit_string;

	item_len = 0;
	is_bit_string = (state->underlying_kind == SEC_ASN1_BIT_STRING)
			? PR_TRUE : PR_FALSE;

	substring = state->subitems_head;
	while (substring != NULL) {
	    



	    if (is_bit_string && (substring->next == NULL)
			      && (substring->len & 0x7)) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return;
	    }
	    item_len += substring->len;
	    substring = substring->next;
	}

	if (is_bit_string) {
	    alloc_len = ((item_len + 7) >> 3);
	} else {
	    




	    if (state->underlying_kind == SEC_ASN1_ANY && state->indefinite)
		item_len += 2; 
	    alloc_len = item_len;
	}

	item = (SECItem *)(state->dest);
	PORT_Assert (item != NULL);
	PORT_Assert (item->data == NULL);
	item->data = (unsigned char*)sec_asn1d_zalloc (state->top->their_pool, 
						       alloc_len);
	if (item->data == NULL) {
	    state->top->status = decodeError;
	    return;
	}
	item->len = item_len;

	where = item->data;
	substring = state->subitems_head;
	while (substring != NULL) {
	    if (is_bit_string)
		item_len = (substring->len + 7) >> 3;
	    else
		item_len = substring->len;
	    PORT_Memcpy (where, substring->data, item_len);
	    where += item_len;
	    substring = substring->next;
	}

	




	state->subitems_head = state->subitems_tail = NULL;
    }

    state->place = afterEndOfContents;
}


static void
sec_asn1d_concat_group (sec_asn1d_state *state)
{
    const void ***placep;

    PORT_Assert (state->place == afterGroup);

    placep = (const void***)state->dest;
    PORT_Assert(state->subitems_head == NULL || placep != NULL);
    if (placep != NULL) {
	struct subitem *item;
	const void **group;
	int count;

	count = 0;
	item = state->subitems_head;
	while (item != NULL) {
	    PORT_Assert (item->next != NULL || item == state->subitems_tail);
	    count++;
	    item = item->next;
	}

	group = (const void**)sec_asn1d_zalloc (state->top->their_pool,
				  (count + 1) * (sizeof(void *)));
	if (group == NULL) {
	    state->top->status = decodeError;
	    return;
	}

	*placep = group;

	item = state->subitems_head;
	while (item != NULL) {
	    *group++ = item->data;
	    item = item->next;
	}
	*group = NULL;

	




	state->subitems_head = state->subitems_tail = NULL;
    }

    state->place = afterEndOfContents;
}






static void
sec_asn1d_absorb_child (sec_asn1d_state *state)
{
    


    PORT_Assert (state->child != NULL);

    



    state->missing = state->child->missing;
    if (state->missing) {
	state->found_tag_number = state->child->found_tag_number;
	state->found_tag_modifiers = state->child->found_tag_modifiers;
	state->endofcontents = state->child->endofcontents;
    }

    



    PORT_Assert (state->place == afterExplicit || state->consumed == 0);
    state->consumed += state->child->consumed;

    



    if (state->pending) {
	PORT_Assert (!state->indefinite);
	PORT_Assert (state->place == afterExplicit);

	



	if (state->pending != state->child->consumed) {
	    if (state->pending < state->child->consumed) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return;
	    }
	    











	    state->consumed += (state->pending - state->child->consumed);
	}
	state->pending = 0;
    }

    


    state->child->consumed = 0;

    





    PORT_Assert (state->place == afterExplicit || (! state->indefinite));
    state->place = beforeEndOfContents;
}


static void
sec_asn1d_prepare_for_end_of_contents (sec_asn1d_state *state)
{
    PORT_Assert (state->place == beforeEndOfContents);

    if (state->indefinite) {
	state->place = duringEndOfContents;
	state->pending = 2;
    } else {
	state->place = afterEndOfContents;
    }
}


static unsigned long
sec_asn1d_parse_end_of_contents (sec_asn1d_state *state,
				 const char *buf, unsigned long len)
{
    unsigned int i;

    PORT_Assert (state->pending <= 2);
    PORT_Assert (state->place == duringEndOfContents);

    if (len == 0) {
	state->top->status = needBytes;
	return 0;
    }

    if (state->pending < len)
	len = state->pending;

    for (i = 0; i < len; i++) {
	if (buf[i] != 0) {
	    


	    PORT_SetError (SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return 0;
	}
    }

    state->pending -= len;

    if (state->pending == 0) {
	state->place = afterEndOfContents;
	state->endofcontents = PR_TRUE;
    }

    return len;
}


static void
sec_asn1d_pop_state (sec_asn1d_state *state)
{
#if 0	
    


    if (state->child != NULL) {
	state->consumed += state->child->consumed;
	if (state->pending) {
	    PORT_Assert (!state->indefinite);
	    if (state->child->consumed > state->pending) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
	    } else {
		state->pending -= state->child->consumed;
	    }
	}
	state->child->consumed = 0;
    }
#endif	

    


    sec_asn1d_free_child (state, PR_FALSE);

    



    state->top->current = state->parent;
}

static sec_asn1d_state *
sec_asn1d_before_choice (sec_asn1d_state *state)
{
    sec_asn1d_state *child;

    if (state->allocate) {
	void *dest;

	dest = sec_asn1d_zalloc(state->top->their_pool, state->theTemplate->size);
	if ((void *)NULL == dest) {
	    state->top->status = decodeError;
	    return (sec_asn1d_state *)NULL;
	}

	state->dest = (char *)dest + state->theTemplate->offset;
    }

    child = sec_asn1d_push_state(state->top, state->theTemplate + 1, 
				 (char *)state->dest - state->theTemplate->offset, 
				 PR_FALSE);
    if ((sec_asn1d_state *)NULL == child) {
	return (sec_asn1d_state *)NULL;
    }

    sec_asn1d_scrub_state(child);
    child = sec_asn1d_init_state_based_on_template(child);
    if ((sec_asn1d_state *)NULL == child) {
	return (sec_asn1d_state *)NULL;
    }

    child->optional = PR_TRUE;

    state->place = duringChoice;

    return child;
}

static sec_asn1d_state *
sec_asn1d_during_choice (sec_asn1d_state *state)
{
    sec_asn1d_state *child = state->child;
    
    PORT_Assert((sec_asn1d_state *)NULL != child);

    if (child->missing) {
	unsigned char child_found_tag_modifiers = 0;
	unsigned long child_found_tag_number = 0;
	void *        dest;

	state->consumed += child->consumed;

	if (child->endofcontents) {
	    






	    child->place = notInUse;
	    state->place = afterChoice;
	    state->endofcontents = PR_TRUE;  
	    if (sec_asn1d_parent_allows_EOC(state))
		return state;
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return NULL;
	}

	dest = (char *)child->dest - child->theTemplate->offset;
	child->theTemplate++;

	if (0 == child->theTemplate->kind) {
	    
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    state->top->status = decodeError;
	    return (sec_asn1d_state *)NULL;
	}
	child->dest = (char *)dest + child->theTemplate->offset;

	
	if (state->pending) {
	    PORT_Assert(!state->indefinite);
	    if (child->consumed > state->pending) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return NULL;
	    }
	    state->pending -= child->consumed;
	    if (0 == state->pending) {
		

		PORT_Assert(0);
		PORT_SetError(SEC_ERROR_BAD_DER);
		state->top->status = decodeError;
		return (sec_asn1d_state *)NULL;
	    }
	}

	child->consumed = 0;
	sec_asn1d_scrub_state(child);

	
	state->top->current = child;

	child_found_tag_modifiers = child->found_tag_modifiers;
	child_found_tag_number = child->found_tag_number;

	child = sec_asn1d_init_state_based_on_template(child);
	if ((sec_asn1d_state *)NULL == child) {
	    return (sec_asn1d_state *)NULL;
	}

	
	child->found_tag_modifiers = child_found_tag_modifiers;
	child->found_tag_number = child_found_tag_number;

	child->optional = PR_TRUE;
	child->place = afterIdentifier;

	return child;
    } 
    if ((void *)NULL != state->dest) {
	
	int *which = (int *)state->dest;
	*which = (int)child->theTemplate->size;
    }

    child->place = notInUse;

    state->place = afterChoice;
    return state;
}

static void
sec_asn1d_after_choice (sec_asn1d_state *state)
{
    state->consumed += state->child->consumed;
    state->child->consumed = 0;
    state->place = afterEndOfContents;
    sec_asn1d_pop_state(state);
}

unsigned long
sec_asn1d_uinteger(SECItem *src)
{
    unsigned long value;
    int len;

    if (src->len > 5 || (src->len > 4 && src->data[0] == 0))
	return 0;

    value = 0;
    len = src->len;
    while (len) {
	value <<= 8;
	value |= src->data[--len];
    }
    return value;
}

SECStatus
SEC_ASN1DecodeInteger(SECItem *src, unsigned long *value)
{
    unsigned long v;
    unsigned int i;
    
    if (src == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (src->len > sizeof(unsigned long)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (src->data == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
    	return SECFailure;
    }

    if (src->data[0] & 0x80)
	v = -1;		
    else
	v = 0;

    for (i= 0; i < src->len; i++) {
	
	v <<= 8;
	v |= src->data[i];
    }
    *value = v;
    return SECSuccess;
}

#ifdef DEBUG_ASN1D_STATES
static void
dump_states(SEC_ASN1DecoderContext *cx)
{
    sec_asn1d_state *state;
    char kindBuf[256];

    for (state = cx->current; state->parent; state = state->parent) {
        ;
    }

    for (; state; state = state->child) {
        int i;
        for (i = 0; i < state->depth; i++) {
            printf("  ");
        }

        i = formatKind(state->theTemplate->kind, kindBuf);
        printf("%s: tmpl %08x, kind%s",
               (state == cx->current) ? "STATE" : "State",
               state->theTemplate,
               kindBuf);
        printf(" %s", (state->place >= 0 && state->place <= notInUse)
                       ? place_names[ state->place ]
                       : "(undefined)");
        if (!i)
            printf(", expect 0x%02x",
                   state->expect_tag_number | state->expect_tag_modifiers);

        printf("%s%s%s %d\n",
               state->indefinite    ? ", indef"   : "",
               state->missing       ? ", miss"    : "",
               state->endofcontents ? ", EOC"     : "",
               state->pending
               );
    }

    return;
}
#endif 

SECStatus
SEC_ASN1DecoderUpdate (SEC_ASN1DecoderContext *cx,
		       const char *buf, unsigned long len)
{
    sec_asn1d_state *state = NULL;
    unsigned long consumed;
    SEC_ASN1EncodingPart what;
    sec_asn1d_state *stateEnd = cx->current;

    if (cx->status == needBytes)
	cx->status = keepGoing;

    while (cx->status == keepGoing) {
	state = cx->current;
	what = SEC_ASN1_Contents;
	consumed = 0;
#ifdef DEBUG_ASN1D_STATES
        printf("\nPLACE = %s, next byte = 0x%02x, %08x[%d]\n",
               (state->place >= 0 && state->place <= notInUse) ?
               place_names[ state->place ] : "(undefined)",
               (unsigned int)((unsigned char *)buf)[ consumed ],
               buf, consumed);
        dump_states(cx);
#endif 
	switch (state->place) {
	  case beforeIdentifier:
	    consumed = sec_asn1d_parse_identifier (state, buf, len);
	    what = SEC_ASN1_Identifier;
	    break;
	  case duringIdentifier:
	    consumed = sec_asn1d_parse_more_identifier (state, buf, len);
	    what = SEC_ASN1_Identifier;
	    break;
	  case afterIdentifier:
	    sec_asn1d_confirm_identifier (state);
	    break;
	  case beforeLength:
	    consumed = sec_asn1d_parse_length (state, buf, len);
	    what = SEC_ASN1_Length;
	    break;
	  case duringLength:
	    consumed = sec_asn1d_parse_more_length (state, buf, len);
	    what = SEC_ASN1_Length;
	    break;
	  case afterLength:
	    sec_asn1d_prepare_for_contents (state);
	    break;
	  case beforeBitString:
	    consumed = sec_asn1d_parse_bit_string (state, buf, len);
	    break;
	  case duringBitString:
	    consumed = sec_asn1d_parse_more_bit_string (state, buf, len);
	    break;
	  case duringConstructedString:
	    sec_asn1d_next_substring (state);
	    break;
	  case duringGroup:
	    sec_asn1d_next_in_group (state);
	    break;
	  case duringLeaf:
	    consumed = sec_asn1d_parse_leaf (state, buf, len);
	    break;
	  case duringSaveEncoding:
	    sec_asn1d_reuse_encoding (state);
	    if (cx->status == decodeError) {
		


		return SECFailure;
	    }
	    if (cx->status == needBytes) {
		
		PORT_SetError (SEC_ERROR_BAD_DER);
		cx->status = decodeError;
	    }
	    break;
	  case duringSequence:
	    sec_asn1d_next_in_sequence (state);
	    break;
	  case afterConstructedString:
	    sec_asn1d_concat_substrings (state);
	    break;
	  case afterExplicit:
	  case afterImplicit:
	  case afterInline:
	  case afterPointer:
	    sec_asn1d_absorb_child (state);
	    break;
	  case afterGroup:
	    sec_asn1d_concat_group (state);
	    break;
	  case afterSaveEncoding:
	    



	    return SECSuccess;
	  case beforeEndOfContents:
	    sec_asn1d_prepare_for_end_of_contents (state);
	    break;
	  case duringEndOfContents:
	    consumed = sec_asn1d_parse_end_of_contents (state, buf, len);
	    what = SEC_ASN1_EndOfContents;
	    break;
	  case afterEndOfContents:
	    sec_asn1d_pop_state (state);
	    break;
          case beforeChoice:
            state = sec_asn1d_before_choice(state);
            break;
          case duringChoice:
            state = sec_asn1d_during_choice(state);
            break;
          case afterChoice:
            sec_asn1d_after_choice(state);
            break;
	  case notInUse:
	  default:
	    
	    PORT_Assert (0);
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    cx->status = decodeError;
	    break;
	}

	if (cx->status == decodeError)
	    break;

	
	PORT_Assert (consumed <= len);
	if (consumed > len) {
	    PORT_SetError (SEC_ERROR_BAD_DER);
	    cx->status = decodeError;
	    break;
	}

	
	state = cx->current;

	
	if (state == NULL) {
	    PORT_Assert (consumed == 0);
#if 0	




	    if (len > 0) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		cx->status = decodeError;
	    } else
#endif
		cx->status = allDone;
	    break;
	}
	else if (state->theTemplate->kind == SEC_ASN1_SKIP_REST) {
	    cx->status = allDone;
	    break;
	}
	  
	if (consumed == 0)
	    continue;

	







	if (state->underlying_kind == SEC_ASN1_ANY
	    && !cx->filter_only && (what == SEC_ASN1_Identifier
				    || what == SEC_ASN1_Length)) {
	    sec_asn1d_record_any_header (state, buf, consumed);
	}

	



	if (state->top->filter_proc != NULL) {
	    int depth;

	    depth = state->depth;
	    if (what == SEC_ASN1_EndOfContents && !state->indefinite) {
		PORT_Assert (state->parent != NULL
			     && state->parent->indefinite);
		depth--;
		PORT_Assert (depth == state->parent->depth);
	    }
	    (* state->top->filter_proc) (state->top->filter_arg,
					 buf, consumed, depth, what);
	}

	state->consumed += consumed;
	buf += consumed;
	len -= consumed;
    }

    if (cx->status == decodeError) {
	while (state != NULL && stateEnd->parent!=state) {
	    sec_asn1d_free_child (state, PR_TRUE);
	    state = state->parent;
	}
#ifdef SEC_ASN1D_FREE_ON_ERROR	









	if (cx->their_pool != NULL) {
	    PORT_Assert (cx->their_mark != NULL);
	    PORT_ArenaRelease (cx->their_pool, cx->their_mark);
	}
#endif
	return SECFailure;
    }

#if 0	







    PORT_Assert (len == 0
		 && (cx->status == needBytes || cx->status == allDone));
#else
    PORT_Assert ((len == 0 && cx->status == needBytes)
		 || cx->status == allDone);
#endif
    return SECSuccess;
}


SECStatus
SEC_ASN1DecoderFinish (SEC_ASN1DecoderContext *cx)
{
    SECStatus rv;

    if (cx->status == needBytes) {
	PORT_SetError (SEC_ERROR_BAD_DER);
	rv = SECFailure;
    } else {
	rv = SECSuccess;
    }

    



    PORT_FreeArena (cx->our_pool, PR_TRUE);

    return rv;
}


SEC_ASN1DecoderContext *
SEC_ASN1DecoderStart (PRArenaPool *their_pool, void *dest,
		      const SEC_ASN1Template *theTemplate)
{
    PRArenaPool *our_pool;
    SEC_ASN1DecoderContext *cx;

    our_pool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
    if (our_pool == NULL)
	return NULL;

    cx = (SEC_ASN1DecoderContext*)PORT_ArenaZAlloc (our_pool, sizeof(*cx));
    if (cx == NULL) {
	PORT_FreeArena (our_pool, PR_FALSE);
	return NULL;
    }

    cx->our_pool = our_pool;
    if (their_pool != NULL) {
	cx->their_pool = their_pool;
#ifdef SEC_ASN1D_FREE_ON_ERROR
	cx->their_mark = PORT_ArenaMark (their_pool);
#endif
    }

    cx->status = needBytes;

    if (sec_asn1d_push_state(cx, theTemplate, dest, PR_FALSE) == NULL
	|| sec_asn1d_init_state_based_on_template (cx->current) == NULL) {
	



	PORT_FreeArena (our_pool, PR_FALSE);
	return NULL;
    }

    return cx;
}


void
SEC_ASN1DecoderSetFilterProc (SEC_ASN1DecoderContext *cx,
			      SEC_ASN1WriteProc fn, void *arg,
			      PRBool only)
{
    
    PORT_Assert (cx->during_notify);

    cx->filter_proc = fn;
    cx->filter_arg = arg;
    cx->filter_only = only;
}


void
SEC_ASN1DecoderClearFilterProc (SEC_ASN1DecoderContext *cx)
{
    
    PORT_Assert (cx->during_notify);

    cx->filter_proc = NULL;
    cx->filter_arg = NULL;
    cx->filter_only = PR_FALSE;
}


void
SEC_ASN1DecoderSetNotifyProc (SEC_ASN1DecoderContext *cx,
			      SEC_ASN1NotifyProc fn, void *arg)
{
    cx->notify_proc = fn;
    cx->notify_arg = arg;
}


void
SEC_ASN1DecoderClearNotifyProc (SEC_ASN1DecoderContext *cx)
{
    cx->notify_proc = NULL;
    cx->notify_arg = NULL;	
}

void
SEC_ASN1DecoderAbort(SEC_ASN1DecoderContext *cx, int error)
{
    PORT_Assert(cx);
    PORT_SetError(error);
    cx->status = decodeError;
}


SECStatus
SEC_ASN1Decode (PRArenaPool *poolp, void *dest,
		const SEC_ASN1Template *theTemplate,
		const char *buf, long len)
{
    SEC_ASN1DecoderContext *dcx;
    SECStatus urv, frv;

    dcx = SEC_ASN1DecoderStart (poolp, dest, theTemplate);
    if (dcx == NULL)
	return SECFailure;

    urv = SEC_ASN1DecoderUpdate (dcx, buf, len);
    frv = SEC_ASN1DecoderFinish (dcx);

    if (urv != SECSuccess)
	return urv;

    return frv;
}


SECStatus
SEC_ASN1DecodeItem (PRArenaPool *poolp, void *dest,
		    const SEC_ASN1Template *theTemplate,
		    const SECItem *src)
{
    return SEC_ASN1Decode (poolp, dest, theTemplate,
			   (const char *)src->data, src->len);
}

#ifdef DEBUG_ASN1D_STATES
void sec_asn1d_Assert(const char *s, const char *file, PRIntn ln)
{
    printf("Assertion failed, \"%s\", file %s, line %d\n", s, file, ln);
    fflush(stdout);
}
#endif

















const SEC_ASN1Template SEC_SequenceOfAnyTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
};

#if 0

const SEC_ASN1Template SEC_PointerToBitStringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_BitStringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfBitStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_BitStringTemplate }
};

const SEC_ASN1Template SEC_SetOfBitStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_BitStringTemplate }
};

const SEC_ASN1Template SEC_PointerToBMPStringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_BMPStringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfBMPStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_BMPStringTemplate }
};

const SEC_ASN1Template SEC_SetOfBMPStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_BMPStringTemplate }
};

const SEC_ASN1Template SEC_PointerToBooleanTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_BooleanTemplate }
};

const SEC_ASN1Template SEC_SequenceOfBooleanTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_BooleanTemplate }
};

const SEC_ASN1Template SEC_SetOfBooleanTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_BooleanTemplate }
};

#endif

const SEC_ASN1Template SEC_EnumeratedTemplate[] = {
    { SEC_ASN1_ENUMERATED, 0, NULL, sizeof(SECItem) }
};

const SEC_ASN1Template SEC_PointerToEnumeratedTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_EnumeratedTemplate }
};

#if 0

const SEC_ASN1Template SEC_SequenceOfEnumeratedTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_EnumeratedTemplate }
};

#endif

const SEC_ASN1Template SEC_SetOfEnumeratedTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_EnumeratedTemplate }
};

const SEC_ASN1Template SEC_PointerToGeneralizedTimeTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_GeneralizedTimeTemplate }
};

#if 0

const SEC_ASN1Template SEC_SequenceOfGeneralizedTimeTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_GeneralizedTimeTemplate }
};

const SEC_ASN1Template SEC_SetOfGeneralizedTimeTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_GeneralizedTimeTemplate }
};

const SEC_ASN1Template SEC_PointerToIA5StringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_IA5StringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfIA5StringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_IA5StringTemplate }
};

const SEC_ASN1Template SEC_SetOfIA5StringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_IA5StringTemplate }
};

const SEC_ASN1Template SEC_PointerToIntegerTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_IntegerTemplate }
};

const SEC_ASN1Template SEC_SequenceOfIntegerTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_IntegerTemplate }
};

const SEC_ASN1Template SEC_SetOfIntegerTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_IntegerTemplate }
};

const SEC_ASN1Template SEC_PointerToNullTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_NullTemplate }
};

const SEC_ASN1Template SEC_SequenceOfNullTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_NullTemplate }
};

const SEC_ASN1Template SEC_SetOfNullTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_NullTemplate }
};

const SEC_ASN1Template SEC_PointerToObjectIDTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_ObjectIDTemplate }
};

#endif

const SEC_ASN1Template SEC_SequenceOfObjectIDTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_ObjectIDTemplate }
};

#if 0

const SEC_ASN1Template SEC_SetOfObjectIDTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_ObjectIDTemplate }
};

const SEC_ASN1Template SEC_SequenceOfOctetStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_OctetStringTemplate }
};

const SEC_ASN1Template SEC_SetOfOctetStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_OctetStringTemplate }
};

#endif

const SEC_ASN1Template SEC_PrintableStringTemplate[] = {
    { SEC_ASN1_PRINTABLE_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};

#if 0

const SEC_ASN1Template SEC_PointerToPrintableStringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_PrintableStringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfPrintableStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_PrintableStringTemplate }
};

const SEC_ASN1Template SEC_SetOfPrintableStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_PrintableStringTemplate }
};

#endif

const SEC_ASN1Template SEC_T61StringTemplate[] = {
    { SEC_ASN1_T61_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

#if 0

const SEC_ASN1Template SEC_PointerToT61StringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_T61StringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfT61StringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_T61StringTemplate }
};

const SEC_ASN1Template SEC_SetOfT61StringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_T61StringTemplate }
};

#endif

const SEC_ASN1Template SEC_UniversalStringTemplate[] = {
    { SEC_ASN1_UNIVERSAL_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};

#if 0

const SEC_ASN1Template SEC_PointerToUniversalStringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_UniversalStringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfUniversalStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_UniversalStringTemplate }
};

const SEC_ASN1Template SEC_SetOfUniversalStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_UniversalStringTemplate }
};

const SEC_ASN1Template SEC_PointerToUTCTimeTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_UTCTimeTemplate }
};

const SEC_ASN1Template SEC_SequenceOfUTCTimeTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_UTCTimeTemplate }
};

const SEC_ASN1Template SEC_SetOfUTCTimeTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_UTCTimeTemplate }
};

const SEC_ASN1Template SEC_PointerToUTF8StringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_UTF8StringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfUTF8StringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_UTF8StringTemplate }
};

const SEC_ASN1Template SEC_SetOfUTF8StringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_UTF8StringTemplate }
};

#endif

const SEC_ASN1Template SEC_VisibleStringTemplate[] = {
    { SEC_ASN1_VISIBLE_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

#if 0

const SEC_ASN1Template SEC_PointerToVisibleStringTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_VisibleStringTemplate }
};

const SEC_ASN1Template SEC_SequenceOfVisibleStringTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_VisibleStringTemplate }
};

const SEC_ASN1Template SEC_SetOfVisibleStringTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_VisibleStringTemplate }
};

#endif








const SEC_ASN1Template SEC_SkipTemplate[] = {
    { SEC_ASN1_SKIP }
};





SEC_ASN1_CHOOSER_IMPLEMENT(SEC_EnumeratedTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToEnumeratedTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SequenceOfAnyTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SequenceOfObjectIDTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SkipTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UniversalStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PrintableStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_T61StringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToGeneralizedTimeTemplate)

