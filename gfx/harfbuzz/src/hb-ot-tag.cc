



























#include "hb-private.hh"

#include <string.h>





static hb_tag_t
hb_ot_old_tag_from_script (hb_script_t script)
{
  

  switch ((hb_tag_t) script) {
    case HB_SCRIPT_INVALID:		return HB_OT_TAG_DEFAULT_SCRIPT;

    
    case HB_SCRIPT_HIRAGANA:		return HB_TAG('k','a','n','a');

    
    case HB_SCRIPT_LAO:			return HB_TAG('l','a','o',' ');
    case HB_SCRIPT_YI:			return HB_TAG('y','i',' ',' ');
    
    case HB_SCRIPT_NKO:			return HB_TAG('n','k','o',' ');
    
    case HB_SCRIPT_VAI:			return HB_TAG('v','a','i',' ');
    
    
  }

  
  return ((hb_tag_t) script) | 0x20000000u;
}

static hb_script_t
hb_ot_old_tag_to_script (hb_tag_t tag)
{
  if (unlikely (tag == HB_OT_TAG_DEFAULT_SCRIPT))
    return HB_SCRIPT_INVALID;

  

  

  if (unlikely ((tag & 0x0000FF00u) == 0x00002000u))
    tag |= (tag >> 8) & 0x0000FF00u; 
  if (unlikely ((tag & 0x000000FFu) == 0x00000020u))
    tag |= (tag >> 8) & 0x000000FFu; 

  
  return (hb_script_t) (tag & ~0x20000000u);
}

static hb_tag_t
hb_ot_new_tag_from_script (hb_script_t script)
{
  switch ((hb_tag_t) script) {
    case HB_SCRIPT_BENGALI:		return HB_TAG('b','n','g','2');
    case HB_SCRIPT_DEVANAGARI:		return HB_TAG('d','e','v','2');
    case HB_SCRIPT_GUJARATI:		return HB_TAG('g','j','r','2');
    case HB_SCRIPT_GURMUKHI:		return HB_TAG('g','u','r','2');
    case HB_SCRIPT_KANNADA:		return HB_TAG('k','n','d','2');
    case HB_SCRIPT_MALAYALAM:		return HB_TAG('m','l','m','2');
    case HB_SCRIPT_ORIYA:		return HB_TAG('o','r','y','2');
    case HB_SCRIPT_TAMIL:		return HB_TAG('t','m','l','2');
    case HB_SCRIPT_TELUGU:		return HB_TAG('t','e','l','2');
    case HB_SCRIPT_MYANMAR:		return HB_TAG('m','y','m','2');
  }

  return HB_OT_TAG_DEFAULT_SCRIPT;
}

static hb_script_t
hb_ot_new_tag_to_script (hb_tag_t tag)
{
  switch (tag) {
    case HB_TAG('b','n','g','2'):	return HB_SCRIPT_BENGALI;
    case HB_TAG('d','e','v','2'):	return HB_SCRIPT_DEVANAGARI;
    case HB_TAG('g','j','r','2'):	return HB_SCRIPT_GUJARATI;
    case HB_TAG('g','u','r','2'):	return HB_SCRIPT_GURMUKHI;
    case HB_TAG('k','n','d','2'):	return HB_SCRIPT_KANNADA;
    case HB_TAG('m','l','m','2'):	return HB_SCRIPT_MALAYALAM;
    case HB_TAG('o','r','y','2'):	return HB_SCRIPT_ORIYA;
    case HB_TAG('t','m','l','2'):	return HB_SCRIPT_TAMIL;
    case HB_TAG('t','e','l','2'):	return HB_SCRIPT_TELUGU;
    case HB_TAG('m','y','m','2'):	return HB_SCRIPT_MYANMAR;
  }

  return HB_SCRIPT_UNKNOWN;
}










void
hb_ot_tags_from_script (hb_script_t  script,
			hb_tag_t    *script_tag_1,
			hb_tag_t    *script_tag_2)
{
  hb_tag_t new_tag;

  *script_tag_2 = HB_OT_TAG_DEFAULT_SCRIPT;
  *script_tag_1 = hb_ot_old_tag_from_script (script);

  new_tag = hb_ot_new_tag_from_script (script);
  if (unlikely (new_tag != HB_OT_TAG_DEFAULT_SCRIPT)) {
    *script_tag_2 = *script_tag_1;
    *script_tag_1 = new_tag;
  }
}

hb_script_t
hb_ot_tag_to_script (hb_tag_t tag)
{
  if (unlikely ((tag & 0x000000FFu) == '2'))
    return hb_ot_new_tag_to_script (tag);

  return hb_ot_old_tag_to_script (tag);
}




typedef struct {
  char language[4];
  hb_tag_t tag;
} LangTag;


















static const LangTag ot_languages[] = {
  {"aa",	HB_TAG('A','F','R',' ')},	
  {"ab",	HB_TAG('A','B','K',' ')},	
  {"abq",	HB_TAG('A','B','A',' ')},	
  {"ach",	HB_TAG('A','C','H',' ')},	
  {"ada",	HB_TAG('D','N','G',' ')},	
  {"ady",	HB_TAG('A','D','Y',' ')},	
  {"af",	HB_TAG('A','F','K',' ')},	
  {"aii",	HB_TAG('S','W','A',' ')},	
  {"aio",	HB_TAG('A','I','O',' ')},	
  {"aiw",	HB_TAG('A','R','I',' ')},	
  {"ak",	HB_TAG('T','W','I',' ')},	
  {"alt",	HB_TAG('A','L','T',' ')},	
  {"am",	HB_TAG('A','M','H',' ')},	
  {"amf",	HB_TAG('H','B','N',' ')},	
  {"an",	HB_TAG('A','R','G',' ')},	
  {"ang",	HB_TAG('A','N','G',' ')},	
  {"ar",	HB_TAG('A','R','A',' ')},	
  {"arb",	HB_TAG('A','R','A',' ')},	
  {"arn",	HB_TAG('M','A','P',' ')},	
  {"ary",	HB_TAG('M','O','R',' ')},	
  {"as",	HB_TAG('A','S','M',' ')},	
  {"ast",	HB_TAG('A','S','T',' ')},	
  {"ath",	HB_TAG('A','T','H',' ')},	
  {"atv",	HB_TAG('A','L','T',' ')},	
  {"av",	HB_TAG('A','V','R',' ')},	
  {"awa",	HB_TAG('A','W','A',' ')},	
  {"ay",	HB_TAG('A','Y','M',' ')},	
  {"az",	HB_TAG('A','Z','E',' ')},	
  {"azb",	HB_TAG('A','Z','B',' ')},	
  {"azj",	HB_TAG('A','Z','E',' ')},	
  {"ba",	HB_TAG('B','S','H',' ')},	
  {"bai",	HB_TAG('B','M','L',' ')},	
  {"bal",	HB_TAG('B','L','I',' ')},	
  {"ban",	HB_TAG('B','A','N',' ')},	
  {"bar",	HB_TAG('B','A','R',' ')},	
  {"bbc",	HB_TAG('B','B','C',' ')},	
  {"bci",	HB_TAG('B','A','U',' ')},	
  {"bcl",	HB_TAG('B','I','K',' ')},	
  {"bcq",	HB_TAG('B','C','H',' ')},	
  {"be",	HB_TAG('B','E','L',' ')},  	
  {"bem",	HB_TAG('B','E','M',' ')},	
  {"ber",	HB_TAG('B','E','R',' ')},  	
  {"bfq",	HB_TAG('B','A','D',' ')},	
  {"bft",	HB_TAG('B','L','T',' ')},	
  {"bfy",	HB_TAG('B','A','G',' ')},	
  {"bg",	HB_TAG('B','G','R',' ')},	
  {"bgc",	HB_TAG('B','G','C',' ')},	
  {"bgq",	HB_TAG('B','G','Q',' ')},	
  {"bhb",	HB_TAG('B','H','I',' ')},	
  {"bhk",	HB_TAG('B','I','K',' ')},	
  {"bho",	HB_TAG('B','H','O',' ')},	
  {"bi",	HB_TAG('B','I','S',' ')},	
  {"bik",	HB_TAG('B','I','K',' ')},	
  {"bin",	HB_TAG('E','D','O',' ')},	
  {"bjj",	HB_TAG('B','J','J',' ')},	
  {"bjt",	HB_TAG('B','L','N',' ')},	
  {"bla",	HB_TAG('B','K','F',' ')},	
  {"ble",	HB_TAG('B','L','N',' ')},	
  {"blk",	HB_TAG('B','L','K',' ')},	
  {"bln",	HB_TAG('B','I','K',' ')},	
  {"bm",	HB_TAG('B','M','B',' ')},	
  {"bn",	HB_TAG('B','E','N',' ')},	
  {"bo",	HB_TAG('T','I','B',' ')},	
  {"bpy",	HB_TAG('B','P','Y',' ')},	
  {"bqi",	HB_TAG('L','R','C',' ')},	
  {"br",	HB_TAG('B','R','E',' ')},	
  {"bra",	HB_TAG('B','R','I',' ')},	
  {"brh",	HB_TAG('B','R','H',' ')},	
  {"brx",	HB_TAG('B','R','X',' ')},	
  {"bs",	HB_TAG('B','O','S',' ')},	
  {"btb",	HB_TAG('B','T','I',' ')},	
  {"bto",	HB_TAG('B','I','K',' ')},	
  {"bts",	HB_TAG('B','T','S',' ')},	
  {"bug",	HB_TAG('B','U','G',' ')},	
  {"bxr",	HB_TAG('R','B','U',' ')},	
  {"byn",	HB_TAG('B','I','L',' ')},	
  {"ca",	HB_TAG('C','A','T',' ')},	
  {"cbk",	HB_TAG('C','B','K',' ')},	
  {"ce",	HB_TAG('C','H','E',' ')},	
  {"ceb",	HB_TAG('C','E','B',' ')},	
  {"cgg",	HB_TAG('C','G','G',' ')},	
  {"ch",	HB_TAG('C','H','A',' ')},	
  {"cho",	HB_TAG('C','H','O',' ')},	
  {"chp",	HB_TAG('C','H','P',' ')},	
  {"chr",	HB_TAG('C','H','R',' ')},	
  {"chy",	HB_TAG('C','H','Y',' ')},	
  {"ckb",	HB_TAG('K','U','R',' ')},	
  {"ckt",	HB_TAG('C','H','K',' ')},	
  {"cop",	HB_TAG('C','O','P',' ')},	
  {"cr",	HB_TAG('C','R','E',' ')},	
  {"crh",	HB_TAG('C','R','T',' ')},	
  {"crj",	HB_TAG('E','C','R',' ')},	
  {"crl",	HB_TAG('E','C','R',' ')},	
  {"crm",	HB_TAG('M','C','R',' ')},	
  {"crx",	HB_TAG('C','R','R',' ')},	
  {"cs",	HB_TAG('C','S','Y',' ')},	
  {"csb",	HB_TAG('C','S','B',' ')},	
  {"ctg",	HB_TAG('C','T','G',' ')},	
  {"cts",	HB_TAG('B','I','K',' ')},	
  {"cu",	HB_TAG('C','S','L',' ')},	
  {"cv",	HB_TAG('C','H','U',' ')},	
  {"cwd",	HB_TAG('D','C','R',' ')},	
  {"cy",	HB_TAG('W','E','L',' ')},	
  {"da",	HB_TAG('D','A','N',' ')},	
  {"dap",	HB_TAG('N','I','S',' ')},	
  {"dar",	HB_TAG('D','A','R',' ')},	
  {"de",	HB_TAG('D','E','U',' ')},	
  {"dgo",	HB_TAG('D','G','O',' ')},	
  {"dhd",	HB_TAG('M','A','W',' ')},	
  {"din",	HB_TAG('D','N','K',' ')},	
  {"diq",	HB_TAG('D','I','Q',' ')},	
  {"dje",	HB_TAG('D','J','R',' ')},	
  {"dng",	HB_TAG('D','U','N',' ')},	
  {"doi",	HB_TAG('D','G','R',' ')},	
  {"dsb",	HB_TAG('L','S','B',' ')},	
  {"dv",	HB_TAG('D','I','V',' ')},	
  {"dyu",	HB_TAG('J','U','L',' ')},	
  {"dz",	HB_TAG('D','Z','N',' ')},	
  {"ee",	HB_TAG('E','W','E',' ')},	
  {"efi",	HB_TAG('E','F','I',' ')},	
  {"ekk",	HB_TAG('E','T','I',' ')},	
  {"el",	HB_TAG('E','L','L',' ')},	
  {"emk",	HB_TAG('M','N','K',' ')},	
  {"en",	HB_TAG('E','N','G',' ')},	
  {"eo",	HB_TAG('N','T','O',' ')},	
  {"eot",	HB_TAG('B','T','I',' ')},	
  {"es",	HB_TAG('E','S','P',' ')},	
  {"et",	HB_TAG('E','T','I',' ')},	
  {"eu",	HB_TAG('E','U','Q',' ')},	
  {"eve",	HB_TAG('E','V','N',' ')},	
  {"evn",	HB_TAG('E','V','K',' ')},	
  {"fa",	HB_TAG('F','A','R',' ')},	
  {"ff",	HB_TAG('F','U','L',' ')},	
  {"fi",	HB_TAG('F','I','N',' ')},	
  {"fil",	HB_TAG('P','I','L',' ')},	
  {"fj",	HB_TAG('F','J','I',' ')},	
  {"fo",	HB_TAG('F','O','S',' ')},	
  {"fon",	HB_TAG('F','O','N',' ')},	
  {"fr",	HB_TAG('F','R','A',' ')},	
  {"frc",	HB_TAG('F','R','C',' ')},	
  {"frp",	HB_TAG('F','R','P',' ')},	
  {"fur",	HB_TAG('F','R','L',' ')},	
  {"fuv",	HB_TAG('F','U','V',' ')},	
  {"fy",	HB_TAG('F','R','I',' ')},	
  {"ga",	HB_TAG('I','R','I',' ')},	
  {"gaa",	HB_TAG('G','A','D',' ')},	
  {"gag",	HB_TAG('G','A','G',' ')},	
  {"gbm",	HB_TAG('G','A','W',' ')},	
  {"gd",	HB_TAG('G','A','E',' ')},	
  {"gez",	HB_TAG('G','E','Z',' ')},	
  {"ggo",	HB_TAG('G','O','N',' ')},	
  {"gl",	HB_TAG('G','A','L',' ')},	
  {"gld",	HB_TAG('N','A','N',' ')},	
  {"glk",	HB_TAG('G','L','K',' ')},	
  {"gn",	HB_TAG('G','U','A',' ')},	
  {"gno",	HB_TAG('G','O','N',' ')},	
  {"gog",	HB_TAG('G','O','G',' ')},	
  {"gon",	HB_TAG('G','O','N',' ')},	
  {"grt",	HB_TAG('G','R','O',' ')},	
  {"gru",	HB_TAG('S','O','G',' ')},	
  {"gu",	HB_TAG('G','U','J',' ')},	
  {"guc",	HB_TAG('G','U','C',' ')},	
  {"guk",	HB_TAG('G','M','Z',' ')},	
	
  {"guz",	HB_TAG('G','U','Z',' ')},	
  {"gv",	HB_TAG('M','N','X',' ')},	
  {"ha",	HB_TAG('H','A','U',' ')},	
  {"har",	HB_TAG('H','R','I',' ')},	
  {"haw",	HB_TAG('H','A','W',' ')},  	
  {"hay",	HB_TAG('H','A','Y',' ')},  	
  {"haz",	HB_TAG('H','A','Z',' ')},  	
  {"he",	HB_TAG('I','W','R',' ')},	
  {"hz",	HB_TAG('H','E','R',' ')},	
  {"hi",	HB_TAG('H','I','N',' ')},	
  {"hil",	HB_TAG('H','I','L',' ')},	
  {"hnd",	HB_TAG('H','N','D',' ')},	
  {"hne",	HB_TAG('C','H','H',' ')},	
  {"hno",	HB_TAG('H','N','D',' ')},	
  {"ho",	HB_TAG('H','M','O',' ')},	
  {"hoc",	HB_TAG('H','O',' ',' ')},	
  {"hoj",	HB_TAG('H','A','R',' ')},	
  {"hr",	HB_TAG('H','R','V',' ')},	
  {"hsb",	HB_TAG('U','S','B',' ')},	
  {"ht",	HB_TAG('H','A','I',' ')},	
  {"hu",	HB_TAG('H','U','N',' ')},	
  {"hy",	HB_TAG('H','Y','E',' ')},	
  {"hz",	HB_TAG('H','E','R',' ')},	
  {"ia",	HB_TAG('I','N','A',' ')},	
  {"ibb",	HB_TAG('I','B','B',' ')},	
  {"id",	HB_TAG('I','N','D',' ')},	
  {"ie",	HB_TAG('I','L','E',' ')},	
  {"ig",	HB_TAG('I','B','O',' ')},	
  {"igb",	HB_TAG('E','B','I',' ')},	
  {"ijc",	HB_TAG('I','J','O',' ')},	
  {"ijo",	HB_TAG('I','J','O',' ')},	
  {"ik",	HB_TAG('I','P','K',' ')},	
  {"ilo",	HB_TAG('I','L','O',' ')},	
  {"inh",	HB_TAG('I','N','G',' ')},	
  {"io",	HB_TAG('I','D','O',' ')},	
  {"is",	HB_TAG('I','S','L',' ')},	
  {"it",	HB_TAG('I','T','A',' ')},	
  {"iu",	HB_TAG('I','N','U',' ')},	
  {"ja",	HB_TAG('J','A','N',' ')},	
  {"jam",	HB_TAG('J','A','M',' ')},	
  {"jbo",	HB_TAG('J','B','O',' ')},	
  {"jv",	HB_TAG('J','A','V',' ')},	
  {"ka",	HB_TAG('K','A','T',' ')},	
  {"kaa",	HB_TAG('K','R','K',' ')},	
  {"kab",	HB_TAG('K','A','B',' ')},	
  {"kam",	HB_TAG('K','M','B',' ')},	
  {"kar",	HB_TAG('K','R','N',' ')},	
  {"kbd",	HB_TAG('K','A','B',' ')},	
  {"kde",	HB_TAG('K','D','E',' ')},	
  {"kdr",	HB_TAG('K','R','M',' ')},	
  {"kdt",	HB_TAG('K','U','Y',' ')},	
  {"kex",	HB_TAG('K','K','N',' ')},	
  {"kfr",	HB_TAG('K','A','C',' ')},	
  {"kfy",	HB_TAG('K','M','N',' ')},	
  {"kg",	HB_TAG('K','O','N',' ')},	
  {"kha",	HB_TAG('K','S','I',' ')},	
  {"khb",	HB_TAG('X','B','D',' ')},	
  {"kht",	HB_TAG('K','H','N',' ')},	
	
  {"khw",	HB_TAG('K','H','W',' ')},	
  {"ki",	HB_TAG('K','I','K',' ')},	
  {"kj",	HB_TAG('K','U','A',' ')},	
  {"kjh",	HB_TAG('K','H','A',' ')},	
  {"kjp",	HB_TAG('K','J','P',' ')},	
  {"kk",	HB_TAG('K','A','Z',' ')},	
  {"kl",	HB_TAG('G','R','N',' ')},	
  {"kln",	HB_TAG('K','A','L',' ')},	
  {"km",	HB_TAG('K','H','M',' ')},	
  {"kmb",	HB_TAG('M','B','N',' ')},	
  {"kmw",	HB_TAG('K','M','O',' ')},	
  {"kn",	HB_TAG('K','A','N',' ')},	
  {"knn",	HB_TAG('K','O','K',' ')},	
  {"ko",	HB_TAG('K','O','R',' ')},	
  {"koi",	HB_TAG('K','O','P',' ')},	
  {"kok",	HB_TAG('K','O','K',' ')},	
  {"kpe",	HB_TAG('K','P','L',' ')},	
  {"kpv",	HB_TAG('K','O','Z',' ')},	
  {"kpy",	HB_TAG('K','Y','K',' ')},	
  {"kqy",	HB_TAG('K','R','T',' ')},	
  {"kr",	HB_TAG('K','N','R',' ')},	
  {"kri",	HB_TAG('K','R','I',' ')},	
  {"krl",	HB_TAG('K','R','L',' ')},	
  {"kru",	HB_TAG('K','U','U',' ')},	
  {"ks",	HB_TAG('K','S','H',' ')},	
  {"ksh",	HB_TAG('K','S','H',' ')},	
	
  {"ksw",	HB_TAG('K','S','W',' ')},	
  {"ku",	HB_TAG('K','U','R',' ')},	
  {"kum",	HB_TAG('K','U','M',' ')},	
  {"kv",	HB_TAG('K','O','M',' ')},	
  {"kvd",	HB_TAG('K','U','I',' ')},	
  {"kw",	HB_TAG('C','O','R',' ')},	
  {"kxc",	HB_TAG('K','M','S',' ')},	
  {"kxu",	HB_TAG('K','U','I',' ')},	
  {"ky",	HB_TAG('K','I','R',' ')},	
  {"kyu",	HB_TAG('K','Y','U',' ')},	
  {"la",	HB_TAG('L','A','T',' ')},	
  {"lad",	HB_TAG('J','U','D',' ')},	
  {"lb",	HB_TAG('L','T','Z',' ')},	
  {"lbe",	HB_TAG('L','A','K',' ')},	
  {"lbj",	HB_TAG('L','D','K',' ')},	
  {"lez",	HB_TAG('L','E','Z',' ')},	
  {"lg",	HB_TAG('L','U','G',' ')},	
  {"li",	HB_TAG('L','I','M',' ')},	
  {"lif",	HB_TAG('L','M','B',' ')},	
  {"lij",	HB_TAG('L','I','J',' ')},	
  {"lis",	HB_TAG('L','I','S',' ')},	
  {"ljp",	HB_TAG('L','J','P',' ')},	
  {"lki",	HB_TAG('L','K','I',' ')},	
  {"lld",	HB_TAG('L','A','D',' ')},	
  {"lmn",	HB_TAG('L','A','M',' ')},	
  {"lmo",	HB_TAG('L','M','O',' ')},	
  {"ln",	HB_TAG('L','I','N',' ')},	
  {"lo",	HB_TAG('L','A','O',' ')},	
  {"lrc",	HB_TAG('L','R','C',' ')},	
  {"lt",	HB_TAG('L','T','H',' ')},	
  {"lu",	HB_TAG('L','U','B',' ')},	
  {"lua",	HB_TAG('L','U','B',' ')},	
  {"luo",	HB_TAG('L','U','O',' ')},	
  {"lus",	HB_TAG('M','I','Z',' ')},	
  {"luy",	HB_TAG('L','U','H',' ')},	
  {"luz",	HB_TAG('L','R','C',' ')},	
  {"lv",	HB_TAG('L','V','I',' ')},	
  {"lzz",	HB_TAG('L','A','Z',' ')},	
  {"mad",	HB_TAG('M','A','D',' ')},	
  {"mag",	HB_TAG('M','A','G',' ')},	
  {"mai",	HB_TAG('M','T','H',' ')},	
  {"mak",	HB_TAG('M','K','R',' ')},	
  {"man",	HB_TAG('M','N','K',' ')},	
  {"mdc",	HB_TAG('M','L','E',' ')},	
  {"mdf",	HB_TAG('M','O','K',' ')},	
  {"mdr",	HB_TAG('M','D','R',' ')},	
  {"mdy",	HB_TAG('M','L','E',' ')},	
  {"men",	HB_TAG('M','D','E',' ')},	
  {"mer",	HB_TAG('M','E','R',' ')},	
  {"mfe",	HB_TAG('M','F','E',' ')},	
  {"mg",	HB_TAG('M','L','G',' ')},	
  {"mh",	HB_TAG('M','A','H',' ')},	
  {"mhr",	HB_TAG('L','M','A',' ')},	
  {"mi",	HB_TAG('M','R','I',' ')},	
  {"min",	HB_TAG('M','I','N',' ')},	
  {"mk",	HB_TAG('M','K','D',' ')},	
  {"mku",	HB_TAG('M','N','K',' ')},	
  {"mkw",	HB_TAG('M','K','W',' ')},	
  {"ml",	HB_TAG('M','L','R',' ')},	
  {"mlq",	HB_TAG('M','N','K',' ')},	
  {"mn",	HB_TAG('M','N','G',' ')},	
  {"mnc",	HB_TAG('M','C','H',' ')},	
  {"mni",	HB_TAG('M','N','I',' ')},	
  {"mnk",	HB_TAG('M','N','D',' ')},	
  {"mns",	HB_TAG('M','A','N',' ')},	
  {"mnw",	HB_TAG('M','O','N',' ')},	
  {"mo",	HB_TAG('M','O','L',' ')},	
  {"moh",	HB_TAG('M','O','H',' ')},	
  {"mos",	HB_TAG('M','O','S',' ')},	
  {"mpe",	HB_TAG('M','A','J',' ')},	
  {"mr",	HB_TAG('M','A','R',' ')},	
  {"mrj",	HB_TAG('H','M','A',' ')},	
  {"ms",	HB_TAG('M','L','Y',' ')},	
  {"msc",	HB_TAG('M','N','K',' ')},	
  {"mt",	HB_TAG('M','T','S',' ')},	
  {"mtr",	HB_TAG('M','A','W',' ')},	
  {"mus",	HB_TAG('M','U','S',' ')},	
  {"mve",	HB_TAG('M','A','W',' ')},	
  {"mwk",	HB_TAG('M','N','K',' ')},	
  {"mwl",	HB_TAG('M','W','L',' ')},	
  {"mwr",	HB_TAG('M','A','W',' ')},	
  {"mww",	HB_TAG('M','W','W',' ')},	
  {"my",	HB_TAG('B','R','M',' ')},	
  {"mym",	HB_TAG('M','E','N',' ')},	
  {"myq",	HB_TAG('M','N','K',' ')},	
  {"myv",	HB_TAG('E','R','Z',' ')},	
  {"mzn",	HB_TAG('M','Z','N',' ')},	
  {"na",	HB_TAG('N','A','U',' ')},	
  {"nag",	HB_TAG('N','A','G',' ')},	
  {"nah",	HB_TAG('N','A','H',' ')},	
  {"nap",	HB_TAG('N','A','P',' ')},	
  {"nb",	HB_TAG('N','O','R',' ')},	
  {"nco",	HB_TAG('S','I','B',' ')},	
  {"nd",	HB_TAG('N','D','B',' ')},	
  {"ndc",	HB_TAG('N','D','C',' ')},	
  {"nds",	HB_TAG('N','D','S',' ')},	
  {"ne",	HB_TAG('N','E','P',' ')},	
  {"new",	HB_TAG('N','E','W',' ')},	
  {"ng",	HB_TAG('N','D','G',' ')},	
  {"nga",	HB_TAG('N','G','A',' ')},	
  {"ngl",	HB_TAG('L','M','W',' ')},	
  {"niu",	HB_TAG('N','I','U',' ')},	
  {"niv",	HB_TAG('G','I','L',' ')},	
  {"nl",	HB_TAG('N','L','D',' ')},	
  {"nn",	HB_TAG('N','Y','N',' ')},	
  {"no",	HB_TAG('N','O','R',' ')},	
  {"nod",	HB_TAG('N','T','A',' ')},	
  {"noe",	HB_TAG('N','O','E',' ')},	
  {"nog",	HB_TAG('N','O','G',' ')},	
  {"nov",	HB_TAG('N','O','V',' ')},	
  {"nqo",	HB_TAG('N','K','O',' ')},	
  {"nr",	HB_TAG('N','D','B',' ')},	
  {"nsk",	HB_TAG('N','A','S',' ')},	
  {"nso",	HB_TAG('S','O','T',' ')},	
  {"ny",	HB_TAG('C','H','I',' ')},	
  {"nym",	HB_TAG('N','Y','M',' ')},	
  {"nyn",	HB_TAG('N','K','L',' ')},	
  {"oc",	HB_TAG('O','C','I',' ')},	
  {"oj",	HB_TAG('O','J','B',' ')},	
  {"ojs",	HB_TAG('O','C','R',' ')},	
  {"om",	HB_TAG('O','R','O',' ')},	
  {"or",	HB_TAG('O','R','I',' ')},	
  {"os",	HB_TAG('O','S','S',' ')},	
  {"pa",	HB_TAG('P','A','N',' ')},	
  {"pag",	HB_TAG('P','A','G',' ')},	
  {"pam",	HB_TAG('P','A','M',' ')},	
  {"pap",	HB_TAG('P','A','P',' ')},	
  {"pcc",	HB_TAG('P','C','C',' ')},	
  {"pcd",	HB_TAG('P','C','D',' ')},	
  {"pce",	HB_TAG('P','L','G',' ')},	
  {"pdc",	HB_TAG('P','D','C',' ')},	
  {"pes",	HB_TAG('F','A','R',' ')},	
  {"phk",	HB_TAG('P','H','K',' ')},	
  {"pi",	HB_TAG('P','A','L',' ')},	
  {"pih",	HB_TAG('P','I','H',' ')},	
  {"pl",	HB_TAG('P','L','K',' ')},	
  {"pll",	HB_TAG('P','L','G',' ')},	
  {"plp",	HB_TAG('P','A','P',' ')},	
  {"pms",	HB_TAG('P','M','S',' ')},	
  {"pnb",	HB_TAG('P','N','B',' ')},	
  {"prs",	HB_TAG('D','R','I',' ')},	
  {"ps",	HB_TAG('P','A','S',' ')},	
  {"pt",	HB_TAG('P','T','G',' ')},	
  {"pwo",	HB_TAG('P','W','O',' ')},	
  {"qu",	HB_TAG('Q','U','Z',' ')},	
  {"quc",	HB_TAG('Q','U','C',' ')},	
  {"quz",	HB_TAG('Q','U','Z',' ')},	
  {"raj",	HB_TAG('R','A','J',' ')},	
  {"rbb",	HB_TAG('P','L','G',' ')},	
  {"rej",	HB_TAG('R','E','J',' ')},	
  {"ria",	HB_TAG('R','I','A',' ')},	
  {"ril",	HB_TAG('R','I','A',' ')},	
  {"rki",	HB_TAG('A','R','K',' ')},	
  {"rm",	HB_TAG('R','M','S',' ')},	
  {"rmy",	HB_TAG('R','M','Y',' ')},	
  {"rn",	HB_TAG('R','U','N',' ')},	
  {"ro",	HB_TAG('R','O','M',' ')},	
  {"rom",	HB_TAG('R','O','Y',' ')},	
  {"ru",	HB_TAG('R','U','S',' ')},	
  {"rue",	HB_TAG('R','S','Y',' ')},	
  {"rup",	HB_TAG('R','U','P',' ')},	
  {"rw",	HB_TAG('R','U','A',' ')},	
  {"rwr",	HB_TAG('M','A','W',' ')},	
  {"sa",	HB_TAG('S','A','N',' ')},	
  {"sah",	HB_TAG('Y','A','K',' ')},	
  {"sas",	HB_TAG('S','A','S',' ')},	
  {"sat",	HB_TAG('S','A','T',' ')},	
  {"sck",	HB_TAG('S','A','D',' ')},	
  {"sc",	HB_TAG('S','R','D',' ')},	
  {"scn",	HB_TAG('S','C','N',' ')},	
  {"sco",	HB_TAG('S','C','O',' ')},	
  {"scs",	HB_TAG('S','L','A',' ')},	
  {"sd",	HB_TAG('S','N','D',' ')},	
  {"se",	HB_TAG('N','S','M',' ')},	
  {"seh",	HB_TAG('S','N','A',' ')},	
  {"sel",	HB_TAG('S','E','L',' ')},	
  {"sg",	HB_TAG('S','G','O',' ')},	
  {"sga",	HB_TAG('S','G','A',' ')},	
  {"sgs",	HB_TAG('S','G','S',' ')},	
  {"sgw",	HB_TAG('C','H','G',' ')},	
	
  {"shn",	HB_TAG('S','H','N',' ')},	
  {"si",	HB_TAG('S','N','H',' ')},	
  {"sid",	HB_TAG('S','I','D',' ')},	
  {"sjd",	HB_TAG('K','S','M',' ')},	
  {"sk",	HB_TAG('S','K','Y',' ')},	
  {"skr",	HB_TAG('S','R','K',' ')},	
  {"sl",	HB_TAG('S','L','V',' ')},	
  {"sm",	HB_TAG('S','M','O',' ')},	
  {"sma",	HB_TAG('S','S','M',' ')},	
  {"smj",	HB_TAG('L','S','M',' ')},	
  {"smn",	HB_TAG('I','S','M',' ')},	
  {"sms",	HB_TAG('S','K','S',' ')},	
  {"sn",	HB_TAG('S','N','A',' ')},	
  {"snk",	HB_TAG('S','N','K',' ')},	
  {"so",	HB_TAG('S','M','L',' ')},	
  {"sop",	HB_TAG('S','O','P',' ')},	
  {"sq",	HB_TAG('S','Q','I',' ')},	
  {"sr",	HB_TAG('S','R','B',' ')},	
  {"srr",	HB_TAG('S','R','R',' ')},	
  {"ss",	HB_TAG('S','W','Z',' ')},	
  {"st",	HB_TAG('S','O','T',' ')},	
  {"stq",	HB_TAG('S','T','Q',' ')},	
  {"stv",	HB_TAG('S','I','G',' ')},	
  {"su",	HB_TAG('S','U','N',' ')},	
  {"suk",	HB_TAG('S','U','K',' ')},	
  {"suq",	HB_TAG('S','U','R',' ')},	
  {"sv",	HB_TAG('S','V','E',' ')},	
  {"sva",	HB_TAG('S','V','A',' ')},	
  {"sw",	HB_TAG('S','W','K',' ')},	
  {"swb",	HB_TAG('C','M','R',' ')},	
  {"swh",	HB_TAG('S','W','K',' ')},	
  {"swv",	HB_TAG('M','A','W',' ')},	
  {"sxu",	HB_TAG('S','X','U',' ')},	
  {"syl",	HB_TAG('S','Y','L',' ')},	
  {"syr",	HB_TAG('S','Y','R',' ')},	
  {"szl",	HB_TAG('S','Z','L',' ')},	
  {"ta",	HB_TAG('T','A','M',' ')},	
  {"tab",	HB_TAG('T','A','B',' ')},	
  {"tcy",	HB_TAG('T','U','L',' ')},	
  {"tdd",	HB_TAG('T','D','D',' ')},	
  {"te",	HB_TAG('T','E','L',' ')},	
  {"tem",	HB_TAG('T','M','N',' ')},	
  {"tet",	HB_TAG('T','E','T',' ')},	
  {"tg",	HB_TAG('T','A','J',' ')},	
  {"th",	HB_TAG('T','H','A',' ')},	
  {"ti",	HB_TAG('T','G','Y',' ')},	
  {"tig",	HB_TAG('T','G','R',' ')},	
  {"tiv",	HB_TAG('T','I','V',' ')},	
  {"tk",	HB_TAG('T','K','M',' ')},	
  {"tl",	HB_TAG('T','G','L',' ')},	
  {"tmh",	HB_TAG('t','m','h',' ')},	
  {"tn",	HB_TAG('T','N','A',' ')},	
  {"to",	HB_TAG('T','G','N',' ')},	
  {"tpi",	HB_TAG('T','P','I',' ')},	
  {"tr",	HB_TAG('T','R','K',' ')},	
  {"tru",	HB_TAG('T','U','A',' ')},	
  {"ts",	HB_TAG('T','S','G',' ')},	
  {"tt",	HB_TAG('T','A','T',' ')},	
  {"tum",	HB_TAG('T','U','M',' ')},	
  {"tw",	HB_TAG('T','W','I',' ')},	
  {"ty",	HB_TAG('T','H','T',' ')},	
  {"tyv",	HB_TAG('T','U','V',' ')},	
  {"tyz",	HB_TAG('T','Y','Z',' ')},	
  {"tzm",	HB_TAG('T','Z','M',' ')},	
  {"udm",	HB_TAG('U','D','M',' ')},	
  {"ug",	HB_TAG('U','Y','G',' ')},	
  {"uk",	HB_TAG('U','K','R',' ')},	
  {"umb",	HB_TAG('U','M','B',' ')},	
  {"unr",	HB_TAG('M','U','N',' ')},	
  {"ur",	HB_TAG('U','R','D',' ')},	
  {"uz",	HB_TAG('U','Z','B',' ')},	
  {"uzn",	HB_TAG('U','Z','B',' ')},	
  {"uzs",	HB_TAG('U','Z','B',' ')},	
  {"ve",	HB_TAG('V','E','N',' ')},	
  {"vec",	HB_TAG('V','E','C',' ')},	
  {"vls",	HB_TAG('F','L','E',' ')},	
  {"vi",	HB_TAG('V','I','T',' ')},	
  {"vmw",	HB_TAG('M','A','K',' ')},	
  {"vo",	HB_TAG('V','O','L',' ')},	
  {"vro",	HB_TAG('V','R','O',' ')},	
  {"wa",	HB_TAG('W','L','N',' ')},	
  {"war",	HB_TAG('W','A','R',' ')},	
  {"wbm",	HB_TAG('W','A',' ',' ')},	
  {"wbr",	HB_TAG('W','A','G',' ')},	
  {"wle",	HB_TAG('S','I','G',' ')},	
  {"wry",	HB_TAG('M','A','W',' ')},	
  {"wtm",	HB_TAG('W','T','M',' ')},	
  {"wo",	HB_TAG('W','L','F',' ')},	
  {"xal",	HB_TAG('K','L','M',' ')},	
  {"xh",	HB_TAG('X','H','S',' ')},	
  {"xog",	HB_TAG('X','O','G',' ')},	
  {"xom",	HB_TAG('K','M','O',' ')},	
  {"xsl",	HB_TAG('S','S','L',' ')},	
  {"xst",	HB_TAG('S','I','G',' ')},	
  {"xwo",	HB_TAG('T','O','D',' ')},	
  {"yao",	HB_TAG('Y','A','O',' ')},	
  {"yi",	HB_TAG('J','I','I',' ')},	
  {"yo",	HB_TAG('Y','B','A',' ')},	
  {"yso",	HB_TAG('N','I','S',' ')},	
  {"za",	HB_TAG('Z','H','A',' ')},	
  {"zea",	HB_TAG('Z','E','A',' ')},	
  {"zne",	HB_TAG('Z','N','D',' ')},	
  {"zu",	HB_TAG('Z','U','L',' ')}, 	
  {"zum",	HB_TAG('L','R','C',' ')}	

  


	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
};

typedef struct {
  char language[8];
  hb_tag_t tag;
} LangTagLong;
static const LangTagLong ot_languages_zh[] = {
  {"zh-cn",	HB_TAG('Z','H','S',' ')},	
  {"zh-hk",	HB_TAG('Z','H','H',' ')},	
  {"zh-mo",	HB_TAG('Z','H','T',' ')},	
  {"zh-sg",	HB_TAG('Z','H','S',' ')},	
  {"zh-tw",	HB_TAG('Z','H','T',' ')},	
  {"zh-hans",	HB_TAG('Z','H','S',' ')},	
  {"zh-hant",	HB_TAG('Z','H','T',' ')},	
};

static int
lang_compare_first_component (const char *a,
			      const char *b)
{
  unsigned int da, db;
  const char *p;

  p = strchr (a, '-');
  da = p ? (unsigned int) (p - a) : strlen (a);

  p = strchr (b, '-');
  db = p ? (unsigned int) (p - b) : strlen (b);

  return strncmp (a, b, MAX (da, db));
}

static hb_bool_t
lang_matches (const char *lang_str, const char *spec)
{
  unsigned int len = strlen (spec);

  return strncmp (lang_str, spec, len) == 0 &&
	 (lang_str[len] == '\0' || lang_str[len] == '-');
}

hb_tag_t
hb_ot_tag_from_language (hb_language_t language)
{
  const char *lang_str, *s;

  if (language == HB_LANGUAGE_INVALID)
    return HB_OT_TAG_DEFAULT_LANGUAGE;

  lang_str = hb_language_to_string (language);

  s = strstr (lang_str, "x-hbot");
  if (s) {
    char tag[4];
    int i;
    s += 6;
    for (i = 0; i < 4 && ISALPHA (s[i]); i++)
      tag[i] = TOUPPER (s[i]);
    if (i) {
      for (; i < 4; i++)
	tag[i] = ' ';
      return HB_TAG_CHAR4 (tag);
    }
  }

  
  {
    const LangTag *lang_tag;
    lang_tag = (LangTag *) bsearch (lang_str, ot_languages,
				    ARRAY_LENGTH (ot_languages), sizeof (LangTag),
				    (hb_compare_func_t) lang_compare_first_component);
    if (lang_tag)
      return lang_tag->tag;
  }

  
  if (0 == lang_compare_first_component (lang_str, "zh"))
  {
    unsigned int i;

    for (i = 0; i < ARRAY_LENGTH (ot_languages_zh); i++)
    {
      const LangTagLong *lang_tag;
      lang_tag = &ot_languages_zh[i];
      if (lang_matches (lang_str, lang_tag->language))
	return lang_tag->tag;
    }

    
    return HB_TAG('Z','H','S',' ');
  }

  s = strchr (lang_str, '-');
  if (!s)
    s = lang_str + strlen (lang_str);
  if (s - lang_str == 3) {
    
    return hb_tag_from_string (lang_str, s - lang_str) & ~0x20202000u;
  }

  return HB_OT_TAG_DEFAULT_LANGUAGE;
}

hb_language_t
hb_ot_tag_to_language (hb_tag_t tag)
{
  unsigned int i;

  if (tag == HB_OT_TAG_DEFAULT_LANGUAGE)
    return NULL;

  for (i = 0; i < ARRAY_LENGTH (ot_languages); i++)
    if (ot_languages[i].tag == tag)
      return hb_language_from_string (ot_languages[i].language, -1);

  
  if ((tag & 0xFFFF0000u)  == 0x5A480000u) {
    switch (tag) {
      case HB_TAG('Z','H','H',' '): return hb_language_from_string ("zh-hk", -1); 
      case HB_TAG('Z','H','S',' '): return hb_language_from_string ("zh-Hans", -1); 
      case HB_TAG('Z','H','T',' '): return hb_language_from_string ("zh-Hant", -1); 
      default: break; 
    }
  }

  
  {
    unsigned char buf[11] = "x-hbot";
    buf[6] = tag >> 24;
    buf[7] = (tag >> 16) & 0xFF;
    buf[8] = (tag >> 8) & 0xFF;
    buf[9] = tag & 0xFF;
    if (buf[9] == 0x20)
      buf[9] = '\0';
    buf[10] = '\0';
    return hb_language_from_string ((char *) buf, -1);
  }
}


