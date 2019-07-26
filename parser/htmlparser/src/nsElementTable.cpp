






#include "nsIAtom.h"
#include "nsElementTable.h"





#define DECL_TAG_LIST(name_, list_)                                           \
  static const eHTMLTags name_##list[] = list_;                               \
  static const TagList name_ = { NS_ARRAY_LENGTH(name_##list), name_##list };

#define COMMA ,


DECL_TAG_LIST(gAParents,{eHTMLTag_map})
DECL_TAG_LIST(gInAddress,{eHTMLTag_address})
DECL_TAG_LIST(gInHead,{eHTMLTag_head})
DECL_TAG_LIST(gInTable,{eHTMLTag_table})
DECL_TAG_LIST(gInHTML,{eHTMLTag_html})
DECL_TAG_LIST(gInBody,{eHTMLTag_body})
DECL_TAG_LIST(gInForm,{eHTMLTag_form})
DECL_TAG_LIST(gInFieldset,{eHTMLTag_fieldset})
DECL_TAG_LIST(gInTR,{eHTMLTag_tr})
DECL_TAG_LIST(gInDL,{eHTMLTag_dl COMMA eHTMLTag_body})
DECL_TAG_LIST(gInFrameset,{eHTMLTag_frameset})
DECL_TAG_LIST(gInNoframes,{eHTMLTag_noframes})


DECL_TAG_LIST(gInP,{eHTMLTag_span})
DECL_TAG_LIST(gOptgroupParents,{eHTMLTag_select COMMA eHTMLTag_optgroup})
DECL_TAG_LIST(gBodyParents,{eHTMLTag_html COMMA eHTMLTag_noframes})
DECL_TAG_LIST(gColParents,{eHTMLTag_table COMMA eHTMLTag_colgroup})
DECL_TAG_LIST(gFramesetParents,{eHTMLTag_html COMMA eHTMLTag_frameset})
DECL_TAG_LIST(gLegendParents,{eHTMLTag_fieldset})
DECL_TAG_LIST(gAreaParent,{eHTMLTag_map})
DECL_TAG_LIST(gParamParents,{eHTMLTag_applet COMMA eHTMLTag_object})
DECL_TAG_LIST(gTRParents,{eHTMLTag_tbody COMMA eHTMLTag_tfoot COMMA eHTMLTag_thead COMMA eHTMLTag_table})
DECL_TAG_LIST(gTREndParents,{eHTMLTag_tbody COMMA eHTMLTag_tfoot COMMA eHTMLTag_thead COMMA eHTMLTag_table COMMA eHTMLTag_applet})
#ifdef MOZ_MEDIA
DECL_TAG_LIST(gSourceParents,{eHTMLTag_video COMMA eHTMLTag_audio})
#endif





DECL_TAG_LIST(gContainsText,{eHTMLTag_text COMMA eHTMLTag_newline COMMA eHTMLTag_whitespace COMMA eHTMLTag_entity})
DECL_TAG_LIST(gUnknownKids,{eHTMLTag_html COMMA eHTMLTag_frameset})







DECL_TAG_LIST(gContainsOpts,{eHTMLTag_option COMMA eHTMLTag_optgroup COMMA eHTMLTag_script COMMA eHTMLTag_input COMMA eHTMLTag_select COMMA eHTMLTag_textarea })

DECL_TAG_LIST(gContainedInOpt,{eHTMLTag_text COMMA eHTMLTag_newline COMMA eHTMLTag_whitespace COMMA eHTMLTag_entity COMMA eHTMLTag_input COMMA eHTMLTag_select COMMA eHTMLTag_textarea})
DECL_TAG_LIST(gContainsParam,{eHTMLTag_param})
DECL_TAG_LIST(gColgroupKids,{eHTMLTag_col}) 
DECL_TAG_LIST(gAddressKids,{eHTMLTag_p})
DECL_TAG_LIST(gBodyKids,{eHTMLTag_dd COMMA eHTMLTag_del COMMA eHTMLTag_dt COMMA eHTMLTag_ins COMMA  eHTMLTag_noscript COMMA eHTMLTag_script COMMA eHTMLTag_li COMMA eHTMLTag_param}) 
DECL_TAG_LIST(gButtonKids,{eHTMLTag_caption COMMA eHTMLTag_legend})

DECL_TAG_LIST(gDLRootTags,{eHTMLTag_body COMMA eHTMLTag_td COMMA eHTMLTag_table COMMA eHTMLTag_applet COMMA eHTMLTag_dd})
DECL_TAG_LIST(gDLKids,{eHTMLTag_dd COMMA eHTMLTag_dt})
DECL_TAG_LIST(gDTKids,{eHTMLTag_dt})
DECL_TAG_LIST(gFieldsetKids,{eHTMLTag_legend COMMA eHTMLTag_text})
DECL_TAG_LIST(gFontKids,{eHTMLTag_legend COMMA eHTMLTag_table COMMA eHTMLTag_text COMMA eHTMLTag_li}) 
DECL_TAG_LIST(gFormKids,{eHTMLTag_keygen})
DECL_TAG_LIST(gFramesetKids,{eHTMLTag_frame COMMA eHTMLTag_frameset COMMA eHTMLTag_noframes})

DECL_TAG_LIST(gHtmlKids,{eHTMLTag_body COMMA eHTMLTag_frameset COMMA eHTMLTag_head COMMA eHTMLTag_noscript COMMA eHTMLTag_noframes COMMA eHTMLTag_script COMMA eHTMLTag_newline COMMA eHTMLTag_whitespace})

DECL_TAG_LIST(gLabelKids,{eHTMLTag_span})
DECL_TAG_LIST(gLIKids,{eHTMLTag_ol COMMA eHTMLTag_ul})
DECL_TAG_LIST(gMapKids,{eHTMLTag_area})
DECL_TAG_LIST(gPreKids,{eHTMLTag_hr COMMA eHTMLTag_center})  

DECL_TAG_LIST(gTableKids,{eHTMLTag_caption COMMA eHTMLTag_col COMMA eHTMLTag_colgroup COMMA eHTMLTag_form COMMA  eHTMLTag_thead COMMA eHTMLTag_tbody COMMA eHTMLTag_tfoot COMMA eHTMLTag_script})
  
DECL_TAG_LIST(gTableElemKids,{eHTMLTag_form COMMA eHTMLTag_noscript COMMA eHTMLTag_script COMMA eHTMLTag_td COMMA eHTMLTag_th COMMA eHTMLTag_tr})
DECL_TAG_LIST(gTRKids,{eHTMLTag_td COMMA eHTMLTag_th COMMA eHTMLTag_form COMMA eHTMLTag_script})
DECL_TAG_LIST(gTBodyKids,{eHTMLTag_tr COMMA eHTMLTag_form}) 
DECL_TAG_LIST(gULKids,{eHTMLTag_li COMMA eHTMLTag_p})
#ifdef MOZ_MEDIA
DECL_TAG_LIST(gVideoKids,{eHTMLTag_source})
DECL_TAG_LIST(gAudioKids,{eHTMLTag_source})
#endif





DECL_TAG_LIST(gRootTags,{eHTMLTag_body COMMA eHTMLTag_td COMMA eHTMLTag_table COMMA eHTMLTag_applet COMMA eHTMLTag_select}) 
DECL_TAG_LIST(gTableRootTags,{eHTMLTag_applet COMMA eHTMLTag_body COMMA eHTMLTag_dl COMMA eHTMLTag_ol COMMA eHTMLTag_td COMMA eHTMLTag_th})
DECL_TAG_LIST(gHTMLRootTags,{eHTMLTag_unknown})
 
DECL_TAG_LIST(gLIRootTags,{eHTMLTag_ul COMMA eHTMLTag_ol COMMA eHTMLTag_dir COMMA eHTMLTag_menu COMMA eHTMLTag_p COMMA eHTMLTag_body COMMA eHTMLTag_td COMMA eHTMLTag_th})

DECL_TAG_LIST(gOLRootTags,{eHTMLTag_body COMMA eHTMLTag_li COMMA eHTMLTag_td COMMA eHTMLTag_th COMMA eHTMLTag_select})
DECL_TAG_LIST(gTDRootTags,{eHTMLTag_tr COMMA eHTMLTag_tbody COMMA eHTMLTag_thead COMMA eHTMLTag_tfoot COMMA eHTMLTag_table COMMA eHTMLTag_applet})
DECL_TAG_LIST(gNoframeRoot,{eHTMLTag_body COMMA eHTMLTag_frameset})





DECL_TAG_LIST(gBodyAutoClose,{eHTMLTag_head})
DECL_TAG_LIST(gTBodyAutoClose,{eHTMLTag_thead COMMA eHTMLTag_tfoot COMMA eHTMLTag_tbody COMMA eHTMLTag_td COMMA eHTMLTag_th})  
DECL_TAG_LIST(gCaptionAutoClose,{eHTMLTag_tbody})
DECL_TAG_LIST(gLIAutoClose,{eHTMLTag_p COMMA eHTMLTag_li})
DECL_TAG_LIST(gPAutoClose,{eHTMLTag_p COMMA eHTMLTag_li})
DECL_TAG_LIST(gHRAutoClose,{eHTMLTag_p})
DECL_TAG_LIST(gOLAutoClose,{eHTMLTag_p COMMA eHTMLTag_ol})
DECL_TAG_LIST(gDivAutoClose,{eHTMLTag_p})

DECL_TAG_LIST(gInputAutoClose,{eHTMLTag_select COMMA eHTMLTag_optgroup COMMA eHTMLTag_option})

DECL_TAG_LIST(gHeadingTags,{eHTMLTag_h1 COMMA eHTMLTag_h2 COMMA eHTMLTag_h3 COMMA eHTMLTag_h4 COMMA eHTMLTag_h5 COMMA eHTMLTag_h6})

DECL_TAG_LIST(gTableCloseTags,{eHTMLTag_td COMMA eHTMLTag_tr COMMA eHTMLTag_th COMMA eHTMLTag_tbody COMMA eHTMLTag_thead COMMA eHTMLTag_tfoot})
DECL_TAG_LIST(gTRCloseTags,{eHTMLTag_tr COMMA eHTMLTag_td COMMA eHTMLTag_th})
DECL_TAG_LIST(gTDCloseTags,{eHTMLTag_td COMMA eHTMLTag_th})
DECL_TAG_LIST(gDTCloseTags,{eHTMLTag_p COMMA eHTMLTag_dd COMMA eHTMLTag_dt})
DECL_TAG_LIST(gULCloseTags,{eHTMLTag_li})
DECL_TAG_LIST(gULAutoClose,{eHTMLTag_p COMMA eHTMLTag_ul}) 

DECL_TAG_LIST(gExcludableParents,{eHTMLTag_pre}) 
DECL_TAG_LIST(gCaptionExcludableParents,{eHTMLTag_td}) 






const int kNoPropRange=0;
const int kDefaultPropRange=1;
const int kBodyPropRange=2;








const nsHTMLElement gHTMLElements[] = {
  {
                                 eHTMLTag_unknown,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,  
     0,0,0,0,
              kNone, kNone, kNone,
           kNonContainer, 10,
                0,&gUnknownKids,
  },
  {
                                 eHTMLTag_a,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kInlineEntity, kNone,  
           kVerifyHierarchy, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_abbr,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_acronym,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_address,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, kInlineEntity, kNone,
           0,kDefaultPropRange,
                0,&gAddressKids,
  },
  {
                                 eHTMLTag_applet,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kInlineEntity|kFlowEntity), kNone,
           kRequiresBody,kDefaultPropRange,
                0,&gContainsParam,
  },
  {
                                 eHTMLTag_area,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gAreaParent,&gRootTags,
     0,0,0,0,
              kNone, kInlineEntity, kSelf,
           kNonContainer,kDefaultPropRange,
                &gAreaParent,0,
  },
  {
                                 eHTMLTag_article,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_aside,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
#if defined(MOZ_MEDIA)
  {
                                 eHTMLTag_audio,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0, 0, 0,0,
              kSpecial, (kFlowEntity|kSelf), kNone,
           0,kDefaultPropRange,
                0,&gAudioKids,
  },
#endif
  {
                                 eHTMLTag_b,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kInlineEntity|kSelf), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_base,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInHead,&gRootTags,
     0,0,0,0,
              kHeadContent, kNone, kNone,
           kNonContainer, kNoPropRange,
                &gInHead,0,
  },
  {
                                 eHTMLTag_basefont,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kInlineEntity, kNone,
           kNonContainer, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_bdo,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_bgsound,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              (kFlowEntity|kHeadMisc), kNone, kNone,
           kNonContainer,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_big,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kInlineEntity|kSelf), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_blink,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kFlowEntity|kSelf), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_blockquote,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,  
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_body,
              eHTMLTag_unknown,eHTMLTag_frameset,
              &gInHTML,&gInHTML,
     &gBodyAutoClose,0,0,0,
              kHTMLContent,(kFlowEntity|kSelf), kNone,
           kOmitEndTag, kBodyPropRange,
                0,&gBodyKids,
  },
  {
                                 eHTMLTag_br,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kNone, kNone,
           kRequiresBody|kNonContainer, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_button,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFormControl, kFlowEntity, kFormControl,
           kRequiresBody,kDefaultPropRange,
                0,&gButtonKids,
  },
  {
                                 eHTMLTag_canvas,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kFlowEntity|kSelf), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_caption,
              eHTMLTag_table,eHTMLTag_unknown,
              &gInTable,&gInTable,
     &gCaptionAutoClose,0,0,0,
              kNone, kFlowEntity, kSelf,
           (kNoPropagate|kNoStyleLeaksOut),kDefaultPropRange,
                &gInTable,0,
  },
  {
                                 eHTMLTag_center,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_cite,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_code,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_col,
              eHTMLTag_table,eHTMLTag_unknown,
              &gColParents,&gColParents,
     0,0,0,0,
              kNone, kNone, kNone,
           kNoPropagate|kNonContainer,kDefaultPropRange,
                &gColParents,0,
  },
  {
                                 eHTMLTag_colgroup,
              eHTMLTag_table,eHTMLTag_unknown,
              &gInTable,&gInTable,
     0,0,0,0,
              kNone, kNone, kNone,
           kNoPropagate,kDefaultPropRange,
                &gInTable,&gColgroupKids,
  },
  {
                                 eHTMLTag_datalist,
                    eHTMLTag_unknown, eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kInlineEntity|kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_dd,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gDTCloseTags,0,&gDLKids,0,
              kInlineEntity, kFlowEntity, kNone,
           kNoPropagate|kMustCloseSelf|kVerifyHierarchy|kRequiresBody,kDefaultPropRange,
                &gInDL,0,
  },
  {
                                 eHTMLTag_del,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, (kSelf|kFlowEntity), kNone,
           0, kDefaultPropRange,
                &gInBody,0,
  },
  {
                                 eHTMLTag_dfn,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_dir,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gOLRootTags,&gOLRootTags,
     &gOLAutoClose, &gULCloseTags, 0,0,
              kList, (kFlowEntity|kSelf), kNone,
           0,kDefaultPropRange,
                0,&gULKids,
  },
  {
                                 eHTMLTag_div,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gDivAutoClose,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_dl,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gDLRootTags,&gRootTags,  
     0,0,0,&gDTKids,           
              kBlock, kSelf|kFlowEntity, kNone,
           0, kNoPropRange,
                0,&gDLKids,
  },
  {
                                 eHTMLTag_dt,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gDTCloseTags,0,&gDLKids,0,
              kInlineEntity, (kFlowEntity-kHeading), kNone,  
           (kNoPropagate|kMustCloseSelf|kVerifyHierarchy|kRequiresBody),kDefaultPropRange,
                &gInDL,0,
  },
  {
                                 eHTMLTag_em,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_embed,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kNone, kNone,
           kNonContainer|kRequiresBody,kDefaultPropRange,
                0,&gContainsParam,
  },
  {
                                 eHTMLTag_fieldset,
                    eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           kNoPropagate,kDefaultPropRange,
                0,&gFieldsetKids,
  },
  {
                                 eHTMLTag_figcaption,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_figure,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_font,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,&gFontKids,
  },
  {
                                 eHTMLTag_footer,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_form,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, kFlowEntity, kNone,
           kNoStyleLeaksIn, kNoPropRange,
                0,&gFormKids,
  },
  {
                                 eHTMLTag_frame, 
              eHTMLTag_frameset,eHTMLTag_unknown,
              &gInFrameset,&gInFrameset,
     0,0,0,0,
              kNone, kNone, kNone,
           kNoPropagate|kNoStyleLeaksIn|kNonContainer, kNoPropRange,
                &gInFrameset,0,
  },
  {
                                 eHTMLTag_frameset,
              eHTMLTag_unknown,eHTMLTag_body,
              &gFramesetParents,&gInHTML,
     0,0,0,0,
              kHTMLContent, kSelf, kAllTags,
           kNoPropagate|kNoStyleLeaksIn, kNoPropRange,
                &gInHTML,&gFramesetKids,
  },

  {
                                 eHTMLTag_h1,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_h2,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_h3,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_h4,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_h5,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_h6,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHeadingTags,  &gHeadingTags, &gHeadingTags,0,
              kHeading, kFlowEntity, kNone,
           kVerifyHierarchy,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_head,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInHTML,&gInHTML,
     0,0,0,0,
              kHTMLContent, (kHeadContent|kHeadMisc), kNone,
           kNoStyleLeaksIn, kDefaultPropRange,
                &gInHTML,0,
  },
  {
                                 eHTMLTag_header,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_hgroup,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_hr,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gHRAutoClose,0,0,0,
              kBlock, kNone, kNone,
           kNonContainer|kRequiresBody,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_html,
              eHTMLTag_unknown,eHTMLTag_html,
              &gHTMLRootTags,&gHTMLRootTags,
     0,0,0,0,
              kNone, kHTMLContent, kNone,
           kSaveMisplaced|kOmitEndTag|kNoStyleLeaksIn, kDefaultPropRange,
                0,&gHtmlKids,
  },
  {
                                 eHTMLTag_i,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_iframe,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kFlowEntity), kNone,
           kNoStyleLeaksIn, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_image,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kNone, kNone,
           kNonContainer,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_img,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kNone, kNone,
           kNonContainer|kRequiresBody,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_input,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     &gInputAutoClose,0,0,0,
              kFormControl, kNone, kNone,
           kNonContainer|kRequiresBody,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_ins,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, (kSelf|kFlowEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_kbd,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_keygen,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           kNonContainer,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_label,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFormControl, kInlineEntity, kSelf,
           0,kDefaultPropRange,
                0,&gLabelKids,
  },
  {
                                 eHTMLTag_legend,
                    eHTMLTag_fieldset,eHTMLTag_unknown,
              &gInFieldset,&gInFieldset,
     0,0,0,0,
              kNone, kInlineEntity, kNone,
           kRequiresBody,kDefaultPropRange,
                &gInFieldset,0,
  },
  {
                                 eHTMLTag_li,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gLIRootTags,&gLIRootTags,
     &gLIAutoClose,0,0,0,
              kBlockEntity, kFlowEntity, kSelf, 
           kNoPropagate|kVerifyHierarchy|kRequiresBody, kDefaultPropRange,
                0,&gLIKids,
  },
  {
                                 eHTMLTag_link,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInHead,&gInHead,
     0,0,0,0,
              kAllTags - kHeadContent, kNone, kNone,
           kNonContainer|kPreferHead|kLegalOpen,kDefaultPropRange,
                &gInHead,0,
  },
  {
                                 eHTMLTag_listing,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPreformatted, (kSelf|kFlowEntity), kNone,  
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_main,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_map,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, kInlineEntity|kBlockEntity, kNone,
           0, kDefaultPropRange,
                0,&gMapKids,
  },
  {
                                 eHTMLTag_mark,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kInlineEntity|kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_marquee,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kFlowEntity), kNone,
           kRequiresBody, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_menu,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kList, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,&gULKids,
  },
  {
                                 eHTMLTag_menuitem,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_meta,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInHead,&gInHead,
     0,0,0,0,
              kHeadContent, kNone, kNone,
           kNoStyleLeaksIn|kNonContainer, kDefaultPropRange,
                &gInHead,0,
  },
  {
                                 eHTMLTag_meter,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFormControl, kFlowEntity, kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_multicol,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, kFlowEntity, kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_nav,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_nobr,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kExtensions, kFlowEntity, kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_noembed, 
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, kFlowEntity, kNone,
           0, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_noframes,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gNoframeRoot,&gNoframeRoot,
     0,0,0,0,
              kFlowEntity, kFlowEntity, kNone,
           0, kNoPropRange,
                &gNoframeRoot,0,
  },
  {
                                 eHTMLTag_noscript,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity|kHeadMisc, kFlowEntity|kSelf, kNone,
           0, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_object,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kFlowEntity|kSelf), kNone,
           kNoStyleLeaksOut|kPreferBody,kDefaultPropRange,
                0,&gContainsParam,
  },
  {
                                 eHTMLTag_ol,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gOLRootTags,&gOLRootTags,
     &gOLAutoClose, &gULCloseTags, 0,0,
              kList, (kFlowEntity|kSelf), kNone,
           0,kDefaultPropRange,   
                0,&gULKids,
  },
  {
                                 eHTMLTag_optgroup,
                    eHTMLTag_select,eHTMLTag_unknown,
              &gOptgroupParents,&gOptgroupParents,
     0,0,0,0,
              kNone, kNone, kNone,
           0,kDefaultPropRange,
                &gOptgroupParents,&gContainsOpts,
  },
  {
                                 eHTMLTag_option,
                    eHTMLTag_select,eHTMLTag_unknown,
              &gOptgroupParents,&gOptgroupParents, 
     0,0,0,0,
              kNone, kPCDATA, kFlowEntity|kHeadMisc,
           kNoStyleLeaksIn|kNoPropagate, kDefaultPropRange,
                &gOptgroupParents,&gContainedInOpt,
  },
  {
                                 eHTMLTag_output,
                    eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kInlineEntity|kSelf), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_p,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, kInlineEntity, kNone,      
           kHandleStrayTag,kDefaultPropRange, 
                0,&gInP,
  },
  {
                                 eHTMLTag_param,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gParamParents,&gParamParents,
     &gPAutoClose,0,0,0,
              kSpecial, kNone, kNone,
           kNonContainer, kNoPropRange,
                &gParamParents,0,
  },
  {
                                 eHTMLTag_plaintext,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kExtensions, kCDATA, kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_pre,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock|kPreformatted, (kSelf|kFlowEntity), kNone,  
           kRequiresBody, kDefaultPropRange,
                0,&gPreKids,
  },
  {
                                 eHTMLTag_progress,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFormControl, kFlowEntity, kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_q,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_s,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_samp,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_script,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              (kSpecial|kHeadContent), kCDATA, kNone,   
           kNoStyleLeaksIn|kLegalOpen, kNoPropRange,
                0,&gContainsText,
  },
  {
                                 eHTMLTag_section,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kBlock, (kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_select,
                    eHTMLTag_unknown, eHTMLTag_unknown,
              &gInForm,&gInForm,
     &gInputAutoClose,0,0,0,
              kFormControl, kNone, kFlowEntity|kDLChild|kHeadMisc, 
           kNoPropagate|kNoStyleLeaksIn|kRequiresBody, kDefaultPropRange,
                &gInForm,&gContainsOpts,
  },
  {
                                 eHTMLTag_small,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
#if defined(MOZ_MEDIA)
  {
                                 eHTMLTag_source,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gSourceParents,&gSourceParents,
     &gPAutoClose, 0, 0,0,
              kSpecial, kNone, kNone,
           kNonContainer,kNoPropRange,
                &gSourceParents,0,
  },
#endif
  {
    
          
          

                                 eHTMLTag_span,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kInlineEntity|kSelf|kFlowEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
  {
    
                                 eHTMLTag_strike,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
    
                                 eHTMLTag_strong,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,  
           0, kDefaultPropRange,
                0,&gContainsText,
  },
  {
    
                                 eHTMLTag_style,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kAllTags - kHeadContent, kCDATA, kNone,
           kNoStyleLeaksIn|kPreferHead|kLegalOpen, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_sub,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
    
                                 eHTMLTag_sup,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kSpecial, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_table,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gTableRootTags,&gTableRootTags,
     0,&gTableCloseTags,0,0,
              kBlock, kNone, (kSelf|kInlineEntity),
           (kBadContentWatch|kNoStyleLeaksIn|kRequiresBody), 2,
                0,&gTableKids,
  },
  {
                                 eHTMLTag_tbody,
                    eHTMLTag_table, eHTMLTag_unknown,
              &gInTable,&gInTable,
     &gTBodyAutoClose,0,0,0,
              kNone, kNone, (kSelf|kInlineEntity),
           (kNoPropagate|kBadContentWatch|kNoStyleLeaksIn|kNoStyleLeaksOut), kDefaultPropRange,
                &gInTable,&gTBodyKids,
  },
  {
                                 eHTMLTag_td,
                    eHTMLTag_table, eHTMLTag_unknown,
              &gTDRootTags,&gTDRootTags,
     &gTDCloseTags,&gTDCloseTags,0,&gExcludableParents,
              kNone, kFlowEntity, kSelf,
           kNoStyleLeaksIn|kNoStyleLeaksOut, kDefaultPropRange,
                &gTDRootTags,&gBodyKids,
  },
  {
                                 eHTMLTag_textarea,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInForm,&gInForm,
     &gInputAutoClose,0,0,0,
              kFormControl, kPCDATA, kNone,
           kRequiresBody|kNoStyleLeaksIn,kDefaultPropRange,
                &gInForm,&gContainsText,
  },
  {
                                 eHTMLTag_tfoot,
                    eHTMLTag_table, eHTMLTag_unknown,
              &gInTable,&gInTable,
     &gTBodyAutoClose,0,0,0,
              kNone, kNone, kSelf,
           (kNoPropagate|kBadContentWatch|kNoStyleLeaksIn|kNoStyleLeaksOut), kNoPropRange,
                &gInTable,&gTableElemKids,
  },
  {
                                 eHTMLTag_th, 
                    eHTMLTag_table, eHTMLTag_unknown,
              &gTDRootTags,&gTDRootTags,
     &gTDCloseTags,&gTDCloseTags,0,0,
              kNone, kFlowEntity, kSelf,
           (kNoStyleLeaksIn|kNoStyleLeaksOut), kDefaultPropRange,
                &gTDRootTags,&gBodyKids,
  },
  {
                                 eHTMLTag_thead,
              eHTMLTag_table,eHTMLTag_unknown,  
              &gInTable,&gInTable,  
     &gTBodyAutoClose,0,0,0,
              kNone, kNone, kSelf,
           (kNoPropagate|kBadContentWatch|kNoStyleLeaksIn|kNoStyleLeaksOut), kNoPropRange,
                &gInTable,&gTableElemKids,
  },
  {
                                 eHTMLTag_time,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_title,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInHead,&gInHead,
     0,0,0,0,
              kHeadContent,kPCDATA, kNone,
           kNoStyleLeaksIn, kNoPropRange,
                &gInHead,&gContainsText,
  },
  {
                                 eHTMLTag_tr,
                    eHTMLTag_table, eHTMLTag_unknown,
              &gTRParents,&gTREndParents,
     &gTRCloseTags,0,0,0,
              kNone, kNone, kInlineEntity,
           (kBadContentWatch|kNoStyleLeaksIn|kNoStyleLeaksOut), kNoPropRange,
                &gTRParents,&gTRKids,
  },
  {
                                 eHTMLTag_tt,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_u,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFontStyle, (kSelf|kInlineEntity), kNone,
           0, kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_ul,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gOLRootTags,&gOLRootTags,
     &gULAutoClose,&gULCloseTags,0,0,
              kList, (kFlowEntity|kSelf), kNone,
           0,kDefaultPropRange,
                0,&gULKids,
  },
  {
                                 eHTMLTag_var,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kPhrase, (kSelf|kInlineEntity), kNone,
           0,kDefaultPropRange,
                0,0,
  },
#if defined(MOZ_MEDIA)
  {
                                 eHTMLTag_video,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0, 0, 0,0,
              kSpecial, (kFlowEntity|kSelf), kNone,
           0,kDefaultPropRange,
                0,&gVideoKids,
  },
#endif
  {
                                 eHTMLTag_wbr,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kExtensions, kNone, kNone,
           kNonContainer|kRequiresBody,kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_xmp,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kInlineEntity|kPreformatted, kCDATA, kNone,
           kNone,kDefaultPropRange,
                0,0,
  },
  {
                                 eHTMLTag_text,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInBody,&gInBody,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           kNonContainer|kRequiresBody,kNoPropRange,
                0,0,
  },
  {
          
          
          

                                 eHTMLTag_whitespace, 
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInBody,&gInBody,
     0,0,0,0,
              kFlowEntity|kHeadMisc, kNone, kNone,
           kNonContainer|kLegalOpen,kNoPropRange,
                0,0,
  },
  {
          
          
          

                                 eHTMLTag_newline,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInBody,&gInBody,
     0,0,0,0,
              kFlowEntity|kHeadMisc, kNone, kNone,
           kNonContainer|kLegalOpen, kNoPropRange,
                0,0,
  },
  {
          
          
          

                                 eHTMLTag_comment,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity|kHeadMisc, kNone, kNone,
           kOmitEndTag|kLegalOpen,kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_entity,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gInBody,&gInBody,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           0, kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_doctypeDecl,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           kOmitEndTag,kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_markupDecl,
              eHTMLTag_unknown,eHTMLTag_unknown,
              &gRootTags,&gRootTags,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           kOmitEndTag,kNoPropRange,
                0,0,
  },
  {
                                 eHTMLTag_instruction,
              eHTMLTag_unknown,eHTMLTag_unknown,
              0,0,
     0,0,0,0,
              kFlowEntity, kNone, kNone,
           kOmitEndTag,kNoPropRange,
                0,0,
  },
  {
          
          
          

                                 eHTMLTag_userdefined,
              eHTMLTag_unknown,eHTMLTag_frameset,
              &gRootTags,&gRootTags,
     &gBodyAutoClose,0,0,0,
              (kFlowEntity|kHeadMisc), (kInlineEntity|kSelf), kNone,  
           kPreferBody, kBodyPropRange,
                &gInNoframes,&gBodyKids,
  }
};

#ifdef DEBUG  
void CheckElementTable() {
  for (eHTMLTags t = eHTMLTag_unknown; t <= eHTMLTag_userdefined; t = eHTMLTags(t + 1)) {
    NS_ASSERTION(gHTMLElements[t].mTagID == t, "gHTMLElements entries does match tag list.");
  }
}
#endif










int32_t nsHTMLElement::GetIndexOfChildOrSynonym(nsDTDContext& aContext,eHTMLTags aChildTag) {
  int32_t theChildIndex=aContext.LastOf(aChildTag);
  if(kNotFound==theChildIndex) {
    const TagList* theSynTags=gHTMLElements[aChildTag].GetSynonymousTags(); 
    if(theSynTags) {
      theChildIndex=LastOf(aContext,*theSynTags);
    } 
  }
  return theChildIndex;
}







bool nsHTMLElement::HasSpecialProperty(int32_t aProperty) const{
  bool result=TestBits(mSpecialProperties,aProperty);
  return result;
}






 
bool nsHTMLElement::IsContainer(eHTMLTags aChild) {
  bool result=(eHTMLTag_unknown==aChild);

  if(!result){
    result=!TestBits(gHTMLElements[aChild].mSpecialProperties,kNonContainer);
  }
  return result;
}










bool nsHTMLElement::IsMemberOf(int32_t aSet) const{
  return TestBits(aSet,mParentBits);
}










bool nsHTMLElement::ContainsSet(int32_t aSet) const{
  return TestBits(mParentBits,aSet);
}








bool nsHTMLElement::IsBlockCloser(eHTMLTags aTag){
  bool result=false;
    
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){

    result=(gHTMLElements[aTag].IsBlock() || 
            gHTMLElements[aTag].IsBlockEntity() ||
            (kHeading==gHTMLElements[aTag].mParentBits));
    if(!result) {
      
      
      
      
      

      static eHTMLTags gClosers[]={ eHTMLTag_table,eHTMLTag_tbody,
                                    eHTMLTag_td,eHTMLTag_th,
                                    eHTMLTag_tr,eHTMLTag_caption,
                                    eHTMLTag_object,eHTMLTag_applet,
                                    eHTMLTag_ol, eHTMLTag_ul,
                                    eHTMLTag_optgroup,
                                    eHTMLTag_nobr,eHTMLTag_dir};

      result=FindTagInSet(aTag,gClosers,sizeof(gClosers)/sizeof(eHTMLTag_body));
    }
  }
  return result;
}








bool nsHTMLElement::IsInlineEntity(eHTMLTags aTag){
  bool result=false;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){
    result=TestBits(gHTMLElements[aTag].mParentBits,kInlineEntity);
  } 
  return result;
}







bool nsHTMLElement::IsFlowEntity(eHTMLTags aTag){
  bool result=false;

  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){
    result=TestBits(gHTMLElements[aTag].mParentBits,kFlowEntity);
  } 
  return result;
}







bool nsHTMLElement::IsBlockParent(eHTMLTags aTag){
  bool result=false;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kBlockEntity);
  } 
  return result;
}







bool nsHTMLElement::IsInlineParent(eHTMLTags aTag){
  bool result=false;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kInlineEntity);
  } 
  return result;
}








bool nsHTMLElement::IsFlowParent(eHTMLTags aTag){
  bool result=false;
  if((aTag>=eHTMLTag_unknown) & (aTag<=eHTMLTag_xmp)){
    result=TestBits(gHTMLElements[aTag].mInclusionBits,kFlowEntity);
  } 
  return result;
}







bool nsHTMLElement::IsSpecialParent(eHTMLTags aTag) const{
  bool result=false;
  if(mSpecialParents) {
    if(FindTagInSet(aTag,mSpecialParents->mTags,mSpecialParents->mCount))
        result=true;
  }
  return result;
}







bool nsHTMLElement::IsSectionTag(eHTMLTags aTag){
  bool result=false;
  switch(aTag){
    case eHTMLTag_html:
    case eHTMLTag_frameset:
    case eHTMLTag_body:
    case eHTMLTag_head:
      result=true;
      break;
    default:
      result=false;
  }
  return result;
}








bool nsHTMLElement::CanContain(eHTMLTags aParent,eHTMLTags aChild,nsDTDMode aMode){
  bool result=false;
  if((aParent>=eHTMLTag_unknown) && (aParent<=eHTMLTag_userdefined)){
    result=gHTMLElements[aParent].CanContain(aChild,aMode);
  } 
  return result;
}







bool nsHTMLElement::CanExclude(eHTMLTags aChild) const{
  bool result=false;

  if(gHTMLElements[aChild].HasSpecialProperty(kLegalOpen)) {
    
    return false;
  }

  
  if(mSpecialKids) {
    if(FindTagInSet(aChild,mSpecialKids->mTags,mSpecialKids->mCount)) {
      return false;
    }
  }

  if(mExclusionBits){
    if(gHTMLElements[aChild].IsMemberOf(mExclusionBits)) {
      result=true;
    }
  }
  return result;
}







bool nsHTMLElement::IsExcludableParent(eHTMLTags aParent) const{
  bool result=false;

  if(!IsTextTag(mTagID)) {
    if(mExcludableParents) {
      const TagList* theParents=mExcludableParents;
      if(FindTagInSet(aParent,theParents->mTags,theParents->mCount))
        result=true;
    }
    if(!result) {
      
      
      
      
      if(nsHTMLElement::IsBlockParent(aParent)){
        switch(mTagID) {
          case eHTMLTag_caption:
          case eHTMLTag_thead:
          case eHTMLTag_tbody:
          case eHTMLTag_tfoot:
          case eHTMLTag_td:
          case eHTMLTag_th:
          case eHTMLTag_tr:
            result=true;
          default:
            break;
        }
      }
    }
  }
  return result;
}







bool nsHTMLElement::CanOmitEndTag(void) const{
  bool result=!IsContainer(mTagID);
  if(!result)
    result=TestBits(mSpecialProperties,kOmitEndTag);
  return result;
}











bool nsHTMLElement::IsChildOfHead(eHTMLTags aChild,bool& aExclusively) {
  aExclusively = true;

  
  if (gHTMLElements[aChild].mParentBits & kHeadContent) {
    return true;
  }


  
  if (gHTMLElements[aChild].mParentBits & kHeadMisc) {
    aExclusively = false;
    return true;
  }

  return false;
}









bool nsHTMLElement::SectionContains(eHTMLTags aChild,bool allowDepthSearch) const {
  bool result=false;
  const TagList* theRootTags=gHTMLElements[aChild].GetRootTags();

  if(theRootTags){
    if(!FindTagInSet(mTagID,theRootTags->mTags,theRootTags->mCount)){
      eHTMLTags theRootBase=theRootTags->mTags[0];
      if((eHTMLTag_unknown!=theRootBase) && (allowDepthSearch))
        result=SectionContains(theRootBase,allowDepthSearch);
    }
    else result=true;
  }
  return result;
}










bool nsHTMLElement::ShouldVerifyHierarchy() const {
  bool result=false;
  
  
  
  
  
  
  if(mTagID!=eHTMLTag_userdefined) {
    result=HasSpecialProperty(kVerifyHierarchy);
  }
  return result;
}
  






bool nsHTMLElement::IsResidualStyleTag(eHTMLTags aChild) {
  bool result=false;
  switch(aChild) {
    case eHTMLTag_a:       
    case eHTMLTag_b:
    case eHTMLTag_bdo:     
    case eHTMLTag_big:       
    case eHTMLTag_blink:
    case eHTMLTag_del:
    case eHTMLTag_em:
    case eHTMLTag_font:    
    case eHTMLTag_i:         
    case eHTMLTag_ins:
    case eHTMLTag_q:
    case eHTMLTag_s:       
    case eHTMLTag_small:
    case eHTMLTag_strong:
    case eHTMLTag_strike:    
    case eHTMLTag_sub:     
    case eHTMLTag_sup:       
    case eHTMLTag_tt:
    case eHTMLTag_u:       
      result=true;
      break;

    case eHTMLTag_abbr:
    case eHTMLTag_acronym:   
    case eHTMLTag_center:  
    case eHTMLTag_cite:      
    case eHTMLTag_code:
    case eHTMLTag_dfn:       
    case eHTMLTag_kbd:     
    case eHTMLTag_samp:      
    case eHTMLTag_span:    
    case eHTMLTag_var:
      result=false;
    default:
      break;
  };
  return result;
}







bool nsHTMLElement::CanContainType(int32_t aType) const{
  int32_t answer=mInclusionBits & aType;
  bool    result=bool(0!=answer);
  return result;
}







bool nsHTMLElement::IsWhitespaceTag(eHTMLTags aChild) {
  bool result=false;

  switch(aChild) {
    case eHTMLTag_newline:
    case eHTMLTag_whitespace:
      result=true;
      break;
    default:
      break;
  }
  return result;
}







bool nsHTMLElement::IsTextTag(eHTMLTags aChild) {
  bool result=false;

  switch(aChild) {
    case eHTMLTag_text:
    case eHTMLTag_entity:
    case eHTMLTag_newline:
    case eHTMLTag_whitespace:
      result=true;
      break;
    default:
      break;
  }
  return result;
}







bool nsHTMLElement::CanContainSelf(void) const {
  bool result=bool(TestBits(mInclusionBits,kSelf)!=0);
  return result;
}












bool nsHTMLElement::CanAutoCloseTag(nsDTDContext& aContext,int32_t aIndex,
                                      eHTMLTags aChildTag) const{

  int32_t thePos;
  bool    result = true;
  eHTMLTags thePrevTag;

  for(thePos = aContext.GetCount() - 1; thePos >= aIndex; thePos--) {
    thePrevTag = aContext.TagAt(thePos);

    if (thePrevTag == eHTMLTag_applet ||
        thePrevTag == eHTMLTag_td) {
          result = false;
          break;
    }
  }
  
  return result;
}







eHTMLTags nsHTMLElement::GetCloseTargetForEndTag(nsDTDContext& aContext,int32_t anIndex,nsDTDMode aMode) const{
  eHTMLTags result=eHTMLTag_unknown;

  int theCount=aContext.GetCount();
  int theIndex=theCount;

  if(IsMemberOf(kPhrase)){

    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag = aContext.TagAt(theIndex);
      if(theTag != mTagID) {
        
        if(eHTMLTag_userdefined == theTag) {
          continue; 
        }

        
        if(CanContainType(kBlock)) { 
          if(gHTMLElements[eHTMLTags(theTag)].IsMemberOf(kBlockEntity) || 
             gHTMLElements[eHTMLTags(theTag)].IsMemberOf(kFlowEntity)) {
            if(HasOptionalEndTag(theTag)) {
              continue; 
            }
          }
        }

        
        
        if(!gHTMLElements[theTag].IsMemberOf(kSpecial | 
                                             kFontStyle |
                                             kPhrase |
                                             kExtensions)) {  
          break; 
        }
      }
      else {
        result=theTag; 
        break;
      }
    }
  }
  
  else if(IsMemberOf(kSpecial)){

    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aContext.TagAt(theIndex);
      if(theTag!=mTagID) {
        
        

        
        if((eHTMLTag_userdefined==theTag) ||
            gHTMLElements[theTag].IsSpecialEntity()  || 
            gHTMLElements[theTag].IsFontStyleEntity()||
            gHTMLElements[theTag].IsPhraseEntity()   ||
            gHTMLElements[theTag].IsMemberOf(kExtensions)) {
          continue;
        }
        else {

          
          if(CanContainType(kBlock)) {
            if(gHTMLElements[eHTMLTags(theTag)].IsMemberOf(kBlockEntity) || 
               gHTMLElements[eHTMLTags(theTag)].IsMemberOf(kFlowEntity)) {
              if(HasOptionalEndTag(theTag)) {
                continue; 
              }
            }
          }
          break; 
        }
      }
      else {
        result=theTag; 
        break;
      }
    }
  }

  else if(ContainsSet(kPreformatted) ||  
          IsMemberOf(kFormControl|kExtensions|kPreformatted)){  

    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aContext.TagAt(theIndex);
      if(theTag!=mTagID) {
        if(!CanContain(theTag,aMode)) {
          break; 
        }
      }
      else {
        result=theTag; 
        break; 
      }
    }
  }

  else if(IsMemberOf(kList)){

    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag=aContext.TagAt(theIndex);
      if(theTag!=mTagID) {
        if(!CanContain(theTag,aMode)) {
          break; 
        }
      }
      else {
        result=theTag; 
        break; 
      }
    }
  }

  else if(IsResidualStyleTag(mTagID)){
    
    
    
    
    

    const TagList* theRootTags=gHTMLElements[mTagID].GetEndRootTags();
    int32_t theIndexCopy=theIndex;
    while(--theIndex>=anIndex){
      eHTMLTags theTag=aContext.TagAt(theIndex);
      if(theTag == mTagID) {
        return theTag; 
      }
      else if (!CanContain(theTag,aMode) || 
               (theRootTags && FindTagInSet(theTag,theRootTags->mTags,theRootTags->mCount))) {
        
        
        
        
        
        
        
        return eHTMLTag_unknown;
      }
    }

    theIndex=theIndexCopy;
    while(--theIndex>=anIndex){
      eHTMLTags theTag=aContext.TagAt(theIndex);
      if(gHTMLElements[theTag].IsMemberOf(mParentBits)) {
        return theTag;
      }
    }    
  }

  else if(gHTMLElements[mTagID].IsTableElement()) {
    
      
      

    int32_t theLastTable=aContext.LastOf(eHTMLTag_table);
    int32_t theLastOfMe=aContext.LastOf(mTagID);
    if(theLastTable<theLastOfMe) {
      return mTagID;
    }
        
  }

  else if(mTagID == eHTMLTag_legend)  {
    while((--theIndex>=anIndex) && (eHTMLTag_unknown==result)){
      eHTMLTags theTag = aContext.TagAt(theIndex);
      if (theTag == mTagID) {
        result = theTag;
        break;
      }

      if (!CanContain(theTag, aMode)) {
        break;
      }
    }
  }

  else if (mTagID == eHTMLTag_head) {
    while (--theIndex >= anIndex) {
      eHTMLTags tag = aContext.TagAt(theIndex);
      if (tag == eHTMLTag_html) {
        
        
        break;
      }

      if (tag == eHTMLTag_head) {
        result = eHTMLTag_head;
        break;
      }
    }
  }

  return result;
}








bool nsHTMLElement::CanContain(eHTMLTags aChild,nsDTDMode aMode) const{


  if(IsContainer(mTagID)){

    if(gHTMLElements[aChild].HasSpecialProperty(kLegalOpen)) {
      
      return true;
    }

    if(mTagID==aChild) {
      return CanContainSelf();  
    }

    const TagList* theCloseTags=gHTMLElements[aChild].GetAutoCloseStartTags();
    if(theCloseTags){
      if(FindTagInSet(mTagID,theCloseTags->mTags,theCloseTags->mCount))
        return false;
    }

    if(gHTMLElements[aChild].mExcludableParents) {
      const TagList* theParents=gHTMLElements[aChild].mExcludableParents;
      if(FindTagInSet(mTagID,theParents->mTags,theParents->mCount))
        return false;
    }
    
    if(gHTMLElements[aChild].IsExcludableParent(mTagID))
      return false;

    if(gHTMLElements[aChild].IsBlockCloser(aChild)){
      if(nsHTMLElement::IsBlockParent(mTagID)){
        return true;
      }
    }

    if(nsHTMLElement::IsInlineEntity(aChild)){
      if(nsHTMLElement::IsInlineParent(mTagID)){
        return true;
      }
    }

    if(nsHTMLElement::IsFlowEntity(aChild)) {
      if(nsHTMLElement::IsFlowParent(mTagID)){
        return true;
      }
    }

    if(nsHTMLElement::IsTextTag(aChild)) {
      
      if(nsHTMLElement::IsInlineParent(mTagID) || CanContainType(kCDATA)){
        return true;
      }
    }

    if(CanContainType(gHTMLElements[aChild].mParentBits)) {
      return true;
    }
 
    if(mSpecialKids) {
      if(FindTagInSet(aChild,mSpecialKids->mTags,mSpecialKids->mCount)) {
        return true;
      }
    }

    
    if (aChild == eHTMLTag_table && mTagID == eHTMLTag_p && aMode == eDTDMode_quirks) {
      return true;
    }
  }
  
  return false;
}

#ifdef DEBUG
void nsHTMLElement::DebugDumpContainment(const char* aFilename,const char* aTitle){
}

void nsHTMLElement::DebugDumpMembership(const char* aFilename){
}

void nsHTMLElement::DebugDumpContainType(const char* aFilename){
}
#endif
