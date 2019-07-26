


























#include "locmap.h"
#include "unicode/uloc.h"
#include "cstring.h"
#include "cmemory.h"

#if U_PLATFORM == U_PF_WINDOWS && defined(_MSC_VER) && (_MSC_VER >= 1500)








#define USE_WINDOWS_LOCALE_API
#endif

#ifdef USE_WINDOWS_LOCALE_API
#include <windows.h>
#include <winnls.h>
#endif


















typedef struct ILcidPosixElement
{
    const uint32_t hostID;
    const char * const posixID;
} ILcidPosixElement;

typedef struct ILcidPosixMap
{
    const uint32_t numRegions;
    const struct ILcidPosixElement* const regionMaps;
} ILcidPosixMap;


















#define ILCID_POSIX_ELEMENT_ARRAY(hostID, languageID, posixID) \
static const ILcidPosixElement locmap_ ## languageID [] = { \
    {LANGUAGE_LCID(hostID), #languageID},     /* parent locale */ \
    {hostID, #posixID}, \
};





#define ILCID_POSIX_SUBTABLE(id) \
static const ILcidPosixElement locmap_ ## id [] =








#define ILCID_POSIX_MAP(_posixID) \
    {sizeof(locmap_ ## _posixID)/sizeof(ILcidPosixElement), locmap_ ## _posixID}

















ILCID_POSIX_ELEMENT_ARRAY(0x0436, af, af_ZA)

ILCID_POSIX_SUBTABLE(ar) {
    {0x01,   "ar"},
    {0x3801, "ar_AE"},
    {0x3c01, "ar_BH"},
    {0x1401, "ar_DZ"},
    {0x0c01, "ar_EG"},
    {0x0801, "ar_IQ"},
    {0x2c01, "ar_JO"},
    {0x3401, "ar_KW"},
    {0x3001, "ar_LB"},
    {0x1001, "ar_LY"},
    {0x1801, "ar_MA"},
    {0x1801, "ar_MO"},
    {0x2001, "ar_OM"},
    {0x4001, "ar_QA"},
    {0x0401, "ar_SA"},
    {0x2801, "ar_SY"},
    {0x1c01, "ar_TN"},
    {0x2401, "ar_YE"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x044d, as, as_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x045e, am, am_ET)
ILCID_POSIX_ELEMENT_ARRAY(0x047a, arn,arn_CL)

ILCID_POSIX_SUBTABLE(az) {
    {0x2c,   "az"},
    {0x082c, "az_Cyrl_AZ"},  
    {0x742c, "az_Cyrl"},  
    {0x042c, "az_Latn_AZ"}, 
    {0x782c, "az_Latn"}, 
    {0x042c, "az_AZ"} 
};

ILCID_POSIX_ELEMENT_ARRAY(0x046d, ba, ba_RU)
ILCID_POSIX_ELEMENT_ARRAY(0x0423, be, be_BY)









ILCID_POSIX_ELEMENT_ARRAY(0x0402, bg, bg_BG)

ILCID_POSIX_ELEMENT_ARRAY(0x0466, bin, bin_NG)

ILCID_POSIX_SUBTABLE(bn) {
    {0x45,   "bn"},
    {0x0845, "bn_BD"},
    {0x0445, "bn_IN"}
};

ILCID_POSIX_SUBTABLE(bo) {
    {0x51,   "bo"},
    {0x0851, "bo_BT"},
    {0x0451, "bo_CN"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x047e, br, br_FR)
ILCID_POSIX_ELEMENT_ARRAY(0x0403, ca, ca_ES)
ILCID_POSIX_ELEMENT_ARRAY(0x0483, co, co_FR)
ILCID_POSIX_ELEMENT_ARRAY(0x045c, chr,chr_US)


ILCID_POSIX_ELEMENT_ARRAY(0x0405, cs, cs_CZ)

ILCID_POSIX_ELEMENT_ARRAY(0x0452, cy, cy_GB)
ILCID_POSIX_ELEMENT_ARRAY(0x0406, da, da_DK)

ILCID_POSIX_SUBTABLE(de) {
    {0x07,   "de"},
    {0x0c07, "de_AT"},
    {0x0807, "de_CH"},
    {0x0407, "de_DE"},
    {0x1407, "de_LI"},
    {0x1007, "de_LU"},
    {0x10407,"de_DE@collation=phonebook"},  
    {0x10407,"de@collation=phonebook"}  
};

ILCID_POSIX_ELEMENT_ARRAY(0x0465, dv, dv_MV)
ILCID_POSIX_ELEMENT_ARRAY(0x0408, el, el_GR)

ILCID_POSIX_SUBTABLE(en) {
    {0x09,   "en"},
    {0x0c09, "en_AU"},
    {0x2809, "en_BZ"},
    {0x1009, "en_CA"},
    {0x0809, "en_GB"},
    {0x3c09, "en_HK"},
    {0x3809, "en_ID"},
    {0x1809, "en_IE"},
    {0x4009, "en_IN"},
    {0x2009, "en_JM"},
    {0x4409, "en_MY"},
    {0x1409, "en_NZ"},
    {0x3409, "en_PH"},
    {0x4809, "en_SG"},
    {0x2C09, "en_TT"},
    {0x0409, "en_US"},
    {0x007f, "en_US_POSIX"}, 
    {0x2409, "en_VI"},  
    {0x1c09, "en_ZA"},
    {0x3009, "en_ZW"},
    {0x2409, "en_029"},
    {0x0409, "en_AS"},  
    {0x0409, "en_GU"},  
    {0x0409, "en_MH"},  
    {0x0409, "en_MP"},  
    {0x0409, "en_UM"}   
};

ILCID_POSIX_SUBTABLE(en_US_POSIX) {
    {0x007f, "en_US_POSIX"} 
};

ILCID_POSIX_SUBTABLE(es) {
    {0x0a,   "es"},
    {0x2c0a, "es_AR"},
    {0x400a, "es_BO"},
    {0x340a, "es_CL"},
    {0x240a, "es_CO"},
    {0x140a, "es_CR"},
    {0x1c0a, "es_DO"},
    {0x300a, "es_EC"},
    {0x0c0a, "es_ES"},      
    {0x100a, "es_GT"},
    {0x480a, "es_HN"},
    {0x080a, "es_MX"},
    {0x4c0a, "es_NI"},
    {0x180a, "es_PA"},
    {0x280a, "es_PE"},
    {0x500a, "es_PR"},
    {0x3c0a, "es_PY"},
    {0x440a, "es_SV"},
    {0x540a, "es_US"},
    {0x380a, "es_UY"},
    {0x200a, "es_VE"},
    {0xe40a, "es_419"},
    {0x040a, "es_ES@collation=traditional"},
    {0x040a, "es@collation=traditional"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0425, et, et_EE)
ILCID_POSIX_ELEMENT_ARRAY(0x042d, eu, eu_ES)


ILCID_POSIX_SUBTABLE(fa) {
    {0x29,   "fa"},
    {0x0429, "fa_IR"},  
    {0x048c, "fa_AF"}   
};


ILCID_POSIX_SUBTABLE(fa_AF) {
    {0x8c,   "fa_AF"},  
    {0x048c, "fa_AF"}   
};

ILCID_POSIX_ELEMENT_ARRAY(0x040b, fi, fi_FI)
ILCID_POSIX_ELEMENT_ARRAY(0x0464, fil,fil_PH)
ILCID_POSIX_ELEMENT_ARRAY(0x0438, fo, fo_FO)

ILCID_POSIX_SUBTABLE(fr) {
    {0x0c,   "fr"},
    {0x080c, "fr_BE"},
    {0x0c0c, "fr_CA"},
    {0x240c, "fr_CD"},
    {0x240c, "fr_CG"},
    {0x100c, "fr_CH"},
    {0x300c, "fr_CI"},
    {0x2c0c, "fr_CM"},
    {0x040c, "fr_FR"},
    {0x3c0c, "fr_HT"},
    {0x140c, "fr_LU"},
    {0x380c, "fr_MA"},
    {0x180c, "fr_MC"},
    {0x340c, "fr_ML"},
    {0x200c, "fr_RE"},
    {0x280c, "fr_SN"},
    {0xe40c, "fr_015"},
    {0x1c0c, "fr_029"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0467, fuv, fuv_NG)

ILCID_POSIX_ELEMENT_ARRAY(0x0462, fy, fy_NL)

ILCID_POSIX_SUBTABLE(ga) { 
    {0x3c,   "ga"},
    {0x083c, "ga_IE"},
    {0x043c, "gd_GB"}
};

ILCID_POSIX_SUBTABLE(gd) { 
    {0x91,   "gd"},
    {0x0491, "gd_GB"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0456, gl, gl_ES)
ILCID_POSIX_ELEMENT_ARRAY(0x0447, gu, gu_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x0474, gn, gn_PY)
ILCID_POSIX_ELEMENT_ARRAY(0x0484, gsw,gsw_FR)

ILCID_POSIX_SUBTABLE(ha) {
    {0x68,   "ha"},
    {0x7c68, "ha_Latn"},
    {0x0468, "ha_Latn_NG"},
};

ILCID_POSIX_ELEMENT_ARRAY(0x0475, haw,haw_US)
ILCID_POSIX_ELEMENT_ARRAY(0x040d, he, he_IL)
ILCID_POSIX_ELEMENT_ARRAY(0x0439, hi, hi_IN)


ILCID_POSIX_SUBTABLE(hr) {
    {0x1a,   "hr"},
    {0x141a, "bs_Latn_BA"},  
    {0x681a, "bs_Latn"},  
    {0x141a, "bs_BA"},  
    {0x781a, "bs"},     
    {0x201a, "bs_Cyrl_BA"},  
    {0x641a, "bs_Cyrl"},  
    {0x101a, "hr_BA"},  
    {0x041a, "hr_HR"},  
    {0x2c1a, "sr_Latn_ME"},
    {0x241a, "sr_Latn_RS"},
    {0x181a, "sr_Latn_BA"}, 
    {0x081a, "sr_Latn_CS"}, 
    {0x701a, "sr_Latn"},    
    {0x1c1a, "sr_Cyrl_BA"}, 
    {0x0c1a, "sr_Cyrl_CS"}, 
    {0x301a, "sr_Cyrl_ME"},
    {0x281a, "sr_Cyrl_RS"},
    {0x6c1a, "sr_Cyrl"},    
    {0x7c1a, "sr"}          
};

ILCID_POSIX_ELEMENT_ARRAY(0x040e, hu, hu_HU)
ILCID_POSIX_ELEMENT_ARRAY(0x042b, hy, hy_AM)
ILCID_POSIX_ELEMENT_ARRAY(0x0469, ibb, ibb_NG)
ILCID_POSIX_ELEMENT_ARRAY(0x0421, id, id_ID)
ILCID_POSIX_ELEMENT_ARRAY(0x0470, ig, ig_NG)
ILCID_POSIX_ELEMENT_ARRAY(0x0478, ii, ii_CN)
ILCID_POSIX_ELEMENT_ARRAY(0x040f, is, is_IS)

ILCID_POSIX_SUBTABLE(it) {
    {0x10,   "it"},
    {0x0810, "it_CH"},
    {0x0410, "it_IT"}
};

ILCID_POSIX_SUBTABLE(iu) {
    {0x5d,   "iu"},
    {0x045d, "iu_Cans_CA"},
    {0x785d, "iu_Cans"},
    {0x085d, "iu_Latn_CA"},
    {0x7c5d, "iu_Latn"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x040d, iw, iw_IL)    
ILCID_POSIX_ELEMENT_ARRAY(0x0411, ja, ja_JP)
ILCID_POSIX_ELEMENT_ARRAY(0x0437, ka, ka_GE)
ILCID_POSIX_ELEMENT_ARRAY(0x043f, kk, kk_KZ)
ILCID_POSIX_ELEMENT_ARRAY(0x046f, kl, kl_GL)
ILCID_POSIX_ELEMENT_ARRAY(0x0453, km, km_KH)
ILCID_POSIX_ELEMENT_ARRAY(0x044b, kn, kn_IN)

ILCID_POSIX_SUBTABLE(ko) {
    {0x12,   "ko"},
    {0x0812, "ko_KP"},
    {0x0412, "ko_KR"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0457, kok, kok_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x0471, kr,  kr_NG)

ILCID_POSIX_SUBTABLE(ks) {         
    {0x60,   "ks"},
    {0x0860, "ks_IN"},              
    {0x0460, "ks_Arab_IN"},
    {0x0860, "ks_Deva_IN"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0440, ky, ky_KG)   
ILCID_POSIX_ELEMENT_ARRAY(0x0476, la, la_IT)   
ILCID_POSIX_ELEMENT_ARRAY(0x046e, lb, lb_LU)
ILCID_POSIX_ELEMENT_ARRAY(0x0454, lo, lo_LA)
ILCID_POSIX_ELEMENT_ARRAY(0x0427, lt, lt_LT)
ILCID_POSIX_ELEMENT_ARRAY(0x0426, lv, lv_LV)
ILCID_POSIX_ELEMENT_ARRAY(0x0481, mi, mi_NZ)
ILCID_POSIX_ELEMENT_ARRAY(0x042f, mk, mk_MK)
ILCID_POSIX_ELEMENT_ARRAY(0x044c, ml, ml_IN)

ILCID_POSIX_SUBTABLE(mn) {
    {0x50,   "mn"},
    {0x0450, "mn_MN"},
    {0x7c50, "mn_Mong"},
    {0x0850, "mn_Mong_CN"},
    {0x0850, "mn_CN"},
    {0x7850, "mn_Cyrl"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0458, mni,mni_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x047c, moh,moh_CA)
ILCID_POSIX_ELEMENT_ARRAY(0x044e, mr, mr_IN)

ILCID_POSIX_SUBTABLE(ms) {
    {0x3e,   "ms"},
    {0x083e, "ms_BN"},   
    {0x043e, "ms_MY"}    
};

ILCID_POSIX_ELEMENT_ARRAY(0x043a, mt, mt_MT)
ILCID_POSIX_ELEMENT_ARRAY(0x0455, my, my_MM)

ILCID_POSIX_SUBTABLE(ne) {
    {0x61,   "ne"},
    {0x0861, "ne_IN"},   
    {0x0461, "ne_NP"}    
};

ILCID_POSIX_SUBTABLE(nl) {
    {0x13,   "nl"},
    {0x0813, "nl_BE"},
    {0x0413, "nl_NL"}
};


ILCID_POSIX_SUBTABLE(no) {
    {0x14,   "no"},     
    {0x7c14, "nb"},     
    {0x0414, "nb_NO"},  
    {0x0414, "no_NO"},  
    {0x0814, "nn_NO"},  
    {0x7814, "nn"},     
    {0x0814, "no_NO_NY"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x046c, nso,nso_ZA)   
ILCID_POSIX_ELEMENT_ARRAY(0x0482, oc, oc_FR)

ILCID_POSIX_SUBTABLE(om) { 
    {0x72,   "om"},
    {0x0472, "om_ET"},
    {0x0472, "gaz_ET"}
};


ILCID_POSIX_SUBTABLE(or_IN) {
    {0x48,   "or"},
    {0x0448, "or_IN"},
};


ILCID_POSIX_SUBTABLE(pa) {
    {0x46,   "pa"},
    {0x0446, "pa_IN"},
    {0x0846, "pa_PK"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0479, pap, pap_AN)
ILCID_POSIX_ELEMENT_ARRAY(0x0415, pl, pl_PL)
ILCID_POSIX_ELEMENT_ARRAY(0x0463, ps, ps_AF)

ILCID_POSIX_SUBTABLE(pt) {
    {0x16,   "pt"},
    {0x0416, "pt_BR"},
    {0x0816, "pt_PT"}
};

ILCID_POSIX_SUBTABLE(qu) {
    {0x6b,   "qu"},
    {0x046b, "qu_BO"},
    {0x086b, "qu_EC"},
    {0x0C6b, "qu_PE"},
    {0x046b, "quz_BO"},
    {0x086b, "quz_EC"},
    {0x0C6b, "quz_PE"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0486, qut, qut_GT) 
ILCID_POSIX_ELEMENT_ARRAY(0x0417, rm, rm_CH)

ILCID_POSIX_SUBTABLE(ro) {
    {0x18,   "ro"},
    {0x0418, "ro_RO"},
    {0x0818, "ro_MD"}
};

ILCID_POSIX_SUBTABLE(root) {
    {0x00,   "root"}
};

ILCID_POSIX_SUBTABLE(ru) {
    {0x19,   "ru"},
    {0x0419, "ru_RU"},
    {0x0819, "ru_MD"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0487, rw, rw_RW)
ILCID_POSIX_ELEMENT_ARRAY(0x044f, sa, sa_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x0485, sah,sah_RU)

ILCID_POSIX_SUBTABLE(sd) {
    {0x59,   "sd"},
    {0x0459, "sd_IN"},
    {0x0859, "sd_PK"}
};

ILCID_POSIX_SUBTABLE(se) {
    {0x3b,   "se"},
    {0x0c3b, "se_FI"},
    {0x043b, "se_NO"},
    {0x083b, "se_SE"},
    {0x783b, "sma"},
    {0x183b, "sma_NO"},
    {0x1c3b, "sma_SE"},
    {0x7c3b, "smj"},
    {0x703b, "smn"},
    {0x743b, "sms"},
    {0x103b, "smj_NO"},
    {0x143b, "smj_SE"},
    {0x243b, "smn_FI"},
    {0x203b, "sms_FI"},
};

ILCID_POSIX_ELEMENT_ARRAY(0x045b, si, si_LK)
ILCID_POSIX_ELEMENT_ARRAY(0x041b, sk, sk_SK)
ILCID_POSIX_ELEMENT_ARRAY(0x0424, sl, sl_SI)

ILCID_POSIX_SUBTABLE(so) { 
    {0x77,   "so"},
    {0x0477, "so_ET"},
    {0x0477, "so_SO"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x041c, sq, sq_AL)
ILCID_POSIX_ELEMENT_ARRAY(0x0430, st, st_ZA)

ILCID_POSIX_SUBTABLE(sv) {
    {0x1d,   "sv"},
    {0x081d, "sv_FI"},
    {0x041d, "sv_SE"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0441, sw, sw_KE)
ILCID_POSIX_ELEMENT_ARRAY(0x045A, syr, syr_SY)
ILCID_POSIX_ELEMENT_ARRAY(0x0449, ta, ta_IN)
ILCID_POSIX_ELEMENT_ARRAY(0x044a, te, te_IN)


ILCID_POSIX_SUBTABLE(tg) {
    {0x28,   "tg"},
    {0x7c28, "tg_Cyrl"},
    {0x0428, "tg_Cyrl_TJ"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x041e, th, th_TH)

ILCID_POSIX_SUBTABLE(ti) {
    {0x73,   "ti"},
    {0x0473, "ti_ER"},
    {0x0873, "ti_ER"},
    {0x0873, "ti_ET"},
    {0x0473, "ti_ET"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0442, tk, tk_TM)

ILCID_POSIX_SUBTABLE(tn) {
    {0x32,   "tn"},
    {0x0432, "tn_BW"},
    {0x0432, "tn_ZA"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x041f, tr, tr_TR)
ILCID_POSIX_ELEMENT_ARRAY(0x0431, ts, ts_ZA)
ILCID_POSIX_ELEMENT_ARRAY(0x0444, tt, tt_RU)

ILCID_POSIX_SUBTABLE(tzm) {
    {0x5f,   "tzm"},
    {0x7c5f, "tzm_Latn"},
    {0x085f, "tzm_Latn_DZ"},
    {0x045f, "tmz"}
};

ILCID_POSIX_SUBTABLE(ug) {
    {0x80,   "ug"},
    {0x0480, "ug_CN"},
    {0x0480, "ug_Arab_CN"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0422, uk, uk_UA)

ILCID_POSIX_SUBTABLE(ur) {
    {0x20,   "ur"},
    {0x0820, "ur_IN"},
    {0x0420, "ur_PK"}
};

ILCID_POSIX_SUBTABLE(uz) {
    {0x43,   "uz"},
    {0x0843, "uz_Cyrl_UZ"},  
    {0x7843, "uz_Cyrl"},  
    {0x0843, "uz_UZ"},  
    {0x0443, "uz_Latn_UZ"}, 
    {0x7c43, "uz_Latn"} 
};

ILCID_POSIX_SUBTABLE(ve) { 
    {0x33,   "ve"},
    {0x0433, "ve_ZA"},
    {0x0433, "ven_ZA"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x042a, vi, vi_VN)

ILCID_POSIX_SUBTABLE(wen) {
    {0x2E,   "wen"},
    {0x042E, "wen_DE"},
    {0x042E, "hsb_DE"},
    {0x082E, "dsb_DE"},
    {0x7C2E, "dsb"},
    {0x2E,   "hsb"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0488, wo, wo_SN)
ILCID_POSIX_ELEMENT_ARRAY(0x0434, xh, xh_ZA)
ILCID_POSIX_ELEMENT_ARRAY(0x043d, yi, yi)
ILCID_POSIX_ELEMENT_ARRAY(0x046a, yo, yo_NG)

ILCID_POSIX_SUBTABLE(zh) {
    {0x0004, "zh_Hans"},
    {0x7804, "zh"},
    {0x0804, "zh_CN"},
    {0x0804, "zh_Hans_CN"},
    {0x0c04, "zh_Hant_HK"},
    {0x0c04, "zh_HK"},
    {0x1404, "zh_Hant_MO"},
    {0x1404, "zh_MO"},
    {0x1004, "zh_Hans_SG"},
    {0x1004, "zh_SG"},
    {0x0404, "zh_Hant_TW"},
    {0x7c04, "zh_Hant"},
    {0x0404, "zh_TW"},
    {0x30404,"zh_Hant_TW"},     
    {0x30404,"zh_TW"},          
    {0x20004,"zh@collation=stroke"},
    {0x20404,"zh_Hant@collation=stroke"},
    {0x20404,"zh_Hant_TW@collation=stroke"},
    {0x20404,"zh_TW@collation=stroke"},
    {0x20804,"zh_Hans@collation=stroke"},
    {0x20804,"zh_Hans_CN@collation=stroke"},
    {0x20804,"zh_CN@collation=stroke"}
};

ILCID_POSIX_ELEMENT_ARRAY(0x0435, zu, zu_ZA)


static const ILcidPosixMap gPosixIDmap[] = {
    ILCID_POSIX_MAP(af),    
    ILCID_POSIX_MAP(am),    
    ILCID_POSIX_MAP(ar),    
    ILCID_POSIX_MAP(arn),   
    ILCID_POSIX_MAP(as),    
    ILCID_POSIX_MAP(az),    
    ILCID_POSIX_MAP(ba),    
    ILCID_POSIX_MAP(be),    

    ILCID_POSIX_MAP(bg),    
    ILCID_POSIX_MAP(bin),   
    ILCID_POSIX_MAP(bn),    
    ILCID_POSIX_MAP(bo),    
    ILCID_POSIX_MAP(br),    
    ILCID_POSIX_MAP(ca),    
    ILCID_POSIX_MAP(chr),   
    ILCID_POSIX_MAP(co),    
    ILCID_POSIX_MAP(cs),    
    ILCID_POSIX_MAP(cy),    
    ILCID_POSIX_MAP(da),    
    ILCID_POSIX_MAP(de),    
    ILCID_POSIX_MAP(dv),    
    ILCID_POSIX_MAP(el),    
    ILCID_POSIX_MAP(en),    
    ILCID_POSIX_MAP(en_US_POSIX), 
    ILCID_POSIX_MAP(es),    
    ILCID_POSIX_MAP(et),    
    ILCID_POSIX_MAP(eu),    
    ILCID_POSIX_MAP(fa),    
    ILCID_POSIX_MAP(fa_AF), 
    ILCID_POSIX_MAP(fi),    
    ILCID_POSIX_MAP(fil),   
    ILCID_POSIX_MAP(fo),    
    ILCID_POSIX_MAP(fr),    
    ILCID_POSIX_MAP(fuv),   
    ILCID_POSIX_MAP(fy),    
    ILCID_POSIX_MAP(ga),    
    ILCID_POSIX_MAP(gd),    
    ILCID_POSIX_MAP(gl),    
    ILCID_POSIX_MAP(gn),    
    ILCID_POSIX_MAP(gsw),   
    ILCID_POSIX_MAP(gu),    
    ILCID_POSIX_MAP(ha),    
    ILCID_POSIX_MAP(haw),   
    ILCID_POSIX_MAP(he),    
    ILCID_POSIX_MAP(hi),    
    ILCID_POSIX_MAP(hr),    
    ILCID_POSIX_MAP(hu),    
    ILCID_POSIX_MAP(hy),    
    ILCID_POSIX_MAP(ibb),   
    ILCID_POSIX_MAP(id),    
    ILCID_POSIX_MAP(ig),    
    ILCID_POSIX_MAP(ii),    
    ILCID_POSIX_MAP(is),    
    ILCID_POSIX_MAP(it),    
    ILCID_POSIX_MAP(iu),    
    ILCID_POSIX_MAP(iw),    
    ILCID_POSIX_MAP(ja),    
    ILCID_POSIX_MAP(ka),    
    ILCID_POSIX_MAP(kk),    
    ILCID_POSIX_MAP(kl),    
    ILCID_POSIX_MAP(km),    
    ILCID_POSIX_MAP(kn),    
    ILCID_POSIX_MAP(ko),    
    ILCID_POSIX_MAP(kok),   
    ILCID_POSIX_MAP(kr),    
    ILCID_POSIX_MAP(ks),    
    ILCID_POSIX_MAP(ky),    
    ILCID_POSIX_MAP(lb),    
    ILCID_POSIX_MAP(la),    
    ILCID_POSIX_MAP(lo),    
    ILCID_POSIX_MAP(lt),    
    ILCID_POSIX_MAP(lv),    
    ILCID_POSIX_MAP(mi),    
    ILCID_POSIX_MAP(mk),    
    ILCID_POSIX_MAP(ml),    
    ILCID_POSIX_MAP(mn),    
    ILCID_POSIX_MAP(mni),   
    ILCID_POSIX_MAP(moh),   
    ILCID_POSIX_MAP(mr),    
    ILCID_POSIX_MAP(ms),    
    ILCID_POSIX_MAP(mt),    
    ILCID_POSIX_MAP(my),    

    ILCID_POSIX_MAP(ne),    
    ILCID_POSIX_MAP(nl),    

    ILCID_POSIX_MAP(no),    
    ILCID_POSIX_MAP(nso),   
    ILCID_POSIX_MAP(oc),    
    ILCID_POSIX_MAP(om),    
    ILCID_POSIX_MAP(or_IN), 
    ILCID_POSIX_MAP(pa),    
    ILCID_POSIX_MAP(pap),   
    ILCID_POSIX_MAP(pl),    
    ILCID_POSIX_MAP(ps),    
    ILCID_POSIX_MAP(pt),    
    ILCID_POSIX_MAP(qu),    
    ILCID_POSIX_MAP(qut),   
    ILCID_POSIX_MAP(rm),    
    ILCID_POSIX_MAP(ro),    
    ILCID_POSIX_MAP(root),  
    ILCID_POSIX_MAP(ru),    
    ILCID_POSIX_MAP(rw),    
    ILCID_POSIX_MAP(sa),    
    ILCID_POSIX_MAP(sah),   
    ILCID_POSIX_MAP(sd),    
    ILCID_POSIX_MAP(se),    

    ILCID_POSIX_MAP(si),    
    ILCID_POSIX_MAP(sk),    
    ILCID_POSIX_MAP(sl),    
    ILCID_POSIX_MAP(so),    
    ILCID_POSIX_MAP(sq),    

    ILCID_POSIX_MAP(st),    
    ILCID_POSIX_MAP(sv),    
    ILCID_POSIX_MAP(sw),    
    ILCID_POSIX_MAP(syr),   
    ILCID_POSIX_MAP(ta),    
    ILCID_POSIX_MAP(te),    
    ILCID_POSIX_MAP(tg),    
    ILCID_POSIX_MAP(th),    
    ILCID_POSIX_MAP(ti),    
    ILCID_POSIX_MAP(tk),    
    ILCID_POSIX_MAP(tn),    
    ILCID_POSIX_MAP(tr),    
    ILCID_POSIX_MAP(ts),    
    ILCID_POSIX_MAP(tt),    
    ILCID_POSIX_MAP(tzm),   
    ILCID_POSIX_MAP(ug),    
    ILCID_POSIX_MAP(uk),    
    ILCID_POSIX_MAP(ur),    
    ILCID_POSIX_MAP(uz),    
    ILCID_POSIX_MAP(ve),    
    ILCID_POSIX_MAP(vi),    
    ILCID_POSIX_MAP(wen),   
    ILCID_POSIX_MAP(wo),    
    ILCID_POSIX_MAP(xh),    
    ILCID_POSIX_MAP(yi),    
    ILCID_POSIX_MAP(yo),    
    ILCID_POSIX_MAP(zh),    
    ILCID_POSIX_MAP(zu),    
};

static const uint32_t gLocaleCount = sizeof(gPosixIDmap)/sizeof(ILcidPosixMap);






static int32_t
idCmp(const char* id1, const char* id2)
{
    int32_t diffIdx = 0;
    while (*id1 == *id2 && *id1 != 0) {
        diffIdx++;
        id1++;
        id2++;
    }
    return diffIdx;
}









static uint32_t
getHostID(const ILcidPosixMap *this_0, const char* posixID, UErrorCode* status)
{
    int32_t bestIdx = 0;
    int32_t bestIdxDiff = 0;
    int32_t posixIDlen = (int32_t)uprv_strlen(posixID);
    uint32_t idx;

    for (idx = 0; idx < this_0->numRegions; idx++ ) {
        int32_t sameChars = idCmp(posixID, this_0->regionMaps[idx].posixID);
        if (sameChars > bestIdxDiff && this_0->regionMaps[idx].posixID[sameChars] == 0) {
            if (posixIDlen == sameChars) {
                
                return this_0->regionMaps[idx].hostID;
            }
            bestIdxDiff = sameChars;
            bestIdx = idx;
        }
    }
    
    
    if ((posixID[bestIdxDiff] == '_' || posixID[bestIdxDiff] == '@')
        && this_0->regionMaps[bestIdx].posixID[bestIdxDiff] == 0)
    {
        *status = U_USING_FALLBACK_WARNING;
        return this_0->regionMaps[bestIdx].hostID;
    }

    
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return this_0->regionMaps->hostID;
}

static const char*
getPosixID(const ILcidPosixMap *this_0, uint32_t hostID)
{
    uint32_t i;
    for (i = 0; i <= this_0->numRegions; i++)
    {
        if (this_0->regionMaps[i].hostID == hostID)
        {
            return this_0->regionMaps[i].posixID;
        }
    }

    

    return this_0->regionMaps[0].posixID;
}








#ifdef USE_WINDOWS_LOCALE_API



#define FIX_LOCALE_ID_TAG_SEPARATOR(buffer, len, i) \
    for(i = 0; i < len; i++) \
        if (buffer[i] == '-') buffer[i] = '_';






#define FIX_LANGUAGE_ID_TAG(buffer, len) \
    if (len >= 3) { \
        if (buffer[0] == 'q' && buffer[1] == 'u' && buffer[2] == 'z') {\
            buffer[2] = 0; \
            uprv_strcat(buffer, buffer+3); \
        } else if (buffer[0] == 'p' && buffer[1] == 'r' && buffer[2] == 's') {\
            buffer[0] = 'f'; buffer[1] = 'a'; buffer[2] = 0; \
            uprv_strcat(buffer, buffer+3); \
        } \
    }

static char gPosixFromLCID[ULOC_FULLNAME_CAPACITY];
#endif
U_CAPI const char *
uprv_convertToPosix(uint32_t hostid, UErrorCode* status)
{
    uint16_t langID;
    uint32_t localeIndex;
#ifdef USE_WINDOWS_LOCALE_API
    int32_t ret = 0;

    uprv_memset(gPosixFromLCID, 0, sizeof(gPosixFromLCID));

    ret = GetLocaleInfoA(hostid, LOCALE_SNAME, (LPSTR)gPosixFromLCID, sizeof(gPosixFromLCID));
    if (ret > 1) {
        FIX_LOCALE_ID_TAG_SEPARATOR(gPosixFromLCID, (uint32_t)ret, localeIndex)
        FIX_LANGUAGE_ID_TAG(gPosixFromLCID, ret)

        return gPosixFromLCID;
    }
#endif
    langID = LANGUAGE_LCID(hostid);

    for (localeIndex = 0; localeIndex < gLocaleCount; localeIndex++)
    {
        if (langID == gPosixIDmap[localeIndex].regionMaps->hostID)
        {
            return getPosixID(&gPosixIDmap[localeIndex], hostid);
        }
    }

    
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
}












U_CAPI uint32_t
uprv_convertToLCID(const char *langID, const char* posixID, UErrorCode* status)
{

    uint32_t   low    = 0;
    uint32_t   high   = gLocaleCount;
    uint32_t   mid;
    uint32_t   oldmid = 0;
    int32_t    compVal;

    uint32_t   value         = 0;
    uint32_t   fallbackValue = (uint32_t)-1;
    UErrorCode myStatus;
    uint32_t   idx;

    
    if (!langID || !posixID || uprv_strlen(langID) < 2 || uprv_strlen(posixID) < 2) {
        return 0;
    }

    

    while (high > low)  {

        mid = (high+low) >> 1; 

        if (mid == oldmid) 
            break;

        compVal = uprv_strcmp(langID, gPosixIDmap[mid].regionMaps->posixID);
        if (compVal < 0){
            high = mid;
        }
        else if (compVal > 0){
            low = mid;
        }
        else {
            return getHostID(&gPosixIDmap[mid], posixID, status);
        }
        oldmid = mid;
    }

    



    for (idx = 0; idx < gLocaleCount; idx++ ) {
        myStatus = U_ZERO_ERROR;
        value = getHostID(&gPosixIDmap[idx], posixID, &myStatus);
        if (myStatus == U_ZERO_ERROR) {
            return value;
        }
        else if (myStatus == U_USING_FALLBACK_WARNING) {
            fallbackValue = value;
        }
    }

    if (fallbackValue != (uint32_t)-1) {
        *status = U_USING_FALLBACK_WARNING;
        return fallbackValue;
    }

    
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;   
}

