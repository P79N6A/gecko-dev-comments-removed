






#include "nsIAtom.h"
#include "nsElementTable.h"







const nsHTMLElement gHTMLElements[] = {
  {
             eHTMLTag_unknown,
     kNone, true
  },
  {
             eHTMLTag_a,
     kSpecial, false
  },
  {
             eHTMLTag_abbr,
     kPhrase, false
  },
  {
             eHTMLTag_acronym,
     kPhrase, false
  },
  {
             eHTMLTag_address,
     kBlock, false
  },
  {
             eHTMLTag_applet,
     kSpecial, false
  },
  {
             eHTMLTag_area,
     kNone, true
  },
  {
             eHTMLTag_article,
     kBlock, false
  },
  {
             eHTMLTag_aside,
     kBlock, false
  },
  {
             eHTMLTag_audio,
     kSpecial, false
  },
  {
             eHTMLTag_b,
     kFontStyle, false
  },
  {
             eHTMLTag_base,
     kHeadContent, true
  },
  {
             eHTMLTag_basefont,
     kSpecial, true
  },
  {
             eHTMLTag_bdo,
     kSpecial, false
  },
  {
             eHTMLTag_bgsound,
     (kFlowEntity|kHeadMisc), true
  },
  {
             eHTMLTag_big,
     kFontStyle, false
  },
  {
             eHTMLTag_blockquote,
     kBlock, false
  },
  {
             eHTMLTag_body,
     kHTMLContent, false
  },
  {
             eHTMLTag_br,
     kSpecial, true
  },
  {
             eHTMLTag_button,
     kFormControl, false
  },
  {
             eHTMLTag_canvas,
     kSpecial, false
  },
  {
             eHTMLTag_caption,
     kNone, false
  },
  {
             eHTMLTag_center,
     kBlock, false
  },
  {
             eHTMLTag_cite,
     kPhrase, false
  },
  {
             eHTMLTag_code,
     kPhrase, false
  },
  {
             eHTMLTag_col,
     kNone, true
  },
  {
             eHTMLTag_colgroup,
     kNone, false
  },
  {
             eHTMLTag_content,
     kNone, false
  },
  {
             eHTMLTag_data,
     kPhrase, false
  },
  {
             eHTMLTag_datalist,
     kSpecial, false
  },
  {
             eHTMLTag_dd,
     kInlineEntity, false
  },
  {
             eHTMLTag_del,
     kFlowEntity, false
  },
  {
             eHTMLTag_dfn,
     kPhrase, false
  },
  {
             eHTMLTag_dir,
     kList, false
  },
  {
             eHTMLTag_div,
     kBlock, false
  },
  {
             eHTMLTag_dl,
     kBlock, false
  },
  {
             eHTMLTag_dt,
     kInlineEntity, false
  },
  {
             eHTMLTag_em,
     kPhrase, false
  },
  {
             eHTMLTag_embed,
     kSpecial, true
  },
  {
             eHTMLTag_fieldset,
     kBlock, false
  },
  {
             eHTMLTag_figcaption,
     kPhrase, false
  },
  {
             eHTMLTag_figure,
     kBlock, false
  },
  {
             eHTMLTag_font,
     kFontStyle, false
  },
  {
             eHTMLTag_footer,
     kBlock, false
  },
  {
             eHTMLTag_form,
     kBlock, false
  },
  {
             eHTMLTag_frame,
     kNone, true
  },
  {
             eHTMLTag_frameset,
     kHTMLContent, false
  },
  {
             eHTMLTag_h1,
     kHeading, false
  },
  {
             eHTMLTag_h2,
     kHeading, false
  },
  {
             eHTMLTag_h3,
     kHeading, false
  },
  {
             eHTMLTag_h4,
     kHeading, false
  },
  {
             eHTMLTag_h5,
     kHeading, false
  },
  {
             eHTMLTag_h6,
     kHeading, false
  },
  {
             eHTMLTag_head,
     kHTMLContent, false
  },
  {
             eHTMLTag_header,
     kBlock, false
  },
  {
             eHTMLTag_hgroup,
     kBlock, false
  },
  {
             eHTMLTag_hr,
     kBlock, true
  },
  {
             eHTMLTag_html,
     kNone, false
  },
  {
             eHTMLTag_i,
     kFontStyle, false
  },
  {
             eHTMLTag_iframe,
     kSpecial, false
  },
  {
             eHTMLTag_image,
     kSpecial, true
  },
  {
             eHTMLTag_img,
     kSpecial, true
  },
  {
             eHTMLTag_input,
     kFormControl, true
  },
  {
             eHTMLTag_ins,
     kFlowEntity, false
  },
  {
             eHTMLTag_kbd,
     kPhrase, false
  },
  {
             eHTMLTag_keygen,
     kFlowEntity, true
  },
  {
             eHTMLTag_label,
     kFormControl, false
  },
  {
             eHTMLTag_legend,
     kNone, false
  },
  {
             eHTMLTag_li,
     kBlockEntity, false
  },
  {
             eHTMLTag_link,
     kAllTags - kHeadContent, true
  },
  {
             eHTMLTag_listing,
     kPreformatted, false
  },
  {
             eHTMLTag_main,
     kBlock, false
  },
  {
             eHTMLTag_map,
     kSpecial, false
  },
  {
             eHTMLTag_mark,
     kSpecial, false
  },
  {
             eHTMLTag_marquee,
     kSpecial, false
  },
  {
             eHTMLTag_menu,
     kList, false
  },
  {
             eHTMLTag_menuitem,
     kFlowEntity, false
  },
  {
             eHTMLTag_meta,
     kHeadContent, true
  },
  {
             eHTMLTag_meter,
     kFormControl, false
  },
  {
             eHTMLTag_multicol,
     kBlock, false
  },
  {
             eHTMLTag_nav,
     kBlock, false
  },
  {
             eHTMLTag_nobr,
     kExtensions, false
  },
  {
             eHTMLTag_noembed,
     kFlowEntity, false
  },
  {
             eHTMLTag_noframes,
     kFlowEntity, false
  },
  {
             eHTMLTag_noscript,
     kFlowEntity|kHeadMisc, false
  },
  {
             eHTMLTag_object,
     kSpecial, false
  },
  {
             eHTMLTag_ol,
     kList, false
  },
  {
             eHTMLTag_optgroup,
     kNone, false
  },
  {
             eHTMLTag_option,
     kNone, false
  },
  {
             eHTMLTag_output,
     kSpecial, false
  },
  {
             eHTMLTag_p,
     kBlock, false
  },
  {
             eHTMLTag_param,
     kSpecial, true
  },
  {
             eHTMLTag_picture,
     kSpecial, false
  },
  {
             eHTMLTag_plaintext,
     kExtensions, false
  },
  {
             eHTMLTag_pre,
     kBlock|kPreformatted, false
  },
  {
             eHTMLTag_progress,
     kFormControl, false
  },
  {
             eHTMLTag_q,
     kSpecial, false
  },
  {
             eHTMLTag_rb,
     kPhrase, false
  },
  {
             eHTMLTag_rp,
     kPhrase, false
  },
  {
             eHTMLTag_rt,
     kPhrase, false
  },
  {
             eHTMLTag_rtc,
     kPhrase, false
  },
  {
             eHTMLTag_ruby,
     kPhrase, false
  },
  {
             eHTMLTag_s,
     kFontStyle, false
  },
  {
             eHTMLTag_samp,
     kPhrase, false
  },
  {
             eHTMLTag_script,
     (kSpecial|kHeadContent), false
  },
  {
             eHTMLTag_section,
     kBlock, false
  },
  {
             eHTMLTag_select,
     kFormControl, false
  },
  {
             eHTMLTag_shadow,
     kFlowEntity, false
  },
  {
             eHTMLTag_small,
     kFontStyle, false
  },
  {
             eHTMLTag_source,
     kSpecial, true
  },
  {
             eHTMLTag_span,
     kSpecial, false
  },
  {
             eHTMLTag_strike,
     kFontStyle, false
  },
  {
             eHTMLTag_strong,
     kPhrase, false
  },
  {
             eHTMLTag_style,
     kAllTags - kHeadContent, false
  },
  {
             eHTMLTag_sub,
     kSpecial, false
  },
  {
             eHTMLTag_sup,
     kSpecial, false
  },
  {
             eHTMLTag_table,
     kBlock, false
  },
  {
             eHTMLTag_tbody,
     kNone, false
  },
  {
             eHTMLTag_td,
     kNone, false
  },
  {
             eHTMLTag_textarea,
     kFormControl, false
  },
  {
             eHTMLTag_tfoot,
     kNone, false
  },
  {
             eHTMLTag_th,
     kNone, false
  },
  {
             eHTMLTag_thead,
     kNone, false
  },
  {
             eHTMLTag_template,
     kNone, false
  },
  {
             eHTMLTag_time,
     kPhrase, false
  },
  {
             eHTMLTag_title,
     kHeadContent, false
  },
  {
             eHTMLTag_tr,
     kNone, false
  },
  {
             eHTMLTag_track,
     kSpecial, true
  },
  {
             eHTMLTag_tt,
     kFontStyle, false
  },
  {
             eHTMLTag_u,
     kFontStyle, false
  },
  {
             eHTMLTag_ul,
     kList, false
  },
  {
             eHTMLTag_var,
     kPhrase, false
  },
  {
             eHTMLTag_video,
     kSpecial, false
  },
  {
             eHTMLTag_wbr,
     kExtensions, true
  },
  {
             eHTMLTag_xmp,
     kInlineEntity|kPreformatted, false
  },
  {
             eHTMLTag_text,
     kFlowEntity, true
  },
  {
             eHTMLTag_whitespace,
     kFlowEntity|kHeadMisc, true
  },
  {
             eHTMLTag_newline,
     kFlowEntity|kHeadMisc, true
  },
  {
             eHTMLTag_comment,
     kFlowEntity|kHeadMisc, false
  },
  {
             eHTMLTag_entity,
     kFlowEntity, false
  },
  {
             eHTMLTag_doctypeDecl,
     kFlowEntity, false
  },
  {
             eHTMLTag_markupDecl,
     kFlowEntity, false
  },
  {
             eHTMLTag_instruction,
     kFlowEntity, false
  },
  {
             eHTMLTag_userdefined,
     (kFlowEntity|kHeadMisc), false
  },
};



bool nsHTMLElement::IsContainer(eHTMLTags aChild)
{
  return !gHTMLElements[aChild].mLeaf;
}

bool nsHTMLElement::IsMemberOf(int32_t aSet) const
{
  return TestBits(aSet,mParentBits);
}

#ifdef DEBUG
void CheckElementTable()
{
  for (eHTMLTags t = eHTMLTag_unknown; t <= eHTMLTag_userdefined; t = eHTMLTags(t + 1)) {
    NS_ASSERTION(gHTMLElements[t].mTagID == t, "gHTMLElements entries does match tag list.");
  }
}
#endif
