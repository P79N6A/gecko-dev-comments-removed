










































#ifndef nsDOMClassInfoID_h__
#define nsDOMClassInfoID_h__

enum nsDOMClassInfoID {
  
  eDOMClassInfo_Window_id,
  eDOMClassInfo_Location_id,
  eDOMClassInfo_Navigator_id,
  eDOMClassInfo_Plugin_id,
  eDOMClassInfo_PluginArray_id,
  eDOMClassInfo_MimeType_id,
  eDOMClassInfo_MimeTypeArray_id,
  eDOMClassInfo_BarProp_id,
  eDOMClassInfo_History_id,
  eDOMClassInfo_Screen_id,
  eDOMClassInfo_Prototype_id,
  eDOMClassInfo_Constructor_id,

  
  eDOMClassInfo_XMLDocument_id,
  eDOMClassInfo_DocumentType_id,
  eDOMClassInfo_DOMImplementation_id,
  eDOMClassInfo_DOMException_id,
  eDOMClassInfo_DocumentFragment_id,
  eDOMClassInfo_Element_id,
  eDOMClassInfo_Attr_id,
  eDOMClassInfo_Text_id,
  eDOMClassInfo_Comment_id,
  eDOMClassInfo_CDATASection_id,
  eDOMClassInfo_ProcessingInstruction_id,
  eDOMClassInfo_Notation_id,
  eDOMClassInfo_NodeList_id,
  eDOMClassInfo_NamedNodeMap_id,

  
  eDOMClassInfo_DocumentStyleSheetList_id,

  
  eDOMClassInfo_Event_id,
  eDOMClassInfo_MutationEvent_id,
  eDOMClassInfo_UIEvent_id,
  eDOMClassInfo_MouseEvent_id,
  eDOMClassInfo_KeyboardEvent_id,
  eDOMClassInfo_PopupBlockedEvent_id,

  
  eDOMClassInfo_HTMLDocument_id,
  eDOMClassInfo_HTMLOptionsCollection_id,
  eDOMClassInfo_HTMLFormControlCollection_id,
  eDOMClassInfo_HTMLGenericCollection_id,

  
  eDOMClassInfo_HTMLAnchorElement_id,
  eDOMClassInfo_HTMLAppletElement_id,
  eDOMClassInfo_HTMLAreaElement_id,
  eDOMClassInfo_HTMLBRElement_id,
  eDOMClassInfo_HTMLBaseElement_id,
  eDOMClassInfo_HTMLBaseFontElement_id,
  eDOMClassInfo_HTMLBodyElement_id,
  eDOMClassInfo_HTMLButtonElement_id,
  eDOMClassInfo_HTMLDListElement_id,
  eDOMClassInfo_HTMLDelElement_id,
  eDOMClassInfo_HTMLDirectoryElement_id,
  eDOMClassInfo_HTMLDivElement_id,
  eDOMClassInfo_HTMLEmbedElement_id,
  eDOMClassInfo_HTMLFieldSetElement_id,
  eDOMClassInfo_HTMLFontElement_id,
  eDOMClassInfo_HTMLFormElement_id,
  eDOMClassInfo_HTMLFrameElement_id,
  eDOMClassInfo_HTMLFrameSetElement_id,
  eDOMClassInfo_HTMLHRElement_id,
  eDOMClassInfo_HTMLHeadElement_id,
  eDOMClassInfo_HTMLHeadingElement_id,
  eDOMClassInfo_HTMLHtmlElement_id,
  eDOMClassInfo_HTMLIFrameElement_id,
  eDOMClassInfo_HTMLImageElement_id,
  eDOMClassInfo_HTMLInputElement_id,
  eDOMClassInfo_HTMLInsElement_id,
  eDOMClassInfo_HTMLIsIndexElement_id,
  eDOMClassInfo_HTMLLIElement_id,
  eDOMClassInfo_HTMLLabelElement_id,
  eDOMClassInfo_HTMLLegendElement_id,
  eDOMClassInfo_HTMLLinkElement_id,
  eDOMClassInfo_HTMLMapElement_id,
  eDOMClassInfo_HTMLMenuElement_id,
  eDOMClassInfo_HTMLMetaElement_id,
  eDOMClassInfo_HTMLOListElement_id,
  eDOMClassInfo_HTMLObjectElement_id,
  eDOMClassInfo_HTMLOptGroupElement_id,
  eDOMClassInfo_HTMLOptionElement_id,
  eDOMClassInfo_HTMLParagraphElement_id,
  eDOMClassInfo_HTMLParamElement_id,
  eDOMClassInfo_HTMLPreElement_id,
  eDOMClassInfo_HTMLQuoteElement_id,
  eDOMClassInfo_HTMLScriptElement_id,
  eDOMClassInfo_HTMLSelectElement_id,
  eDOMClassInfo_HTMLSpacerElement_id,
  eDOMClassInfo_HTMLSpanElement_id,
  eDOMClassInfo_HTMLStyleElement_id,
  eDOMClassInfo_HTMLTableCaptionElement_id,
  eDOMClassInfo_HTMLTableCellElement_id,
  eDOMClassInfo_HTMLTableColElement_id,
  eDOMClassInfo_HTMLTableElement_id,
  eDOMClassInfo_HTMLTableRowElement_id,
  eDOMClassInfo_HTMLTableSectionElement_id,
  eDOMClassInfo_HTMLTextAreaElement_id,
  eDOMClassInfo_HTMLTitleElement_id,
  eDOMClassInfo_HTMLUListElement_id,
  eDOMClassInfo_HTMLUnknownElement_id,
  eDOMClassInfo_HTMLWBRElement_id,

  
  eDOMClassInfo_CSSStyleRule_id,
  eDOMClassInfo_CSSCharsetRule_id,
  eDOMClassInfo_CSSImportRule_id,
  eDOMClassInfo_CSSMediaRule_id,
  eDOMClassInfo_CSSNameSpaceRule_id,
  eDOMClassInfo_CSSRuleList_id,
  eDOMClassInfo_CSSGroupRuleRuleList_id,
  eDOMClassInfo_MediaList_id,
  eDOMClassInfo_StyleSheetList_id,
  eDOMClassInfo_CSSStyleSheet_id,
  eDOMClassInfo_CSSStyleDeclaration_id,
  eDOMClassInfo_ComputedCSSStyleDeclaration_id,
  eDOMClassInfo_ROCSSPrimitiveValue_id,

  
  eDOMClassInfo_Range_id,
  eDOMClassInfo_Selection_id,

  
#ifdef MOZ_XUL
  eDOMClassInfo_XULDocument_id,
  eDOMClassInfo_XULElement_id,
  eDOMClassInfo_XULCommandDispatcher_id,
#endif
  eDOMClassInfo_XULControllers_id,
#ifdef MOZ_XUL
  eDOMClassInfo_BoxObject_id,
  eDOMClassInfo_TreeSelection_id,
  eDOMClassInfo_TreeContentView_id,
#endif

  
  eDOMClassInfo_Crypto_id,
  eDOMClassInfo_CRMFObject_id,
  eDOMClassInfo_Pkcs11_id,
  
  
  eDOMClassInfo_TreeWalker_id,

  
  eDOMClassInfo_CSSRect_id,

  
  eDOMClassInfo_ChromeWindow_id,

  
  eDOMClassInfo_CSSRGBColor_id,

  eDOMClassInfo_RangeException_id,

  
  
  eDOMClassInfo_CSSValueList_id,

  
  eDOMClassInfo_ContentList_id,
  
  
  eDOMClassInfo_XMLStylesheetProcessingInstruction_id,
  
  eDOMClassInfo_ImageDocument_id,

#ifdef MOZ_XUL
  eDOMClassInfo_XULTemplateBuilder_id,
  eDOMClassInfo_XULTreeBuilder_id,
#endif

  
  eDOMClassInfo_DOMStringList_id,

  
  eDOMClassInfo_NameList_id,

#ifdef MOZ_XUL
  eDOMClassInfo_TreeColumn_id,
  eDOMClassInfo_TreeColumns_id,
#endif

  eDOMClassInfo_CSSMozDocumentRule_id,

  eDOMClassInfo_BeforeUnloadEvent_id,

#ifdef MOZ_SVG
  
  eDOMClassInfo_SVGDocument_id,

  
  eDOMClassInfo_SVGAElement_id,
  eDOMClassInfo_SVGCircleElement_id,
  eDOMClassInfo_SVGClipPathElement_id,
  eDOMClassInfo_SVGDefsElement_id,
  eDOMClassInfo_SVGDescElement_id,
  eDOMClassInfo_SVGEllipseElement_id,
  eDOMClassInfo_SVGFEBlendElement_id,
  eDOMClassInfo_SVGFEDiffuseLightingElement_id,
  eDOMClassInfo_SVGFEDistantLightElement_id,
  eDOMClassInfo_SVGFEColorMatrixElement_id,
  eDOMClassInfo_SVGFEComponentTransferElement_id,
  eDOMClassInfo_SVGFECompositeElement_id,
  eDOMClassInfo_SVGFEConvolveMatrixElement_id,
  eDOMClassInfo_SVGFEFloodElement_id,
  eDOMClassInfo_SVGFEFuncAElement_id,
  eDOMClassInfo_SVGFEFuncBElement_id,
  eDOMClassInfo_SVGFEFuncGElement_id,
  eDOMClassInfo_SVGFEFuncRElement_id,
  eDOMClassInfo_SVGFEGaussianBlurElement_id,
  eDOMClassInfo_SVGFEMergeElement_id,
  eDOMClassInfo_SVGFEMergeNodeElement_id,
  eDOMClassInfo_SVGFEMorphologyElement_id,
  eDOMClassInfo_SVGFEOffsetElement_id,
  eDOMClassInfo_SVGFEPointLightElement_id,
  eDOMClassInfo_SVGFESpecularLightingElement_id,
  eDOMClassInfo_SVGFESpotLightElement_id,
  eDOMClassInfo_SVGFETurbulenceElement_id,
  eDOMClassInfo_SVGFEUnimplementedMOZElement_id,
  eDOMClassInfo_SVGFilterElement_id,
  eDOMClassInfo_SVGGElement_id,
  eDOMClassInfo_SVGImageElement_id,
  eDOMClassInfo_SVGLinearGradientElement_id,
  eDOMClassInfo_SVGLineElement_id,
  eDOMClassInfo_SVGMarkerElement_id,
  eDOMClassInfo_SVGMaskElement_id,
  eDOMClassInfo_SVGMetadataElement_id,
  eDOMClassInfo_SVGPathElement_id,
  eDOMClassInfo_SVGPatternElement_id,
  eDOMClassInfo_SVGPolygonElement_id,
  eDOMClassInfo_SVGPolylineElement_id,
  eDOMClassInfo_SVGRadialGradientElement_id,
  eDOMClassInfo_SVGRectElement_id,
  eDOMClassInfo_SVGScriptElement_id,
  eDOMClassInfo_SVGStopElement_id,
  eDOMClassInfo_SVGStyleElement_id,
  eDOMClassInfo_SVGSVGElement_id,
  eDOMClassInfo_SVGSwitchElement_id,
  eDOMClassInfo_SVGSymbolElement_id,
  eDOMClassInfo_SVGTextElement_id,
  eDOMClassInfo_SVGTextPathElement_id,
  eDOMClassInfo_SVGTitleElement_id,
  eDOMClassInfo_SVGTSpanElement_id,
  eDOMClassInfo_SVGUseElement_id,

  
  eDOMClassInfo_SVGAngle_id,
  eDOMClassInfo_SVGAnimatedAngle_id,
  eDOMClassInfo_SVGAnimatedBoolean_id,
  eDOMClassInfo_SVGAnimatedEnumeration_id,
  eDOMClassInfo_SVGAnimatedInteger_id,
  eDOMClassInfo_SVGAnimatedLength_id,
  eDOMClassInfo_SVGAnimatedLengthList_id,
  eDOMClassInfo_SVGAnimatedNumber_id,
  eDOMClassInfo_SVGAnimatedNumberList_id,
  eDOMClassInfo_SVGAnimatedPreserveAspectRatio_id,
  eDOMClassInfo_SVGAnimatedRect_id,
  eDOMClassInfo_SVGAnimatedString_id,
  eDOMClassInfo_SVGAnimatedTransformList_id,
  eDOMClassInfo_SVGEvent_id,
  eDOMClassInfo_SVGException_id,
  eDOMClassInfo_SVGLength_id,
  eDOMClassInfo_SVGLengthList_id,
  eDOMClassInfo_SVGMatrix_id,
  eDOMClassInfo_SVGNumber_id,
  eDOMClassInfo_SVGNumberList_id,
  eDOMClassInfo_SVGPathSegArcAbs_id,
  eDOMClassInfo_SVGPathSegArcRel_id,
  eDOMClassInfo_SVGPathSegClosePath_id,
  eDOMClassInfo_SVGPathSegCurvetoCubicAbs_id,
  eDOMClassInfo_SVGPathSegCurvetoCubicRel_id,
  eDOMClassInfo_SVGPathSegCurvetoCubicSmoothAbs_id,
  eDOMClassInfo_SVGPathSegCurvetoCubicSmoothRel_id,
  eDOMClassInfo_SVGPathSegCurvetoQuadraticAbs_id,
  eDOMClassInfo_SVGPathSegCurvetoQuadraticRel_id,
  eDOMClassInfo_SVGPathSegCurvetoQuadraticSmoothAbs_id,
  eDOMClassInfo_SVGPathSegCurvetoQuadraticSmoothRel_id,
  eDOMClassInfo_SVGPathSegLinetoAbs_id,
  eDOMClassInfo_SVGPathSegLinetoHorizontalAbs_id,
  eDOMClassInfo_SVGPathSegLinetoHorizontalRel_id,
  eDOMClassInfo_SVGPathSegLinetoRel_id,
  eDOMClassInfo_SVGPathSegLinetoVerticalAbs_id,
  eDOMClassInfo_SVGPathSegLinetoVerticalRel_id,
  eDOMClassInfo_SVGPathSegList_id,
  eDOMClassInfo_SVGPathSegMovetoAbs_id,
  eDOMClassInfo_SVGPathSegMovetoRel_id,
  eDOMClassInfo_SVGPoint_id,
  eDOMClassInfo_SVGPointList_id,
  eDOMClassInfo_SVGPreserveAspectRatio_id,
  eDOMClassInfo_SVGRect_id,
  eDOMClassInfo_SVGTransform_id,
  eDOMClassInfo_SVGTransformList_id,
  eDOMClassInfo_SVGZoomEvent_id,
#endif 

  
  eDOMClassInfo_HTMLCanvasElement_id,
#ifdef MOZ_ENABLE_CANVAS
  eDOMClassInfo_CanvasRenderingContext2D_id,
  eDOMClassInfo_CanvasGradient_id,
  eDOMClassInfo_CanvasPattern_id,
#endif
  
  
  eDOMClassInfo_SmartCardEvent_id,
  
  
  eDOMClassInfo_PageTransitionEvent_id,

  
  eDOMClassInfo_WindowUtils_id,

  
  eDOMClassInfo_XSLTProcessor_id,

  
  eDOMClassInfo_XPathEvaluator_id,
  eDOMClassInfo_XPathException_id,
  eDOMClassInfo_XPathExpression_id,
  eDOMClassInfo_XPathNSResolver_id,
  eDOMClassInfo_XPathResult_id,

  
  eDOMClassInfo_Storage_id,
  eDOMClassInfo_StorageList_id,
  eDOMClassInfo_StorageItem_id,
  eDOMClassInfo_StorageEvent_id,

  eDOMClassInfo_WindowRoot_id,

  
  eDOMClassInfo_DOMParser_id,
  eDOMClassInfo_XMLSerializer_id,

  
  eDOMClassInfo_XMLHttpProgressEvent_id,
  eDOMClassInfo_XMLHttpRequest_id,

  eDOMClassInfo_TextRectangle_id,
  eDOMClassInfo_TextRectangleList_id,

  
  
  
  

#if defined(MOZ_SVG) && defined(MOZ_SVG_FOREIGNOBJECT)
  eDOMClassInfo_SVGForeignObjectElement_id,
#endif

  eDOMClassInfo_XULCommandEvent_id,
  eDOMClassInfo_CommandEvent_id,

  eDOMClassInfo_OfflineResourceList_id,
  eDOMClassInfo_LoadStatusList_id,
  eDOMClassInfo_LoadStatus_id,
  eDOMClassInfo_LoadStatusEvent_id,

  eDOMClassInfo_FileList_id,
  eDOMClassInfo_File_id,
  eDOMClassInfo_FileException_id,

  
  eDOMClassInfo_ModalContentWindow_id,

  
  eDOMClassInfoIDCount
};





class nsIClassInfo;

extern nsIClassInfo*
NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID);

#define NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(_class)                          \
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {                                \
    foundInterface = NS_GetDOMClassInfoInstance(eDOMClassInfo_##_class##_id); \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#endif 
