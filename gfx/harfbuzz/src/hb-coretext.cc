



























#define HB_SHAPER coretext
#define hb_coretext_shaper_face_data_t CGFont
#include "hb-shaper-impl-private.hh"

#include "hb-coretext.h"


#ifndef HB_DEBUG_CORETEXT
#define HB_DEBUG_CORETEXT (HB_DEBUG+0)
#endif


static void
release_table_data (void *user_data)
{
  CFDataRef cf_data = reinterpret_cast<CFDataRef> (user_data);
  CFRelease(cf_data);
}

static hb_blob_t *
reference_table  (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  CGFontRef cg_font = reinterpret_cast<CGFontRef> (user_data);
  CFDataRef cf_data = CGFontCopyTableForTag (cg_font, tag);
  if (unlikely (!cf_data))
    return NULL;

  const char *data = reinterpret_cast<const char*> (CFDataGetBytePtr (cf_data));
  const size_t length = CFDataGetLength (cf_data);
  if (!data || !length)
    return NULL;

  return hb_blob_create (data, length, HB_MEMORY_MODE_READONLY,
			 reinterpret_cast<void *> (const_cast<__CFData *> (cf_data)),
			 release_table_data);
}

hb_face_t *
hb_coretext_face_create (CGFontRef cg_font)
{
  return hb_face_create_for_tables (reference_table, CGFontRetain (cg_font), (hb_destroy_func_t) CGFontRelease);
}


HB_SHAPER_DATA_ENSURE_DECLARE(coretext, face)
HB_SHAPER_DATA_ENSURE_DECLARE(coretext, font)






static void
release_data (void *info, const void *data, size_t size)
{
  assert (hb_blob_get_length ((hb_blob_t *) info) == size &&
          hb_blob_get_data ((hb_blob_t *) info, NULL) == data);

  hb_blob_destroy ((hb_blob_t *) info);
}

hb_coretext_shaper_face_data_t *
_hb_coretext_shaper_face_data_create (hb_face_t *face)
{
  hb_coretext_shaper_face_data_t *data = NULL;

  if (face->destroy == (hb_destroy_func_t) CGFontRelease)
  {
    data = CGFontRetain ((CGFontRef) face->user_data);
  }
  else
  {
    hb_blob_t *blob = hb_face_reference_blob (face);
    unsigned int blob_length;
    const char *blob_data = hb_blob_get_data (blob, &blob_length);
    if (unlikely (!blob_length))
      DEBUG_MSG (CORETEXT, face, "Face has empty blob");

    CGDataProviderRef provider = CGDataProviderCreateWithData (blob, blob_data, blob_length, &release_data);
    if (likely (provider))
    {
      data = CGFontCreateWithDataProvider (provider);
      CGDataProviderRelease (provider);
    }
  }

  if (unlikely (!data)) {
    DEBUG_MSG (CORETEXT, face, "Face CGFontCreateWithDataProvider() failed");
  }

  return data;
}

void
_hb_coretext_shaper_face_data_destroy (hb_coretext_shaper_face_data_t *data)
{
  CFRelease (data);
}

CGFontRef
hb_coretext_face_get_cg_font (hb_face_t *face)
{
  if (unlikely (!hb_coretext_shaper_face_data_ensure (face))) return NULL;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  return face_data;
}






struct hb_coretext_shaper_font_data_t {
  CTFontRef ct_font;
};

hb_coretext_shaper_font_data_t *
_hb_coretext_shaper_font_data_create (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_face_data_ensure (font->face))) return NULL;

  hb_coretext_shaper_font_data_t *data = (hb_coretext_shaper_font_data_t *) calloc (1, sizeof (hb_coretext_shaper_font_data_t));
  if (unlikely (!data))
    return NULL;

  hb_face_t *face = font->face;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);

  data->ct_font = CTFontCreateWithGraphicsFont (face_data, font->y_scale, NULL, NULL);
  if (unlikely (!data->ct_font)) {
    DEBUG_MSG (CORETEXT, font, "Font CTFontCreateWithGraphicsFont() failed");
    free (data);
    return NULL;
  }

  return data;
}

void
_hb_coretext_shaper_font_data_destroy (hb_coretext_shaper_font_data_t *data)
{
  CFRelease (data->ct_font);
  free (data);
}






struct hb_coretext_shaper_shape_plan_data_t {};

hb_coretext_shaper_shape_plan_data_t *
_hb_coretext_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					     const hb_feature_t *user_features HB_UNUSED,
					     unsigned int        num_user_features HB_UNUSED)
{
  return (hb_coretext_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_coretext_shaper_shape_plan_data_destroy (hb_coretext_shaper_shape_plan_data_t *data HB_UNUSED)
{
}

CTFontRef
hb_coretext_font_get_ct_font (hb_font_t *font)
{
  if (unlikely (!hb_coretext_shaper_font_data_ensure (font))) return NULL;
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);
  return font_data->ct_font;
}






struct feature_record_t {
  unsigned int feature;
  unsigned int setting;
};

struct active_feature_t {
  feature_record_t rec;
  unsigned int order;

  static int cmp (const active_feature_t *a, const active_feature_t *b) {
    return a->rec.feature < b->rec.feature ? -1 : a->rec.feature > b->rec.feature ? 1 :
	   a->order < b->order ? -1 : a->order > b->order ? 1 :
	   a->rec.setting < b->rec.setting ? -1 : a->rec.setting > b->rec.setting ? 1 :
	   0;
  }
  bool operator== (const active_feature_t *f) {
    return cmp (this, f) == 0;
  }
};

struct feature_event_t {
  unsigned int index;
  bool start;
  active_feature_t feature;

  static int cmp (const feature_event_t *a, const feature_event_t *b) {
    return a->index < b->index ? -1 : a->index > b->index ? 1 :
	   a->start < b->start ? -1 : a->start > b->start ? 1 :
	   active_feature_t::cmp (&a->feature, &b->feature);
  }
};

struct range_record_t {
  CTFontRef font;
  unsigned int index_first; 
  unsigned int index_last;  
};



#define kAltHalfWidthTextSelector		6
#define kAltProportionalTextSelector		5
#define kAlternateHorizKanaOffSelector		1
#define kAlternateHorizKanaOnSelector		0
#define kAlternateKanaType			34
#define kAlternateVertKanaOffSelector		3
#define kAlternateVertKanaOnSelector		2
#define kCaseSensitiveLayoutOffSelector		1
#define kCaseSensitiveLayoutOnSelector		0
#define kCaseSensitiveLayoutType		33
#define kCaseSensitiveSpacingOffSelector	3
#define kCaseSensitiveSpacingOnSelector		2
#define kContextualAlternatesOffSelector	1
#define kContextualAlternatesOnSelector		0
#define kContextualAlternatesType		36
#define kContextualLigaturesOffSelector		19
#define kContextualLigaturesOnSelector		18
#define kContextualSwashAlternatesOffSelector	5
#define kContextualSwashAlternatesOnSelector	4
#define kDefaultLowerCaseSelector		0
#define kDefaultUpperCaseSelector		0
#define kHistoricalLigaturesOffSelector		21
#define kHistoricalLigaturesOnSelector		20
#define kHojoCharactersSelector			12
#define kJIS2004CharactersSelector		11
#define kLowerCasePetiteCapsSelector		2
#define kLowerCaseSmallCapsSelector		1
#define kLowerCaseType				37
#define kMathematicalGreekOffSelector		11
#define kMathematicalGreekOnSelector		10
#define kNLCCharactersSelector			13
#define kQuarterWidthTextSelector		4
#define kScientificInferiorsSelector		4
#define kStylisticAltEightOffSelector		17
#define kStylisticAltEightOnSelector		16
#define kStylisticAltEighteenOffSelector	37
#define kStylisticAltEighteenOnSelector		36
#define kStylisticAltElevenOffSelector		23
#define kStylisticAltElevenOnSelector		22
#define kStylisticAltFifteenOffSelector		31
#define kStylisticAltFifteenOnSelector		30
#define kStylisticAltFiveOffSelector		11
#define kStylisticAltFiveOnSelector		10
#define kStylisticAltFourOffSelector		9
#define kStylisticAltFourOnSelector		8
#define kStylisticAltFourteenOffSelector	29
#define kStylisticAltFourteenOnSelector		28
#define kStylisticAltNineOffSelector		19
#define kStylisticAltNineOnSelector		18
#define kStylisticAltNineteenOffSelector	39
#define kStylisticAltNineteenOnSelector		38
#define kStylisticAltOneOffSelector		3
#define kStylisticAltOneOnSelector		2
#define kStylisticAltSevenOffSelector		15
#define kStylisticAltSevenOnSelector		14
#define kStylisticAltSeventeenOffSelector	35
#define kStylisticAltSeventeenOnSelector	34
#define kStylisticAltSixOffSelector		13
#define kStylisticAltSixOnSelector		12
#define kStylisticAltSixteenOffSelector		33
#define kStylisticAltSixteenOnSelector		32
#define kStylisticAltTenOffSelector		21
#define kStylisticAltTenOnSelector		20
#define kStylisticAltThirteenOffSelector	27
#define kStylisticAltThirteenOnSelector		26
#define kStylisticAltThreeOffSelector		7
#define kStylisticAltThreeOnSelector		6
#define kStylisticAltTwelveOffSelector		25
#define kStylisticAltTwelveOnSelector		24
#define kStylisticAltTwentyOffSelector		41
#define kStylisticAltTwentyOnSelector		40
#define kStylisticAltTwoOffSelector		5
#define kStylisticAltTwoOnSelector		4
#define kStylisticAlternativesType		35
#define kSwashAlternatesOffSelector		3
#define kSwashAlternatesOnSelector		2
#define kThirdWidthTextSelector			3
#define kTraditionalNamesCharactersSelector	14
#define kUpperCasePetiteCapsSelector		2
#define kUpperCaseSmallCapsSelector		1
#define kUpperCaseType				38


static const struct feature_mapping_t {
    FourCharCode otFeatureTag;
    uint16_t aatFeatureType;
    uint16_t selectorToEnable;
    uint16_t selectorToDisable;
} feature_mappings[] = {
    { 'c2pc',   kUpperCaseType,             kUpperCasePetiteCapsSelector,           kDefaultUpperCaseSelector },
    { 'c2sc',   kUpperCaseType,             kUpperCaseSmallCapsSelector,            kDefaultUpperCaseSelector },
    { 'calt',   kContextualAlternatesType,  kContextualAlternatesOnSelector,        kContextualAlternatesOffSelector },
    { 'case',   kCaseSensitiveLayoutType,   kCaseSensitiveLayoutOnSelector,         kCaseSensitiveLayoutOffSelector },
    { 'clig',   kLigaturesType,             kContextualLigaturesOnSelector,         kContextualLigaturesOffSelector },
    { 'cpsp',   kCaseSensitiveLayoutType,   kCaseSensitiveSpacingOnSelector,        kCaseSensitiveSpacingOffSelector },
    { 'cswh',   kContextualAlternatesType,  kContextualSwashAlternatesOnSelector,   kContextualSwashAlternatesOffSelector },
    { 'dlig',   kLigaturesType,             kRareLigaturesOnSelector,               kRareLigaturesOffSelector },
    { 'expt',   kCharacterShapeType,        kExpertCharactersSelector,              16 },
    { 'frac',   kFractionsType,             kDiagonalFractionsSelector,             kNoFractionsSelector },
    { 'fwid',   kTextSpacingType,           kMonospacedTextSelector,                7 },
    { 'halt',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'hist',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hkna',   kAlternateKanaType,         kAlternateHorizKanaOnSelector,          kAlternateHorizKanaOffSelector, },
    { 'hlig',   kLigaturesType,             kHistoricalLigaturesOnSelector,         kHistoricalLigaturesOffSelector },
    { 'hngl',   kTransliterationType,       kHanjaToHangulSelector,                 kNoTransliterationSelector },
    { 'hojo',   kCharacterShapeType,        kHojoCharactersSelector,                16 },
    { 'hwid',   kTextSpacingType,           kHalfWidthTextSelector,                 7 },
    { 'ital',   kItalicCJKRomanType,        kCJKItalicRomanOnSelector,              kCJKItalicRomanOffSelector },
    { 'jp04',   kCharacterShapeType,        kJIS2004CharactersSelector,             16 },
    { 'jp78',   kCharacterShapeType,        kJIS1978CharactersSelector,             16 },
    { 'jp83',   kCharacterShapeType,        kJIS1983CharactersSelector,             16 },
    { 'jp90',   kCharacterShapeType,        kJIS1990CharactersSelector,             16 },
    { 'liga',   kLigaturesType,             kCommonLigaturesOnSelector,             kCommonLigaturesOffSelector },
    { 'lnum',   kNumberCaseType,            kUpperCaseNumbersSelector,              2 },
    { 'mgrk',   kMathematicalExtrasType,    kMathematicalGreekOnSelector,           kMathematicalGreekOffSelector },
    { 'nlck',   kCharacterShapeType,        kNLCCharactersSelector,                 16 },
    { 'onum',   kNumberCaseType,            kLowerCaseNumbersSelector,              2 },
    { 'ordn',   kVerticalPositionType,      kOrdinalsSelector,                      kNormalPositionSelector },
    { 'palt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'pcap',   kLowerCaseType,             kLowerCasePetiteCapsSelector,           kDefaultLowerCaseSelector },
    { 'pkna',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'pnum',   kNumberSpacingType,         kProportionalNumbersSelector,           4 },
    { 'pwid',   kTextSpacingType,           kProportionalTextSelector,              7 },
    { 'qwid',   kTextSpacingType,           kQuarterWidthTextSelector,              7 },
    { 'ruby',   kRubyKanaType,              kRubyKanaOnSelector,                    kRubyKanaOffSelector },
    { 'sinf',   kVerticalPositionType,      kScientificInferiorsSelector,           kNormalPositionSelector },
    { 'smcp',   kLowerCaseType,             kLowerCaseSmallCapsSelector,            kDefaultLowerCaseSelector },
    { 'smpl',   kCharacterShapeType,        kSimplifiedCharactersSelector,          16 },
    { 'ss01',   kStylisticAlternativesType, kStylisticAltOneOnSelector,             kStylisticAltOneOffSelector },
    { 'ss02',   kStylisticAlternativesType, kStylisticAltTwoOnSelector,             kStylisticAltTwoOffSelector },
    { 'ss03',   kStylisticAlternativesType, kStylisticAltThreeOnSelector,           kStylisticAltThreeOffSelector },
    { 'ss04',   kStylisticAlternativesType, kStylisticAltFourOnSelector,            kStylisticAltFourOffSelector },
    { 'ss05',   kStylisticAlternativesType, kStylisticAltFiveOnSelector,            kStylisticAltFiveOffSelector },
    { 'ss06',   kStylisticAlternativesType, kStylisticAltSixOnSelector,             kStylisticAltSixOffSelector },
    { 'ss07',   kStylisticAlternativesType, kStylisticAltSevenOnSelector,           kStylisticAltSevenOffSelector },
    { 'ss08',   kStylisticAlternativesType, kStylisticAltEightOnSelector,           kStylisticAltEightOffSelector },
    { 'ss09',   kStylisticAlternativesType, kStylisticAltNineOnSelector,            kStylisticAltNineOffSelector },
    { 'ss10',   kStylisticAlternativesType, kStylisticAltTenOnSelector,             kStylisticAltTenOffSelector },
    { 'ss11',   kStylisticAlternativesType, kStylisticAltElevenOnSelector,          kStylisticAltElevenOffSelector },
    { 'ss12',   kStylisticAlternativesType, kStylisticAltTwelveOnSelector,          kStylisticAltTwelveOffSelector },
    { 'ss13',   kStylisticAlternativesType, kStylisticAltThirteenOnSelector,        kStylisticAltThirteenOffSelector },
    { 'ss14',   kStylisticAlternativesType, kStylisticAltFourteenOnSelector,        kStylisticAltFourteenOffSelector },
    { 'ss15',   kStylisticAlternativesType, kStylisticAltFifteenOnSelector,         kStylisticAltFifteenOffSelector },
    { 'ss16',   kStylisticAlternativesType, kStylisticAltSixteenOnSelector,         kStylisticAltSixteenOffSelector },
    { 'ss17',   kStylisticAlternativesType, kStylisticAltSeventeenOnSelector,       kStylisticAltSeventeenOffSelector },
    { 'ss18',   kStylisticAlternativesType, kStylisticAltEighteenOnSelector,        kStylisticAltEighteenOffSelector },
    { 'ss19',   kStylisticAlternativesType, kStylisticAltNineteenOnSelector,        kStylisticAltNineteenOffSelector },
    { 'ss20',   kStylisticAlternativesType, kStylisticAltTwentyOnSelector,          kStylisticAltTwentyOffSelector },
    { 'subs',   kVerticalPositionType,      kInferiorsSelector,                     kNormalPositionSelector },
    { 'sups',   kVerticalPositionType,      kSuperiorsSelector,                     kNormalPositionSelector },
    { 'swsh',   kContextualAlternatesType,  kSwashAlternatesOnSelector,             kSwashAlternatesOffSelector },
    { 'titl',   kStyleOptionsType,          kTitlingCapsSelector,                   kNoStyleOptionsSelector },
    { 'tnam',   kCharacterShapeType,        kTraditionalNamesCharactersSelector,    16 },
    { 'tnum',   kNumberSpacingType,         kMonospacedNumbersSelector,             4 },
    { 'trad',   kCharacterShapeType,        kTraditionalCharactersSelector,         16 },
    { 'twid',   kTextSpacingType,           kThirdWidthTextSelector,                7 },
    { 'unic',   kLetterCaseType,            14,                                     15 },
    { 'valt',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vert',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'vhal',   kTextSpacingType,           kAltHalfWidthTextSelector,              7 },
    { 'vkna',   kAlternateKanaType,         kAlternateVertKanaOnSelector,           kAlternateVertKanaOffSelector },
    { 'vpal',   kTextSpacingType,           kAltProportionalTextSelector,           7 },
    { 'vrt2',   kVerticalSubstitutionType,  kSubstituteVerticalFormsOnSelector,     kSubstituteVerticalFormsOffSelector },
    { 'zero',   kTypographicExtrasType,     kSlashedZeroOnSelector,                 kSlashedZeroOffSelector },
};

static int
_hb_feature_mapping_cmp (const void *key_, const void *entry_)
{
  unsigned int key = * (unsigned int *) key_;
  const feature_mapping_t * entry = (const feature_mapping_t *) entry_;
  return key < entry->otFeatureTag ? -1 :
	 key > entry->otFeatureTag ? 1 :
	 0;
}

hb_bool_t
_hb_coretext_shape (hb_shape_plan_t    *shape_plan,
		    hb_font_t          *font,
                    hb_buffer_t        *buffer,
                    const hb_feature_t *features,
                    unsigned int        num_features)
{
  hb_face_t *face = font->face;
  hb_coretext_shaper_face_data_t *face_data = HB_SHAPER_DATA_GET (face);
  hb_coretext_shaper_font_data_t *font_data = HB_SHAPER_DATA_GET (font);

  






  {
    hb_unicode_funcs_t *unicode = buffer->unicode;
    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    for (unsigned int i = 1; i < count; i++)
      if (HB_UNICODE_GENERAL_CATEGORY_IS_MARK (unicode->general_category (info[i].codepoint)))
	buffer->merge_clusters (i - 1, i + 1);
  }

  hb_auto_array_t<feature_record_t> feature_records;
  hb_auto_array_t<range_record_t> range_records;

  



  if (num_features)
  {
    
    hb_auto_array_t<feature_event_t> feature_events;
    for (unsigned int i = 0; i < num_features; i++)
    {
      const feature_mapping_t * mapping = (const feature_mapping_t *) bsearch (&features[i].tag,
									       feature_mappings,
									       ARRAY_LENGTH (feature_mappings),
									       sizeof (feature_mappings[0]),
									       _hb_feature_mapping_cmp);
      if (!mapping)
        continue;

      active_feature_t feature;
      feature.rec.feature = mapping->aatFeatureType;
      feature.rec.setting = features[i].value ? mapping->selectorToEnable : mapping->selectorToDisable;
      feature.order = i;

      feature_event_t *event;

      event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = features[i].start;
      event->start = true;
      event->feature = feature;

      event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = features[i].end;
      event->start = false;
      event->feature = feature;
    }
    feature_events.qsort ();
    
    {
      active_feature_t feature;
      feature.rec.feature = HB_TAG_NONE;
      feature.rec.setting = 0;
      feature.order = num_features + 1;

      feature_event_t *event = feature_events.push ();
      if (unlikely (!event))
	goto fail_features;
      event->index = 0; 
      event->start = false;
      event->feature = feature;
    }

    
    hb_auto_array_t<active_feature_t> active_features;
    unsigned int last_index = 0;
    for (unsigned int i = 0; i < feature_events.len; i++)
    {
      feature_event_t *event = &feature_events[i];

      if (event->index != last_index)
      {
        
	range_record_t *range = range_records.push ();
	if (unlikely (!range))
	  goto fail_features;

	if (active_features.len)
	{
	  CFMutableArrayRef features_array = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

	  
	  
	  for (unsigned int j = 0; j < active_features.len; j++)
	  {
	    CFStringRef keys[2] = {
	      kCTFontFeatureTypeIdentifierKey,
	      kCTFontFeatureSelectorIdentifierKey
	    };
	    CFNumberRef values[2] = {
	      CFNumberCreate (kCFAllocatorDefault, kCFNumberIntType, &active_features[j].rec.feature),
	      CFNumberCreate (kCFAllocatorDefault, kCFNumberIntType, &active_features[j].rec.setting)
	    };
	    CFDictionaryRef dict = CFDictionaryCreate (kCFAllocatorDefault,
						       (const void **) keys,
						       (const void **) values,
						       2,
						       &kCFTypeDictionaryKeyCallBacks,
						       &kCFTypeDictionaryValueCallBacks);
	    CFRelease (values[0]);
	    CFRelease (values[1]);

	    CFArrayAppendValue (features_array, dict);
	    CFRelease (dict);

	  }

	  CFDictionaryRef attributes = CFDictionaryCreate (kCFAllocatorDefault,
							   (const void **) &kCTFontFeatureSettingsAttribute,
							   (const void **) &features_array,
							   1,
							   &kCFTypeDictionaryKeyCallBacks,
							   &kCFTypeDictionaryValueCallBacks);
	  CFRelease (features_array);

	  CTFontDescriptorRef font_desc = CTFontDescriptorCreateWithAttributes (attributes);
	  CFRelease (attributes);

	  range->font = CTFontCreateCopyWithAttributes (font_data->ct_font, 0.0, NULL, font_desc);
	  CFRelease (font_desc);
	}
	else
	{
	  range->font = NULL;
	}

	range->index_first = last_index;
	range->index_last  = event->index - 1;

	last_index = event->index;
      }

      if (event->start) {
        active_feature_t *feature = active_features.push ();
	if (unlikely (!feature))
	  goto fail_features;
	*feature = event->feature;
      } else {
        active_feature_t *feature = active_features.find (&event->feature);
	if (feature)
	  active_features.remove (feature - active_features.array);
      }
    }

    if (!range_records.len) 
      goto fail_features;
  }
  else
  {
  fail_features:
    num_features = 0;
  }

  unsigned int scratch_size;
  hb_buffer_t::scratch_buffer_t *scratch = buffer->get_scratch_buffer (&scratch_size);

#define ALLOCATE_ARRAY(Type, name, len, on_no_room) \
  Type *name = (Type *) scratch; \
  { \
    unsigned int _consumed = DIV_CEIL ((len) * sizeof (Type), sizeof (*scratch)); \
    if (unlikely (_consumed > scratch_size)) \
    { \
      on_no_room; \
      assert (0); \
    } \
    scratch += _consumed; \
    scratch_size -= _consumed; \
  }

  ALLOCATE_ARRAY (UniChar, pchars, buffer->len * 2, );
  unsigned int chars_len = 0;
  for (unsigned int i = 0; i < buffer->len; i++) {
    hb_codepoint_t c = buffer->info[i].codepoint;
    if (likely (c <= 0xFFFFu))
      pchars[chars_len++] = c;
    else if (unlikely (c > 0x10FFFFu))
      pchars[chars_len++] = 0xFFFDu;
    else {
      pchars[chars_len++] = 0xD800u + ((c - 0x10000u) >> 10);
      pchars[chars_len++] = 0xDC00u + ((c - 0x10000u) & ((1 << 10) - 1));
    }
  }

  ALLOCATE_ARRAY (unsigned int, log_clusters, chars_len, );
  chars_len = 0;
  for (unsigned int i = 0; i < buffer->len; i++)
  {
    hb_codepoint_t c = buffer->info[i].codepoint;
    unsigned int cluster = buffer->info[i].cluster;
    log_clusters[chars_len++] = cluster;
    if (hb_in_range (c, 0x10000u, 0x10FFFFu))
      log_clusters[chars_len++] = cluster; 
  }

#define FAIL(...) \
  HB_STMT_START { \
    DEBUG_MSG (CORETEXT, NULL, __VA_ARGS__); \
    ret = false; \
    goto fail; \
  } HB_STMT_END;

  bool ret = true;
  CFStringRef string_ref = NULL;
  CTLineRef line = NULL;

  if (0)
  {
resize_and_retry:
    DEBUG_MSG (CORETEXT, buffer, "Buffer resize");
    

    assert (string_ref);
    assert (line);
    CFRelease (string_ref);
    CFRelease (line);
    string_ref = NULL;
    line = NULL;

    

    unsigned int old_scratch_used;
    hb_buffer_t::scratch_buffer_t *old_scratch;
    old_scratch = buffer->get_scratch_buffer (&old_scratch_used);
    old_scratch_used = scratch - old_scratch;

    if (unlikely (!buffer->ensure (buffer->allocated * 2)))
      FAIL ("Buffer resize failed");

    

    scratch = buffer->get_scratch_buffer (&scratch_size);
    pchars = reinterpret_cast<UniChar *> (((char *) scratch + ((char *) pchars - (char *) old_scratch)));
    log_clusters = reinterpret_cast<unsigned int *> (((char *) scratch + ((char *) log_clusters - (char *) old_scratch)));
    scratch += old_scratch_used;
    scratch_size -= old_scratch_used;
  }
retry:
  {
    string_ref = CFStringCreateWithCharactersNoCopy (NULL,
						     pchars, chars_len,
						     kCFAllocatorNull);
    if (unlikely (!string_ref))
      FAIL ("CFStringCreateWithCharactersNoCopy failed");

    
    {
      CFMutableAttributedStringRef attr_string = CFAttributedStringCreateMutable (kCFAllocatorDefault,
										  chars_len);
      if (unlikely (!attr_string))
	FAIL ("CFAttributedStringCreateMutable failed");
      CFAttributedStringReplaceString (attr_string, CFRangeMake (0, 0), string_ref);
      if (HB_DIRECTION_IS_VERTICAL (buffer->props.direction))
      {
	CFAttributedStringSetAttribute (attr_string, CFRangeMake (0, chars_len),
					kCTVerticalFormsAttributeName, kCFBooleanTrue);
      }

      if (buffer->props.language)
      {



#if MAC_OS_X_VERSION_MIN_REQUIRED < 1090
#  define kCTLanguageAttributeName CFSTR ("NSLanguage")
#endif
        CFStringRef lang = CFStringCreateWithCStringNoCopy (kCFAllocatorDefault,
							    hb_language_to_string (buffer->props.language),
							    kCFStringEncodingUTF8,
							    kCFAllocatorNull);
	if (unlikely (!lang))
	  FAIL ("CFStringCreateWithCStringNoCopy failed");
	CFAttributedStringSetAttribute (attr_string, CFRangeMake (0, chars_len),
					kCTLanguageAttributeName, lang);
	CFRelease (lang);
      }
      CFAttributedStringSetAttribute (attr_string, CFRangeMake (0, chars_len),
				      kCTFontAttributeName, font_data->ct_font);

      if (num_features)
      {
	unsigned int start = 0;
	range_record_t *last_range = &range_records[0];
	for (unsigned int k = 0; k < chars_len; k++)
	{
	  range_record_t *range = last_range;
	  while (log_clusters[k] < range->index_first)
	    range--;
	  while (log_clusters[k] > range->index_last)
	    range++;
	  if (range != last_range)
	  {
	    if (last_range->font)
	      CFAttributedStringSetAttribute (attr_string, CFRangeMake (start, k - start),
					      kCTFontAttributeName, last_range->font);

	    start = k;
	  }

	  last_range = range;
	}
	if (start != chars_len && last_range->font)
	  CFAttributedStringSetAttribute (attr_string, CFRangeMake (start, chars_len - start),
					  kCTFontAttributeName, last_range->font);
      }

      int level = HB_DIRECTION_IS_FORWARD (buffer->props.direction) ? 0 : 1;
      CFNumberRef level_number = CFNumberCreate (kCFAllocatorDefault, kCFNumberIntType, &level);
      CFDictionaryRef options = CFDictionaryCreate (kCFAllocatorDefault,
						    (const void **) &kCTTypesetterOptionForcedEmbeddingLevel,
						    (const void **) &level_number,
						    1,
						    &kCFTypeDictionaryKeyCallBacks,
						    &kCFTypeDictionaryValueCallBacks);
      if (unlikely (!options))
        FAIL ("CFDictionaryCreate failed");

      CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedStringAndOptions (attr_string, options);
      CFRelease (options);
      CFRelease (attr_string);
      if (unlikely (!typesetter))
	FAIL ("CTTypesetterCreateWithAttributedStringAndOptions failed");

      line = CTTypesetterCreateLine (typesetter, CFRangeMake(0, 0));
      CFRelease (typesetter);
      if (unlikely (!line))
	FAIL ("CTTypesetterCreateLine failed");
    }

    CFArrayRef glyph_runs = CTLineGetGlyphRuns (line);
    unsigned int num_runs = CFArrayGetCount (glyph_runs);
    DEBUG_MSG (CORETEXT, NULL, "Num runs: %d", num_runs);

    buffer->len = 0;
    uint32_t status_and = ~0, status_or = 0;

    const CFRange range_all = CFRangeMake (0, 0);

    for (unsigned int i = 0; i < num_runs; i++)
    {
      CTRunRef run = static_cast<CTRunRef>(CFArrayGetValueAtIndex (glyph_runs, i));
      CTRunStatus run_status = CTRunGetStatus (run);
      status_or  |= run_status;
      status_and &= run_status;
      DEBUG_MSG (CORETEXT, run, "CTRunStatus: %x", run_status);

      





      CFDictionaryRef attributes = CTRunGetAttributes (run);
      CTFontRef run_ct_font = static_cast<CTFontRef>(CFDictionaryGetValue (attributes, kCTFontAttributeName));
      if (!CFEqual (run_ct_font, font_data->ct_font))
      {
	



























	bool matched = false;
	for (unsigned int i = 0; i < range_records.len; i++)
	  if (range_records[i].font && CFEqual (run_ct_font, range_records[i].font))
	  {
	    matched = true;
	    break;
	  }
	if (!matched)
	{
	  CGFontRef run_cg_font = CTFontCopyGraphicsFont (run_ct_font, 0);
	  if (run_cg_font)
	  {
	    matched = CFEqual (run_cg_font, face_data);
	    CFRelease (run_cg_font);
	  }
	}
	if (!matched)
	{
	  CFStringRef font_ps_name = CTFontCopyName (font_data->ct_font, kCTFontPostScriptNameKey);
	  CFStringRef run_ps_name = CTFontCopyName (run_ct_font, kCTFontPostScriptNameKey);
	  CFComparisonResult result = CFStringCompare (run_ps_name, font_ps_name, 0);
	  CFRelease (run_ps_name);
	  CFRelease (font_ps_name);
	  if (result == kCFCompareEqualTo)
	    matched = true;
	}
	if (!matched)
	{
	  CFRange range = CTRunGetStringRange (run);
          DEBUG_MSG (CORETEXT, run, "Run used fallback font: %ld..%ld",
		     range.location, range.location + range.length);
	  if (!buffer->ensure_inplace (buffer->len + range.length))
	    goto resize_and_retry;
	  hb_glyph_info_t *info = buffer->info + buffer->len;

	  CGGlyph notdef = 0;
	  double advance = CTFontGetAdvancesForGlyphs (font_data->ct_font, kCTFontHorizontalOrientation, &notdef, NULL, 1);

	  unsigned int old_len = buffer->len;
	  for (CFIndex j = range.location; j < range.location + range.length; j++)
	  {
	      UniChar ch = CFStringGetCharacterAtIndex (string_ref, j);
	      if (hb_in_range<UniChar> (ch, 0xDC00u, 0xDFFFu) && range.location < j)
	      {
		ch = CFStringGetCharacterAtIndex (string_ref, j - 1);
		if (hb_in_range<UniChar> (ch, 0xD800u, 0xDBFFu))
		  

		  continue;
	      }

	      info->codepoint = notdef;
	      info->cluster = log_clusters[j];

	      info->mask = advance;
	      info->var1.u32 = 0;
	      info->var2.u32 = 0;

	      info++;
	      buffer->len++;
	  }
	  if (HB_DIRECTION_IS_BACKWARD (buffer->props.direction))
	    buffer->reverse_range (old_len, buffer->len);
	  continue;
	}
      }

      unsigned int num_glyphs = CTRunGetGlyphCount (run);
      if (num_glyphs == 0)
	continue;

      if (!buffer->ensure_inplace (buffer->len + num_glyphs))
	goto resize_and_retry;

      hb_glyph_info_t *run_info = buffer->info + buffer->len;

      





#define USE_PTR true

#define SCRATCH_SAVE() \
  unsigned int scratch_size_saved = scratch_size; \
  hb_buffer_t::scratch_buffer_t *scratch_saved = scratch

#define SCRATCH_RESTORE() \
  scratch_size = scratch_size_saved; \
  scratch = scratch_saved;

      {
        SCRATCH_SAVE();
	const CGGlyph* glyphs = USE_PTR ? CTRunGetGlyphsPtr (run) : NULL;
	if (!glyphs) {
	  ALLOCATE_ARRAY (CGGlyph, glyph_buf, num_glyphs, goto resize_and_retry);
	  CTRunGetGlyphs (run, range_all, glyph_buf);
	  glyphs = glyph_buf;
	}
	const CFIndex* string_indices = USE_PTR ? CTRunGetStringIndicesPtr (run) : NULL;
	if (!string_indices) {
	  ALLOCATE_ARRAY (CFIndex, index_buf, num_glyphs, goto resize_and_retry);
	  CTRunGetStringIndices (run, range_all, index_buf);
	  string_indices = index_buf;
	}
	hb_glyph_info_t *info = run_info;
	for (unsigned int j = 0; j < num_glyphs; j++)
	{
	  info->codepoint = glyphs[j];
	  info->cluster = log_clusters[string_indices[j]];
	  info++;
	}
	SCRATCH_RESTORE();
      }
      {
        SCRATCH_SAVE();
	const CGPoint* positions = USE_PTR ? CTRunGetPositionsPtr (run) : NULL;
	if (!positions) {
	  ALLOCATE_ARRAY (CGPoint, position_buf, num_glyphs, goto resize_and_retry);
	  CTRunGetPositions (run, range_all, position_buf);
	  positions = position_buf;
	}
	double run_advance = CTRunGetTypographicBounds (run, range_all, NULL, NULL, NULL);
	DEBUG_MSG (CORETEXT, run, "Run advance: %g", run_advance);
	hb_glyph_info_t *info = run_info;
	if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
	{
	  for (unsigned int j = 0; j < num_glyphs; j++)
	  {
	    double advance = (j + 1 < num_glyphs ? positions[j + 1].x : positions[0].x + run_advance) - positions[j].x;
	    info->mask = advance;
	    info->var1.u32 = positions[0].x; 
	    info->var2.u32 = positions[j].y;
	    info++;
	  }
	}
	else
	{
	  run_advance = -run_advance;
	  for (unsigned int j = 0; j < num_glyphs; j++)
	  {
	    double advance = (j + 1 < num_glyphs ? positions[j + 1].y : positions[0].y + run_advance) - positions[j].y;
	    info->mask = advance;
	    info->var1.u32 = positions[j].x;
	    info->var2.u32 = positions[0].y; 
	    info++;
	  }
	}
	SCRATCH_RESTORE();
      }
#undef SCRATCH_RESTORE
#undef SCRATCH_SAVE
#undef USE_PTR
#undef ALLOCATE_ARRAY

      buffer->len += num_glyphs;
    }

    
    bool backward = HB_DIRECTION_IS_BACKWARD (buffer->props.direction);
    assert (bool (status_and & kCTRunStatusRightToLeft) == backward);
    assert (bool (status_or  & kCTRunStatusRightToLeft) == backward);

    buffer->clear_positions ();

    unsigned int count = buffer->len;
    hb_glyph_info_t *info = buffer->info;
    hb_glyph_position_t *pos = buffer->pos;
    if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
      for (unsigned int i = 0; i < count; i++)
      {
	pos->x_advance = info->mask;
	pos->x_offset = info->var1.u32;
	pos->y_offset = info->var2.u32;
	info++, pos++;
      }
    else
      for (unsigned int i = 0; i < count; i++)
      {
	pos->y_advance = info->mask;
	pos->x_offset = info->var1.u32;
	pos->y_offset = info->var2.u32;
	info++, pos++;
      }

    









    if (count > 1 && (status_or & kCTRunStatusNonMonotonic))
    {
      hb_glyph_info_t *info = buffer->info;
      if (HB_DIRECTION_IS_FORWARD (buffer->props.direction))
      {
	unsigned int cluster = info[count - 1].cluster;
	for (unsigned int i = count - 1; i > 0; i--)
	{
	  cluster = MIN (cluster, info[i - 1].cluster);
	  info[i - 1].cluster = cluster;
	}
      }
      else
      {
	unsigned int cluster = info[0].cluster;
	for (unsigned int i = 1; i < count; i++)
	{
	  cluster = MIN (cluster, info[i].cluster);
	  info[i].cluster = cluster;
	}
      }
    }
  }

#undef FAIL

fail:
  if (string_ref)
    CFRelease (string_ref);
  if (line)
    CFRelease (line);

  for (unsigned int i = 0; i < range_records.len; i++)
    if (range_records[i].font)
      CFRelease (range_records[i].font);

  return ret;
}






HB_SHAPER_DATA_ENSURE_DECLARE(coretext_aat, face)
HB_SHAPER_DATA_ENSURE_DECLARE(coretext_aat, font)






struct hb_coretext_aat_shaper_face_data_t {};

hb_coretext_aat_shaper_face_data_t *
_hb_coretext_aat_shaper_face_data_create (hb_face_t *face)
{
  hb_blob_t *mort_blob = face->reference_table (HB_CORETEXT_TAG_MORT);
  

  if (!hb_blob_get_length (mort_blob))
  {
    hb_blob_destroy (mort_blob);
    mort_blob = face->reference_table (HB_CORETEXT_TAG_MORX);
    if (!hb_blob_get_length (mort_blob))
    {
      hb_blob_destroy (mort_blob);
      return NULL;
    }
  }
  hb_blob_destroy (mort_blob);

  return hb_coretext_shaper_face_data_ensure (face) ? (hb_coretext_aat_shaper_face_data_t *) HB_SHAPER_DATA_SUCCEEDED : NULL;
}

void
_hb_coretext_aat_shaper_face_data_destroy (hb_coretext_aat_shaper_face_data_t *data HB_UNUSED)
{
}






struct hb_coretext_aat_shaper_font_data_t {};

hb_coretext_aat_shaper_font_data_t *
_hb_coretext_aat_shaper_font_data_create (hb_font_t *font)
{
  return hb_coretext_shaper_font_data_ensure (font) ? (hb_coretext_aat_shaper_font_data_t *) HB_SHAPER_DATA_SUCCEEDED : NULL;
}

void
_hb_coretext_aat_shaper_font_data_destroy (hb_coretext_aat_shaper_font_data_t *data HB_UNUSED)
{
}






struct hb_coretext_aat_shaper_shape_plan_data_t {};

hb_coretext_aat_shaper_shape_plan_data_t *
_hb_coretext_aat_shaper_shape_plan_data_create (hb_shape_plan_t    *shape_plan HB_UNUSED,
					     const hb_feature_t *user_features HB_UNUSED,
					     unsigned int        num_user_features HB_UNUSED)
{
  return (hb_coretext_aat_shaper_shape_plan_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_coretext_aat_shaper_shape_plan_data_destroy (hb_coretext_aat_shaper_shape_plan_data_t *data HB_UNUSED)
{
}






hb_bool_t
_hb_coretext_aat_shape (hb_shape_plan_t    *shape_plan,
			hb_font_t          *font,
			hb_buffer_t        *buffer,
			const hb_feature_t *features,
			unsigned int        num_features)
{
  return _hb_coretext_shape (shape_plan, font, buffer, features, num_features);
}
