

























#ifndef HB_OT_LAYOUT_JSTF_TABLE_HH
#define HB_OT_LAYOUT_JSTF_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-layout-gpos-table.hh"


namespace OT {






typedef IndexArray JstfModList;






typedef OffsetListOf<PosLookup> JstfMax;






struct JstfPriority
{
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 shrinkageEnableGSUB.sanitize (c, this) &&
			 shrinkageDisableGSUB.sanitize (c, this) &&
			 shrinkageEnableGPOS.sanitize (c, this) &&
			 shrinkageDisableGPOS.sanitize (c, this) &&
			 shrinkageJstfMax.sanitize (c, this) &&
			 extensionEnableGSUB.sanitize (c, this) &&
			 extensionDisableGSUB.sanitize (c, this) &&
			 extensionEnableGPOS.sanitize (c, this) &&
			 extensionDisableGPOS.sanitize (c, this) &&
			 extensionJstfMax.sanitize (c, this));
  }

  protected:
  OffsetTo<JstfModList>
		shrinkageEnableGSUB;	


  OffsetTo<JstfModList>
		shrinkageDisableGSUB;	


  OffsetTo<JstfModList>
		shrinkageEnableGPOS;	


  OffsetTo<JstfModList>
		shrinkageDisableGPOS;	


  OffsetTo<JstfMax>
		shrinkageJstfMax;	


  OffsetTo<JstfModList>
		extensionEnableGSUB;	


  OffsetTo<JstfModList>
		extensionDisableGSUB;	


  OffsetTo<JstfModList>
		extensionEnableGPOS;	


  OffsetTo<JstfModList>
		extensionDisableGPOS;	


  OffsetTo<JstfMax>
		extensionJstfMax;	



  public:
  DEFINE_SIZE_STATIC (20);
};






struct JstfLangSys : OffsetListOf<JstfPriority>
{
  inline bool sanitize (hb_sanitize_context_t *c,
			const Record<JstfLangSys>::sanitize_closure_t * = NULL) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (OffsetListOf<JstfPriority>::sanitize (c));
  }
};






typedef SortedArrayOf<GlyphID> ExtenderGlyphs;






struct JstfScript
{
  inline unsigned int get_lang_sys_count (void) const
  { return langSys.len; }
  inline const Tag& get_lang_sys_tag (unsigned int i) const
  { return langSys.get_tag (i); }
  inline unsigned int get_lang_sys_tags (unsigned int start_offset,
					 unsigned int *lang_sys_count ,
					 hb_tag_t     *lang_sys_tags ) const
  { return langSys.get_tags (start_offset, lang_sys_count, lang_sys_tags); }
  inline const JstfLangSys& get_lang_sys (unsigned int i) const
  {
    if (i == Index::NOT_FOUND_INDEX) return get_default_lang_sys ();
    return this+langSys[i].offset;
  }
  inline bool find_lang_sys_index (hb_tag_t tag, unsigned int *index) const
  { return langSys.find_index (tag, index); }

  inline bool has_default_lang_sys (void) const { return defaultLangSys != 0; }
  inline const JstfLangSys& get_default_lang_sys (void) const { return this+defaultLangSys; }

  inline bool sanitize (hb_sanitize_context_t *c,
			const Record<JstfScript>::sanitize_closure_t * = NULL) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (extenderGlyphs.sanitize (c, this) &&
			 defaultLangSys.sanitize (c, this) &&
			 langSys.sanitize (c, this));
  }

  protected:
  OffsetTo<ExtenderGlyphs>
		extenderGlyphs;	

  OffsetTo<JstfLangSys>
		defaultLangSys;	

  RecordArrayOf<JstfLangSys>
		langSys;	

  public:
  DEFINE_SIZE_ARRAY (6, langSys);
};






struct JSTF
{
  static const hb_tag_t tableTag	= HB_OT_TAG_JSTF;

  inline unsigned int get_script_count (void) const
  { return scriptList.len; }
  inline const Tag& get_script_tag (unsigned int i) const
  { return scriptList.get_tag (i); }
  inline unsigned int get_script_tags (unsigned int start_offset,
				       unsigned int *script_count ,
				       hb_tag_t     *script_tags ) const
  { return scriptList.get_tags (start_offset, script_count, script_tags); }
  inline const JstfScript& get_script (unsigned int i) const
  { return this+scriptList[i].offset; }
  inline bool find_script_index (hb_tag_t tag, unsigned int *index) const
  { return scriptList.find_index (tag, index); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (version.sanitize (c) && likely (version.major == 1) &&
			 scriptList.sanitize (c, this));
  }

  protected:
  FixedVersion	version;	

  RecordArrayOf<JstfScript>
		scriptList;  	

  public:
  DEFINE_SIZE_ARRAY (6, scriptList);
};


} 


#endif 
