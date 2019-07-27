















#include "hb-ot-shape-complex-indic-private.hh"


#define ISC_A	INDIC_SYLLABIC_CATEGORY_AVAGRAHA		/*  13 chars; Avagraha */
#define ISC_Bi	INDIC_SYLLABIC_CATEGORY_BINDU			/*  59 chars; Bindu */
#define ISC_BJN	INDIC_SYLLABIC_CATEGORY_BRAHMI_JOINING_NUMBER	/*  20 chars; Brahmi_Joining_Number */
#define ISC_Ca	INDIC_SYLLABIC_CATEGORY_CANTILLATION_MARK	/*  30 chars; Cantillation_Mark */
#define ISC_C	INDIC_SYLLABIC_CATEGORY_CONSONANT		/* 1744 chars; Consonant */
#define ISC_CD	INDIC_SYLLABIC_CATEGORY_CONSONANT_DEAD		/*   7 chars; Consonant_Dead */
#define ISC_CF	INDIC_SYLLABIC_CATEGORY_CONSONANT_FINAL		/*  61 chars; Consonant_Final */
#define ISC_CHL	INDIC_SYLLABIC_CATEGORY_CONSONANT_HEAD_LETTER	/*   5 chars; Consonant_Head_Letter */
#define ISC_CM	INDIC_SYLLABIC_CATEGORY_CONSONANT_MEDIAL	/*  19 chars; Consonant_Medial */
#define ISC_CP	INDIC_SYLLABIC_CATEGORY_CONSONANT_PLACEHOLDER	/*  11 chars; Consonant_Placeholder */
#define ISC_CPR	INDIC_SYLLABIC_CATEGORY_CONSONANT_PRECEDING_REPHA	/*   1 chars; Consonant_Preceding_Repha */
#define ISC_CS	INDIC_SYLLABIC_CATEGORY_CONSONANT_SUBJOINED	/*  61 chars; Consonant_Subjoined */
#define ISC_CSR	INDIC_SYLLABIC_CATEGORY_CONSONANT_SUCCEEDING_REPHA	/*   4 chars; Consonant_Succeeding_Repha */
#define ISC_GM	INDIC_SYLLABIC_CATEGORY_GEMINATION_MARK		/*   2 chars; Gemination_Mark */
#define ISC_IS	INDIC_SYLLABIC_CATEGORY_INVISIBLE_STACKER	/*   7 chars; Invisible_Stacker */
#define ISC_ZWJ	INDIC_SYLLABIC_CATEGORY_JOINER			/*   1 chars; Joiner */
#define ISC_ML	INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER	/*   1 chars; Modifying_Letter */
#define ISC_ZWNJ	INDIC_SYLLABIC_CATEGORY_NON_JOINER		/*   1 chars; Non_Joiner */
#define ISC_N	INDIC_SYLLABIC_CATEGORY_NUKTA			/*  18 chars; Nukta */
#define ISC_Nd	INDIC_SYLLABIC_CATEGORY_NUMBER			/* 408 chars; Number */
#define ISC_NJ	INDIC_SYLLABIC_CATEGORY_NUMBER_JOINER		/*   1 chars; Number_Joiner */
#define ISC_x	INDIC_SYLLABIC_CATEGORY_OTHER			/*   1 chars; Other */
#define ISC_PK	INDIC_SYLLABIC_CATEGORY_PURE_KILLER		/*  15 chars; Pure_Killer */
#define ISC_RS	INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER	/*   3 chars; Register_Shifter */
#define ISC_TL	INDIC_SYLLABIC_CATEGORY_TONE_LETTER		/*   7 chars; Tone_Letter */
#define ISC_TM	INDIC_SYLLABIC_CATEGORY_TONE_MARK		/*  62 chars; Tone_Mark */
#define ISC_V	INDIC_SYLLABIC_CATEGORY_VIRAMA			/*  22 chars; Virama */
#define ISC_Vs	INDIC_SYLLABIC_CATEGORY_VISARGA			/*  29 chars; Visarga */
#define ISC_Vo	INDIC_SYLLABIC_CATEGORY_VOWEL			/*  30 chars; Vowel */
#define ISC_M	INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT		/* 553 chars; Vowel_Dependent */
#define ISC_VI	INDIC_SYLLABIC_CATEGORY_VOWEL_INDEPENDENT	/* 395 chars; Vowel_Independent */

#define IMC_B	INDIC_MATRA_CATEGORY_BOTTOM			/* 142 chars; Bottom */
#define IMC_BR	INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT		/*   2 chars; Bottom_And_Right */
#define IMC_L	INDIC_MATRA_CATEGORY_LEFT			/*  57 chars; Left */
#define IMC_LR	INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT		/*  21 chars; Left_And_Right */
#define IMC_x	INDIC_MATRA_CATEGORY_NOT_APPLICABLE		/*   1 chars; Not_Applicable */
#define IMC_O	INDIC_MATRA_CATEGORY_OVERSTRUCK			/*   2 chars; Overstruck */
#define IMC_R	INDIC_MATRA_CATEGORY_RIGHT			/* 163 chars; Right */
#define IMC_T	INDIC_MATRA_CATEGORY_TOP			/* 169 chars; Top */
#define IMC_TB	INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM		/*  10 chars; Top_And_Bottom */
#define IMC_TBR	INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT	/*   1 chars; Top_And_Bottom_And_Right */
#define IMC_TL	INDIC_MATRA_CATEGORY_TOP_AND_LEFT		/*   6 chars; Top_And_Left */
#define IMC_TLR	INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT	/*   4 chars; Top_And_Left_And_Right */
#define IMC_TR	INDIC_MATRA_CATEGORY_TOP_AND_RIGHT		/*  13 chars; Top_And_Right */
#define IMC_VOL	INDIC_MATRA_CATEGORY_VISUAL_ORDER_LEFT		/*  15 chars; Visual_Order_Left */

#define _(S,M) INDIC_COMBINE_CATEGORIES (ISC_##S, IMC_##M)


static const INDIC_TABLE_ELEMENT_TYPE indic_table[] = {


#define indic_offset_0x0028u 0


  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(CP,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x00d0u 24


  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(CP,x),

#define indic_offset_0x0900u 32


  

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
    _(x,x), _(TM,x), _(TM,x),  _(x,x),  _(x,x),  _(M,T),  _(M,B),  _(M,B),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),

  

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
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
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
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Bi,x), _(GM,T), _(CP,x), _(CP,x),  _(x,x), _(CM,x),  _(x,x),  _(x,x),
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
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
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
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
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
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(A,x),  _(M,T),  _(M,T),
    _(M,T),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(x,x),  _(M,T),  _(M,T),
   _(M,TB),  _(x,x),  _(M,T),  _(M,T),  _(M,T),  _(V,T),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,T),  _(M,B),  _(x,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
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
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x),
   _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(A,x),  _(M,R),  _(M,R),
    _(M,R),  _(M,R),  _(M,R),  _(M,B),  _(M,B),  _(x,x),  _(M,L),  _(M,L),
    _(M,L),  _(x,x), _(M,LR), _(M,LR), _(M,LR),  _(V,T),_(CPR,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,B),  _(M,B),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
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
    _(M,R),  _(M,L), _(M,TL),  _(M,L), _(M,LR),_(M,TLR), _(M,LR),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(M,R),  _(M,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x1000u 1304


  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,B),
    _(M,B),  _(M,L),  _(M,T),  _(M,T),  _(M,T),  _(M,T), _(Bi,x), _(TM,x),
   _(Vs,x), _(IS,x), _(PK,T), _(CM,x), _(CM,x), _(CM,x), _(CM,x),  _(C,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(CP,x),  _(x,x),
    _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(M,R),  _(M,R),
    _(M,B),  _(M,B),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(CM,x), _(CM,x),
   _(CM,x),  _(C,x),  _(M,R), _(TM,x), _(TM,x),  _(C,x),  _(C,x),  _(M,R),
    _(M,R), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(C,x),  _(C,x),
    _(C,x),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x), _(CM,x),  _(M,R),  _(M,L),  _(M,T),  _(M,T), _(TM,x),
   _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),  _(C,x), _(TM,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(TM,x), _(TM,x),  _(M,R),  _(M,T),  _(x,x),  _(x,x),

#define indic_offset_0x1700u 1464


  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,B), _(PK,B),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(M,T),  _(M,B), _(PK,B),  _(x,x),  _(x,x),  _(x,x),
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
    _(M,R), _(RS,T), _(RS,T), _(RS,T),_(CSR,T),  _(M,T),  _(M,T),  _(M,T),
    _(M,T), _(PK,T), _(IS,x),  _(M,T),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(A,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x1900u 1704


  

   _(CP,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),
    _(M,T),  _(M,T),  _(M,B),  _(M,R),  _(M,R), _(M,TR), _(M,TR),  _(M,T),
    _(M,T), _(CS,x), _(CS,x), _(CS,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(CF,x), _(CF,x), _(Bi,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),

  

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
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,T),
    _(M,B),  _(M,L),  _(M,R),  _(M,T),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x), _(CM,L), _(CM,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),  _(x,x),
   _(IS,x),  _(M,R),  _(M,T),  _(M,R),  _(M,R),  _(M,T),  _(M,T),  _(M,T),
    _(M,T),  _(M,B),  _(M,B),  _(M,T),  _(M,B),  _(M,R),  _(M,L),  _(M,L),
    _(M,L),  _(M,L),  _(M,L),  _(M,T),  _(M,T), _(TM,x), _(TM,x), _(TM,x),
   _(TM,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x1b00u 2120


  

   _(Bi,x), _(Bi,x), _(Bi,x),_(CSR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(N,x),  _(M,R),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(M,B), _(M,BR), _(M,TB),_(M,TBR),  _(M,L),  _(M,L),
   _(M,LR), _(M,LR),  _(M,T), _(M,TR),  _(V,R),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x),_(CSR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x), _(CS,x), _(CS,x), _(CS,x),  _(M,T),  _(M,B),  _(M,L),  _(M,R),
    _(M,T),  _(M,T), _(PK,R), _(IS,x), _(CS,x), _(CS,x),  _(C,x),  _(C,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(A,x),  _(C,x),  _(C,x),  _(C,x), _(CF,x), _(CF,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x),  _(N,x),  _(M,R),
    _(M,T),  _(M,T),  _(M,R),  _(M,R),  _(M,R),  _(M,T),  _(M,R),  _(M,T),
   _(CF,x), _(CF,x), _(PK,R), _(PK,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x), _(CS,x), _(CS,x),  _(M,R),  _(M,L),
    _(M,L), _(M,TL),  _(M,R),  _(M,R),  _(M,B), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(Bi,L), _(Bi,L),  _(x,x),  _(N,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),

#define indic_offset_0x1cd0u 2456


  

   _(TM,x), _(TM,x), _(TM,x),  _(x,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),
   _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x), _(TM,x),
   _(TM,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(Vs,x), _(Vs,x), _(TM,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x2008u 2496


  

    _(x,x),  _(x,x),  _(x,x),  _(x,x),_(ZWNJ,x),_(ZWJ,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x), _(CP,x), _(CP,x), _(CP,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0xa800u 2512


  

   _(VI,x), _(VI,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x), _(PK,T),  _(C,x),
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
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x),
   _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x),
   _(Ca,x), _(Ca,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x),
   _(Vo,x), _(Vo,x), _(Vo,x), _(TM,x), _(TM,x), _(TM,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,B),
    _(M,B),  _(M,B),  _(M,T),  _(M,B),  _(M,B),  _(M,B),  _(M,B), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(PK,R),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Bi,x), _(Bi,x),_(CSR,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(N,x),  _(M,R),  _(M,R),  _(M,T),  _(M,T),
    _(M,B),  _(M,B),  _(M,L),  _(M,L),  _(M,T), _(CS,x), _(CM,x), _(CM,x),
   _(V,BR),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(M,B),  _(M,T),  _(M,L),
    _(M,L),  _(M,T),  _(M,B), _(CM,x), _(CM,L), _(CM,x), _(CM,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(C,x), _(TM,x), _(TM,x), _(TM,x),  _(C,x),  _(C,x),

  

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
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(Vs,x), _(IS,x),  _(x,x),

#define indic_offset_0xabc0u 3272


  

    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(VI,x), _(VI,x),
    _(C,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x), _(CF,x),
   _(CF,x), _(CF,x), _(CF,x),  _(M,R),  _(M,R),  _(M,T),  _(M,R),  _(M,R),
    _(M,B),  _(M,R),  _(M,R),  _(x,x), _(TM,x), _(PK,B),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x10a00u 3336


  

    _(C,x),  _(M,O),  _(M,B),  _(M,B),  _(x,x),  _(M,T),  _(M,O),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,B),  _(x,x), _(Bi,x), _(Vs,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(IS,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),

#define indic_offset_0x11000u 3408


  

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
    _(x,x),  _(x,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),
  _(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),
  _(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x),_(BJN,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x), _(NJ,x),

  

   _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,R),  _(M,L),  _(M,R),  _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,R),
    _(M,R),  _(V,B),  _(N,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11100u 3600


  

   _(Bi,x), _(Bi,x), _(Vs,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,T),
    _(M,T),  _(M,T),  _(M,B),  _(M,B),  _(M,L),  _(M,T), _(M,TB), _(M,TB),
    _(M,T),  _(M,B),  _(M,B), _(IS,x), _(PK,T),  _(x,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x), _(Vo,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(N,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

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
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,R),  _(M,R),  _(M,R),  _(M,B),
    _(M,T),  _(M,T), _(M,TR), _(M,TR), _(Bi,x),  _(V,R),  _(N,x), _(GM,T),

#define indic_offset_0x112b0u 3912


  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x), _(Bi,x),
    _(M,R),  _(M,L),  _(M,R),  _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T),
    _(M,T),  _(N,x), _(PK,B),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

    _(x,x), _(Bi,x), _(Bi,x), _(Vs,x),  _(x,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(x,x),  _(x,x), _(VI,x),
   _(VI,x),  _(x,x),  _(x,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(x,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(x,x),  _(x,x),  _(N,x),  _(A,x),  _(M,R),  _(M,R),
    _(M,T),  _(M,R),  _(M,R),  _(M,R),  _(M,R),  _(x,x),  _(x,x),  _(M,L),
    _(M,L),  _(x,x),  _(x,x), _(M,LR), _(M,LR),  _(V,R),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(M,R),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(VI,x), _(VI,x),  _(M,R),  _(M,R),  _(x,x),  _(x,x), _(Ca,x), _(Ca,x),
   _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x),  _(x,x),  _(x,x),  _(x,x),
   _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x), _(Ca,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11480u 4112


  

    _(x,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,R),  _(M,L),  _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(M,B),
    _(M,B),  _(M,L),  _(M,T), _(M,TL), _(M,LR),  _(M,R), _(M,LR), _(Bi,x),
   _(Bi,x), _(Vs,x),  _(V,B),  _(N,x),  _(A,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11580u 4208


  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(M,R),
    _(M,L),  _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(x,x),  _(x,x),
    _(M,L), _(M,TL), _(M,LR),_(M,TLR), _(Bi,x), _(Bi,x), _(Vs,x),  _(V,B),
    _(N,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

#define indic_offset_0x11600u 4280


  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(M,R),  _(M,R),  _(M,R),  _(M,B),  _(M,B),  _(M,B),  _(M,B),  _(M,B),
    _(M,B),  _(M,T),  _(M,T),  _(M,R),  _(M,R), _(Bi,x), _(Vs,x),  _(V,B),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

  

   _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x), _(VI,x),
   _(VI,x), _(VI,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),  _(C,x),
    _(C,x),  _(C,x),  _(C,x), _(Bi,x), _(Vs,x),  _(M,T),  _(M,L),  _(M,R),
    _(M,B),  _(M,B),  _(M,T),  _(M,T),  _(M,T),  _(M,T),  _(V,T),  _(N,x),
    _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),
   _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x), _(Nd,x),
   _(Nd,x), _(Nd,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),  _(x,x),

}; 

INDIC_TABLE_ELEMENT_TYPE
hb_indic_get_categories (hb_codepoint_t u)
{
  switch (u >> 12)
  {
    case 0x0u:
      if (hb_in_range (u, 0x0028u, 0x0040u)) return indic_table[u - 0x0028u + indic_offset_0x0028u];
      if (hb_in_range (u, 0x00D0u, 0x00D8u)) return indic_table[u - 0x00D0u + indic_offset_0x00d0u];
      if (hb_in_range (u, 0x0900u, 0x0DF8u)) return indic_table[u - 0x0900u + indic_offset_0x0900u];
      if (unlikely (u == 0x00A0u)) return _(CP,x);
      break;

    case 0x1u:
      if (hb_in_range (u, 0x1000u, 0x10A0u)) return indic_table[u - 0x1000u + indic_offset_0x1000u];
      if (hb_in_range (u, 0x1700u, 0x17F0u)) return indic_table[u - 0x1700u + indic_offset_0x1700u];
      if (hb_in_range (u, 0x1900u, 0x1AA0u)) return indic_table[u - 0x1900u + indic_offset_0x1900u];
      if (hb_in_range (u, 0x1B00u, 0x1C50u)) return indic_table[u - 0x1B00u + indic_offset_0x1b00u];
      if (hb_in_range (u, 0x1CD0u, 0x1CF8u)) return indic_table[u - 0x1CD0u + indic_offset_0x1cd0u];
      break;

    case 0x2u:
      if (hb_in_range (u, 0x2008u, 0x2018u)) return indic_table[u - 0x2008u + indic_offset_0x2008u];
      if (unlikely (u == 0x25CCu)) return _(CP,x);
      break;

    case 0xAu:
      if (hb_in_range (u, 0xA800u, 0xAAF8u)) return indic_table[u - 0xA800u + indic_offset_0xa800u];
      if (hb_in_range (u, 0xABC0u, 0xAC00u)) return indic_table[u - 0xABC0u + indic_offset_0xabc0u];
      break;

    case 0x10u:
      if (hb_in_range (u, 0x10A00u, 0x10A48u)) return indic_table[u - 0x10A00u + indic_offset_0x10a00u];
      break;

    case 0x11u:
      if (hb_in_range (u, 0x11000u, 0x110C0u)) return indic_table[u - 0x11000u + indic_offset_0x11000u];
      if (hb_in_range (u, 0x11100u, 0x11238u)) return indic_table[u - 0x11100u + indic_offset_0x11100u];
      if (hb_in_range (u, 0x112B0u, 0x11378u)) return indic_table[u - 0x112B0u + indic_offset_0x112b0u];
      if (hb_in_range (u, 0x11480u, 0x114E0u)) return indic_table[u - 0x11480u + indic_offset_0x11480u];
      if (hb_in_range (u, 0x11580u, 0x115C8u)) return indic_table[u - 0x11580u + indic_offset_0x11580u];
      if (hb_in_range (u, 0x11600u, 0x116D0u)) return indic_table[u - 0x11600u + indic_offset_0x11600u];
      break;

    default:
      break;
  }
  return _(x,x);
}

#undef _

#undef ISC_A
#undef ISC_Bi
#undef ISC_BJN
#undef ISC_Ca
#undef ISC_C
#undef ISC_CD
#undef ISC_CF
#undef ISC_CHL
#undef ISC_CM
#undef ISC_CP
#undef ISC_CPR
#undef ISC_CS
#undef ISC_CSR
#undef ISC_GM
#undef ISC_IS
#undef ISC_ZWJ
#undef ISC_ML
#undef ISC_ZWNJ
#undef ISC_N
#undef ISC_Nd
#undef ISC_NJ
#undef ISC_x
#undef ISC_PK
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


