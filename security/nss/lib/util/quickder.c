








#include "secerr.h"
#include "secasn1.h" 
#include "secitem.h"





static unsigned char* definite_length_decoder(const unsigned char *buf,
                                              const unsigned int buf_length,
                                              unsigned int *out_data_length,
                                              PRBool includeTag)
{
    unsigned char tag;
    unsigned int used_length = 0;
    unsigned int data_length = 0;
    unsigned char length_field_len = 0;
    unsigned char byte;
    unsigned int i;

    if (used_length >= buf_length)
    {
        
        return NULL;
    }
    tag = buf[used_length++];

    if (tag == 0)
    {
        

        return NULL;
    }

    if ((tag & 0x1F) == 0x1F)
    {
        
        return NULL;
    }

    if (used_length >= buf_length)
    {
        
        return NULL;
    }
    byte = buf[used_length++];

    if (!(byte & 0x80))
    {
        
        data_length = byte; 
    }
    else
    {
        
        length_field_len = byte & 0x7F;
        if (length_field_len == 0)
        {
            
            return NULL;
        }

        if (length_field_len > sizeof(data_length))
        {
            

            return NULL;
        }

        if (length_field_len > (buf_length - used_length))
        {
            
            return NULL;
        }

        
        for (i = 0; i < length_field_len; i++)
        {
            byte = buf[used_length++];
            data_length = (data_length << 8) | byte;

            if (i == 0)
            {
                PRBool too_long = PR_FALSE;
                if (length_field_len == 1)
                {
                    too_long = ((byte & 0x80) == 0); 
                }
                else
                {
                    too_long = (byte == 0); 
                }
                if (too_long)
                {
                    
                    return NULL;
                }
            }
        }
    }

    if (data_length > (buf_length - used_length))
    {
        
        return NULL;
    }

    if (includeTag)
    {
        data_length += used_length;
    }

    *out_data_length = data_length;
    return ((unsigned char*)buf + (includeTag ? 0 : used_length));
}

static SECStatus GetItem(SECItem* src, SECItem* dest, PRBool includeTag)
{
    if ( (!src) || (!dest) || (!src->data && src->len) )
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (!src->len)
    {
        
        dest->data = NULL;
        dest->len = 0;
        return SECSuccess;
    }

    dest->data = definite_length_decoder(src->data,  src->len, &dest->len,
        includeTag);
    if (dest->data == NULL)
    {
        PORT_SetError(SEC_ERROR_BAD_DER);
        return SECFailure;
    }
    src->len -= (dest->data - src->data) + dest->len;
    src->data = dest->data + dest->len;
    return SECSuccess;
}



static SECStatus MatchComponentType(const SEC_ASN1Template* templateEntry,
                                    SECItem* item, PRBool* match, void* dest)
{
    unsigned long kind = 0;
    unsigned char tag = 0;

    if ( (!item) || (!item->data && item->len) || (!templateEntry) || (!match) )
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (!item->len)
    {
        *match = PR_FALSE;
        return SECSuccess;
    }

    kind = templateEntry->kind;
    tag = *(unsigned char*) item->data;

    if ( ( (kind & SEC_ASN1_INLINE) ||
           (kind & SEC_ASN1_POINTER) ) &&
           (0 == (kind & SEC_ASN1_TAG_MASK) ) )
    {
        


        if (!(kind & SEC_ASN1_OPTIONAL))
        {
            



            *match = PR_TRUE;
            return SECSuccess;
        }
        else
        {
            

            const SEC_ASN1Template* subTemplate = 
                SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
            if (!subTemplate)
            {
                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                return SECFailure;
            }
            if ( (subTemplate->kind & SEC_ASN1_INLINE) ||
                 (subTemplate->kind & SEC_ASN1_POINTER) )
            {
                




                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                return SECFailure;
            }
            return MatchComponentType(subTemplate, item, match,
                                      (void*)((char*)dest + templateEntry->offset));
        }
    }

    if (kind & SEC_ASN1_CHOICE)
    {
        
        




        unsigned choiceIndex = 1;
        const SEC_ASN1Template* choiceEntry;
        while ( (choiceEntry = &templateEntry[choiceIndex++]) && (choiceEntry->kind))
        {
            if ( (SECSuccess == MatchComponentType(choiceEntry, item, match,
                                (void*)((char*)dest + choiceEntry->offset))) &&
                 (PR_TRUE == *match) )
            {
                return SECSuccess;
            }
        }
	
        *match = PR_FALSE;
        return SECSuccess;
    }

    if (kind & SEC_ASN1_ANY)
    {
        
        *match = PR_TRUE;
        return SECSuccess;
    }

    if ( (0 == ((unsigned char)kind & SEC_ASN1_TAGNUM_MASK)) &&
         (!(kind & SEC_ASN1_EXPLICIT)) &&
         ( ( (kind & SEC_ASN1_SAVE) ||
             (kind & SEC_ASN1_SKIP) ) &&
           (!(kind & SEC_ASN1_OPTIONAL)) 
         )
       )
    {
        





        *match = PR_TRUE;
        return SECSuccess;
    }

    
    if ( (tag & SEC_ASN1_CLASS_MASK) !=
         (((unsigned char)kind) & SEC_ASN1_CLASS_MASK) )
    {
#ifdef DEBUG
        
        unsigned char tagclass = tag & SEC_ASN1_CLASS_MASK;
        unsigned char expectedclass = (unsigned char)kind & SEC_ASN1_CLASS_MASK;
        tagclass = tagclass;
        expectedclass = expectedclass;
#endif
        *match = PR_FALSE;
        return SECSuccess;
    }

    
    if ( ((unsigned char)kind & SEC_ASN1_TAGNUM_MASK) !=
         (tag & SEC_ASN1_TAGNUM_MASK))
    {
        *match = PR_FALSE;
        return SECSuccess;
    }

    
    switch (tag & SEC_ASN1_CLASS_MASK)
    {
    case SEC_ASN1_UNIVERSAL:
        

        switch (tag & SEC_ASN1_TAGNUM_MASK)
        {
        case SEC_ASN1_SEQUENCE:
        case SEC_ASN1_SET:
        case SEC_ASN1_EMBEDDED_PDV:
            
            
            if (tag & SEC_ASN1_CONSTRUCTED)
            {
                *match = PR_TRUE;
                return SECSuccess;
            }
            break;

        default:
            
            if (! (tag & SEC_ASN1_CONSTRUCTED))
            {
                *match = PR_TRUE;
                return SECSuccess;
            }
            break;
        }
        break;

    default:
        
        if ( (unsigned char)(kind & SEC_ASN1_METHOD_MASK) ==
             (tag & SEC_ASN1_METHOD_MASK) )
        {
            *match = PR_TRUE;
            return SECSuccess;
        }
        
        break;
    }

    *match = PR_FALSE;
    return SECSuccess;
}

#ifdef DEBUG

static SECStatus CheckSequenceTemplate(const SEC_ASN1Template* sequenceTemplate)
{
    SECStatus rv = SECSuccess;
    const SEC_ASN1Template* sequenceEntry = NULL;
    unsigned long seqIndex = 0;
    unsigned long lastEntryIndex = 0;
    unsigned long ambiguityIndex = 0;
    PRBool foundAmbiguity = PR_FALSE;

    do
    {
        sequenceEntry = &sequenceTemplate[seqIndex++];
        if (sequenceEntry->kind)
        {
            

            

            if ( (PR_FALSE == foundAmbiguity) &&
                 (sequenceEntry->kind & SEC_ASN1_OPTIONAL) &&
                 (sequenceEntry->kind & SEC_ASN1_ANY) )
            {
                foundAmbiguity = PR_TRUE;
                ambiguityIndex = seqIndex - 1;
            }
        }
    } while (sequenceEntry->kind);

    lastEntryIndex = seqIndex - 2;

    if (PR_FALSE != foundAmbiguity)
    {
        if (ambiguityIndex < lastEntryIndex)
        {
            
            PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
            rv = SECFailure;
        }
    }

    


    return rv;
}

#endif

static SECStatus DecodeItem(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag);

static SECStatus DecodeSequence(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem source;
    SECItem sequence;
    const SEC_ASN1Template* sequenceTemplate = &(templateEntry[1]);
    const SEC_ASN1Template* sequenceEntry = NULL;
    unsigned long seqindex = 0;

#ifdef DEBUG
    
    rv = CheckSequenceTemplate(sequenceTemplate);
#endif

    source = *src;

    
    if (SECSuccess == rv)
    {
        rv = GetItem(&source, &sequence, PR_FALSE);
    }

    
    if (SECSuccess == rv)
    do
    {
        sequenceEntry = &sequenceTemplate[seqindex++];
        if ( (sequenceEntry && sequenceEntry->kind) &&
             (sequenceEntry->kind != SEC_ASN1_SKIP_REST) )
        {
            rv = DecodeItem(dest, sequenceEntry, &sequence, arena, PR_TRUE);
        }
    } while ( (SECSuccess == rv) &&
              (sequenceEntry->kind &&
               sequenceEntry->kind != SEC_ASN1_SKIP_REST) );
    

    if (SECSuccess == rv && sequence.len &&
        sequenceEntry && sequenceEntry->kind != SEC_ASN1_SKIP_REST)
    {
        


        PORT_SetError(SEC_ERROR_BAD_DER);
        rv = SECFailure;
    }

    return rv;
}

static SECStatus DecodeInline(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    const SEC_ASN1Template* inlineTemplate = 
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
    return DecodeItem((void*)((char*)dest + templateEntry->offset),
                            inlineTemplate, src, arena, checkTag);
}

static SECStatus DecodePointer(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    const SEC_ASN1Template* ptrTemplate = 
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
    void* subdata = PORT_ArenaZAlloc(arena, ptrTemplate->size);
    *(void**)((char*)dest + templateEntry->offset) = subdata;
    if (subdata)
    {
        return DecodeItem(subdata, ptrTemplate, src, arena, checkTag);
    }
    else
    {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
}

static SECStatus DecodeImplicit(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    if (templateEntry->kind & SEC_ASN1_POINTER)
    {
        return DecodePointer((void*)((char*)dest ),
                             templateEntry, src, arena, PR_FALSE);
    }
    else
    {
        return DecodeInline((void*)((char*)dest ),
                             templateEntry, src, arena, PR_FALSE);
    }
}

static SECStatus DecodeChoice(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem choice;
    const SEC_ASN1Template* choiceTemplate = &(templateEntry[1]);
    const SEC_ASN1Template* choiceEntry = NULL;
    unsigned long choiceindex = 0;

    


    

    
    do
    {
        choice = *src;
        choiceEntry = &choiceTemplate[choiceindex++];
        if (choiceEntry->kind)
        {
            rv = DecodeItem(dest, choiceEntry, &choice, arena, PR_TRUE);
        }
    } while ( (SECFailure == rv) && (choiceEntry->kind));

    if (SECFailure == rv)
    {
        
        PORT_SetError(SEC_ERROR_BAD_DER);
    }
    else
    {
        
        int *which = (int *)((char *)dest + templateEntry->offset);
        *which = (int)choiceEntry->size;
    }

    
    
    if (SECSuccess == rv && choice.len)
    {
        
        PORT_SetError(SEC_ERROR_BAD_DER);
        rv = SECFailure;
    }
    return rv;
}

static SECStatus DecodeGroup(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem source;
    SECItem group;
    PRUint32 totalEntries = 0;
    PRUint32 entryIndex = 0;
    void** entries = NULL;

    const SEC_ASN1Template* subTemplate =
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);

    source = *src;

    
    if (SECSuccess == rv)
    {
        rv = GetItem(&source, &group, PR_FALSE);
    }

    
    if (SECSuccess == rv)
    {
        



        SECItem counter = group;
        do
        {
            SECItem anitem;
            rv = GetItem(&counter, &anitem, PR_TRUE);
            if (SECSuccess == rv && (anitem.len) )
            {
                totalEntries++;
            }
        }  while ( (SECSuccess == rv) && (counter.len) );

        if (SECSuccess == rv)
        {
            
            
            entries = (void**)PORT_ArenaZAlloc(arena, sizeof(void*)*
                                          (totalEntries + 1 ) + 
                                          subTemplate->size*totalEntries); 

            if (entries)
            {
                entries[totalEntries] = NULL; 
            }
            else
            {
                PORT_SetError(SEC_ERROR_NO_MEMORY);
                rv = SECFailure;
            }
            if (SECSuccess == rv)
            {
                void* entriesData = (unsigned char*)entries + (unsigned long)(sizeof(void*)*(totalEntries + 1 ));
                
                PRUint32 entriesIndex = 0;
                for (entriesIndex = 0;entriesIndex<totalEntries;entriesIndex++)
                {
                    entries[entriesIndex] =
                        (char*)entriesData + (subTemplate->size*entriesIndex);
                }
            }
        }
    }

    if (SECSuccess == rv && totalEntries)
    do
    {
        if (!(entryIndex<totalEntries))
        {
            rv = SECFailure;
            break;
        }
        rv = DecodeItem(entries[entryIndex++], subTemplate, &group, arena, PR_TRUE);
    } while ( (SECSuccess == rv) && (group.len) );
        
    
    memcpy(((char*)dest + templateEntry->offset), &entries, sizeof(void**));

    return rv;
}

static SECStatus DecodeExplicit(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem subItem;
    SECItem constructed = *src;

    rv = GetItem(&constructed, &subItem, PR_FALSE);

    if (SECSuccess == rv)
    {
        if (templateEntry->kind & SEC_ASN1_POINTER)
        {
            rv = DecodePointer(dest, templateEntry, &subItem, arena, PR_TRUE);
        }
        else
        {
            rv = DecodeInline(dest, templateEntry, &subItem, arena, PR_TRUE);
        }
    }

    return rv;
}



static SECStatus DecodeItem(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    SECStatus rv = SECSuccess;
    SECItem temp;
    SECItem mark;
    PRBool pop = PR_FALSE;
    PRBool decode = PR_TRUE;
    PRBool save = PR_FALSE;
    unsigned long kind;
    PRBool match = PR_TRUE;
    PRBool optional = PR_FALSE;

    PR_ASSERT(src && dest && templateEntry && arena);
#if 0
    if (!src || !dest || !templateEntry || !arena)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        rv = SECFailure;
    }
#endif

    if (SECSuccess == rv)
    {
        
        kind = templateEntry->kind;
        optional = (0 != (kind & SEC_ASN1_OPTIONAL));
        if (!kind)
        {
            PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
            rv = SECFailure;
        }
    }

    if (SECSuccess == rv)
    {
#ifdef DEBUG
        if (kind & SEC_ASN1_DEBUG_BREAK)
        {
            






            PRBool dontassert = PR_FALSE;
            PR_ASSERT(dontassert); 
        }
#endif

        if ((kind & SEC_ASN1_SKIP) ||
            (kind & SEC_ASN1_SAVE))
        {
            
            decode = PR_FALSE;
        }
    
        if (kind & (SEC_ASN1_SAVE | SEC_ASN1_OPTIONAL))
        {
            

            mark = *src;
            if (kind & SEC_ASN1_SAVE)
            {
                save = PR_TRUE;
                if (0 == (kind & SEC_ASN1_SKIP))
                {
                    




                    pop = PR_TRUE;
                }
            }
        }

        rv = GetItem(src, &temp, PR_TRUE);
    }

    if (SECSuccess == rv)
    {
        

        if (PR_TRUE == checkTag)

        {
            rv = MatchComponentType(templateEntry, &temp, &match, dest);
        }

        if ( (SECSuccess == rv) && (PR_TRUE != match) )
        {
            if (kind & SEC_ASN1_OPTIONAL)
            {

                
                
                pop = PR_TRUE;
                decode = PR_FALSE;
                save = PR_FALSE;
            }
            else
            {
                
                PORT_SetError(SEC_ERROR_BAD_DER);
                rv = SECFailure;
            }
        }
    }

    if ((SECSuccess == rv) && (PR_TRUE == decode))
    {
        
        
        
        if (kind & SEC_ASN1_INLINE)
        {
            
            rv = DecodeInline(dest, templateEntry, &temp , arena, PR_TRUE);
        }

        else
        if (kind & SEC_ASN1_EXPLICIT)
        {
            rv = DecodeExplicit(dest, templateEntry, &temp, arena);
        }
        else
        if ( (SEC_ASN1_UNIVERSAL != (kind & SEC_ASN1_CLASS_MASK)) &&

              (!(kind & SEC_ASN1_EXPLICIT)))
        {

            
            rv = DecodeImplicit(dest, templateEntry, &temp , arena);
        }
        else
        if (kind & SEC_ASN1_POINTER)
        {
            rv = DecodePointer(dest, templateEntry, &temp, arena, PR_TRUE);
        }
        else
        if (kind & SEC_ASN1_CHOICE)
        {
            rv = DecodeChoice(dest, templateEntry, &temp, arena);
        }
        else
        if (kind & SEC_ASN1_ANY)
        {
            
            save = PR_TRUE;
            if (kind & SEC_ASN1_INNER)
            {
                
                SECItem newtemp = temp;
                rv = GetItem(&newtemp, &temp, PR_FALSE);
            }
        }
        else
        if (kind & SEC_ASN1_GROUP)
        {
            if ( (SEC_ASN1_SEQUENCE == (kind & SEC_ASN1_TAGNUM_MASK)) ||
                 (SEC_ASN1_SET == (kind & SEC_ASN1_TAGNUM_MASK)) )
            {
                rv = DecodeGroup(dest, templateEntry, &temp , arena);
            }
            else
            {
                
                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                rv = SECFailure;
            }
        }
        else
        if (SEC_ASN1_SEQUENCE == (kind & SEC_ASN1_TAGNUM_MASK))
        {
            
            rv = DecodeSequence(dest, templateEntry, &temp , arena);
        }
        else
        {
            
            
            SECItem newtemp = temp;
            rv = GetItem(&newtemp, &temp, PR_FALSE);
            save = PR_TRUE;
            if ((SECSuccess == rv) &&
                SEC_ASN1_UNIVERSAL == (kind & SEC_ASN1_CLASS_MASK))
            {
                unsigned long tagnum = kind & SEC_ASN1_TAGNUM_MASK;
                if ( temp.len == 0 && (tagnum == SEC_ASN1_BOOLEAN ||
                                       tagnum == SEC_ASN1_INTEGER ||
                                       tagnum == SEC_ASN1_BIT_STRING ||
                                       tagnum == SEC_ASN1_OBJECT_ID ||
                                       tagnum == SEC_ASN1_ENUMERATED ||
                                       tagnum == SEC_ASN1_UTC_TIME ||
                                       tagnum == SEC_ASN1_GENERALIZED_TIME) )
                {
                    
                    PORT_SetError(SEC_ERROR_BAD_DER);
                    rv = SECFailure;
                }
                else
                switch (tagnum)
                {
                
                case SEC_ASN1_INTEGER:
                    {
                        


                        SECItem* destItem = (SECItem*) ((char*)dest +
                                            templateEntry->offset);
                        if (destItem && (siUnsignedInteger == destItem->type))
                        {
                            while (temp.len > 1 && temp.data[0] == 0)
                            {              
                                temp.data++;
                                temp.len--;
                            }
                        }
                        break;
                    }

                case SEC_ASN1_BIT_STRING:
                    {
                        

                        temp.len = (temp.len-1)*8 - (temp.data[0] & 0x7);
                        temp.data++;
                        break;
                    }

                default:
                    {
                        break;
                    }
                }
            }
        }
    }

    if ((SECSuccess == rv) && (PR_TRUE == save))
    {
        SECItem* destItem = (SECItem*) ((char*)dest + templateEntry->offset);
        if (destItem)
        {
            



            destItem->data = temp.len ? temp.data : NULL;
            destItem->len = temp.len;
        }
        else
        {
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
            rv = SECFailure;
        }
    }

    if (PR_TRUE == pop)
    {
        
        *src = mark;
    }
    return rv;
}



SECStatus SEC_QuickDERDecodeItem(PLArenaPool* arena, void* dest,
                     const SEC_ASN1Template* templateEntry,
                     const SECItem* src)
{
    SECStatus rv = SECSuccess;
    SECItem newsrc;

    if (!arena || !templateEntry || !src)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        rv = SECFailure;
    }

    if (SECSuccess == rv)
    {
        newsrc = *src;
        rv = DecodeItem(dest, templateEntry, &newsrc, arena, PR_TRUE);
        if (SECSuccess == rv && newsrc.len)
        {
            rv = SECFailure;
            PORT_SetError(SEC_ERROR_EXTRA_INPUT);
        }
    }

    return rv;
}

