















#include "hb-ot-shape-complex-indic-private.hh"


#define ISC_A	INDIC_SYLLABIC_CATEGORY_AVAGRAHA		/*  11 chars; Avagraha */
#define ISC_Bi	INDIC_SYLLABIC_CATEGORY_BINDU			/*  34 chars; Bindu */
#define ISC_C	INDIC_SYLLABIC_CATEGORY_CONSONANT		/* 123 chars; Consonant */
#define ISC_CD	INDIC_SYLLABIC_CATEGORY_CONSONANT_DEAD		/*   2 chars; Consonant_Dead */
#define ISC_CF	INDIC_SYLLABIC_CATEGORY_CONSONANT_FINAL		/*  17 chars; Consonant_Final */
#define ISC_CHL	INDIC_SYLLABIC_CATEGORY_CONSONANT_HEAD_LETTER	/*   1 chars; Consonant_Head_Letter */
#define ISC_CM	INDIC_SYLLABIC_CATEGORY_CONSONANT_MEDIAL	/*  12 chars; Consonant_Medial */
#define ISC_CP	INDIC_SYLLABIC_CATEGORY_CONSONANT_PLACEHOLDER	/*   4 chars; Consonant_Placeholder */
#define ISC_CR	INDIC_SYLLABIC_CATEGORY_CONSONANT_REPHA		/*   5 chars; Consonant_Repha */
#define ISC_CS	INDIC_SYLLABIC_CATEGORY_CONSONANT_SUBJOINED	/*  10 chars; Consonant_Subjoined */
#define ISC_ML	INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER	/*   1 chars; Modifying_Letter */
#define ISC_N	INDIC_SYLLABIC_CATEGORY_NUKTA			/*  12 chars; Nukta */
#define ISC_x	INDIC_SYLLABIC_CATEGORY_OTHER			/*   1 chars; Other */
#define ISC_RS	INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER	/*   1 chars; Register_Shifter */
#define ISC_TL	INDIC_SYLLABIC_CATEGORY_TONE_LETTER		/*   3 chars; Tone_Letter */
#define ISC_TM	INDIC_SYLLABIC_CATEGORY_TONE_MARK		/*  16 chars; Tone_Mark */
#define ISC_V	INDIC_SYLLABIC_CATEGORY_VIRAMA			/*  34 chars; Virama */
#define ISC_Vs	INDIC_SYLLABIC_CATEGORY_VISARGA			/*  25 chars; Visarga */
#define ISC_Vo	INDIC_SYLLABIC_CATEGORY_VOWEL			/*   5 chars; Vowel */
#define ISC_M	INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT		/* 165 chars; Vowel_Dependent */
#define ISC_VI	INDIC_SYLLABIC_CATEGORY_VOWEL_INDEPENDENT	/*  59 chars; Vowel_Independent */

#define IMC_B	INDIC_MATRA_CATEGORY_BOTTOM			/*  65 chars; Bottom */
#define IMC_BR	INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT		/*   2 chars; Bottom_And_Right */
#define IMC_I	INDIC_MATRA_CATEGORY_INVISIBLE			/*   6 chars; Invisible */
#define IMC_L	INDIC_MATRA_CATEGORY_LEFT			/*  30 chars; Left */
#define IMC_LR	INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT		/*   8 chars; Left_And_Right */
#define IMC_x	INDIC_MATRA_CATEGORY_NOT_APPLICABLE		/*   1 chars; Not_Applicable */
#define IMC_O	INDIC_MATRA_CATEGORY_OVERSTRUCK			/*   2 chars; Overstruck */
#define IMC_R	INDIC_MATRA_CATEGORY_RIGHT			/*  75 chars; Right */
#define IMC_T	INDIC_MATRA_CATEGORY_TOP			/*  83 chars; Top */
#define IMC_TB	INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM		/*   6 chars; Top_And_Bottom */
#define IMC_TBR	INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT	/*   1 chars; Top_And_Bottom_And_Right */
#define IMC_TL	INDIC_MATRA_CATEGORY_TOP_AND_LEFT		/*   4 chars; Top_And_Left */
#define IMC_TLR	INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT	/*   2 chars; Top_And_Left_And_Right */
#define IMC_TR	INDIC_MATRA_CATEGORY_TOP_AND_RIGHT		/*   8 chars; Top_And_Right */
#define IMC_VOL	INDIC_MATRA_CATEGORY_VISUAL_ORDER_LEFT		/*   5 chars; Visual_Order_Left */

#define _(S,M) INDIC_COMBINE_CATEGORIES (ISC_##S, IMC_##M)


static const INDIC_TABLE_ELEMENT_TYPE indic_table[] = {


#define indic_offset_0x0900 0


  

   _(Bi,x), _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,R),  _(N,x),  _(A,x),  _(M,R),  _(M,L),
    _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T),
    _(M,T),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(V,B),  _(M,L),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,T),  _(M,B),  _(M,B),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x), _(VI,x),
   _(VI,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(A,x),  _(M,R),  _(M,L),
    _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(M,L),
    _(M,L),  _(x,x),  _(x,x), _(M,LR), _(M,LR),  _(V,B), _(CD,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(VI,x),
   _(VI,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(x,x),  _(M,R),  _(M,L),
    _(M,R),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,T),
    _(M,T),  _(x,x),  _(x,x),  _(M,T),  _(M,T),  _(V,B),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Bi,x),  _(x,x), _(CP,x), _(CP,x),  _(x,x), _(CM,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x),
   _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(A,x),  _(M,R),  _(M,L),
    _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(M,T),  _(x,x),  _(M,T),
    _(M,T), _(M,TR),  _(x,x),  _(M,R),  _(M,R),  _(V,B),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x), _(VI,x),
   _(VI,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(A,x),  _(M,R),  _(M,T),
    _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(M,L),
   _(M,TL),  _(x,x),  _(x,x), _(M,LR),_(M,TLR),  _(V,B),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,T), _(M,TR),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x), _(Bi,x), _(ML,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(x,x),  _(x,x),
    _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),
    _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),
    _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),  _(M,R),
    _(M,T),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(M,L),  _(M,L),
    _(M,L),  _(x,x), _(M,LR), _(M,LR), _(M,LR),  _(V,T),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(A,x),  _(M,T),  _(M,T),
    _(M,T),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(x,x),  _(M,T),  _(M,T),
   _(M,TB),  _(x,x),  _(M,T),  _(M,T),  _(M,T),  _(V,T),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,T),  _(M,B),  _(x,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(A,x),  _(M,R),  _(M,T),
   _(M,TR),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(x,x),  _(M,T), _(M,TR),
   _(M,TR),  _(x,x), _(M,TR), _(M,TR),  _(M,T),  _(V,T),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),  _(M,R),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(A,x),  _(M,R),  _(M,R),
    _(M,R),  _(M,R),  _(M,R),  _(M,B),  _(M,B),  _(x,x),  _(M,L),  _(M,L),
    _(M,L),  _(x,x), _(M,LR), _(M,LR), _(M,LR),  _(V,T), _(CR,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(CD,x), _(CD,x), _(CD,x), _(CD,x), _(CD,x), _(CD,x),

  

    _(x,x),  _(x,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x),
    _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(x,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),
    _(x,x),  _(x,x),  _(V,T),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,B),  _(x,x),  _(M,B),  _(x,x),
    _(M,R),  _(M,L), _(M,TL),  _(M,L), _(M,LR), _(M,LR), _(M,LR),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(M,R),  _(M,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),
    _(M,R),  _(M,T),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(V,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
  _(M,VOL),_(M,VOL),_(M,VOL),_(M,VOL),_(M,VOL),  _(M,R),  _(x,x),  _(M,T),
   _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(x,x), _(Bi,x),  _(V,T),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(x,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(x,x),  _(C,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(x,x),  _(C,x),
    _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),
    _(M,R),  _(M,T),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(x,x),  _(M,T), _(CM,x), _(CM,x),  _(x,x),  _(x,x),
  _(M,VOL),_(M,VOL),_(M,VOL),_(M,VOL),_(M,VOL),  _(x,x),  _(x,x),  _(x,x),
   _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(x,x), _(Bi,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(M,B),  _(M,T), _(M,TB),  _(M,B),  _(M,B), _(M,TB), _(M,TB),
   _(M,TB), _(M,TB),  _(M,T),  _(M,T),  _(M,T),  _(M,T), _(Bi,x), _(Vs,x),
    _(M,T), _(M,TB), _(Bi,x), _(Bi,x),  _(V,B),  _(A,x),  _(x,x),  _(x,x),
  _(CHL,x),_(CHL,x),_(CHL,x),_(CHL,x),_(CHL,x), _(CS,x), _(CS,x), _(CS,x),
   _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),
    _(x,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),
   _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),
   _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),
   _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),
   _(CS,x), _(CS,x), _(CS,x), _(CS,x), _(CS,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,B),
    _(M,B),  _(M,L),  _(M,T),  _(M,T),  _(M,T),  _(M,T), _(Bi,x), _(TM,x),
   _(Vs,x),  _(V,I),  _(V,T), _(CM,x), _(CM,x), _(CM,x), _(CM,x),  _(C,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(M,R),  _(M,R),
    _(M,B),  _(M,B),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(CM,x), _(CM,x),
   _(CM,x),  _(C,x),  _(M,R), _(TM,x), _(TM,x),  _(C,x),  _(C,x),  _(M,R),
    _(M,R), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(C,x),  _(C,x),
    _(C,x),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x), _(CM,x),  _(M,R),  _(M,L),  _(M,T),  _(M,T), _(TM,x),
   _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(C,x), _(TM,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(TM,x), _(TM,x),  _(M,R),  _(M,T),  _(x,x),  _(x,x),

#define indic_offset_0x1700 1952


  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,B),  _(V,B),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,B),  _(V,B),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(M,T),  _(M,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x),  _(M,R),  _(M,T),
    _(M,T),  _(M,T),  _(M,T),  _(M,B),  _(M,B),  _(M,B), _(M,TL),_(M,TLR),
   _(M,LR),  _(M,L),  _(M,L),  _(M,L), _(M,LR), _(M,LR), _(Bi,x), _(Vs,x),
    _(M,R), _(RS,x), _(RS,x),  _(x,x), _(CR,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(V,T),  _(V,I),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(A,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x1900 2208


  

   _(CP,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),
    _(M,T),  _(M,T),  _(M,B),  _(M,R),  _(M,R), _(M,TR), _(M,TR),  _(M,T),
    _(M,T), _(CS,x), _(CS,x), _(CS,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(CF,x), _(CF,x), _(Bi,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x),
   _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x),  _(x,x),  _(x,x),
   _(TL,x), _(TL,x), _(TL,x), _(TL,x), _(TL,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,L),  _(M,L),  _(M,L),
    _(M,R),  _(M,R),  _(M,L),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),
    _(M,R), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(TM,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,T),
    _(M,B),  _(M,L),  _(M,R),  _(M,L),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x), _(CM,x), _(CM,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),  _(x,x),
    _(V,I),  _(M,R),  _(M,T),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,T),
    _(M,T),  _(M,B),  _(M,B),  _(M,T),  _(M,B),  _(M,R),  _(M,L),  _(M,L),
    _(M,L),  _(M,L),  _(M,L),  _(M,T),  _(M,T), _(TM,x), _(TM,x), _(TM,x),
   _(TM,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x1b00 2640


  

   _(Bi,x), _(Bi,x), _(Bi,x), _(CR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(N,x),  _(M,R),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(M,B), _(M,BR), _(M,TB),_(M,TBR),  _(M,L),  _(M,L),
   _(M,LR), _(M,LR),  _(M,T), _(M,TR),  _(V,R),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(CR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x), _(CS,x), _(CS,x), _(CS,x),  _(M,T),  _(M,B),  _(M,L),  _(M,R),
    _(M,T),  _(M,T),  _(V,R),  _(V,x), _(CS,x), _(CS,x),  _(C,x),  _(C,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(A,x),  _(C,x),  _(C,x),  _(C,x), _(CF,x), _(CF,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x),  _(N,x),  _(M,x),
    _(M,x),  _(M,x),  _(M,x),  _(M,x),  _(M,x),  _(M,x),  _(M,x),  _(M,x),
   _(CF,x), _(CF,x),  _(V,R),  _(V,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x), _(CS,x), _(CS,x),  _(M,R),  _(M,L),
    _(M,L), _(M,TL),  _(M,R),  _(M,R),  _(M,B), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(Bi,x), _(Bi,x),  _(x,x),  _(N,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),

#define indic_offset_0x1cd0 2976


  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(Vs,x), _(Vs,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0xa800 3024


  

   _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(V,T),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(Bi,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(M,R),  _(M,R),  _(M,B),  _(M,T),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(Vo,x), _(Vo,x),
   _(Vo,x), _(Vo,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(Vo,x), _(CS,x),
   _(CS,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x), _(CS,x),  _(C,x), _(Bi,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x), _(CF,x),  _(M,R),  _(M,R),  _(M,R),
    _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(M,R),
    _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(V,B),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x),
   _(Vo,x), _(Vo,x), _(Vo,x), _(TM,x), _(TM,x), _(TM,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,B),
    _(M,B),  _(M,B),  _(M,T),  _(M,B),  _(M,B),  _(M,B),  _(M,B), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x),  _(V,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(Bi,x), _(CR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(N,x),  _(M,R),  _(M,R),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(M,L),  _(M,L),  _(M,T), _(CS,x), _(CM,x), _(CM,x),
   _(V,BR),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(M,B),  _(M,T),  _(M,L),
    _(M,L),  _(M,T),  _(M,B), _(CM,x), _(CM,x), _(CM,x), _(CM,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(C,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,T),  _(M,R),  _(M,T),  _(M,T),  _(M,B),_(M,VOL),_(M,VOL),  _(M,T),
    _(M,T),_(M,VOL),  _(M,R),_(M,VOL),_(M,VOL),  _(M,R),  _(M,T), _(TM,x),
   _(TL,x), _(TM,x), _(TL,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(M,L),  _(M,B),  _(M,T),  _(M,L),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Vs,x),  _(V,I),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0xabc0 3792


  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x),
    _(C,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x),  _(M,R),  _(M,R),  _(M,T),  _(M,R),  _(M,R),
    _(M,B),  _(M,R),  _(M,R),  _(x,x), _(TM,x),  _(V,B),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x10a00 3856


  

    _(C,x),  _(M,O),  _(M,B),  _(M,B),  _(x,x),  _(M,T),  _(M,O),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,B),  _(x,x), _(Bi,x), _(Vs,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(V,I),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11000 3952


  

   _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(M,B),  _(M,B),  _(M,B),  _(M,B),
    _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(V,T),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,R),  _(M,L),  _(M,R),  _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,R),
    _(M,R),  _(V,B),  _(N,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11100 4160


  

   _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,T),
    _(M,T),  _(M,T),  _(M,B),  _(M,B),  _(M,L),  _(M,T), _(M,TB), _(M,TB),
    _(M,T),  _(M,B),  _(M,B),  _(V,I),  _(V,T),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11180 4240


  

   _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(M,R),  _(M,L),  _(M,R),  _(M,B),  _(M,B),
    _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T), _(M,TR),
    _(V,R),  _(A,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11680 4336


  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(Bi,x), _(Vs,x),  _(M,T),  _(M,L),  _(M,R),
    _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(V,T),  _(N,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_total 4416

}; 

INDIC_TABLE_ELEMENT_TYPE
hb_indic_get_categories (hb_codepoint_t u)
{
  if (0x0900 <= u && u <= 0x10A0) return indic_table[u - 0x0900 + indic_offset_0x0900];
  if (0x1700 <= u && u <= 0x1800) return indic_table[u - 0x1700 + indic_offset_0x1700];
  if (0x1900 <= u && u <= 0x1AB0) return indic_table[u - 0x1900 + indic_offset_0x1900];
  if (0x1B00 <= u && u <= 0x1C50) return indic_table[u - 0x1B00 + indic_offset_0x1b00];
  if (0x1CD0 <= u && u <= 0x1D00) return indic_table[u - 0x1CD0 + indic_offset_0x1cd0];
  if (0xA800 <= u && u <= 0xAB00) return indic_table[u - 0xA800 + indic_offset_0xa800];
  if (0xABC0 <= u && u <= 0xAC00) return indic_table[u - 0xABC0 + indic_offset_0xabc0];
  if (0x10A00 <= u && u <= 0x10A60) return indic_table[u - 0x10A00 + indic_offset_0x10a00];
  if (0x11000 <= u && u <= 0x110D0) return indic_table[u - 0x11000 + indic_offset_0x11000];
  if (0x11100 <= u && u <= 0x11150) return indic_table[u - 0x11100 + indic_offset_0x11100];
  if (0x11180 <= u && u <= 0x111E0) return indic_table[u - 0x11180 + indic_offset_0x11180];
  if (0x11680 <= u && u <= 0x116D0) return indic_table[u - 0x11680 + indic_offset_0x11680];
  if (unlikely (u == 0x00A0)) return _(CP,x);
  if (unlikely (u == 0x25CC)) return _(CP,x);
  return _(x,x);
}

#undef _

#undef ISC_A
#undef ISC_Bi
#undef ISC_C
#undef ISC_CD
#undef ISC_CF
#undef ISC_CHL
#undef ISC_CM
#undef ISC_CP
#undef ISC_CR
#undef ISC_CS
#undef ISC_ML
#undef ISC_N
#undef ISC_x
#undef ISC_RS
#undef ISC_TL
#undef ISC_TM
#undef ISC_V
#undef ISC_Vs
#undef ISC_Vo
#undef ISC_M
#undef ISC_VI

#undef IMC_B
#undef IMC_BR
#undef IMC_I
#undef IMC_L
#undef IMC_LR
#undef IMC_x
#undef IMC_O
#undef IMC_R
#undef IMC_T
#undef IMC_TB
#undef IMC_TBR
#undef IMC_TL
#undef IMC_TLR
#undef IMC_TR
#undef IMC_VOL


