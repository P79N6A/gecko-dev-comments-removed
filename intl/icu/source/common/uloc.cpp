





























#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uloc.h"

#include "putilimp.h"
#include "ustr_imp.h"
#include "ulocimp.h"
#include "umutex.h"
#include "cstring.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "locmap.h"
#include "uarrsort.h"
#include "uenumimp.h"
#include "uassert.h"

#include <stdio.h> 




U_CFUNC void locale_set_default(const char *id);
U_CFUNC const char *locale_get_default(void);
U_CFUNC int32_t
locale_getKeywords(const char *localeID,
            char prev,
            char *keywords, int32_t keywordCapacity,
            char *values, int32_t valuesCapacity, int32_t *valLen,
            UBool valuesToo,
            UErrorCode *status);


































static const char * const LANGUAGES[] = {
    "aa",  "ab",  "ace", "ach", "ada", "ady", "ae",  "af",  "afa",
    "afh", "agq", "ain", "ak",  "akk", "ale", "alg", "alt", "am",  "an",  
    "ang", "anp", "apa",
    "ar",  "arc", "arn", "arp", "art", "arw", "as", "asa", "ast",
    "ath", "aus", "av",  "awa", "ay",  "az",  "ba",  "bad",
    "bai", "bal", "ban", "bas", "bat", "be",  "bej",
    "bem", "ber", "bez", "bg",  "bh",  "bho", "bi",  "bik", "bin",
    "bla", "bm",  "bn",  "bnt", "bo",  "br",  "bra", "brx", "bs",
    "btk", "bua", "bug", "byn", "ca",  "cad", "cai", "car", "cau",
    "cch", "ce",  "ceb", "cel", "cgg", "ch",  "chb", "chg", "chk", "chm",
    "chn", "cho", "chp", "chr", "chy", "cmc", "co",  "cop",
    "cpe", "cpf", "cpp", "cr",  "crh", "crp", "cs",  "csb", "cu",  "cus",
    "cv",  "cy",  "da",  "dak", "dar", "dav", "day", "de",  "del", "den",
    "dgr", "din", "dje", "doi", "dra", "dsb", "dua", "dum", "dv",  "dyo", "dyu",
    "dz",  "ebu", "ee",  "efi", "egy", "eka", "el",  "elx", "en",
    "enm", "eo",  "es",  "et",  "eu",  "ewo", "fa",
    "fan", "fat", "ff",  "fi",  "fil", "fiu", "fj",  "fo",  "fon",
    "fr",  "frm", "fro", "frr", "frs", "fur", "fy",  
    "ga",  "gaa", "gan", "gay", "gba", "gd",  "gem", "gez", "gil", 
    "gl",  "gmh", "gn",  "goh", "gon", "gor", "got", "grb", 
    "grc", "gsw", "gu",  "guz", "gv",  "gwi",
    "ha",  "hai", "hak", "haw", "he",  "hi",  "hil", "him",
    "hit", "hmn", "ho",  "hr",  "hsb", "hsn", "ht",  "hu",  "hup", "hy",  "hz",
    "ia",  "iba", "id",  "ie",  "ig",  "ii",  "ijo", "ik",
    "ilo", "inc", "ine", "inh", "io",  "ira", "iro", "is",  "it",
    "iu",  "ja",  "jbo", "jgo", "jmc", "jpr", "jrb", "jv",  "ka",  "kaa", "kab",
    "kac", "kaj", "kam", "kar", "kaw", "kbd", "kcg", "kde", "kea", "kfo", "kg",  "kha", "khi",
    "kho", "khq", "ki",  "kj",  "kk",  "kl",  "kln", "km",  "kmb", "kn",
    "ko",  "kok", "kos", "kpe", "kr",  "krc", "krl", "kro", "kru", "ks", "ksb", "ksf",
    "ku",  "kum", "kut", "kv",  "kw",  "ky",  "la",  "lad", "lag",
    "lah", "lam", "lb",  "lez", "lg",  "li",  "ln",  "lo",  "lol",
    "loz", "lt",  "lu",  "lua", "lui", "lun", "luo", "lus", "luy",
    "lv",  "mad", "mag", "mai", "mak", "man", "map", "mas",
    "mdf", "mdr", "men", "mer", "mfe", "mg",  "mga", "mgh", "mgo", "mh",  "mi",  "mic", "min",
    "mis", "mk",  "mkh", "ml",  "mn",  "mnc", "mni", "mno",
    "mo",  "moh", "mos", "mr",  "ms",  "mt",  "mua", "mul", "mun",
    "mus", "mwl", "mwr", "my",  "myn", "myv", "na",  "nah", "nai", "nan", "nap", "naq",
    "nb",  "nd",  "nds", "ne",  "new", "ng",  "nia", "nic",
    "niu", "nl",  "nmg", "nn",  "no",  "nog", "non", "nqo", "nr",  "nso", "nub", "nus",
    "nv",  "nwc", "ny",  "nym", "nyn", "nyo", "nzi", "oc",  "oj",
    "om",  "or",  "os",  "osa", "ota", "oto", "pa",  "paa",
    "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",
    "pi",  "pl",  "pon", "pra", "pro", "ps",  "pt",  "qu",
    "raj", "rap", "rar", "rm",  "rn",  "ro",  "roa", "rof", "rom",
    "ru",  "rup", "rw",  "rwk", "sa",  "sad", "sah", "sai", "sal", "sam", "saq",
    "sas", "sat", "sbp", "sc",  "scn", "sco", "sd",  "se",  "seh", "sel", "sem", "ses",
    "sg",  "sga", "sgn", "shi", "shn", "si",  "sid", "sio", "sit",
    "sk",  "sl",  "sla", "sm",  "sma", "smi", "smj", "smn",
    "sms", "sn",  "snk", "so",  "sog", "son", "sq",  "sr",
    "srn", "srr", "ss",  "ssa", "st",  "su",  "suk", "sus", "sux",
    "sv",  "sw",  "swc", "syc", "syr", "ta",  "tai", "te",  "tem", "teo", "ter",
    "tet", "tg",  "th",  "ti",  "tig", "tiv", "tk",  "tkl",
    "tl",  "tlh", "tli", "tmh", "tn",  "to",  "tog", "tpi", "tr", "trv",
    "ts",  "tsi", "tt",  "tum", "tup", "tut", "tvl", "tw", "twq",
    "ty",  "tyv", "tzm", "udm", "ug",  "uga", "uk",  "umb", "und", "ur",
    "uz",  "vai", "ve",  "vi",  "vo",  "vot", "vun", "wa",  "wak",
    "wal", "war", "was", "wen", "wo",  "wuu", "xal", "xh",  "xog", "yao", "yap", "yav",
    "yi",  "yo",  "ypk", "yue", "za",  "zap", "zbl", "zen", "zh",  "znd",
    "zu",  "zun", "zxx", "zza",
NULL,
    "in",  "iw",  "ji",  "jw",  "sh",    
NULL
};
static const char* const DEPRECATED_LANGUAGES[]={
    "in", "iw", "ji", "jw", NULL, NULL
};
static const char* const REPLACEMENT_LANGUAGES[]={
    "id", "he", "yi", "jv", NULL, NULL
};

















static const char * const LANGUAGES_3[] = {

    "aar", "abk", "ace", "ach", "ada", "ady", "ave", "afr", "afa",

    "afh", "agq", "ain", "aka", "akk", "ale", "alg", "alt", "amh", "arg", "ang", "anp", "apa",

    "ara", "arc", "arn", "arp", "art", "arw", "asm", "asa", "ast",

    "ath", "aus", "ava", "awa", "aym", "aze", "bak", "bad",

    "bai", "bal", "ban", "bas", "bat", "bel", "bej",

    "bem", "ber", "bez", "bul", "bih", "bho", "bis", "bik", "bin",

    "bla", "bam", "ben", "bnt", "bod", "bre", "bra", "brx", "bos",

    "btk", "bua", "bug", "byn", "cat", "cad", "cai", "car", "cau",

    "cch", "che", "ceb", "cel", "cgg", "cha", "chb", "chg", "chk", "chm",

    "chn", "cho", "chp", "chr", "chy", "cmc", "cos", "cop",

    "cpe", "cpf", "cpp", "cre", "crh", "crp", "ces", "csb", "chu", "cus",

    "chv", "cym", "dan", "dak", "dar", "dav", "day", "deu", "del", "den",

    "dgr", "din", "dje", "doi", "dra", "dsb", "dua", "dum", "div", "dyo", "dyu",

    "dzo", "ebu", "ewe", "efi", "egy", "eka", "ell", "elx", "eng",

    "enm", "epo", "spa", "est", "eus", "ewo", "fas",

    "fan", "fat", "ful", "fin", "fil", "fiu", "fij", "fao", "fon",

    "fra", "frm", "fro", "frr", "frs", "fur", "fry", "gle", "gaa", "gan", "gay",

    "gba", "gla", "gem", "gez", "gil", "glg", "gmh", "grn",

    "goh", "gon", "gor", "got", "grb", "grc", "gsw", "guj", "guz", "glv",

    "gwi", "hau", "hai", "hak", "haw", "heb", "hin", "hil", "him",

    "hit", "hmn", "hmo", "hrv", "hsb", "hsn", "hat", "hun", "hup", "hye", "her",

    "ina", "iba", "ind", "ile", "ibo", "iii", "ijo", "ipk",

    "ilo", "inc", "ine", "inh", "ido", "ira", "iro", "isl", "ita",

    "iku", "jpn", "jbo", "jgo", "jmc", "jpr", "jrb", "jav", "kat", "kaa", "kab",

    "kac", "kaj", "kam", "kar", "kaw", "kbd", "kcg", "kde", "kea", "kfo", "kg",  "kha", "khi",

    "kho", "khq", "kik", "kua", "kaz", "kal", "kln", "khm", "kmb", "kan",

    "kor", "kok", "kos", "kpe", "kau", "krc", "krl", "kro", "kru", "kas", "ksb", "ksf",

    "kur", "kum", "kut", "kom", "cor", "kir", "lat", "lad", "lag",

    "lah", "lam", "ltz", "lez", "lug", "lim", "lin", "lao", "lol",

    "loz", "lit", "lub", "lua", "lui", "lun", "luo", "lus", "luy",

    "lav", "mad", "mag", "mai", "mak", "man", "map", "mas",

    "mdf", "mdr", "men", "mer", "mfe", "mlg", "mga", "mgh", "mgo", "mah", "mri", "mic", "min",

    "mis", "mkd", "mkh", "mal", "mon", "mnc", "mni", "mno",

    "mol", "moh", "mos", "mar", "msa", "mlt", "mua", "mul", "mun",

    "mus", "mwl", "mwr", "mya", "myn", "myv", "nau", "nah", "nai", "nan", "nap", "naq",

    "nob", "nde", "nds", "nep", "new", "ndo", "nia", "nic",

    "niu", "nld", "nmg", "nno", "nor", "nog", "non", "nqo", "nbl", "nso", "nub", "nus",

    "nav", "nwc", "nya", "nym", "nyn", "nyo", "nzi", "oci", "oji",

    "orm", "ori", "oss", "osa", "ota", "oto", "pan", "paa",

    "pag", "pal", "pam", "pap", "pau", "peo", "phi", "phn",

    "pli", "pol", "pon", "pra", "pro", "pus", "por", "que",

    "raj", "rap", "rar", "roh", "run", "ron", "roa", "rof", "rom",

    "rus", "rup", "kin", "rwk", "san", "sad", "sah", "sai", "sal", "sam", "saq",

    "sas", "sat", "sbp", "srd", "scn", "sco", "snd", "sme", "seh", "sel", "sem", "ses",

    "sag", "sga", "sgn", "shi", "shn", "sin", "sid", "sio", "sit",

    "slk", "slv", "sla", "smo", "sma", "smi", "smj", "smn",

    "sms", "sna", "snk", "som", "sog", "son", "sqi", "srp",

    "srn", "srr", "ssw", "ssa", "sot", "sun", "suk", "sus", "sux",

    "swe", "swa", "swc", "syc", "syr", "tam", "tai", "tel", "tem", "teo", "ter",

    "tet", "tgk", "tha", "tir", "tig", "tiv", "tuk", "tkl",

    "tgl", "tlh", "tli", "tmh", "tsn", "ton", "tog", "tpi", "tur", "trv",

    "tso", "tsi", "tat", "tum", "tup", "tut", "tvl", "twi", "twq",

    "tah", "tyv", "tzm", "udm", "uig", "uga", "ukr", "umb", "und", "urd",

    "uzb", "vai", "ven", "vie", "vol", "vot", "vun", "wln", "wak",

    "wal", "war", "was", "wen", "wol", "wuu", "xal", "xho", "xog", "yao", "yap", "yav",

    "yid", "yor", "ypk", "yue", "zha", "zap", "zbl", "zen", "zho", "znd",

    "zul", "zun", "zxx", "zza",
NULL,

    "ind", "heb", "yid", "jaw", "srp",
NULL
};

























static const char * const COUNTRIES[] = {
    "AD",  "AE",  "AF",  "AG",  "AI",  "AL",  "AM",  "AN",
    "AO",  "AQ",  "AR",  "AS",  "AT",  "AU",  "AW",  "AX",  "AZ",
    "BA",  "BB",  "BD",  "BE",  "BF",  "BG",  "BH",  "BI",
    "BJ",  "BL",  "BM",  "BN",  "BO",  "BR",  "BS",  "BT",  "BV",
    "BW",  "BY",  "BZ",  "CA",  "CC",  "CD",  "CF",  "CG",
    "CH",  "CI",  "CK",  "CL",  "CM",  "CN",  "CO",  "CR",
    "CU",  "CV",  "CX",  "CY",  "CZ",  "DE",  "DJ",  "DK",
    "DM",  "DO",  "DZ",  "EC",  "EE",  "EG",  "EH",  "ER",
    "ES",  "ET",  "FI",  "FJ",  "FK",  "FM",  "FO",  "FR",
    "GA",  "GB",  "GD",  "GE",  "GF",  "GG",  "GH",  "GI",  "GL",
    "GM",  "GN",  "GP",  "GQ",  "GR",  "GS",  "GT",  "GU",
    "GW",  "GY",  "HK",  "HM",  "HN",  "HR",  "HT",  "HU",
    "ID",  "IE",  "IL",  "IM",  "IN",  "IO",  "IQ",  "IR",  "IS",
    "IT",  "JE",  "JM",  "JO",  "JP",  "KE",  "KG",  "KH",  "KI",
    "KM",  "KN",  "KP",  "KR",  "KW",  "KY",  "KZ",  "LA",
    "LB",  "LC",  "LI",  "LK",  "LR",  "LS",  "LT",  "LU",
    "LV",  "LY",  "MA",  "MC",  "MD",  "ME",  "MF",  "MG",  "MH",  "MK",
    "ML",  "MM",  "MN",  "MO",  "MP",  "MQ",  "MR",  "MS",
    "MT",  "MU",  "MV",  "MW",  "MX",  "MY",  "MZ",  "NA",
    "NC",  "NE",  "NF",  "NG",  "NI",  "NL",  "NO",  "NP",
    "NR",  "NU",  "NZ",  "OM",  "PA",  "PE",  "PF",  "PG",
    "PH",  "PK",  "PL",  "PM",  "PN",  "PR",  "PS",  "PT",
    "PW",  "PY",  "QA",  "RE",  "RO",  "RS",  "RU",  "RW",  "SA",
    "SB",  "SC",  "SD",  "SE",  "SG",  "SH",  "SI",  "SJ",
    "SK",  "SL",  "SM",  "SN",  "SO",  "SR",  "ST",  "SV",
    "SY",  "SZ",  "TC",  "TD",  "TF",  "TG",  "TH",  "TJ",
    "TK",  "TL",  "TM",  "TN",  "TO",  "TR",  "TT",  "TV",
    "TW",  "TZ",  "UA",  "UG",  "UM",  "US",  "UY",  "UZ",
    "VA",  "VC",  "VE",  "VG",  "VI",  "VN",  "VU",  "WF",
    "WS",  "YE",  "YT",  "ZA",  "ZM",  "ZW",
NULL,
    "FX",  "CS",  "RO",  "TP",  "YU",  "ZR",   
NULL
};

static const char* const DEPRECATED_COUNTRIES[] ={
    "BU", "CS", "DY", "FX", "HV", "NH", "RH", "TP", "YU", "ZR", NULL, NULL 
};
static const char* const REPLACEMENT_COUNTRIES[] = {

    "MM", "RS", "BJ", "FR", "BF", "VU", "ZW", "TL", "RS", "CD", NULL, NULL        
};
    













static const char * const COUNTRIES_3[] = {

    "AND", "ARE", "AFG", "ATG", "AIA", "ALB", "ARM", "ANT",

    "AGO", "ATA", "ARG", "ASM", "AUT", "AUS", "ABW", "ALA", "AZE",

    "BIH", "BRB", "BGD", "BEL", "BFA", "BGR", "BHR", "BDI",

    "BEN", "BLM", "BMU", "BRN", "BOL", "BRA", "BHS", "BTN", "BVT",

    "BWA", "BLR", "BLZ", "CAN", "CCK", "COD", "CAF", "COG",

    "CHE", "CIV", "COK", "CHL", "CMR", "CHN", "COL", "CRI",

    "CUB", "CPV", "CXR", "CYP", "CZE", "DEU", "DJI", "DNK",

    "DMA", "DOM", "DZA", "ECU", "EST", "EGY", "ESH", "ERI",

    "ESP", "ETH", "FIN", "FJI", "FLK", "FSM", "FRO", "FRA",

    "GAB", "GBR", "GRD", "GEO", "GUF", "GGY", "GHA", "GIB", "GRL",

    "GMB", "GIN", "GLP", "GNQ", "GRC", "SGS", "GTM", "GUM",

    "GNB", "GUY", "HKG", "HMD", "HND", "HRV", "HTI", "HUN",

    "IDN", "IRL", "ISR", "IMN", "IND", "IOT", "IRQ", "IRN", "ISL",

    "ITA", "JEY", "JAM", "JOR", "JPN", "KEN", "KGZ", "KHM", "KIR",

    "COM", "KNA", "PRK", "KOR", "KWT", "CYM", "KAZ", "LAO",

    "LBN", "LCA", "LIE", "LKA", "LBR", "LSO", "LTU", "LUX",

    "LVA", "LBY", "MAR", "MCO", "MDA", "MNE", "MAF", "MDG", "MHL", "MKD",

    "MLI", "MMR", "MNG", "MAC", "MNP", "MTQ", "MRT", "MSR",

    "MLT", "MUS", "MDV", "MWI", "MEX", "MYS", "MOZ", "NAM",

    "NCL", "NER", "NFK", "NGA", "NIC", "NLD", "NOR", "NPL",

    "NRU", "NIU", "NZL", "OMN", "PAN", "PER", "PYF", "PNG",

    "PHL", "PAK", "POL", "SPM", "PCN", "PRI", "PSE", "PRT",

    "PLW", "PRY", "QAT", "REU", "ROU", "SRB", "RUS", "RWA", "SAU",

    "SLB", "SYC", "SDN", "SWE", "SGP", "SHN", "SVN", "SJM",

    "SVK", "SLE", "SMR", "SEN", "SOM", "SUR", "STP", "SLV",

    "SYR", "SWZ", "TCA", "TCD", "ATF", "TGO", "THA", "TJK",

    "TKL", "TLS", "TKM", "TUN", "TON", "TUR", "TTO", "TUV",

    "TWN", "TZA", "UKR", "UGA", "UMI", "USA", "URY", "UZB",

    "VAT", "VCT", "VEN", "VGB", "VIR", "VNM", "VUT", "WLF",

    "WSM", "YEM", "MYT", "ZAF", "ZMB", "ZWE",
NULL,

    "FXX", "SCG", "ROM", "TMP", "YUG", "ZAR",
NULL
};

typedef struct CanonicalizationMap {
    const char *id;          
    const char *canonicalID; 
    const char *keyword;     
    const char *value;       
} CanonicalizationMap;





static const CanonicalizationMap CANONICALIZE_MAP[] = {
    { "",               "en_US_POSIX", NULL, NULL }, 
    { "c",              "en_US_POSIX", NULL, NULL }, 
    { "posix",          "en_US_POSIX", NULL, NULL }, 
    { "art_LOJBAN",     "jbo", NULL, NULL }, 
    { "az_AZ_CYRL",     "az_Cyrl_AZ", NULL, NULL }, 
    { "az_AZ_LATN",     "az_Latn_AZ", NULL, NULL }, 
    { "ca_ES_PREEURO",  "ca_ES", "currency", "ESP" },
    { "de__PHONEBOOK",  "de", "collation", "phonebook" }, 
    { "de_AT_PREEURO",  "de_AT", "currency", "ATS" },
    { "de_DE_PREEURO",  "de_DE", "currency", "DEM" },
    { "de_LU_PREEURO",  "de_LU", "currency", "LUF" },
    { "el_GR_PREEURO",  "el_GR", "currency", "GRD" },
    { "en_BE_PREEURO",  "en_BE", "currency", "BEF" },
    { "en_IE_PREEURO",  "en_IE", "currency", "IEP" },
    { "es__TRADITIONAL", "es", "collation", "traditional" }, 
    { "es_ES_PREEURO",  "es_ES", "currency", "ESP" },
    { "eu_ES_PREEURO",  "eu_ES", "currency", "ESP" },
    { "fi_FI_PREEURO",  "fi_FI", "currency", "FIM" },
    { "fr_BE_PREEURO",  "fr_BE", "currency", "BEF" },
    { "fr_FR_PREEURO",  "fr_FR", "currency", "FRF" },
    { "fr_LU_PREEURO",  "fr_LU", "currency", "LUF" },
    { "ga_IE_PREEURO",  "ga_IE", "currency", "IEP" },
    { "gl_ES_PREEURO",  "gl_ES", "currency", "ESP" },
    { "hi__DIRECT",     "hi", "collation", "direct" }, 
    { "it_IT_PREEURO",  "it_IT", "currency", "ITL" },
    { "ja_JP_TRADITIONAL", "ja_JP", "calendar", "japanese" }, 
    { "nb_NO_NY",       "nn_NO", NULL, NULL },  
    { "nl_BE_PREEURO",  "nl_BE", "currency", "BEF" },
    { "nl_NL_PREEURO",  "nl_NL", "currency", "NLG" },
    { "pt_PT_PREEURO",  "pt_PT", "currency", "PTE" },
    { "sr_SP_CYRL",     "sr_Cyrl_RS", NULL, NULL }, 
    { "sr_SP_LATN",     "sr_Latn_RS", NULL, NULL }, 
    { "sr_YU_CYRILLIC", "sr_Cyrl_RS", NULL, NULL }, 
    { "th_TH_TRADITIONAL", "th_TH", "calendar", "buddhist" }, 
    { "uz_UZ_CYRILLIC", "uz_Cyrl_UZ", NULL, NULL }, 
    { "uz_UZ_CYRL",     "uz_Cyrl_UZ", NULL, NULL }, 
    { "uz_UZ_LATN",     "uz_Latn_UZ", NULL, NULL }, 
    { "zh_CHS",         "zh_Hans", NULL, NULL }, 
    { "zh_CHT",         "zh_Hant", NULL, NULL }, 
    { "zh_GAN",         "gan", NULL, NULL }, 
    { "zh_GUOYU",       "zh", NULL, NULL }, 
    { "zh_HAKKA",       "hak", NULL, NULL }, 
    { "zh_MIN_NAN",     "nan", NULL, NULL }, 
    { "zh_WUU",         "wuu", NULL, NULL }, 
    { "zh_XIANG",       "hsn", NULL, NULL }, 
    { "zh_YUE",         "yue", NULL, NULL }, 
};

typedef struct VariantMap {
    const char *variant;          
    const char *keyword;     
    const char *value;       
} VariantMap;

static const VariantMap VARIANT_MAP[] = {
    { "EURO",   "currency", "EUR" },
    { "PINYIN", "collation", "pinyin" }, 
    { "STROKE", "collation", "stroke" }  
};



#define _hasBCP47Extension(id) (id && uprv_strstr(id, "@") == NULL && getShortestSubtagLength(localeID) == 1)

#define _ConvertBCP47(finalID, id, buffer, length,err) \
        if (uloc_forLanguageTag(id, buffer, length, NULL, err) <= 0 || U_FAILURE(*err)) { \
            finalID=id; \
        } else { \
            finalID=buffer; \
        }

static int32_t getShortestSubtagLength(const char *localeID) {
    int32_t localeIDLength = uprv_strlen(localeID);
    int32_t length = localeIDLength;
    int32_t tmpLength = 0;
    int32_t i;
    UBool reset = TRUE;

    for (i = 0; i < localeIDLength; i++) {
        if (localeID[i] != '_' && localeID[i] != '-') {
            if (reset) {
                tmpLength = 0;
                reset = FALSE;
            }
            tmpLength++;
        } else {
            if (tmpLength != 0 && tmpLength < length) {
                length = tmpLength;
            }
            reset = TRUE;
        }
    }

    return length;
}



#define ULOC_KEYWORD_BUFFER_LEN 25
#define ULOC_MAX_NO_KEYWORDS 25

U_CAPI const char * U_EXPORT2
locale_getKeywordsStart(const char *localeID) {
    const char *result = NULL;
    if((result = uprv_strchr(localeID, '@')) != NULL) {
        return result;
    }
#if (U_CHARSET_FAMILY == U_EBCDIC_FAMILY)
    else {
        


        static const uint8_t ebcdicSigns[] = { 0x7C, 0x44, 0x66, 0x80, 0xAC, 0xAE, 0xAF, 0xB5, 0xEC, 0xEF, 0x00 };
        const uint8_t *charToFind = ebcdicSigns;
        while(*charToFind) {
            if((result = uprv_strchr(localeID, *charToFind)) != NULL) {
                return result;
            }
            charToFind++;
        }
    }
#endif
    return NULL;
}







static int32_t locale_canonKeywordName(char *buf, const char *keywordName, UErrorCode *status)
{
  int32_t i;
  int32_t keywordNameLen = (int32_t)uprv_strlen(keywordName);
  
  if(keywordNameLen >= ULOC_KEYWORD_BUFFER_LEN) {
    
    *status = U_INTERNAL_PROGRAM_ERROR;
          return 0;
  }
  
  
  for(i = 0; i < keywordNameLen; i++) {
    buf[i] = uprv_tolower(keywordName[i]);
  }
  buf[i] = 0;
    
  return keywordNameLen;
}

typedef struct {
    char keyword[ULOC_KEYWORD_BUFFER_LEN];
    int32_t keywordLen;
    const char *valueStart;
    int32_t valueLen;
} KeywordStruct;

static int32_t U_CALLCONV
compareKeywordStructs(const void * , const void *left, const void *right) {
    const char* leftString = ((const KeywordStruct *)left)->keyword;
    const char* rightString = ((const KeywordStruct *)right)->keyword;
    return uprv_strcmp(leftString, rightString);
}







static int32_t
_getKeywords(const char *localeID,
             char prev,
             char *keywords, int32_t keywordCapacity,
             char *values, int32_t valuesCapacity, int32_t *valLen,
             UBool valuesToo,
             const char* addKeyword,
             const char* addValue,
             UErrorCode *status)
{
    KeywordStruct keywordList[ULOC_MAX_NO_KEYWORDS];
    
    int32_t maxKeywords = ULOC_MAX_NO_KEYWORDS;
    int32_t numKeywords = 0;
    const char* pos = localeID;
    const char* equalSign = NULL;
    const char* semicolon = NULL;
    int32_t i = 0, j, n;
    int32_t keywordsLen = 0;
    int32_t valuesLen = 0;

    if(prev == '@') { 
        
        do {
            UBool duplicate = FALSE;
            
            while(*pos == ' ') {
                pos++;
            }
            if (!*pos) { 
                break;
            }
            if(numKeywords == maxKeywords) {
                *status = U_INTERNAL_PROGRAM_ERROR;
                return 0;
            }
            equalSign = uprv_strchr(pos, '=');
            semicolon = uprv_strchr(pos, ';');
            
            
            if(!equalSign || (semicolon && semicolon<equalSign)) {
                *status = U_INVALID_FORMAT_ERROR;
                return 0;
            }
            
            if(equalSign - pos >= ULOC_KEYWORD_BUFFER_LEN) {
                
                *status = U_INTERNAL_PROGRAM_ERROR;
                return 0;
            }
            for(i = 0, n = 0; i < equalSign - pos; ++i) {
                if (pos[i] != ' ') {
                    keywordList[numKeywords].keyword[n++] = uprv_tolower(pos[i]);
                }
            }
            keywordList[numKeywords].keyword[n] = 0;
            keywordList[numKeywords].keywordLen = n;
            
            equalSign++;
            
            while(*equalSign == ' ') {
                equalSign++;
            }
            keywordList[numKeywords].valueStart = equalSign;
            
            pos = semicolon;
            i = 0;
            if(pos) {
                while(*(pos - i - 1) == ' ') {
                    i++;
                }
                keywordList[numKeywords].valueLen = (int32_t)(pos - equalSign - i);
                pos++;
            } else {
                i = (int32_t)uprv_strlen(equalSign);
                while(i && equalSign[i-1] == ' ') {
                    i--;
                }
                keywordList[numKeywords].valueLen = i;
            }
            
            for (j=0; j<numKeywords; ++j) {
                if (uprv_strcmp(keywordList[j].keyword, keywordList[numKeywords].keyword) == 0) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate) {
                ++numKeywords;
            }
        } while(pos);

        
        if (addKeyword != NULL) {
            UBool duplicate = FALSE;
            U_ASSERT(addValue != NULL);
            

            for (j=0; j<numKeywords; ++j) {
                if (uprv_strcmp(keywordList[j].keyword, addKeyword) == 0) {
                    duplicate = TRUE;
                    break;
                }
            }
            if (!duplicate) {
                if (numKeywords == maxKeywords) {
                    *status = U_INTERNAL_PROGRAM_ERROR;
                    return 0;
                }
                uprv_strcpy(keywordList[numKeywords].keyword, addKeyword);
                keywordList[numKeywords].keywordLen = (int32_t)uprv_strlen(addKeyword);
                keywordList[numKeywords].valueStart = addValue;
                keywordList[numKeywords].valueLen = (int32_t)uprv_strlen(addValue);
                ++numKeywords;
            }
        } else {
            U_ASSERT(addValue == NULL);
        }

        
        
        uprv_sortArray(keywordList, numKeywords, sizeof(KeywordStruct), compareKeywordStructs, NULL, FALSE, status);
        
        
        for(i = 0; i < numKeywords; i++) {
            if(keywordsLen + keywordList[i].keywordLen + 1< keywordCapacity) {
                uprv_strcpy(keywords+keywordsLen, keywordList[i].keyword);
                if(valuesToo) {
                    keywords[keywordsLen + keywordList[i].keywordLen] = '=';
                } else {
                    keywords[keywordsLen + keywordList[i].keywordLen] = 0;
                }
            }
            keywordsLen += keywordList[i].keywordLen + 1;
            if(valuesToo) {
                if(keywordsLen + keywordList[i].valueLen < keywordCapacity) {
                    uprv_strncpy(keywords+keywordsLen, keywordList[i].valueStart, keywordList[i].valueLen);
                }
                keywordsLen += keywordList[i].valueLen;
                
                if(i < numKeywords - 1) {
                    if(keywordsLen < keywordCapacity) {       
                        keywords[keywordsLen] = ';';
                    }
                    keywordsLen++;
                }
            }
            if(values) {
                if(valuesLen + keywordList[i].valueLen + 1< valuesCapacity) {
                    uprv_strcpy(values+valuesLen, keywordList[i].valueStart);
                    values[valuesLen + keywordList[i].valueLen] = 0;
                }
                valuesLen += keywordList[i].valueLen + 1;
            }
        }
        if(values) {
            values[valuesLen] = 0;
            if(valLen) {
                *valLen = valuesLen;
            }
        }
        return u_terminateChars(keywords, keywordCapacity, keywordsLen, status);   
    } else {
        return 0;
    }
}

U_CFUNC int32_t
locale_getKeywords(const char *localeID,
                   char prev,
                   char *keywords, int32_t keywordCapacity,
                   char *values, int32_t valuesCapacity, int32_t *valLen,
                   UBool valuesToo,
                   UErrorCode *status) {
    return _getKeywords(localeID, prev, keywords, keywordCapacity,
                        values, valuesCapacity, valLen, valuesToo,
                        NULL, NULL, status);
}

U_CAPI int32_t U_EXPORT2
uloc_getKeywordValue(const char* localeID,
                     const char* keywordName,
                     char* buffer, int32_t bufferCapacity,
                     UErrorCode* status)
{ 
    const char* startSearchHere = NULL;
    const char* nextSeparator = NULL;
    char keywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    char localeKeywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    int32_t i = 0;
    int32_t result = 0;

    if(status && U_SUCCESS(*status) && localeID) {
      char tempBuffer[ULOC_FULLNAME_CAPACITY];
      const char* tmpLocaleID;

      if (_hasBCP47Extension(localeID)) {
          _ConvertBCP47(tmpLocaleID, localeID, tempBuffer, sizeof(tempBuffer), status);
      } else {
          tmpLocaleID=localeID;
      }
    
      startSearchHere = uprv_strchr(tmpLocaleID, '@'); 
      if(startSearchHere == NULL) {
          
          return 0;
      }

      locale_canonKeywordName(keywordNameBuffer, keywordName, status);
      if(U_FAILURE(*status)) {
        return 0;
      }
    
      
      while(startSearchHere) {
          startSearchHere++;
          
          while(*startSearchHere == ' ') {
              startSearchHere++;
          }
          nextSeparator = uprv_strchr(startSearchHere, '=');
          
          if(!nextSeparator) {
              break;
          }
          if(nextSeparator - startSearchHere >= ULOC_KEYWORD_BUFFER_LEN) {
              
              *status = U_INTERNAL_PROGRAM_ERROR;
              return 0;
          }
          for(i = 0; i < nextSeparator - startSearchHere; i++) {
              localeKeywordNameBuffer[i] = uprv_tolower(startSearchHere[i]);
          }
          
          while(startSearchHere[i-1] == ' ') {
              i--;
              U_ASSERT(i>=0);
          }
          localeKeywordNameBuffer[i] = 0;
        
          startSearchHere = uprv_strchr(nextSeparator, ';');
        
          if(uprv_strcmp(keywordNameBuffer, localeKeywordNameBuffer) == 0) {
              nextSeparator++;
              while(*nextSeparator == ' ') {
                  nextSeparator++;
              }
              
              if(startSearchHere && startSearchHere - nextSeparator < bufferCapacity) {
                  while(*(startSearchHere-1) == ' ') {
                      startSearchHere--;
                  }
                  uprv_strncpy(buffer, nextSeparator, startSearchHere - nextSeparator);
                  result = u_terminateChars(buffer, bufferCapacity, (int32_t)(startSearchHere - nextSeparator), status);
              } else if(!startSearchHere && (int32_t)uprv_strlen(nextSeparator) < bufferCapacity) { 
                  i = (int32_t)uprv_strlen(nextSeparator);
                  while(nextSeparator[i - 1] == ' ') {
                      i--;
                  }
                  uprv_strncpy(buffer, nextSeparator, i);
                  result = u_terminateChars(buffer, bufferCapacity, i, status);
              } else {
                  
                  *status = U_BUFFER_OVERFLOW_ERROR;
                  if(startSearchHere) {
                      result = (int32_t)(startSearchHere - nextSeparator);
                  } else {
                      result = (int32_t)uprv_strlen(nextSeparator); 
                  }
              }
              return result;
          }
      }
    }
    return 0;
}

U_CAPI int32_t U_EXPORT2
uloc_setKeywordValue(const char* keywordName,
                     const char* keywordValue,
                     char* buffer, int32_t bufferCapacity,
                     UErrorCode* status)
{
    
    int32_t keywordNameLen;
    int32_t keywordValueLen;
    int32_t bufLen;
    int32_t needLen = 0;
    int32_t foundValueLen;
    int32_t keywordAtEnd = 0; 
    char keywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    char localeKeywordNameBuffer[ULOC_KEYWORD_BUFFER_LEN];
    int32_t i = 0;
    int32_t rc;
    char* nextSeparator = NULL;
    char* nextEqualsign = NULL;
    char* startSearchHere = NULL;
    char* keywordStart = NULL;
    char *insertHere = NULL;
    if(U_FAILURE(*status)) { 
        return -1; 
    }
    if(bufferCapacity>1) {
        bufLen = (int32_t)uprv_strlen(buffer);
    } else {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(bufferCapacity<bufLen) {
        
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(keywordValue && !*keywordValue) { 
        keywordValue = NULL;
    }
    if(keywordValue) {
        keywordValueLen = (int32_t)uprv_strlen(keywordValue);
    } else { 
        keywordValueLen = 0;
    }
    keywordNameLen = locale_canonKeywordName(keywordNameBuffer, keywordName, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    startSearchHere = (char*)locale_getKeywordsStart(buffer);
    if(startSearchHere == NULL || (startSearchHere[1]==0)) {
        if(!keywordValue) { 
            return bufLen; 
        }

        needLen = bufLen+1+keywordNameLen+1+keywordValueLen;
        if(startSearchHere) {  
            needLen--; 
            
        } else {
            startSearchHere=buffer+bufLen;
        }
        if(needLen >= bufferCapacity) {
            *status = U_BUFFER_OVERFLOW_ERROR;
            return needLen; 
        }
        *startSearchHere = '@';
        startSearchHere++;
        uprv_strcpy(startSearchHere, keywordNameBuffer);
        startSearchHere += keywordNameLen;
        *startSearchHere = '=';
        startSearchHere++;
        uprv_strcpy(startSearchHere, keywordValue);
        startSearchHere+=keywordValueLen;
        return needLen;
    } 
    
    keywordStart = startSearchHere;
    
    while(keywordStart) {
        keywordStart++;
        
        while(*keywordStart == ' ') {
            keywordStart++;
        }
        nextEqualsign = uprv_strchr(keywordStart, '=');
        
        if(!nextEqualsign) {
            break;
        }
        if(nextEqualsign - keywordStart >= ULOC_KEYWORD_BUFFER_LEN) {
            
            *status = U_INTERNAL_PROGRAM_ERROR;
            return 0;
        }
        for(i = 0; i < nextEqualsign - keywordStart; i++) {
            localeKeywordNameBuffer[i] = uprv_tolower(keywordStart[i]);
        }
        
        while(keywordStart[i-1] == ' ') {
            i--;
        }
        U_ASSERT(i>=0 && i<ULOC_KEYWORD_BUFFER_LEN);
        localeKeywordNameBuffer[i] = 0;

        nextSeparator = uprv_strchr(nextEqualsign, ';');
        rc = uprv_strcmp(keywordNameBuffer, localeKeywordNameBuffer);
        if(rc == 0) {
            nextEqualsign++;
            while(*nextEqualsign == ' ') {
                nextEqualsign++;
            }
            
            if (nextSeparator) {
                keywordAtEnd = 0;
                foundValueLen = (int32_t)(nextSeparator - nextEqualsign);
            } else {
                keywordAtEnd = 1;
                foundValueLen = (int32_t)uprv_strlen(nextEqualsign);
            }
            if(keywordValue) { 
              if(foundValueLen == keywordValueLen) {
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                return bufLen; 
              } else if(foundValueLen > keywordValueLen) {
                int32_t delta = foundValueLen - keywordValueLen;
                if(nextSeparator) { 
                  uprv_memmove(nextSeparator - delta, nextSeparator, bufLen-(nextSeparator-buffer));
                }
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                bufLen -= delta;
                buffer[bufLen]=0;
                return bufLen;
              } else { 
                int32_t delta = keywordValueLen - foundValueLen;
                if((bufLen+delta) >= bufferCapacity) {
                  *status = U_BUFFER_OVERFLOW_ERROR;
                  return bufLen+delta;
                }
                if(nextSeparator) { 
                  uprv_memmove(nextSeparator+delta,nextSeparator, bufLen-(nextSeparator-buffer));
                }
                uprv_strncpy(nextEqualsign, keywordValue, keywordValueLen);
                bufLen += delta;
                buffer[bufLen]=0;
                return bufLen;
              }
            } else { 
              if(keywordAtEnd) {
                
                keywordStart[-1] = 0;
                return (int32_t)((keywordStart-buffer)-1); 
              } else {
                uprv_memmove(keywordStart, nextSeparator+1, bufLen-((nextSeparator+1)-buffer));
                keywordStart[bufLen-((nextSeparator+1)-buffer)]=0;
                return (int32_t)(bufLen-((nextSeparator+1)-keywordStart));
              }
            }
        } else if(rc<0){ 
          
          insertHere = keywordStart;
        }
        keywordStart = nextSeparator;
    } 
    
    if(!keywordValue) {
      return bufLen; 
    }

    
    needLen = bufLen+1+keywordNameLen+1+keywordValueLen;
    if(needLen >= bufferCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return needLen; 
    }
    
    if(insertHere) {
      uprv_memmove(insertHere+(1+keywordNameLen+1+keywordValueLen), insertHere, bufLen-(insertHere-buffer));
      keywordStart = insertHere;
    } else {
      keywordStart = buffer+bufLen;
      *keywordStart = ';';
      keywordStart++;
    }
    uprv_strncpy(keywordStart, keywordNameBuffer, keywordNameLen);
    keywordStart += keywordNameLen;
    *keywordStart = '=';
    keywordStart++;
    uprv_strncpy(keywordStart, keywordValue, keywordValueLen); 
    keywordStart+=keywordValueLen;
    if(insertHere) {
      *keywordStart = ';';
      keywordStart++;
    }
    buffer[needLen]=0;
    return needLen;
}



#define _isPrefixLetter(a) ((a=='x')||(a=='X')||(a=='i')||(a=='I'))



#define _isIDPrefix(s) (_isPrefixLetter(s[0])&&_isIDSeparator(s[1]))




#define _isTerminator(a)  ((a==0)||(a=='.')||(a=='@'))

static char* _strnchr(const char* str, int32_t len, char c) {
    U_ASSERT(str != 0 && len >= 0);
    while (len-- != 0) {
        char d = *str;
        if (d == c) {
            return (char*) str;
        } else if (d == 0) {
            break;
        }
        ++str;
    }
    return NULL;
}








static int16_t _findIndex(const char* const* list, const char* key)
{
    const char* const* anchor = list;
    int32_t pass = 0;

    
    while (pass++ < 2) {
        while (*list) {
            if (uprv_strcmp(key, *list) == 0) {
                return (int16_t)(list - anchor);
            }
            list++;
        }
        ++list;     
    }
    return -1;
}


static inline int32_t
_copyCount(char *dest, int32_t destCapacity, const char *src) {
    const char *anchor;
    char c;

    anchor=src;
    for(;;) {
        if((c=*src)==0) {
            return (int32_t)(src-anchor);
        }
        if(destCapacity<=0) {
            return (int32_t)((src-anchor)+uprv_strlen(src));
        }
        ++src;
        *dest++=c;
        --destCapacity;
    }
}

U_CFUNC const char* 
uloc_getCurrentCountryID(const char* oldID){
    int32_t offset = _findIndex(DEPRECATED_COUNTRIES, oldID);
    if (offset >= 0) {
        return REPLACEMENT_COUNTRIES[offset];
    }
    return oldID;
}
U_CFUNC const char* 
uloc_getCurrentLanguageID(const char* oldID){
    int32_t offset = _findIndex(DEPRECATED_LANGUAGES, oldID);
    if (offset >= 0) {
        return REPLACEMENT_LANGUAGES[offset];
    }
    return oldID;        
}








U_CFUNC int32_t
ulocimp_getLanguage(const char *localeID,
                    char *language, int32_t languageCapacity,
                    const char **pEnd) {
    int32_t i=0;
    int32_t offset;
    char lang[4]={ 0, 0, 0, 0 }; 

    
    if(_isIDPrefix(localeID)) {
        if(i<languageCapacity) {
            language[i]=(char)uprv_tolower(*localeID);
        }
        if(i<languageCapacity) {
            language[i+1]='-';
        }
        i+=2;
        localeID+=2;
    }
    
    
    while(!_isTerminator(*localeID) && !_isIDSeparator(*localeID)) {
        if(i<languageCapacity) {
            language[i]=(char)uprv_tolower(*localeID);
        }
        if(i<3) {
            U_ASSERT(i>=0);
            lang[i]=(char)uprv_tolower(*localeID);
        }
        i++;
        localeID++;
    }

    if(i==3) {
        
        offset=_findIndex(LANGUAGES_3, lang);
        if(offset>=0) {
            i=_copyCount(language, languageCapacity, LANGUAGES[offset]);
        }
    }

    if(pEnd!=NULL) {
        *pEnd=localeID;
    }
    return i;
}

U_CFUNC int32_t
ulocimp_getScript(const char *localeID,
                  char *script, int32_t scriptCapacity,
                  const char **pEnd)
{
    int32_t idLen = 0;

    if (pEnd != NULL) {
        *pEnd = localeID;
    }

    
    while(!_isTerminator(localeID[idLen]) && !_isIDSeparator(localeID[idLen])
            && uprv_isASCIILetter(localeID[idLen])) {
        idLen++;
    }

    
    if (idLen == 4) {
        int32_t i;
        if (pEnd != NULL) {
            *pEnd = localeID+idLen;
        }
        if(idLen > scriptCapacity) {
            idLen = scriptCapacity;
        }
        if (idLen >= 1) {
            script[0]=(char)uprv_toupper(*(localeID++));
        }
        for (i = 1; i < idLen; i++) {
            script[i]=(char)uprv_tolower(*(localeID++));
        }
    }
    else {
        idLen = 0;
    }
    return idLen;
}

U_CFUNC int32_t
ulocimp_getCountry(const char *localeID,
                   char *country, int32_t countryCapacity,
                   const char **pEnd)
{
    int32_t idLen=0;
    char cnty[ULOC_COUNTRY_CAPACITY]={ 0, 0, 0, 0 };
    int32_t offset;

    
    while(!_isTerminator(localeID[idLen]) && !_isIDSeparator(localeID[idLen])) {
        if(idLen<(ULOC_COUNTRY_CAPACITY-1)) {   
            cnty[idLen]=(char)uprv_toupper(localeID[idLen]);
        }
        idLen++;
    }

    
    if (idLen == 2 || idLen == 3) {
        UBool gotCountry = FALSE;
        
        if(idLen==3) {
            offset=_findIndex(COUNTRIES_3, cnty);
            if(offset>=0) {
                idLen=_copyCount(country, countryCapacity, COUNTRIES[offset]);
                gotCountry = TRUE;
            }
        }
        if (!gotCountry) {
            int32_t i = 0;
            for (i = 0; i < idLen; i++) {
                if (i < countryCapacity) {
                    country[i]=(char)uprv_toupper(localeID[i]);
                }
            }
        }
        localeID+=idLen;
    } else {
        idLen = 0;
    }

    if(pEnd!=NULL) {
        *pEnd=localeID;
    }

    return idLen;
}





static int32_t
_getVariantEx(const char *localeID,
              char prev,
              char *variant, int32_t variantCapacity,
              UBool needSeparator) {
    int32_t i=0;

    
    if(_isIDSeparator(prev)) {
        
        while(!_isTerminator(*localeID)) {
            if (needSeparator) {
                if (i<variantCapacity) {
                    variant[i] = '_';
                }
                ++i;
                needSeparator = FALSE;
            }
            if(i<variantCapacity) {
                variant[i]=(char)uprv_toupper(*localeID);
                if(variant[i]=='-') {
                    variant[i]='_';
                }
            }
            i++;
            localeID++;
        }
    }

    
    if(i==0) {
        if(prev=='@') {
            
        } else if((localeID=locale_getKeywordsStart(localeID))!=NULL) {
            ++localeID; 
        } else {
            return 0;
        }
        while(!_isTerminator(*localeID)) {
            if (needSeparator) {
                if (i<variantCapacity) {
                    variant[i] = '_';
                }
                ++i;
                needSeparator = FALSE;
            }
            if(i<variantCapacity) {
                variant[i]=(char)uprv_toupper(*localeID);
                if(variant[i]=='-' || variant[i]==',') {
                    variant[i]='_';
                }
            }
            i++;
            localeID++;
        }
    }
    
    return i;
}

static int32_t
_getVariant(const char *localeID,
            char prev,
            char *variant, int32_t variantCapacity) {
    return _getVariantEx(localeID, prev, variant, variantCapacity, FALSE);
}













static int32_t
_deleteVariant(char* variants, int32_t variantsLen,
               const char* toDelete, int32_t toDeleteLen)
{
    int32_t delta = 0; 
    for (;;) {
        UBool flag = FALSE;
        if (variantsLen < toDeleteLen) {
            return delta;
        }
        if (uprv_strncmp(variants, toDelete, toDeleteLen) == 0 &&
            (variantsLen == toDeleteLen ||
             (flag=(variants[toDeleteLen] == '_'))))
        {
            int32_t d = toDeleteLen + (flag?1:0);
            variantsLen -= d;
            delta += d;
            if (variantsLen > 0) {
                uprv_memmove(variants, variants+d, variantsLen);
            }
        } else {
            char* p = _strnchr(variants, variantsLen, '_');
            if (p == NULL) {
                return delta;
            }
            ++p;
            variantsLen -= (int32_t)(p - variants);
            variants = p;
        }
    }
}



typedef struct UKeywordsContext {
    char* keywords;
    char* current;
} UKeywordsContext;

static void U_CALLCONV
uloc_kw_closeKeywords(UEnumeration *enumerator) {
    uprv_free(((UKeywordsContext *)enumerator->context)->keywords);
    uprv_free(enumerator->context);
    uprv_free(enumerator);
}

static int32_t U_CALLCONV
uloc_kw_countKeywords(UEnumeration *en, UErrorCode * ) {
    char *kw = ((UKeywordsContext *)en->context)->keywords;
    int32_t result = 0;
    while(*kw) {
        result++;
        kw += uprv_strlen(kw)+1;
    }
    return result;
}

static const char* U_CALLCONV 
uloc_kw_nextKeyword(UEnumeration* en,
                    int32_t* resultLength,
                    UErrorCode* ) {
    const char* result = ((UKeywordsContext *)en->context)->current;
    int32_t len = 0;
    if(*result) {
        len = (int32_t)uprv_strlen(((UKeywordsContext *)en->context)->current);
        ((UKeywordsContext *)en->context)->current += len+1;
    } else {
        result = NULL;
    }
    if (resultLength) {
        *resultLength = len;
    }
    return result;
}

static void U_CALLCONV 
uloc_kw_resetKeywords(UEnumeration* en, 
                      UErrorCode* ) {
    ((UKeywordsContext *)en->context)->current = ((UKeywordsContext *)en->context)->keywords;
}

static const UEnumeration gKeywordsEnum = {
    NULL,
    NULL,
    uloc_kw_closeKeywords,
    uloc_kw_countKeywords,
    uenum_unextDefault,
    uloc_kw_nextKeyword,
    uloc_kw_resetKeywords
};

U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywordList(const char *keywordList, int32_t keywordListSize, UErrorCode* status)
{
    UKeywordsContext *myContext = NULL;
    UEnumeration *result = NULL;

    if(U_FAILURE(*status)) {
        return NULL;
    }
    result = (UEnumeration *)uprv_malloc(sizeof(UEnumeration));
    
    if (result == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(result, &gKeywordsEnum, sizeof(UEnumeration));
    myContext = static_cast<UKeywordsContext *>(uprv_malloc(sizeof(UKeywordsContext)));
    if (myContext == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(result);
        return NULL;
    }
    myContext->keywords = (char *)uprv_malloc(keywordListSize+1);
    uprv_memcpy(myContext->keywords, keywordList, keywordListSize);
    myContext->keywords[keywordListSize] = 0;
    myContext->current = myContext->keywords;
    result->context = myContext;
    return result;
}

U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywords(const char* localeID,
                        UErrorCode* status) 
{
    int32_t i=0;
    char keywords[256];
    int32_t keywordsCapacity = 256;
    char tempBuffer[ULOC_FULLNAME_CAPACITY];
    const char* tmpLocaleID;

    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }
    
    if (_hasBCP47Extension(localeID)) {
        _ConvertBCP47(tmpLocaleID, localeID, tempBuffer, sizeof(tempBuffer), status);
    } else {
        if (localeID==NULL) {
           localeID=uloc_getDefault();
        }
        tmpLocaleID=localeID;
    }

    
    ulocimp_getLanguage(tmpLocaleID, NULL, 0, &tmpLocaleID);
    if(_isIDSeparator(*tmpLocaleID)) {
        const char *scriptID;
        
        ulocimp_getScript(tmpLocaleID+1, NULL, 0, &scriptID);
        if(scriptID != tmpLocaleID+1) {
            
            tmpLocaleID = scriptID;
        }
        
        if (_isIDSeparator(*tmpLocaleID)) {
            ulocimp_getCountry(tmpLocaleID+1, NULL, 0, &tmpLocaleID);
            if(_isIDSeparator(*tmpLocaleID)) {
                _getVariant(tmpLocaleID+1, *tmpLocaleID, NULL, 0);
            }
        }
    }

    
    if((tmpLocaleID = locale_getKeywordsStart(tmpLocaleID)) != NULL) {
        i=locale_getKeywords(tmpLocaleID+1, '@', keywords, keywordsCapacity, NULL, 0, NULL, FALSE, status);
    }

    if(i) {
        return uloc_openKeywordList(keywords, i, status);
    } else {
        return NULL;
    }
}



#define _ULOC_STRIP_KEYWORDS 0x2
#define _ULOC_CANONICALIZE   0x1

#define OPTION_SET(options, mask) ((options & mask) != 0)

static const char i_default[] = {'i', '-', 'd', 'e', 'f', 'a', 'u', 'l', 't'};
#define I_DEFAULT_LENGTH (sizeof i_default / sizeof i_default[0])








static int32_t
_canonicalize(const char* localeID,
              char* result,
              int32_t resultCapacity,
              uint32_t options,
              UErrorCode* err) {
    int32_t j, len, fieldCount=0, scriptSize=0, variantSize=0, nameCapacity;
    char localeBuffer[ULOC_FULLNAME_CAPACITY];
    char tempBuffer[ULOC_FULLNAME_CAPACITY];
    const char* origLocaleID;
    const char* tmpLocaleID;
    const char* keywordAssign = NULL;
    const char* separatorIndicator = NULL;
    const char* addKeyword = NULL;
    const char* addValue = NULL;
    char* name;
    char* variant = NULL; 

    if (U_FAILURE(*err)) {
        return 0;
    }
    
    if (_hasBCP47Extension(localeID)) {
        _ConvertBCP47(tmpLocaleID, localeID, tempBuffer, sizeof(tempBuffer), err);
    } else {
        if (localeID==NULL) {
           localeID=uloc_getDefault();
        }
        tmpLocaleID=localeID;
    }

    origLocaleID=tmpLocaleID;

    

    if (
        (result == NULL || resultCapacity < (int32_t)sizeof(localeBuffer))) {
        name = localeBuffer;
        nameCapacity = (int32_t)sizeof(localeBuffer);
    } else {
        name = result;
        nameCapacity = resultCapacity;
    }

    
    len=ulocimp_getLanguage(tmpLocaleID, name, nameCapacity, &tmpLocaleID);

    if(len == I_DEFAULT_LENGTH && uprv_strncmp(origLocaleID, i_default, len) == 0) {
        const char *d = uloc_getDefault();
        
        len = (int32_t)uprv_strlen(d);

        if (name != NULL) {
            uprv_strncpy(name, d, len);
        }
    } else if(_isIDSeparator(*tmpLocaleID)) {
        const char *scriptID;

        ++fieldCount;
        if(len<nameCapacity) {
            name[len]='_';
        }
        ++len;

        scriptSize=ulocimp_getScript(tmpLocaleID+1,
            (len<nameCapacity ? name+len : NULL), nameCapacity-len, &scriptID);
        if(scriptSize > 0) {
            
            tmpLocaleID = scriptID;
            ++fieldCount;
            len+=scriptSize;
            if (_isIDSeparator(*tmpLocaleID)) {
                
                if(len<nameCapacity) {
                    name[len]='_';
                }
                ++len;
            }
        }

        if (_isIDSeparator(*tmpLocaleID)) {
            const char *cntryID;
            int32_t cntrySize = ulocimp_getCountry(tmpLocaleID+1,
                (len<nameCapacity ? name+len : NULL), nameCapacity-len, &cntryID);
            if (cntrySize > 0) {
                
                tmpLocaleID = cntryID;
                len+=cntrySize;
            }
            if(_isIDSeparator(*tmpLocaleID)) {
                
                if (cntrySize >= 0 && ! _isIDSeparator(*(tmpLocaleID+1)) ) {
                    ++fieldCount;
                    if(len<nameCapacity) {
                        name[len]='_';
                    }
                    ++len;
                }

                variantSize = _getVariant(tmpLocaleID+1, *tmpLocaleID,
                    (len<nameCapacity ? name+len : NULL), nameCapacity-len);
                if (variantSize > 0) {
                    variant = len<nameCapacity ? name+len : NULL;
                    len += variantSize;
                    tmpLocaleID += variantSize + 1; 
                }
            }
        }
    }

    
    if (!OPTION_SET(options, _ULOC_CANONICALIZE) && *tmpLocaleID == '.') {
        UBool done = FALSE;
        do {
            char c = *tmpLocaleID;
            switch (c) {
            case 0:
            case '@':
                done = TRUE;
                break;
            default:
                if (len<nameCapacity) {
                    name[len] = c;
                }
                ++len;
                ++tmpLocaleID;
                break;
            }
        } while (!done);
    }

    

    if ((tmpLocaleID=locale_getKeywordsStart(tmpLocaleID))!=NULL) {
        keywordAssign = uprv_strchr(tmpLocaleID, '=');
        separatorIndicator = uprv_strchr(tmpLocaleID, ';');
    }

    
    if (!OPTION_SET(options, _ULOC_CANONICALIZE) &&
        tmpLocaleID != NULL && keywordAssign == NULL) {
        for (;;) {
            char c = *tmpLocaleID;
            if (c == 0) {
                break;
            }
            if (len<nameCapacity) {
                name[len] = c;
            }
            ++len;
            ++tmpLocaleID;
        }
    }

    if (OPTION_SET(options, _ULOC_CANONICALIZE)) {
        
        if (tmpLocaleID!=NULL && keywordAssign==NULL) {
            int32_t posixVariantSize;
            
            if (fieldCount < 2 || (fieldCount < 3 && scriptSize > 0)) {
                do {
                    if(len<nameCapacity) {
                        name[len]='_';
                    }
                    ++len;
                    ++fieldCount;
                } while(fieldCount<2);
            }
            posixVariantSize = _getVariantEx(tmpLocaleID+1, '@', name+len, nameCapacity-len,
                                             (UBool)(variantSize > 0));
            if (posixVariantSize > 0) {
                if (variant == NULL) {
                    variant = name+len;
                }
                len += posixVariantSize;
                variantSize += posixVariantSize;
            }
        }

        
        if (variant) {
            for (j=0; j<(int32_t)(sizeof(VARIANT_MAP)/sizeof(VARIANT_MAP[0])); j++) {
                const char* variantToCompare = VARIANT_MAP[j].variant;
                int32_t n = (int32_t)uprv_strlen(variantToCompare);
                int32_t variantLen = _deleteVariant(variant, uprv_min(variantSize, (nameCapacity-len)), variantToCompare, n);
                len -= variantLen;
                if (variantLen > 0) {
                    if (len > 0 && name[len-1] == '_') { 
                        --len;
                    }
                    addKeyword = VARIANT_MAP[j].keyword;
                    addValue = VARIANT_MAP[j].value;
                    break;
                }
            }
            if (len > 0 && len <= nameCapacity && name[len-1] == '_') { 
                --len;
            }
        }

        
        for (j=0; j<(int32_t)(sizeof(CANONICALIZE_MAP)/sizeof(CANONICALIZE_MAP[0])); j++) {
            const char* id = CANONICALIZE_MAP[j].id;
            int32_t n = (int32_t)uprv_strlen(id);
            if (len == n && uprv_strncmp(name, id, n) == 0) {
                if (n == 0 && tmpLocaleID != NULL) {
                    break; 
                }
                len = _copyCount(name, nameCapacity, CANONICALIZE_MAP[j].canonicalID);
                if (CANONICALIZE_MAP[j].keyword) {
                    addKeyword = CANONICALIZE_MAP[j].keyword;
                    addValue = CANONICALIZE_MAP[j].value;
                }
                break;
            }
        }
    }

    if (!OPTION_SET(options, _ULOC_STRIP_KEYWORDS)) {
        if (tmpLocaleID!=NULL && keywordAssign!=NULL &&
            (!separatorIndicator || separatorIndicator > keywordAssign)) {
            if(len<nameCapacity) {
                name[len]='@';
            }
            ++len;
            ++fieldCount;
            len += _getKeywords(tmpLocaleID+1, '@', (len<nameCapacity ? name+len : NULL), nameCapacity-len,
                                NULL, 0, NULL, TRUE, addKeyword, addValue, err);
        } else if (addKeyword != NULL) {
            U_ASSERT(addValue != NULL && len < nameCapacity);
            
            len += _copyCount(name+len, nameCapacity-len, "@");
            len += _copyCount(name+len, nameCapacity-len, addKeyword);
            len += _copyCount(name+len, nameCapacity-len, "=");
            len += _copyCount(name+len, nameCapacity-len, addValue);
        }
    }

    if (U_SUCCESS(*err) && result != NULL && name == localeBuffer) {
        uprv_strncpy(result, localeBuffer, (len > resultCapacity) ? resultCapacity : len);
    }

    return u_terminateChars(result, resultCapacity, len, err);
}



U_CAPI int32_t  U_EXPORT2
uloc_getParent(const char*    localeID,
               char* parent,
               int32_t parentCapacity,
               UErrorCode* err)
{
    const char *lastUnderscore;
    int32_t i;
    
    if (U_FAILURE(*err))
        return 0;
    
    if (localeID == NULL)
        localeID = uloc_getDefault();

    lastUnderscore=uprv_strrchr(localeID, '_');
    if(lastUnderscore!=NULL) {
        i=(int32_t)(lastUnderscore-localeID);
    } else {
        i=0;
    }

    if(i>0 && parent != localeID) {
        uprv_memcpy(parent, localeID, uprv_min(i, parentCapacity));
    }
    return u_terminateChars(parent, parentCapacity, i, err);
}

U_CAPI int32_t U_EXPORT2
uloc_getLanguage(const char*    localeID,
         char* language,
         int32_t languageCapacity,
         UErrorCode* err)
{
    
    int32_t i=0;

    if (err==NULL || U_FAILURE(*err)) {
        return 0;
    }
    
    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    i=ulocimp_getLanguage(localeID, language, languageCapacity, NULL);
    return u_terminateChars(language, languageCapacity, i, err);
}

U_CAPI int32_t U_EXPORT2
uloc_getScript(const char*    localeID,
         char* script,
         int32_t scriptCapacity,
         UErrorCode* err)
{
    int32_t i=0;

    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }

    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    
    ulocimp_getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        i=ulocimp_getScript(localeID+1, script, scriptCapacity, NULL);
    }
    return u_terminateChars(script, scriptCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getCountry(const char* localeID,
            char* country,
            int32_t countryCapacity,
            UErrorCode* err) 
{
    int32_t i=0;

    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }

    if(localeID==NULL) {
        localeID=uloc_getDefault();
    }

    
    ulocimp_getLanguage(localeID, NULL, 0, &localeID);
    if(_isIDSeparator(*localeID)) {
        const char *scriptID;
        
        ulocimp_getScript(localeID+1, NULL, 0, &scriptID);
        if(scriptID != localeID+1) {
            
            localeID = scriptID;
        }
        if(_isIDSeparator(*localeID)) {
            i=ulocimp_getCountry(localeID+1, country, countryCapacity, NULL);
        }
    }
    return u_terminateChars(country, countryCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getVariant(const char* localeID,
                char* variant,
                int32_t variantCapacity,
                UErrorCode* err) 
{
    char tempBuffer[ULOC_FULLNAME_CAPACITY];
    const char* tmpLocaleID;
    int32_t i=0;
    
    if(err==NULL || U_FAILURE(*err)) {
        return 0;
    }
    
    if (_hasBCP47Extension(localeID)) {
        _ConvertBCP47(tmpLocaleID, localeID, tempBuffer, sizeof(tempBuffer), err);
    } else {
        if (localeID==NULL) {
           localeID=uloc_getDefault();
        }
        tmpLocaleID=localeID;
    }
    
    
    ulocimp_getLanguage(tmpLocaleID, NULL, 0, &tmpLocaleID);
    if(_isIDSeparator(*tmpLocaleID)) {
        const char *scriptID;
        
        ulocimp_getScript(tmpLocaleID+1, NULL, 0, &scriptID);
        if(scriptID != tmpLocaleID+1) {
            
            tmpLocaleID = scriptID;
        }
        
        if (_isIDSeparator(*tmpLocaleID)) {
            const char *cntryID;
            ulocimp_getCountry(tmpLocaleID+1, NULL, 0, &cntryID);
            if (cntryID != tmpLocaleID+1) {
                
                tmpLocaleID = cntryID;
            }
            if(_isIDSeparator(*tmpLocaleID)) {
                
                if (tmpLocaleID != cntryID && _isIDSeparator(tmpLocaleID[1])) {
                    tmpLocaleID++;
                }
                i=_getVariant(tmpLocaleID+1, *tmpLocaleID, variant, variantCapacity);
            }
        }
    }
    
    
    





    return u_terminateChars(variant, variantCapacity, i, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getName(const char* localeID,
             char* name,
             int32_t nameCapacity,
             UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, 0, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_getBaseName(const char* localeID,
                 char* name,
                 int32_t nameCapacity,
                 UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, _ULOC_STRIP_KEYWORDS, err);
}

U_CAPI int32_t  U_EXPORT2
uloc_canonicalize(const char* localeID,
                  char* name,
                  int32_t nameCapacity,
                  UErrorCode* err)  
{
    return _canonicalize(localeID, name, nameCapacity, _ULOC_CANONICALIZE, err);
}
  
U_CAPI const char*  U_EXPORT2
uloc_getISO3Language(const char* localeID) 
{
    int16_t offset;
    char lang[ULOC_LANG_CAPACITY];
    UErrorCode err = U_ZERO_ERROR;
    
    if (localeID == NULL)
    {
        localeID = uloc_getDefault();
    }
    uloc_getLanguage(localeID, lang, ULOC_LANG_CAPACITY, &err);
    if (U_FAILURE(err))
        return "";
    offset = _findIndex(LANGUAGES, lang);
    if (offset < 0)
        return "";
    return LANGUAGES_3[offset];
}

U_CAPI const char*  U_EXPORT2
uloc_getISO3Country(const char* localeID) 
{
    int16_t offset;
    char cntry[ULOC_LANG_CAPACITY];
    UErrorCode err = U_ZERO_ERROR;
    
    if (localeID == NULL)
    {
        localeID = uloc_getDefault();
    }
    uloc_getCountry(localeID, cntry, ULOC_LANG_CAPACITY, &err);
    if (U_FAILURE(err))
        return "";
    offset = _findIndex(COUNTRIES, cntry);
    if (offset < 0)
        return "";
    
    return COUNTRIES_3[offset];
}

U_CAPI uint32_t  U_EXPORT2
uloc_getLCID(const char* localeID) 
{
    UErrorCode status = U_ZERO_ERROR;
    char       langID[ULOC_FULLNAME_CAPACITY];

    uloc_getLanguage(localeID, langID, sizeof(langID), &status);
    if (U_FAILURE(status)) {
        return 0;
    }

    return uprv_convertToLCID(langID, localeID, &status);
}

U_CAPI int32_t U_EXPORT2
uloc_getLocaleForLCID(uint32_t hostid, char *locale, int32_t localeCapacity,
                UErrorCode *status)
{
    int32_t length;
    const char *posix = uprv_convertToPosix(hostid, status);
    if (U_FAILURE(*status) || posix == NULL) {
        return 0;
    }
    length = (int32_t)uprv_strlen(posix);
    if (length+1 > localeCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    else {
        uprv_strcpy(locale, posix);
    }
    return length;
}



U_CAPI const char*  U_EXPORT2
uloc_getDefault()
{
    return locale_get_default();
}

U_CAPI void  U_EXPORT2
uloc_setDefault(const char*   newDefaultLocale,
             UErrorCode* err) 
{
    if (U_FAILURE(*err))
        return;
    
    
    
    locale_set_default(newDefaultLocale);
}







U_CAPI const char* const*  U_EXPORT2
uloc_getISOLanguages() 
{
    return LANGUAGES;
}







U_CAPI const char* const*  U_EXPORT2
uloc_getISOCountries() 
{
    return COUNTRIES;
}



static char gDecimal = 0;

static 
double

_uloc_strtod(const char *start, char **end) {
    char *decimal;
    char *myEnd;
    char buf[30];
    double rv;
    if (!gDecimal) {
        char rep[5];
        


        sprintf(rep, "%+1.1f", 1.0);
        gDecimal = rep[2];
    }

    if(gDecimal == '.') {
        return uprv_strtod(start, end); 
    } else {
        uprv_strncpy(buf, start, 29);
        buf[29]=0;
        decimal = uprv_strchr(buf, '.');
        if(decimal) {
            *decimal = gDecimal;
        } else {
            return uprv_strtod(start, end); 
        }
        rv = uprv_strtod(buf, &myEnd);
        if(end) {
            *end = (char*)(start+(myEnd-buf)); 
        }
        return rv;
    }
}

typedef struct { 
    float q;
    int32_t dummy;  
    char *locale;
} _acceptLangItem;

static int32_t U_CALLCONV
uloc_acceptLanguageCompare(const void * , const void *a, const void *b)
{
    const _acceptLangItem *aa = (const _acceptLangItem*)a;
    const _acceptLangItem *bb = (const _acceptLangItem*)b;

    int32_t rc = 0;
    if(bb->q < aa->q) {
        rc = -1;  
    } else if(bb->q > aa->q) {
        rc = 1;   
    } else {
        rc = 0;   
    }

    if(rc==0) {
        rc = uprv_stricmp(aa->locale, bb->locale);
    }

#if defined(ULOC_DEBUG)
    



#endif

    return rc;
}





U_CAPI int32_t U_EXPORT2
uloc_acceptLanguageFromHTTP(char *result, int32_t resultAvailable, UAcceptResult *outResult,
                            const char *httpAcceptLanguage,
                            UEnumeration* availableLocales,
                            UErrorCode *status)
{
    _acceptLangItem *j;
    _acceptLangItem smallBuffer[30];
    char **strs;
    char tmp[ULOC_FULLNAME_CAPACITY +1];
    int32_t n = 0;
    const char *itemEnd;
    const char *paramEnd;
    const char *s;
    const char *t;
    int32_t res;
    int32_t i;
    int32_t l = (int32_t)uprv_strlen(httpAcceptLanguage);
    int32_t jSize;
    char *tempstr; 

    j = smallBuffer;
    jSize = sizeof(smallBuffer)/sizeof(smallBuffer[0]);
    if(U_FAILURE(*status)) {
        return -1;
    }

    for(s=httpAcceptLanguage;s&&*s;) {
        while(isspace(*s)) 
            s++;
        itemEnd=uprv_strchr(s,',');
        paramEnd=uprv_strchr(s,';');
        if(!itemEnd) {
            itemEnd = httpAcceptLanguage+l; 
        }
        if(paramEnd && paramEnd<itemEnd) { 
            
            t = paramEnd+1;
            if(*t=='q') {
                t++;
            }
            while(isspace(*t)) {
                t++;
            }
            if(*t=='=') {
                t++;
            }
            while(isspace(*t)) {
                t++;
            }
            j[n].q = (float)_uloc_strtod(t,NULL);
        } else {
            
            j[n].q = 1.0f;
            paramEnd = itemEnd;
        }
        j[n].dummy=0;
        
        for(t=(paramEnd-1);(paramEnd>s)&&isspace(*t);t--)
            ;
        
        tempstr = uprv_strndup(s,(int32_t)((t+1)-s));
        if (tempstr == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
        j[n].locale = tempstr;
        uloc_canonicalize(j[n].locale,tmp,sizeof(tmp)/sizeof(tmp[0]),status);
        if(strcmp(j[n].locale,tmp)) {
            uprv_free(j[n].locale);
            j[n].locale=uprv_strdup(tmp);
        }
#if defined(ULOC_DEBUG)
        
#endif
        n++;
        s = itemEnd;
        while(*s==',') { 
            s++;
        }
        if(n>=jSize) {
            if(j==smallBuffer) {  
                j = static_cast<_acceptLangItem *>(uprv_malloc(sizeof(j[0])*(jSize*2)));
                if(j!=NULL) {
                    uprv_memcpy(j,smallBuffer,sizeof(j[0])*jSize);
                }
#if defined(ULOC_DEBUG)
                fprintf(stderr,"malloced at size %d\n", jSize);
#endif
            } else {
                j = static_cast<_acceptLangItem *>(uprv_realloc(j, sizeof(j[0])*jSize*2));
#if defined(ULOC_DEBUG)
                fprintf(stderr,"re-alloced at size %d\n", jSize);
#endif
            }
            jSize *= 2;
            if(j==NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return -1;
            }
        }
    }
    uprv_sortArray(j, n, sizeof(j[0]), uloc_acceptLanguageCompare, NULL, TRUE, status);
    if(U_FAILURE(*status)) {
        if(j != smallBuffer) {
#if defined(ULOC_DEBUG)
            fprintf(stderr,"freeing j %p\n", j);
#endif
            uprv_free(j);
        }
        return -1;
    }
    strs = static_cast<char **>(uprv_malloc((size_t)(sizeof(strs[0])*n)));
    
    if (strs == NULL) {
        uprv_free(j); 
        *status = U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }
    for(i=0;i<n;i++) {
#if defined(ULOC_DEBUG)
        
#endif
        strs[i]=j[i].locale;
    }
    res =  uloc_acceptLanguage(result, resultAvailable, outResult, 
        (const char**)strs, n, availableLocales, status);
    for(i=0;i<n;i++) {
        uprv_free(strs[i]);
    }
    uprv_free(strs);
    if(j != smallBuffer) {
#if defined(ULOC_DEBUG)
        fprintf(stderr,"freeing j %p\n", j);
#endif
        uprv_free(j);
    }
    return res;
}


U_CAPI int32_t U_EXPORT2
uloc_acceptLanguage(char *result, int32_t resultAvailable, 
                    UAcceptResult *outResult, const char **acceptList,
                    int32_t acceptListCount,
                    UEnumeration* availableLocales,
                    UErrorCode *status)
{
    int32_t i,j;
    int32_t len;
    int32_t maxLen=0;
    char tmp[ULOC_FULLNAME_CAPACITY+1];
    const char *l;
    char **fallbackList;
    if(U_FAILURE(*status)) {
        return -1;
    }
    fallbackList = static_cast<char **>(uprv_malloc((size_t)(sizeof(fallbackList[0])*acceptListCount)));
    if(fallbackList==NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return -1;
    }
    for(i=0;i<acceptListCount;i++) {
#if defined(ULOC_DEBUG)
        fprintf(stderr,"%02d: %s\n", i, acceptList[i]);
#endif
        while((l=uenum_next(availableLocales, NULL, status))) {
#if defined(ULOC_DEBUG)
            fprintf(stderr,"  %s\n", l);
#endif
            len = (int32_t)uprv_strlen(l);
            if(!uprv_strcmp(acceptList[i], l)) {
                if(outResult) { 
                    *outResult = ULOC_ACCEPT_VALID;
                }
#if defined(ULOC_DEBUG)
                fprintf(stderr, "MATCH! %s\n", l);
#endif
                if(len>0) {
                    uprv_strncpy(result, l, uprv_min(len, resultAvailable));
                }
                for(j=0;j<i;j++) {
                    uprv_free(fallbackList[j]);
                }
                uprv_free(fallbackList);
                return u_terminateChars(result, resultAvailable, len, status);   
            }
            if(len>maxLen) {
                maxLen = len;
            }
        }
        uenum_reset(availableLocales, status);    
        
        if(uloc_getParent(acceptList[i], tmp, sizeof(tmp)/sizeof(tmp[0]), status)!=0) {
            fallbackList[i] = uprv_strdup(tmp);
        } else {
            fallbackList[i]=0;
        }
    }

    for(maxLen--;maxLen>0;maxLen--) {
        for(i=0;i<acceptListCount;i++) {
            if(fallbackList[i] && ((int32_t)uprv_strlen(fallbackList[i])==maxLen)) {
#if defined(ULOC_DEBUG)
                fprintf(stderr,"Try: [%s]", fallbackList[i]);
#endif
                while((l=uenum_next(availableLocales, NULL, status))) {
#if defined(ULOC_DEBUG)
                    fprintf(stderr,"  %s\n", l);
#endif
                    len = (int32_t)uprv_strlen(l);
                    if(!uprv_strcmp(fallbackList[i], l)) {
                        if(outResult) { 
                            *outResult = ULOC_ACCEPT_FALLBACK;
                        }
#if defined(ULOC_DEBUG)
                        fprintf(stderr, "fallback MATCH! %s\n", l);
#endif
                        if(len>0) {
                            uprv_strncpy(result, l, uprv_min(len, resultAvailable));
                        }
                        for(j=0;j<acceptListCount;j++) {
                            uprv_free(fallbackList[j]);
                        }
                        uprv_free(fallbackList);
                        return u_terminateChars(result, resultAvailable, len, status);
                    }
                }
                uenum_reset(availableLocales, status);    

                if(uloc_getParent(fallbackList[i], tmp, sizeof(tmp)/sizeof(tmp[0]), status)!=0) {
                    uprv_free(fallbackList[i]);
                    fallbackList[i] = uprv_strdup(tmp);
                } else {
                    uprv_free(fallbackList[i]);
                    fallbackList[i]=0;
                }
            }
        }
        if(outResult) { 
            *outResult = ULOC_ACCEPT_FAILED;
        }
    }
    for(i=0;i<acceptListCount;i++) {
        uprv_free(fallbackList[i]);
    }
    uprv_free(fallbackList);
    return -1;
}


