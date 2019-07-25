

























#include "hb-private.h"
#include "hb-ot.h"

#include <string.h>

HB_BEGIN_DECLS






static const hb_tag_t ot_scripts[][3] = {
  {HB_TAG('D','F','L','T')},	
  {HB_TAG('D','F','L','T')},	
  {HB_TAG('a','r','a','b')},	
  {HB_TAG('a','r','m','n')},	
  {HB_TAG('b','n','g','2'), HB_TAG('b','e','n','g')},	
  {HB_TAG('b','o','p','o')},	
  {HB_TAG('c','h','e','r')},	
  {HB_TAG('c','o','p','t')},	
  {HB_TAG('c','y','r','l')},	
  {HB_TAG('d','s','r','t')},	
  {HB_TAG('d','e','v','2'), HB_TAG('d','e','v','a')},	
  {HB_TAG('e','t','h','i')},	
  {HB_TAG('g','e','o','r')},	
  {HB_TAG('g','o','t','h')},	
  {HB_TAG('g','r','e','k')},	
  {HB_TAG('g','j','r','2'), HB_TAG('g','u','j','r')},	
  {HB_TAG('g','u','r','2'), HB_TAG('g','u','r','u')},	
  {HB_TAG('h','a','n','i')},	
  {HB_TAG('h','a','n','g')},	
  {HB_TAG('h','e','b','r')},	
  {HB_TAG('k','a','n','a')},	
  {HB_TAG('k','n','d','2'), HB_TAG('k','n','d','a')},	
  {HB_TAG('k','a','n','a')},	
  {HB_TAG('k','h','m','r')},	
  {HB_TAG('l','a','o',' ')},	
  {HB_TAG('l','a','t','n')},	
  {HB_TAG('m','l','m','2'), HB_TAG('m','l','y','m')},	
  {HB_TAG('m','o','n','g')},	
  {HB_TAG('m','y','m','r')},	
  {HB_TAG('o','g','a','m')},	
  {HB_TAG('i','t','a','l')},	
  {HB_TAG('o','r','y','2'), HB_TAG('o','r','y','a')},	
  {HB_TAG('r','u','n','r')},	
  {HB_TAG('s','i','n','h')},	
  {HB_TAG('s','y','r','c')},	
  {HB_TAG('t','m','l','2'), HB_TAG('t','a','m','l')},	
  {HB_TAG('t','e','l','2'), HB_TAG('t','e','l','u')},	
  {HB_TAG('t','h','a','a')},	
  {HB_TAG('t','h','a','i')},	
  {HB_TAG('t','i','b','t')},	
  {HB_TAG('c','a','n','s')},	
  {HB_TAG('y','i',' ',' ')},	
  {HB_TAG('t','g','l','g')},	
  {HB_TAG('h','a','n','o')},	
  {HB_TAG('b','u','h','d')},	
  {HB_TAG('t','a','g','b')},	

  
  {HB_TAG('b','r','a','i')},	
  {HB_TAG('c','p','r','t')},	
  {HB_TAG('l','i','m','b')},	
  {HB_TAG('o','s','m','a')},	
  {HB_TAG('s','h','a','w')},	
  {HB_TAG('l','i','n','b')},	
  {HB_TAG('t','a','l','e')},	
  {HB_TAG('u','g','a','r')},	

  
  {HB_TAG('t','a','l','u')},	
  {HB_TAG('b','u','g','i')},	
  {HB_TAG('g','l','a','g')},	
  {HB_TAG('t','f','n','g')},	
  {HB_TAG('s','y','l','o')},	
  {HB_TAG('x','p','e','o')},	
  {HB_TAG('k','h','a','r')},	

  
  {HB_TAG('D','F','L','T')},	
  {HB_TAG('b','a','l','i')},	
  {HB_TAG('x','s','u','x')},	
  {HB_TAG('p','h','n','x')},	
  {HB_TAG('p','h','a','g')},	
  {HB_TAG('n','k','o',' ')},	

  
  {HB_TAG('k','a','l','i')},	
  {HB_TAG('l','e','p','c')},	
  {HB_TAG('r','j','n','g')},	
  {HB_TAG('s','u','n','d')},	
  {HB_TAG('s','a','u','r')},	
  {HB_TAG('c','h','a','m')},	
  {HB_TAG('o','l','c','k')},	
  {HB_TAG('v','a','i',' ')},	
  {HB_TAG('c','a','r','i')},	
  {HB_TAG('l','y','c','i')},	
  {HB_TAG('l','y','d','i')},	

  
  {HB_TAG('a','v','s','t')},	
  {HB_TAG('b','a','m','u')},	
  {HB_TAG('e','g','y','p')},	
  {HB_TAG('a','r','m','i')},	
  {HB_TAG('p','h','l','i')},	
  {HB_TAG('p','r','t','i')},	
  {HB_TAG('j','a','v','a')},	
  {HB_TAG('k','t','h','i')},	
  {HB_TAG('l','i','s','u')},	
  {HB_TAG('m','y','e','i')},	
  {HB_TAG('s','a','r','b')},	
  {HB_TAG('o','r','k','h')},	
  {HB_TAG('s','a','m','r')},	
  {HB_TAG('l','a','n','a')},	
  {HB_TAG('t','a','v','t')},	

  
  {HB_TAG('b','a','t','k')},	
  {HB_TAG('b','r','a','h')},	
  {HB_TAG('m','a','n','d')} 	
};

const hb_tag_t *
hb_ot_tags_from_script (hb_script_t script)
{
  static const hb_tag_t def_tag[] = {HB_OT_TAG_DEFAULT_SCRIPT, HB_TAG_NONE};

  if (unlikely ((unsigned int) script >= ARRAY_LENGTH (ot_scripts)))
    return def_tag;

  return ot_scripts[script];
}

hb_script_t
hb_ot_tag_to_script (hb_tag_t tag)
{
  int i;

  for (i = 0; i < ARRAY_LENGTH (ot_scripts); i++) {
    const hb_tag_t *p;
    for (p = ot_scripts[i]; *p; p++)
      if (tag == *p)
        return i;
  }

  return HB_SCRIPT_UNKNOWN;
}

typedef struct {
  char language[6];
  hb_tag_t tag;
} LangTag;












static const LangTag ot_languages[] = {
  {"aa",	HB_TAG('A','F','R',' ')},	
  {"ab",	HB_TAG('A','B','K',' ')},	
  {"abq",	HB_TAG('A','B','A',' ')},	
  {"ady",	HB_TAG('A','D','Y',' ')},	
  {"af",	HB_TAG('A','F','K',' ')},	
  {"aiw",	HB_TAG('A','R','I',' ')},	
  {"am",	HB_TAG('A','M','H',' ')},	
  {"ar",	HB_TAG('A','R','A',' ')},	
  {"arn",	HB_TAG('M','A','P',' ')},	
  {"as",	HB_TAG('A','S','M',' ')},	
  {"av",	HB_TAG('A','V','R',' ')},	
  {"awa",	HB_TAG('A','W','A',' ')},	
  {"ay",	HB_TAG('A','Y','M',' ')},	
  {"az",	HB_TAG('A','Z','E',' ')},	
  {"ba",	HB_TAG('B','S','H',' ')},	
  {"bal",	HB_TAG('B','L','I',' ')},	
  {"bcq",	HB_TAG('B','C','H',' ')},	
  {"bem",	HB_TAG('B','E','M',' ')},	
  {"bfq",	HB_TAG('B','A','D',' ')},	
  {"bft",	HB_TAG('B','L','T',' ')},	
  {"bg",	HB_TAG('B','G','R',' ')},	
  {"bhb",	HB_TAG('B','H','I',' ')},	
  {"bho",	HB_TAG('B','H','O',' ')},	
  {"bik",	HB_TAG('B','I','K',' ')},	
  {"bin",	HB_TAG('E','D','O',' ')},	
  {"bm",	HB_TAG('B','M','B',' ')},	
  {"bn",	HB_TAG('B','E','N',' ')},	
  {"bo",	HB_TAG('T','I','B',' ')},	
  {"br",	HB_TAG('B','R','E',' ')},	
  {"brh",	HB_TAG('B','R','H',' ')},	
  {"bs",	HB_TAG('B','O','S',' ')},	
  {"btb",	HB_TAG('B','T','I',' ')},	
  {"ca",	HB_TAG('C','A','T',' ')},	
  {"ce",	HB_TAG('C','H','E',' ')},	
  {"ceb",	HB_TAG('C','E','B',' ')},	
  {"chp",	HB_TAG('C','H','P',' ')},	
  {"chr",	HB_TAG('C','H','R',' ')},	
  {"cop",	HB_TAG('C','O','P',' ')},	
  {"cr",	HB_TAG('C','R','E',' ')},	
  {"crh",	HB_TAG('C','R','T',' ')},	
  {"crm",	HB_TAG('M','C','R',' ')},	
  {"crx",	HB_TAG('C','R','R',' ')},	
  {"cs",	HB_TAG('C','S','Y',' ')},	
  {"cu",	HB_TAG('C','S','L',' ')},	
  {"cv",	HB_TAG('C','H','U',' ')},	
  {"cwd",	HB_TAG('D','C','R',' ')},	
  {"cy",	HB_TAG('W','E','L',' ')},	
  {"da",	HB_TAG('D','A','N',' ')},	
  {"dap",	HB_TAG('N','I','S',' ')},	
  {"dar",	HB_TAG('D','A','R',' ')},	
  {"de",	HB_TAG('D','E','U',' ')},	
  {"din",	HB_TAG('D','N','K',' ')},	
  {"dng",	HB_TAG('D','U','N',' ')},	
  {"doi",	HB_TAG('D','G','R',' ')},	
  {"dsb",	HB_TAG('L','S','B',' ')},	
  {"dv",	HB_TAG('D','I','V',' ')},	
  {"dz",	HB_TAG('D','Z','N',' ')},	
  {"ee",	HB_TAG('E','W','E',' ')},	
  {"efi",	HB_TAG('E','F','I',' ')},	
  {"el",	HB_TAG('E','L','L',' ')},	
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
  {"fur",	HB_TAG('F','R','L',' ')},	
  {"fy",	HB_TAG('F','R','I',' ')},	
  {"ga",	HB_TAG('I','R','I',' ')},	
  {"gaa",	HB_TAG('G','A','D',' ')},	
  {"gag",	HB_TAG('G','A','G',' ')},	
  {"gbm",	HB_TAG('G','A','W',' ')},	
  {"gd",	HB_TAG('G','A','E',' ')},	
  {"gl",	HB_TAG('G','A','L',' ')},	
  {"gld",	HB_TAG('N','A','N',' ')},	
  {"gn",	HB_TAG('G','U','A',' ')},	
  {"gon",	HB_TAG('G','O','N',' ')},	
  {"grt",	HB_TAG('G','R','O',' ')},	
  {"gu",	HB_TAG('G','U','J',' ')},	
  {"guk",	HB_TAG('G','M','Z',' ')},	
  {"gv",	HB_TAG('M','N','X',' ')},	
  {"ha",	HB_TAG('H','A','U',' ')},	
  {"har",	HB_TAG('H','R','I',' ')},	
  {"he",	HB_TAG('I','W','R',' ')},	
  {"hi",	HB_TAG('H','I','N',' ')},	
  {"hil",	HB_TAG('H','I','L',' ')},	
  {"hoc",	HB_TAG('H','O',' ',' ')},	
  {"hr",	HB_TAG('H','R','V',' ')},	
  {"hsb",	HB_TAG('U','S','B',' ')},	
  {"ht",	HB_TAG('H','A','I',' ')},	
  {"hu",	HB_TAG('H','U','N',' ')},	
  {"hy",	HB_TAG('H','Y','E',' ')},	
  {"id",	HB_TAG('I','N','D',' ')},	
  {"ig",	HB_TAG('I','B','O',' ')},	
  {"igb",	HB_TAG('E','B','I',' ')},	
  {"inh",	HB_TAG('I','N','G',' ')},	
  {"is",	HB_TAG('I','S','L',' ')},	
  {"it",	HB_TAG('I','T','A',' ')},	
  {"iu",	HB_TAG('I','N','U',' ')},	
  {"ja",	HB_TAG('J','A','N',' ')},	
  {"jv",	HB_TAG('J','A','V',' ')},	
  {"ka",	HB_TAG('K','A','T',' ')},	
  {"kam",	HB_TAG('K','M','B',' ')},	
  {"kbd",	HB_TAG('K','A','B',' ')},	
  {"kdr",	HB_TAG('K','R','M',' ')},	
  {"kdt",	HB_TAG('K','U','Y',' ')},	
  {"kfr",	HB_TAG('K','A','C',' ')},	
  {"kfy",	HB_TAG('K','M','N',' ')},	
  {"kha",	HB_TAG('K','S','I',' ')},	
  {"khw",	HB_TAG('K','H','W',' ')},	
  {"ki",	HB_TAG('K','I','K',' ')},	
  {"kk",	HB_TAG('K','A','Z',' ')},	
  {"kl",	HB_TAG('G','R','N',' ')},	
  {"kln",	HB_TAG('K','A','L',' ')},	
  {"km",	HB_TAG('K','H','M',' ')},	
  {"kmw",	HB_TAG('K','M','O',' ')},	
  {"kn",	HB_TAG('K','A','N',' ')},	
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
  {"ku",	HB_TAG('K','U','R',' ')},	
  {"kum",	HB_TAG('K','U','M',' ')},	
  {"kvd",	HB_TAG('K','U','I',' ')},	
  {"kxu",	HB_TAG('K','U','I',' ')},	
  {"ky",	HB_TAG('K','I','R',' ')},	
  {"la",	HB_TAG('L','A','T',' ')},	
  {"lad",	HB_TAG('J','U','D',' ')},	
  {"lb",	HB_TAG('L','T','Z',' ')},	
  {"lbe",	HB_TAG('L','A','K',' ')},	
  {"lbj",	HB_TAG('L','D','K',' ')},	
  {"lif",	HB_TAG('L','M','B',' ')},	
  {"lld",	HB_TAG('L','A','D',' ')},	
  {"ln",	HB_TAG('L','I','N',' ')},	
  {"lo",	HB_TAG('L','A','O',' ')},	
  {"lt",	HB_TAG('L','T','H',' ')},	
  {"luo",	HB_TAG('L','U','O',' ')},	
  {"luw",	HB_TAG('L','U','O',' ')},	
  {"lv",	HB_TAG('L','V','I',' ')},	
  {"lzz",	HB_TAG('L','A','Z',' ')},	
  {"mai",	HB_TAG('M','T','H',' ')},	
  {"mdc",	HB_TAG('M','L','E',' ')},	
  {"mdf",	HB_TAG('M','O','K',' ')},	
  {"mdy",	HB_TAG('M','L','E',' ')},	
  {"men",	HB_TAG('M','D','E',' ')},	
  {"mg",	HB_TAG('M','L','G',' ')},	
  {"mi",	HB_TAG('M','R','I',' ')},	
  {"mk",	HB_TAG('M','K','D',' ')},	
  {"ml",	HB_TAG('M','L','R',' ')},	
  {"mn",	HB_TAG('M','N','G',' ')},	
  {"mnc",	HB_TAG('M','C','H',' ')},	
  {"mni",	HB_TAG('M','N','I',' ')},	
  {"mnk",	HB_TAG('M','N','D',' ')},	
  {"mns",	HB_TAG('M','A','N',' ')},	
  {"mnw",	HB_TAG('M','O','N',' ')},	
  {"mo",	HB_TAG('M','O','L',' ')},	
  {"moh",	HB_TAG('M','O','H',' ')},	
  {"mpe",	HB_TAG('M','A','J',' ')},	
  {"mr",	HB_TAG('M','A','R',' ')},	
  {"ms",	HB_TAG('M','L','Y',' ')},	
  {"mt",	HB_TAG('M','T','S',' ')},	
  {"mwr",	HB_TAG('M','A','W',' ')},	
  {"my",	HB_TAG('B','R','M',' ')},	
  {"mym",	HB_TAG('M','E','N',' ')},	
  {"myv",	HB_TAG('E','R','Z',' ')},	
  {"nb",	HB_TAG('N','O','R',' ')},	
  {"nco",	HB_TAG('S','I','B',' ')},	
  {"ne",	HB_TAG('N','E','P',' ')},	
  {"new",	HB_TAG('N','E','W',' ')},	
  {"ng",	HB_TAG('N','D','G',' ')},	
  {"ngl",	HB_TAG('L','M','W',' ')},	
  {"niu",	HB_TAG('N','I','U',' ')},	
  {"niv",	HB_TAG('G','I','L',' ')},	
  {"nl",	HB_TAG('N','L','D',' ')},	
  {"nn",	HB_TAG('N','Y','N',' ')},	
  {"no",	HB_TAG('N','O','R',' ')},	
  {"nog",	HB_TAG('N','O','G',' ')},	
  {"nqo",	HB_TAG('N','K','O',' ')},	
  {"nsk",	HB_TAG('N','A','S',' ')},	
  {"ny",	HB_TAG('C','H','I',' ')},	
  {"oc",	HB_TAG('O','C','I',' ')},	
  {"oj",	HB_TAG('O','J','B',' ')},	
  {"om",	HB_TAG('O','R','O',' ')},	
  {"or",	HB_TAG('O','R','I',' ')},	
  {"os",	HB_TAG('O','S','S',' ')},	
  {"pa",	HB_TAG('P','A','N',' ')},	
  {"pi",	HB_TAG('P','A','L',' ')},	
  {"pl",	HB_TAG('P','L','K',' ')},	
  {"plp",	HB_TAG('P','A','P',' ')},	
  {"prs",	HB_TAG('D','R','I',' ')},	
  {"ps",	HB_TAG('P','A','S',' ')},	
  {"pt",	HB_TAG('P','T','G',' ')},	
  {"raj",	HB_TAG('R','A','J',' ')},	
  {"ria",	HB_TAG('R','I','A',' ')},	
  {"ril",	HB_TAG('R','I','A',' ')},	
  {"ro",	HB_TAG('R','O','M',' ')},	
  {"rom",	HB_TAG('R','O','Y',' ')},	
  {"ru",	HB_TAG('R','U','S',' ')},	
  {"rue",	HB_TAG('R','S','Y',' ')},	
  {"sa",	HB_TAG('S','A','N',' ')},	
  {"sah",	HB_TAG('Y','A','K',' ')},	
  {"sat",	HB_TAG('S','A','T',' ')},	
  {"sck",	HB_TAG('S','A','D',' ')},	
  {"sd",	HB_TAG('S','N','D',' ')},	
  {"se",	HB_TAG('N','S','M',' ')},	
  {"seh",	HB_TAG('S','N','A',' ')},	
  {"sel",	HB_TAG('S','E','L',' ')},	
  {"sg",	HB_TAG('S','G','O',' ')},	
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
  {"snk",	HB_TAG('S','N','K',' ')},	
  {"so",	HB_TAG('S','M','L',' ')},	
  {"sq",	HB_TAG('S','Q','I',' ')},	
  {"sr",	HB_TAG('S','R','B',' ')},	
  {"srr",	HB_TAG('S','R','R',' ')},	
  {"suq",	HB_TAG('S','U','R',' ')},	
  {"sv",	HB_TAG('S','V','E',' ')},	
  {"sva",	HB_TAG('S','V','A',' ')},	
  {"sw",	HB_TAG('S','W','K',' ')},	
  {"swb",	HB_TAG('C','M','R',' ')},	
  {"syr",	HB_TAG('S','Y','R',' ')},	
  {"ta",	HB_TAG('T','A','M',' ')},	
  {"tcy",	HB_TAG('T','U','L',' ')},	
  {"te",	HB_TAG('T','E','L',' ')},	
  {"tg",	HB_TAG('T','A','J',' ')},	
  {"th",	HB_TAG('T','H','A',' ')},	
  {"ti",	HB_TAG('T','G','Y',' ')},	
  {"tig",	HB_TAG('T','G','R',' ')},	
  {"tk",	HB_TAG('T','K','M',' ')},	
  {"tn",	HB_TAG('T','N','A',' ')},	
  {"tnz",	HB_TAG('T','N','G',' ')},	
  {"to",	HB_TAG('T','N','G',' ')},	
  {"tog",	HB_TAG('T','N','G',' ')},	
  {"toi",	HB_TAG('T','N','G',' ')},	
  {"tr",	HB_TAG('T','R','K',' ')},	
  {"ts",	HB_TAG('T','S','G',' ')},	
  {"tt",	HB_TAG('T','A','T',' ')},	
  {"tw",	HB_TAG('T','W','I',' ')},	
  {"ty",	HB_TAG('T','H','T',' ')},	
  {"udm",	HB_TAG('U','D','M',' ')},	
  {"ug",	HB_TAG('U','Y','G',' ')},	
  {"uk",	HB_TAG('U','K','R',' ')},	
  {"unr",	HB_TAG('M','U','N',' ')},	
  {"ur",	HB_TAG('U','R','D',' ')},	
  {"uz",	HB_TAG('U','Z','B',' ')},	
  {"ve",	HB_TAG('V','E','N',' ')},	
  {"vi",	HB_TAG('V','I','T',' ')},	
  {"wbm",	HB_TAG('W','A',' ',' ')},	
  {"wbr",	HB_TAG('W','A','G',' ')},	
  {"wo",	HB_TAG('W','L','F',' ')},	
  {"xal",	HB_TAG('K','L','M',' ')},	
  {"xh",	HB_TAG('X','H','S',' ')},	
  {"xom",	HB_TAG('K','M','O',' ')},	
  {"xsl",	HB_TAG('S','S','L',' ')},	
  {"yi",	HB_TAG('J','I','I',' ')},	
  {"yo",	HB_TAG('Y','B','A',' ')},	
  {"yso",	HB_TAG('N','I','S',' ')},	
  {"zh-cn",	HB_TAG('Z','H','S',' ')},	
  {"zh-hk",	HB_TAG('Z','H','H',' ')},	
  {"zh-mo",	HB_TAG('Z','H','T',' ')},	
  {"zh-sg",	HB_TAG('Z','H','S',' ')},	
  {"zh-tw",	HB_TAG('Z','H','T',' ')},	
  {"zne",	HB_TAG('Z','N','D',' ')},	
  {"zu",	HB_TAG('Z','U','L',' ')} 	

  

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
};

static int
lang_compare_first_component (const char *a,
			      const char *b)
{
  unsigned int da, db;
  const char *p;

  p = strstr (a, "-");
  da = p ? (unsigned int) (p - a) : strlen (a);

  p = strstr (b, "-");
  db = p ? (unsigned int) (p - b) : strlen (b);

  return strncmp (a, b, MAX (da, db));
}

static hb_bool_t
lang_matches (const char *lang_str, const char *spec)
{
  unsigned int len = strlen (spec);

  return lang_str && strncmp (lang_str, spec, len) == 0 &&
	 (lang_str[len] == '\0' || lang_str[len] == '-');
}

hb_tag_t
hb_ot_tag_from_language (hb_language_t language)
{
  const char *lang_str;
  LangTag *lang_tag;

  if (language == NULL)
    return HB_OT_TAG_DEFAULT_LANGUAGE;

  lang_str = hb_language_to_string (language);

  if (0 == strcmp (lang_str, "x-hbot")) {
    char tag[4];
    int i;
    lang_str += 6;
#define IS_LETTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define TO_UPPER(c) (((c) >= 'a' && (c) <= 'z') ? (c) + 'A' - 'a' : (c))
    for (i = 0; i < 4 && IS_LETTER (lang_str[i]); i++)
      tag[i] = TO_UPPER (lang_str[i]);
    for (; i < 4; i++)
      tag[i] = ' ';
    return HB_TAG_STR (tag);
  }

  
  lang_tag = bsearch (lang_str, ot_languages,
		      ARRAY_LENGTH (ot_languages), sizeof (LangTag),
		      (hb_compare_func_t) lang_compare_first_component);

  
  if (lang_tag)
  {
    hb_bool_t found = FALSE;

    
    while (lang_tag + 1 < ot_languages + ARRAY_LENGTH (ot_languages) &&
	   lang_compare_first_component (lang_str, (lang_tag + 1)->language) == 0)
      lang_tag++;

    
    while (lang_tag >= ot_languages &&
	   lang_compare_first_component (lang_str, lang_tag->language) == 0)
    {
      if (lang_matches (lang_str, lang_tag->language)) {
	found = TRUE;
	break;
      }

      lang_tag--;
    }

    if (!found)
      lang_tag = NULL;
  }

  if (lang_tag)
    return lang_tag->tag;

  return HB_OT_TAG_DEFAULT_LANGUAGE;
}

hb_language_t
hb_ot_tag_to_language (hb_tag_t tag)
{
  unsigned int i;
  unsigned char buf[11] = "x-hbot";

  for (i = 0; i < ARRAY_LENGTH (ot_languages); i++)
    if (ot_languages[i].tag == tag)
      return hb_language_from_string (ot_languages[i].language);

  buf[6] = tag >> 24;
  buf[7] = (tag >> 16) & 0xFF;
  buf[8] = (tag >> 8) & 0xFF;
  buf[9] = tag & 0xFF;
  buf[10] = '\0';
  return hb_language_from_string ((char *) buf);
}


HB_END_DECLS
