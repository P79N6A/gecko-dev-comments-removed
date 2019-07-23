











































#ifndef _SECASN1_H_
#define _SECASN1_H_

#include "utilrename.h"
#include "plarena.h"

#include "seccomon.h"
#include "secasn1t.h"



SEC_BEGIN_PROTOS









extern SEC_ASN1DecoderContext *SEC_ASN1DecoderStart(PLArenaPool *pool,
						    void *dest,
						    const SEC_ASN1Template *t);


extern SECStatus SEC_ASN1DecoderUpdate(SEC_ASN1DecoderContext *cx,
				       const char *buf,
				       unsigned long len);

extern SECStatus SEC_ASN1DecoderFinish(SEC_ASN1DecoderContext *cx);


extern void SEC_ASN1DecoderAbort(SEC_ASN1DecoderContext *cx, int error);

extern void SEC_ASN1DecoderSetFilterProc(SEC_ASN1DecoderContext *cx,
					 SEC_ASN1WriteProc fn,
					 void *arg, PRBool no_store);

extern void SEC_ASN1DecoderClearFilterProc(SEC_ASN1DecoderContext *cx);

extern void SEC_ASN1DecoderSetNotifyProc(SEC_ASN1DecoderContext *cx,
					 SEC_ASN1NotifyProc fn,
					 void *arg);

extern void SEC_ASN1DecoderClearNotifyProc(SEC_ASN1DecoderContext *cx);

extern SECStatus SEC_ASN1Decode(PLArenaPool *pool, void *dest,
				const SEC_ASN1Template *t,
				const char *buf, long len);








extern SECStatus SEC_ASN1DecodeItem(PLArenaPool *pool, void *dest,
				    const SEC_ASN1Template *t,
				    const SECItem *src);

extern SECStatus SEC_QuickDERDecodeItem(PLArenaPool* arena, void* dest,
                     const SEC_ASN1Template* templateEntry,
                     const SECItem* src);





extern SEC_ASN1EncoderContext *SEC_ASN1EncoderStart(const void *src,
						    const SEC_ASN1Template *t,
						    SEC_ASN1WriteProc fn,
						    void *output_arg);


extern SECStatus SEC_ASN1EncoderUpdate(SEC_ASN1EncoderContext *cx,
				       const char *buf,
				       unsigned long len);

extern void SEC_ASN1EncoderFinish(SEC_ASN1EncoderContext *cx);


extern void SEC_ASN1EncoderAbort(SEC_ASN1EncoderContext *cx, int error);

extern void SEC_ASN1EncoderSetNotifyProc(SEC_ASN1EncoderContext *cx,
					 SEC_ASN1NotifyProc fn,
					 void *arg);

extern void SEC_ASN1EncoderClearNotifyProc(SEC_ASN1EncoderContext *cx);

extern void SEC_ASN1EncoderSetStreaming(SEC_ASN1EncoderContext *cx);

extern void SEC_ASN1EncoderClearStreaming(SEC_ASN1EncoderContext *cx);

extern void sec_ASN1EncoderSetDER(SEC_ASN1EncoderContext *cx);

extern void sec_ASN1EncoderClearDER(SEC_ASN1EncoderContext *cx);

extern void SEC_ASN1EncoderSetTakeFromBuf(SEC_ASN1EncoderContext *cx);

extern void SEC_ASN1EncoderClearTakeFromBuf(SEC_ASN1EncoderContext *cx);

extern SECStatus SEC_ASN1Encode(const void *src, const SEC_ASN1Template *t,
				SEC_ASN1WriteProc output_proc,
				void *output_arg);

extern SECItem * SEC_ASN1EncodeItem(PLArenaPool *pool, SECItem *dest,
				    const void *src, const SEC_ASN1Template *t);

extern SECItem * SEC_ASN1EncodeInteger(PLArenaPool *pool,
				       SECItem *dest, long value);

extern SECItem * SEC_ASN1EncodeUnsignedInteger(PLArenaPool *pool,
					       SECItem *dest,
					       unsigned long value);

extern SECStatus SEC_ASN1DecodeInteger(SECItem *src,
				       unsigned long *value);









extern int SEC_ASN1LengthLength (unsigned long len);



extern int SEC_ASN1EncodeLength(unsigned char *buf,int value);












extern const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *inTemplate, void *thing,
			PRBool encoding);




extern PRBool SEC_ASN1IsTemplateSimple(const SEC_ASN1Template *theTemplate);














extern const SEC_ASN1Template SEC_AnyTemplate[];
extern const SEC_ASN1Template SEC_BitStringTemplate[];
extern const SEC_ASN1Template SEC_BMPStringTemplate[];
extern const SEC_ASN1Template SEC_BooleanTemplate[];
extern const SEC_ASN1Template SEC_EnumeratedTemplate[];
extern const SEC_ASN1Template SEC_GeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_IA5StringTemplate[];
extern const SEC_ASN1Template SEC_IntegerTemplate[];
extern const SEC_ASN1Template SEC_NullTemplate[];
extern const SEC_ASN1Template SEC_ObjectIDTemplate[];
extern const SEC_ASN1Template SEC_OctetStringTemplate[];
extern const SEC_ASN1Template SEC_PrintableStringTemplate[];
extern const SEC_ASN1Template SEC_T61StringTemplate[];
extern const SEC_ASN1Template SEC_UniversalStringTemplate[];
extern const SEC_ASN1Template SEC_UTCTimeTemplate[];
extern const SEC_ASN1Template SEC_UTF8StringTemplate[];
extern const SEC_ASN1Template SEC_VisibleStringTemplate[];

extern const SEC_ASN1Template SEC_PointerToAnyTemplate[];
extern const SEC_ASN1Template SEC_PointerToBitStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToBMPStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToBooleanTemplate[];
extern const SEC_ASN1Template SEC_PointerToEnumeratedTemplate[];
extern const SEC_ASN1Template SEC_PointerToGeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_PointerToIA5StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToIntegerTemplate[];
extern const SEC_ASN1Template SEC_PointerToNullTemplate[];
extern const SEC_ASN1Template SEC_PointerToObjectIDTemplate[];
extern const SEC_ASN1Template SEC_PointerToOctetStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToT61StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToUniversalStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_PointerToUTF8StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToVisibleStringTemplate[];

extern const SEC_ASN1Template SEC_SequenceOfAnyTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBitStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBMPStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBooleanTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfEnumeratedTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfGeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfIA5StringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfIntegerTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfNullTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfObjectIDTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfOctetStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfT61StringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfUniversalStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfUTF8StringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfVisibleStringTemplate[];

extern const SEC_ASN1Template SEC_SetOfAnyTemplate[];
extern const SEC_ASN1Template SEC_SetOfBitStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfBMPStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfBooleanTemplate[];
extern const SEC_ASN1Template SEC_SetOfEnumeratedTemplate[];
extern const SEC_ASN1Template SEC_SetOfGeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_SetOfIA5StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfIntegerTemplate[];
extern const SEC_ASN1Template SEC_SetOfNullTemplate[];
extern const SEC_ASN1Template SEC_SetOfObjectIDTemplate[];
extern const SEC_ASN1Template SEC_SetOfOctetStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfT61StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfUniversalStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_SetOfUTF8StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfVisibleStringTemplate[];




extern const SEC_ASN1Template SEC_SkipTemplate[];




SEC_ASN1_CHOOSER_DECLARE(SEC_AnyTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_BMPStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_BooleanTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_BitStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_GeneralizedTimeTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_IA5StringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_IntegerTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_NullTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_ObjectIDTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_OctetStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_UTCTimeTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_UTF8StringTemplate)

SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToAnyTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToOctetStringTemplate)

SEC_ASN1_CHOOSER_DECLARE(SEC_SetOfAnyTemplate)

SEC_ASN1_CHOOSER_DECLARE(SEC_EnumeratedTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToEnumeratedTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_SequenceOfAnyTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_SequenceOfObjectIDTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_SkipTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_UniversalStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_PrintableStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_T61StringTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToGeneralizedTimeTemplate)
SEC_END_PROTOS
#endif 
