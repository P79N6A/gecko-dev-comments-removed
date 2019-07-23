






































#include "nscore.h"
#include "nsDOMClassInfo.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIComponentRegistrar.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIXPCSecurityManager.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "xptcall.h"
#include "prprf.h"
#include "nsTArray.h"
#include "nsCSSValue.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"


#include "jsapi.h"
#include "jsprvtd.h"    
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "jsnum.h"


#include "nsGlobalWindow.h"
#include "nsIContent.h"
#include "nsIAttribute.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOM3Document.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventListener.h"
#include "nsContentUtils.h"
#include "nsDOMWindowUtils.h"


#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIScriptExternalNameSet.h"
#include "nsJSUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIScriptObjectOwner.h"
#include "nsIJSNativeInitializer.h"
#include "nsJSEnvironment.h"


#include "nsIDOMPluginArray.h"
#include "nsIDOMPlugin.h"
#include "nsIDOMMimeTypeArray.h"
#include "nsIDOMMimeType.h"
#include "nsIDOMLocation.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMJSWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMHistory.h"
#include "nsIDOMMediaList.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMConstructor.h"
#include "nsClientRect.h"


#include "nsDOMError.h"
#include "nsIDOMDOMException.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
#include "nsIDOM3Attr.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDOMDOMTokenList.h"
#include "nsIDOMNameList.h"
#include "nsIDOMNSElement.h"


#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLFormControlList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIHTMLCollection.h"
#include "nsHTMLDocument.h"


#include "nsIDOMHTMLSelectElement.h"


#include "nsIPluginInstance.h"
#include "nsIObjectFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsIPluginHost.h"


#ifdef XP_WIN
#undef GetClassName
#endif


#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNSHTMLOptionElement.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMNSHTMLOptionCollectn.h"
#include "nsIDOMHTMLOptionsCollection.h"


#include "nsContentList.h"
#include "nsGenericElement.h"


#include "nsIEventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEventTarget.h"


#include "nsIDOMStyleSheet.h"
#include "nsIDOMStyleSheetList.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSRule.h"
#include "nsICSSRule.h"
#include "nsICSSRuleList.h"
#include "nsIDOMRect.h"
#include "nsIDOMRGBColor.h"
#include "nsIDOMNSRGBAColor.h"
#include "nsDOMCSSAttrDeclaration.h"


#include "nsIXBLService.h"
#include "nsXBLBinding.h"
#include "nsBindingManager.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsStyleSet.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"


#include "nsIDOMXPathEvaluator.h"
#include "nsIXSLTProcessor.h"
#include "nsIXSLTProcessorObsolete.h"
#include "nsIXSLTProcessorPrivate.h"

#include "nsIDOMLSProgressEvent.h"
#include "nsIDOMParser.h"
#include "nsIDOMSerializer.h"
#include "nsXMLHttpRequest.h"


#include "nsIDOMNavigator.h"
#include "nsIDOMBarProp.h"
#include "nsIDOMScreen.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMAttr.h"
#include "nsIDOMText.h"
#include "nsIDOM3Text.h"
#include "nsIDOMComment.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMNotation.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDataContainerEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMMouseScrollEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMCommandEvent.h"
#include "nsIDOMPopupBlockedEvent.h"
#include "nsIDOMBeforeUnloadEvent.h"
#include "nsIDOMMutationEvent.h"
#include "nsIDOMSmartCardEvent.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIDOMPageTransitionEvent.h"
#include "nsIDOMMessageEvent.h"
#include "nsIDOMNotifyPaintEvent.h"
#include "nsIDOMNSDocumentStyle.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMDocumentTraversal.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMNSHTMLAnchorElement2.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMNSHTMLAreaElement2.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsIDOMHTMLBaseElement.h"
#include "nsIDOMHTMLBaseFontElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMNSHTMLButtonElement.h"
#include "nsIDOMHTMLCanvasElement.h"
#include "nsIDOMHTMLDListElement.h"
#include "nsIDOMHTMLDirectoryElement.h"
#include "nsIDOMHTMLDivElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLFontElement.h"
#include "nsIDOMNSHTMLFormElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDOMNSHTMLFrameElement.h"
#include "nsIDOMHTMLHRElement.h"
#include "nsIDOMNSHTMLHRElement.h"
#include "nsIDOMHTMLHeadElement.h"
#include "nsIDOMHTMLHeadingElement.h"
#include "nsIDOMHTMLHtmlElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMNSHTMLImageElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMHTMLIsIndexElement.h"
#include "nsIDOMHTMLLIElement.h"
#include "nsIDOMHTMLLabelElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLMenuElement.h"
#include "nsIDOMHTMLMetaElement.h"
#include "nsIDOMHTMLModElement.h"
#include "nsIDOMHTMLOListElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMHTMLParagraphElement.h"
#include "nsIDOMHTMLParamElement.h"
#include "nsIDOMHTMLPreElement.h"
#include "nsIDOMHTMLQuoteElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMNSHTMLSelectElement.h"
#include "nsIDOMHTMLStyleElement.h"
#include "nsIDOMHTMLTableCaptionElem.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLTableColElement.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMHTMLTitleElement.h"
#include "nsIDOMHTMLUListElement.h"
#include "nsIDOMHTMLMediaError.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsIDOMHTMLVideoElement.h"
#include "nsIDOMHTMLAudioElement.h"
#include "nsIDOMProgressEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMNSCSS2Properties.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsDOMCSSValueList.h"
#include "nsIDOMOrientationEvent.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMRangeException.h"
#include "nsIDOMNodeIterator.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMCrypto.h"
#include "nsIDOMCRMFObject.h"
#include "nsIControllers.h"
#include "nsISelection.h"
#include "nsIBoxObject.h"
#ifdef MOZ_XUL
#include "nsITreeSelection.h"
#include "nsITreeContentView.h"
#include "nsITreeView.h"
#include "nsIXULTemplateBuilder.h"
#include "nsTreeColumns.h"
#endif
#include "nsIDOMXPathException.h"
#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathResult.h"

#ifdef MOZ_SVG
#include "nsIDOMGetSVGDocument.h"
#include "nsIDOMSVGAElement.h"
#include "nsIDOMSVGAngle.h"
#include "nsIDOMSVGAnimatedAngle.h"
#include "nsIDOMSVGAnimatedBoolean.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGAnimatedInteger.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsIDOMSVGAnimatedNumberList.h"
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsIDOMSVGAnimatedPoints.h"
#include "nsIDOMSVGAnimPresAspRatio.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGAnimatedString.h"
#ifdef MOZ_SMIL
#include "nsIDOMSVGAnimateElement.h"
#include "nsIDOMSVGAnimateTransformElement.h"
#include "nsIDOMSVGSetElement.h"
#include "nsIDOMSVGAnimationElement.h"
#include "nsIDOMElementTimeControl.h"
#endif 
#include "nsIDOMSVGAnimTransformList.h"
#include "nsIDOMSVGCircleElement.h"
#include "nsIDOMSVGClipPathElement.h"
#include "nsIDOMSVGDefsElement.h"
#include "nsIDOMSVGDescElement.h"
#include "nsIDOMSVGDocument.h"
#include "nsIDOMSVGElement.h"
#include "nsIDOMSVGEllipseElement.h"
#include "nsIDOMSVGEvent.h"
#include "nsIDOMSVGException.h"
#include "nsIDOMSVGFilterElement.h"
#include "nsIDOMSVGFilters.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsIDOMSVGForeignObjectElem.h"
#include "nsIDOMSVGGElement.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsIDOMSVGImageElement.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGLengthList.h"
#include "nsIDOMSVGLineElement.h"
#include "nsIDOMSVGLocatable.h"
#include "nsIDOMSVGMarkerElement.h"
#include "nsIDOMSVGMaskElement.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGMetadataElement.h"
#include "nsIDOMSVGNumber.h"
#include "nsIDOMSVGNumberList.h"
#include "nsIDOMSVGPathElement.h"
#include "nsIDOMSVGPathSeg.h"
#include "nsIDOMSVGPathSegList.h"
#include "nsIDOMSVGPatternElement.h"
#include "nsIDOMSVGPoint.h"
#include "nsIDOMSVGPointList.h"
#include "nsIDOMSVGPolygonElement.h"
#include "nsIDOMSVGPolylineElement.h"
#include "nsIDOMSVGPresAspectRatio.h"
#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGRectElement.h"
#include "nsIDOMSVGScriptElement.h"
#include "nsIDOMSVGStopElement.h"
#include "nsIDOMSVGStylable.h"
#include "nsIDOMSVGStyleElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGSwitchElement.h"
#include "nsIDOMSVGSymbolElement.h"
#include "nsIDOMSVGTextElement.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsIDOMSVGTitleElement.h"
#include "nsIDOMSVGTransform.h"
#include "nsIDOMSVGTransformable.h"
#include "nsIDOMSVGTransformList.h"
#include "nsIDOMSVGTSpanElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGUseElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsIDOMSVGZoomAndPan.h"
#include "nsIDOMSVGZoomEvent.h"
#endif 

#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextWebGL.h"
#include "WebGLArray.h"

#include "nsIImageDocument.h"


#include "nsDOMStorage.h"


#include "nsIDOMDataTransfer.h"


#include "nsIDOMLoadStatus.h"

#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionCoords.h"
#include "nsIDOMGeoPositionError.h"


#include "nsDOMWorker.h"

#include "nsDOMFile.h"
#include "nsIDOMFileException.h"


#include "nsIDOMSimpleGestureEvent.h"

#include "nsIDOMNSMouseEvent.h"

static NS_DEFINE_CID(kDOMSOF_CID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

static const char kDOMStringBundleURL[] =
  "chrome://global/locale/dom/dom.properties";




#define WINDOW_SCRIPTABLE_FLAGS                                               \
 (nsIXPCScriptable::WANT_GETPROPERTY |                                        \
  nsIXPCScriptable::WANT_SETPROPERTY |                                        \
  nsIXPCScriptable::WANT_PRECREATE |                                          \
  nsIXPCScriptable::WANT_ADDPROPERTY |                                        \
  nsIXPCScriptable::WANT_DELPROPERTY |                                        \
  nsIXPCScriptable::WANT_NEWENUMERATE |                                       \
  nsIXPCScriptable::WANT_FINALIZE |                                           \
  nsIXPCScriptable::WANT_EQUALITY |                                           \
  nsIXPCScriptable::WANT_OUTER_OBJECT |                                       \
  nsIXPCScriptable::WANT_INNER_OBJECT |                                       \
  nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)

#define NODE_SCRIPTABLE_FLAGS                                                 \
 ((DOM_DEFAULT_SCRIPTABLE_FLAGS |                                             \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_ADDPROPERTY |                                       \
   nsIXPCScriptable::WANT_SETPROPERTY) &                                      \
  ~nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY)






#define ELEMENT_SCRIPTABLE_FLAGS                                              \
  ((NODE_SCRIPTABLE_FLAGS & ~nsIXPCScriptable::CLASSINFO_INTERFACES_ONLY) |   \
   nsIXPCScriptable::WANT_POSTCREATE |                                        \
   nsIXPCScriptable::WANT_ENUMERATE)

#define EXTERNAL_OBJ_SCRIPTABLE_FLAGS                                         \
  ((ELEMENT_SCRIPTABLE_FLAGS &                                                \
    ~nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY) |                          \
   nsIXPCScriptable::WANT_POSTCREATE |                                        \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_SETPROPERTY |                                       \
   nsIXPCScriptable::WANT_CALL)

#define DOCUMENT_SCRIPTABLE_FLAGS                                             \
  (NODE_SCRIPTABLE_FLAGS |                                                    \
   nsIXPCScriptable::WANT_POSTCREATE |                                        \
   nsIXPCScriptable::WANT_ADDPROPERTY |                                       \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_ENUMERATE)

#define ARRAY_SCRIPTABLE_FLAGS                                                \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_ENUMERATE)

#define EVENTTARGET_SCRIPTABLE_FLAGS                                          \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_ADDPROPERTY)

#define DOMCLASSINFO_STANDARD_FLAGS                                           \
  (nsIClassInfo::MAIN_THREAD_ONLY | nsIClassInfo::DOM_OBJECT)


#ifdef NS_DEBUG
#define NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                \
    eDOMClassInfo_##_class##_id,
#else
#define NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                \
  // nothing
#endif


#define NS_DEFINE_CLASSINFO_DATA_WITH_NAME(_class, _name, _helper,            \
                                           _flags)                            \
  { #_name,                                                                   \
    nsnull,                                                                   \
    { _helper::doCreate },                                                    \
    nsnull,                                                                   \
    nsnull,                                                                   \
    nsnull,                                                                   \
    _flags,                                                                   \
    PR_TRUE,                                                                  \
    NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                    \
  },

#define NS_DEFINE_CLASSINFO_DATA(_class, _helper, _flags)                     \
  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(_class, _class, _helper, _flags)












static nsDOMClassInfoData sClassInfoData[] = {
  

  
  
  
  
  


  NS_DEFINE_CLASSINFO_DATA(Window, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(Location, nsLocationSH,
                           (DOM_DEFAULT_SCRIPTABLE_FLAGS |
                            nsIXPCScriptable::WANT_PRECREATE) &
                           ~nsIXPCScriptable::ALLOW_PROP_MODS_TO_PROTOTYPE)

  NS_DEFINE_CLASSINFO_DATA(Navigator, nsNavigatorSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE)
  NS_DEFINE_CLASSINFO_DATA(Plugin, nsPluginSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(PluginArray, nsPluginArraySH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MimeType, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MimeTypeArray, nsMimeTypeArraySH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(BarProp, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(History, nsHistorySH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Screen, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DOMPrototype, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)
  NS_DEFINE_CLASSINFO_DATA(DOMConstructor, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::WANT_CALL |
                           nsIXPCScriptable::WANT_CONSTRUCT |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)

  
  NS_DEFINE_CLASSINFO_DATA(XMLDocument, nsDocumentSH,
                           DOCUMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DocumentType, nsNodeSH,
                           NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DOMImplementation, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DOMException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DOMTokenList, nsDOMTokenListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DocumentFragment, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Element, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Attr, nsAttributeSH,
                           NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Text, nsNodeSH,
                           NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Comment, nsNodeSH,
                           NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CDATASection, nsNodeSH, NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(ProcessingInstruction, nsNodeSH,
                           NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Notation, nsNodeSH, NODE_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(NodeList, nsNodeListSH, ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(NamedNodeMap, nsNamedNodeMapSH,
                           ARRAY_SCRIPTABLE_FLAGS)

  

  
  NS_DEFINE_CLASSINFO_DATA(Event, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MutationEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(UIEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MouseEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MouseScrollEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DragEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(KeyboardEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(PopupBlockedEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(OrientationEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(HTMLDocument, nsHTMLDocumentSH,
                           DOCUMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionsCollection,
                           nsHTMLOptionsCollectionSH,
                           ARRAY_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_SETPROPERTY)
  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(HTMLFormControlCollection, HTMLCollection,
                                     nsHTMLCollectionSH,
                                     ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(HTMLGenericCollection, HTMLCollection,
                                     nsHTMLCollectionSH,
                                     ARRAY_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(HTMLAnchorElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLAppletElement, nsHTMLPluginObjElementSH,
                           EXTERNAL_OBJ_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLAreaElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLBRElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLBaseFontElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLBodyElement, nsHTMLBodyElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLButtonElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLDListElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLDelElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLDirectoryElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLDivElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLEmbedElement, nsHTMLPluginObjElementSH,
                           EXTERNAL_OBJ_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLFieldSetElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLFontElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLFormElement, nsHTMLFormElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_GETPROPERTY |
                           nsIXPCScriptable::WANT_NEWENUMERATE)
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLFrameSetElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLHRElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLHeadingElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLHtmlElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLIFrameElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLImageElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLInputElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLInsElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLIsIndexElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLLIElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLLabelElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLLegendElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLLinkElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLMapElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLMenuElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLMetaElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLOListElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLObjectElement, nsHTMLPluginObjElementSH,
                           EXTERNAL_OBJ_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLOptGroupElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLOptionElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLParagraphElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLParamElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLPreElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLQuoteElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLScriptElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLSelectElement, nsHTMLSelectElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_GETPROPERTY)
  NS_DEFINE_CLASSINFO_DATA(HTMLSpacerElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLSpanElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLStyleElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCaptionElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableCellElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableColElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableRowElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTableSectionElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTextAreaElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLTitleElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLUListElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLUnknownElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLWBRElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(CSSStyleRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSCharsetRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSImportRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSMediaRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSNameSpaceRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSRuleList, nsCSSRuleListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSGroupRuleRuleList, nsCSSRuleListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MediaList, nsMediaListSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(StyleSheetList, nsStyleSheetListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSStyleSheet, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSStyleDeclaration, nsCSSStyleDeclSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(ComputedCSSStyleDeclaration, nsCSSStyleDeclSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(ROCSSPrimitiveValue, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(Range, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(Selection, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)

  
#ifdef MOZ_XUL
  NS_DEFINE_CLASSINFO_DATA(XULDocument, nsDocumentSH,
                           DOCUMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XULElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XULCommandDispatcher, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif
  NS_DEFINE_CLASSINFO_DATA(XULControllers, nsNonDOMObjectSH,
                           DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(BoxObject, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)
#ifdef MOZ_XUL
  NS_DEFINE_CLASSINFO_DATA(TreeSelection, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(TreeContentView, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)
#endif

  
  NS_DEFINE_CLASSINFO_DATA(Crypto, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CRMFObject, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(TreeWalker, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  
  
  NS_DEFINE_CLASSINFO_DATA(CSSRect, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(ChromeWindow, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSRGBColor, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(RangeException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSValueList, nsCSSValueListSH,
                           ARRAY_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(ContentList, HTMLCollection,
                                     nsContentListSH, ARRAY_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XMLStylesheetProcessingInstruction, nsNodeSH,
                           NODE_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(ImageDocument, nsHTMLDocumentSH,
                           DOCUMENT_SCRIPTABLE_FLAGS)

#ifdef MOZ_XUL
  NS_DEFINE_CLASSINFO_DATA(XULTemplateBuilder, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XULTreeBuilder, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(DOMStringList, nsStringListSH,
                           ARRAY_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(NameList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

#ifdef MOZ_XUL
  NS_DEFINE_CLASSINFO_DATA(TreeColumn, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(TreeColumns, nsTreeColumnsSH,
                           ARRAY_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(CSSMozDocumentRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(BeforeUnloadEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

#ifdef MOZ_SVG
  
  NS_DEFINE_CLASSINFO_DATA(SVGDocument, nsDocumentSH,
                           DOCUMENT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(SVGAElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
#ifdef MOZ_SMIL
  NS_DEFINE_CLASSINFO_DATA(SVGAnimateElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimateTransformElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGSetElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
#endif 
  NS_DEFINE_CLASSINFO_DATA(SVGCircleElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGClipPathElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGDefsElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGDescElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGEllipseElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEBlendElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEColorMatrixElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEComponentTransferElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFECompositeElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEConvolveMatrixElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEDiffuseLightingElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEDisplacementMapElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEDistantLightElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEFloodElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEFuncAElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEFuncBElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEFuncGElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEFuncRElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEGaussianBlurElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEImageElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEMergeElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEMergeNodeElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEMorphologyElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEOffsetElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFEPointLightElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFESpecularLightingElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFESpotLightElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFETileElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFETurbulenceElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGFilterElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGGElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGImageElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGLinearGradientElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGLineElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGMarkerElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGMaskElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGMetadataElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPatternElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPolygonElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPolylineElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGRadialGradientElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGRectElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGScriptElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGStopElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGStyleElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGSVGElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGSwitchElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGSymbolElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTextElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTextPathElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTitleElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTSpanElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGUseElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(SVGAngle, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedAngle, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedBoolean, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedEnumeration, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedInteger, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedLength, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedLengthList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedNumber, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)    
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedNumberList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)    
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedPreserveAspectRatio, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedRect, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedString, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGAnimatedTransformList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGLength, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGLengthList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGMatrix, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGNumber, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGNumberList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)    
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegArcAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegArcRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegClosePath, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoCubicAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoCubicRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoCubicSmoothAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoCubicSmoothRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoQuadraticAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoQuadraticRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoQuadraticSmoothAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegCurvetoQuadraticSmoothRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoHorizontalAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoHorizontalRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoVerticalAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegLinetoVerticalRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegMovetoAbs, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPathSegMovetoRel, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPoint, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPointList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGPreserveAspectRatio, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGRect, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTransform, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGTransformList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGUnitTypes, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGZoomEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif 

  NS_DEFINE_CLASSINFO_DATA(HTMLCanvasElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasRenderingContext2D, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasGradient, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasPattern, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(TextMetrics, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(SmartCardEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(PageTransitionEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WindowUtils, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XSLTProcessor, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XPathEvaluator, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathExpression, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathNSResolver, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathResult, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  

  
  
  
  
  NS_DEFINE_CLASSINFO_DATA(StorageObsolete, nsStorageSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_NEWRESOLVE |
                           nsIXPCScriptable::WANT_GETPROPERTY |
                           nsIXPCScriptable::WANT_SETPROPERTY |
                           nsIXPCScriptable::WANT_DELPROPERTY |
                           nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |
                           nsIXPCScriptable::WANT_NEWENUMERATE)

  NS_DEFINE_CLASSINFO_DATA(Storage, nsStorage2SH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_NEWRESOLVE |
                           nsIXPCScriptable::WANT_GETPROPERTY |
                           nsIXPCScriptable::WANT_SETPROPERTY |
                           nsIXPCScriptable::WANT_DELPROPERTY |
                           nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |
                           nsIXPCScriptable::WANT_NEWENUMERATE)
  NS_DEFINE_CLASSINFO_DATA(StorageList, nsStorageListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(StorageItem, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(StorageEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  
  
  NS_DEFINE_CLASSINFO_DATA(WindowRoot, nsEventReceiverSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(DOMParser, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XMLSerializer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XMLHttpProgressEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XMLHttpRequest, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(ClientRect, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(ClientRectList, nsClientRectListSH,
                           ARRAY_SCRIPTABLE_FLAGS)

#ifdef MOZ_SVG
  NS_DEFINE_CLASSINFO_DATA(SVGForeignObjectElement, nsElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(XULCommandEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CommandEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(OfflineResourceList, nsOfflineResourceListSH,
                           ARRAY_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(LoadStatus, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(FileList, nsFileListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(File, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(FileException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(ModalContentWindow, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(DataContainerEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MessageEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(GeoGeolocation, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  
  NS_DEFINE_CLASSINFO_DATA(GeoPosition, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS) 
  
  NS_DEFINE_CLASSINFO_DATA(GeoPositionCoords, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(GeoPositionError, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  
  NS_DEFINE_CLASSINFO_DATA(CSSFontFaceRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSFontFaceStyleDecl, nsCSSStyleDeclSH,
                           ARRAY_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(HTMLVideoElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLSourceElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLMediaError, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(HTMLAudioElement, nsHTMLElementSH,
                           ELEMENT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(ProgressEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XMLHttpRequestUpload, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(NodeIterator, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(DataTransfer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(NotifyPaintEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(SimpleGestureEvent, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

#ifdef MOZ_MATHML
  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(MathMLElement, Element, nsElementSH,
                                     ELEMENT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(Worker, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CanvasRenderingContextWebGL, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLBuffer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLTexture, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLProgram, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLShader, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLFramebuffer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(WebGLRenderbuffer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasFloatArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasByteArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasUnsignedByteArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasShortArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasUnsignedShortArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasIntArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CanvasUnsignedIntArray, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
};


struct nsContractIDMapData
{
  PRInt32 mDOMClassInfoID;
  const char *mContractID;
};

#define NS_DEFINE_CONSTRUCTOR_DATA(_class, _contract_id)                      \
  { eDOMClassInfo_##_class##_id, _contract_id },

static const nsContractIDMapData kConstructorMap[] =
{
  NS_DEFINE_CONSTRUCTOR_DATA(DOMParser, NS_DOMPARSER_CONTRACTID)
  NS_DEFINE_CONSTRUCTOR_DATA(XMLSerializer, NS_XMLSERIALIZER_CONTRACTID)
  NS_DEFINE_CONSTRUCTOR_DATA(XMLHttpRequest, NS_XMLHTTPREQUEST_CONTRACTID)
  NS_DEFINE_CONSTRUCTOR_DATA(XPathEvaluator, NS_XPATH_EVALUATOR_CONTRACTID)
  NS_DEFINE_CONSTRUCTOR_DATA(XSLTProcessor,
                             "@mozilla.org/document-transformer;1?type=xslt")
};

struct nsConstructorFuncMapData
{
  PRInt32 mDOMClassInfoID;
  nsDOMConstructorFunc mConstructorFunc;
};

#define NS_DEFINE_CONSTRUCTOR_FUNC_DATA(_class, _func)                        \
  { eDOMClassInfo_##_class##_id, _func },

static const nsConstructorFuncMapData kConstructorFuncMap[] =
{
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(Worker, nsDOMWorker::NewWorker)

  
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasFloatArray, NS_NewCanvasFloatArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasByteArray, NS_NewCanvasByteArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasUnsignedByteArray, NS_NewCanvasUnsignedByteArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasShortArray, NS_NewCanvasShortArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasUnsignedShortArray, NS_NewCanvasUnsignedShortArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasIntArray, NS_NewCanvasIntArray)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(CanvasUnsignedIntArray, NS_NewCanvasUnsignedIntArray)
};

nsIXPConnect *nsDOMClassInfo::sXPConnect = nsnull;
nsIScriptSecurityManager *nsDOMClassInfo::sSecMan = nsnull;
PRBool nsDOMClassInfo::sIsInitialized = PR_FALSE;
PRBool nsDOMClassInfo::sDisableDocumentAllSupport = PR_FALSE;
PRBool nsDOMClassInfo::sDisableGlobalScopePollutionSupport = PR_FALSE;


jsval nsDOMClassInfo::sTop_id             = JSVAL_VOID;
jsval nsDOMClassInfo::sParent_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollbars_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sLocation_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sConstructor_id     = JSVAL_VOID;
jsval nsDOMClassInfo::s_content_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sContent_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sMenubar_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sToolbar_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sLocationbar_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sPersonalbar_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sStatusbar_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sDirectories_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sControllers_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sLength_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sInnerHeight_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sInnerWidth_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOuterHeight_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOuterWidth_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sScreenX_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sScreenY_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sStatus_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sName_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sOnmousedown_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOnmouseup_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sOnclick_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOndblclick_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOncontextmenu_id   = JSVAL_VOID;
jsval nsDOMClassInfo::sOnmouseover_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOnmouseout_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOnkeydown_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sOnkeyup_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnkeypress_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOnmousemove_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOnfocus_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnblur_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sOnsubmit_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOnreset_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnchange_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOnselect_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOnload_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sOnbeforeunload_id  = JSVAL_VOID;
jsval nsDOMClassInfo::sOnunload_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOnhashchange_id    = JSVAL_VOID;
jsval nsDOMClassInfo::sOnpageshow_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOnpagehide_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOnabort_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnerror_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnpaint_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sOnresize_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOnscroll_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sOndrag_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sOndragend_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sOndragenter_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOndragleave_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOndragover_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOndragstart_id     = JSVAL_VOID;
jsval nsDOMClassInfo::sOndrop_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollIntoView_id  = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollX_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollY_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollMaxX_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sScrollMaxY_id      = JSVAL_VOID;
jsval nsDOMClassInfo::sOpen_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sItem_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sNamedItem_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sEnumerate_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sNavigator_id       = JSVAL_VOID;
jsval nsDOMClassInfo::sDocument_id        = JSVAL_VOID;
jsval nsDOMClassInfo::sWindow_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sFrames_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sSelf_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sOpener_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sAdd_id             = JSVAL_VOID;
jsval nsDOMClassInfo::sAll_id             = JSVAL_VOID;
jsval nsDOMClassInfo::sTags_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sAddEventListener_id= JSVAL_VOID;
jsval nsDOMClassInfo::sBaseURIObject_id   = JSVAL_VOID;
jsval nsDOMClassInfo::sNodePrincipal_id   = JSVAL_VOID;
jsval nsDOMClassInfo::sDocumentURIObject_id=JSVAL_VOID;
jsval nsDOMClassInfo::sOncopy_id          = JSVAL_VOID;
jsval nsDOMClassInfo::sOncut_id           = JSVAL_VOID;
jsval nsDOMClassInfo::sOnpaste_id         = JSVAL_VOID;
jsval nsDOMClassInfo::sJava_id            = JSVAL_VOID;
jsval nsDOMClassInfo::sPackages_id        = JSVAL_VOID;

static const JSClass *sObjectClass = nsnull;
const JSClass *nsDOMClassInfo::sXPCNativeWrapperClass = nsnull;




static void
FindObjectClass(JSObject* aGlobalObject)
{
  NS_ASSERTION(!sObjectClass,
               "Double set of sObjectClass");
  JSObject *obj, *proto = aGlobalObject;
  do {
    obj = proto;
    proto = STOBJ_GET_PROTO(obj);
  } while (proto);

  sObjectClass = STOBJ_GET_CLASS(obj);
}

static void
PrintWarningOnConsole(JSContext *cx, const char *stringBundleProperty)
{
  nsCOMPtr<nsIStringBundleService>
    stringService(do_GetService(NS_STRINGBUNDLE_CONTRACTID));
  if (!stringService) {
    return;
  }

  nsCOMPtr<nsIStringBundle> bundle;
  stringService->CreateBundle(kDOMStringBundleURL, getter_AddRefs(bundle));
  if (!bundle) {
    return;
  }

  nsXPIDLString msg;
  bundle->GetStringFromName(NS_ConvertASCIItoUTF16(stringBundleProperty).get(),
                            getter_Copies(msg));

  if (msg.IsEmpty()) {
    NS_ERROR("Failed to get strings from dom.properties!");
    return;
  }

  nsCOMPtr<nsIConsoleService> consoleService
    (do_GetService("@mozilla.org/consoleservice;1"));
  if (!consoleService) {
    return;
  }

  nsCOMPtr<nsIScriptError> scriptError =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
  if (!scriptError) {
    return;
  }

  JSStackFrame *fp, *iterator = nsnull;
  fp = ::JS_FrameIterator(cx, &iterator);
  PRUint32 lineno = 0;
  nsAutoString sourcefile;
  if (fp) {
    JSScript* script = ::JS_GetFrameScript(cx, fp);
    if (script) {
      const char* filename = ::JS_GetScriptFilename(cx, script);
      if (filename) {
        CopyUTF8toUTF16(nsDependentCString(filename), sourcefile);
      }
      jsbytecode* pc = ::JS_GetFramePC(cx, fp);
      if (pc) {
        lineno = ::JS_PCToLineNumber(cx, script, pc);
      }
    }
  }
  nsresult rv = scriptError->Init(msg.get(),
                                  sourcefile.get(),
                                  EmptyString().get(),
                                  lineno,
                                  0, 
                                  nsIScriptError::warningFlag,
                                  "DOM:HTML");
  if (NS_SUCCEEDED(rv)){
    consoleService->LogMessage(scriptError);
  }
}

static jsval
GetInternedJSVal(JSContext *cx, const char *str)
{
  JSString *s = ::JS_InternString(cx, str);

  if (!s) {
    return JSVAL_VOID;
  }

  return STRING_TO_JSVAL(s);
}


nsresult
nsDOMClassInfo::DefineStaticJSVals(JSContext *cx)
{
#define SET_JSVAL_TO_STRING(_val, _cx, _str)                                  \
  _val = GetInternedJSVal(_cx, _str);                                         \
  if (!JSVAL_IS_STRING(_val)) {                                               \
    return NS_ERROR_OUT_OF_MEMORY;                                            \
  }

  JSAutoRequest ar(cx);

  SET_JSVAL_TO_STRING(sTop_id,             cx, "top");
  SET_JSVAL_TO_STRING(sParent_id,          cx, "parent");
  SET_JSVAL_TO_STRING(sScrollbars_id,      cx, "scrollbars");
  SET_JSVAL_TO_STRING(sLocation_id,        cx, "location");
  SET_JSVAL_TO_STRING(sConstructor_id,     cx, "constructor");
  SET_JSVAL_TO_STRING(s_content_id,        cx, "_content");
  SET_JSVAL_TO_STRING(sContent_id,         cx, "content");
  SET_JSVAL_TO_STRING(sMenubar_id,         cx, "menubar");
  SET_JSVAL_TO_STRING(sToolbar_id,         cx, "toolbar");
  SET_JSVAL_TO_STRING(sLocationbar_id,     cx, "locationbar");
  SET_JSVAL_TO_STRING(sPersonalbar_id,     cx, "personalbar");
  SET_JSVAL_TO_STRING(sStatusbar_id,       cx, "statusbar");
  SET_JSVAL_TO_STRING(sDirectories_id,     cx, "directories");
  SET_JSVAL_TO_STRING(sControllers_id,     cx, "controllers");
  SET_JSVAL_TO_STRING(sLength_id,          cx, "length");
  SET_JSVAL_TO_STRING(sInnerHeight_id,     cx, "innerHeight");
  SET_JSVAL_TO_STRING(sInnerWidth_id,      cx, "innerWidth");
  SET_JSVAL_TO_STRING(sOuterHeight_id,     cx, "outerHeight");
  SET_JSVAL_TO_STRING(sOuterWidth_id,      cx, "outerWidth");
  SET_JSVAL_TO_STRING(sScreenX_id,         cx, "screenX");
  SET_JSVAL_TO_STRING(sScreenY_id,         cx, "screenY");
  SET_JSVAL_TO_STRING(sStatus_id,          cx, "status");
  SET_JSVAL_TO_STRING(sName_id,            cx, "name");
  SET_JSVAL_TO_STRING(sOnmousedown_id,     cx, "onmousedown");
  SET_JSVAL_TO_STRING(sOnmouseup_id,       cx, "onmouseup");
  SET_JSVAL_TO_STRING(sOnclick_id,         cx, "onclick");
  SET_JSVAL_TO_STRING(sOndblclick_id,      cx, "ondblclick");
  SET_JSVAL_TO_STRING(sOncontextmenu_id,   cx, "oncontextmenu");
  SET_JSVAL_TO_STRING(sOnmouseover_id,     cx, "onmouseover");
  SET_JSVAL_TO_STRING(sOnmouseout_id,      cx, "onmouseout");
  SET_JSVAL_TO_STRING(sOnkeydown_id,       cx, "onkeydown");
  SET_JSVAL_TO_STRING(sOnkeyup_id,         cx, "onkeyup");
  SET_JSVAL_TO_STRING(sOnkeypress_id,      cx, "onkeypress");
  SET_JSVAL_TO_STRING(sOnmousemove_id,     cx, "onmousemove");
  SET_JSVAL_TO_STRING(sOnfocus_id,         cx, "onfocus");
  SET_JSVAL_TO_STRING(sOnblur_id,          cx, "onblur");
  SET_JSVAL_TO_STRING(sOnsubmit_id,        cx, "onsubmit");
  SET_JSVAL_TO_STRING(sOnreset_id,         cx, "onreset");
  SET_JSVAL_TO_STRING(sOnchange_id,        cx, "onchange");
  SET_JSVAL_TO_STRING(sOnselect_id,        cx, "onselect");
  SET_JSVAL_TO_STRING(sOnload_id,          cx, "onload");
  SET_JSVAL_TO_STRING(sOnbeforeunload_id,  cx, "onbeforeunload");
  SET_JSVAL_TO_STRING(sOnunload_id,        cx, "onunload");
  SET_JSVAL_TO_STRING(sOnhashchange_id,    cx, "onhashchange");
  SET_JSVAL_TO_STRING(sOnpageshow_id,      cx, "onpageshow");
  SET_JSVAL_TO_STRING(sOnpagehide_id,      cx, "onpagehide");
  SET_JSVAL_TO_STRING(sOnabort_id,         cx, "onabort");
  SET_JSVAL_TO_STRING(sOnerror_id,         cx, "onerror");
  SET_JSVAL_TO_STRING(sOnpaint_id,         cx, "onpaint");
  SET_JSVAL_TO_STRING(sOnresize_id,        cx, "onresize");
  SET_JSVAL_TO_STRING(sOnscroll_id,        cx, "onscroll");
  SET_JSVAL_TO_STRING(sOndrag_id,          cx, "ondrag");
  SET_JSVAL_TO_STRING(sOndragend_id,       cx, "ondragend");
  SET_JSVAL_TO_STRING(sOndragenter_id,     cx, "ondragenter");
  SET_JSVAL_TO_STRING(sOndragleave_id,     cx, "ondragleave");
  SET_JSVAL_TO_STRING(sOndragover_id,      cx, "ondragover");
  SET_JSVAL_TO_STRING(sOndragstart_id,     cx, "ondragstart");
  SET_JSVAL_TO_STRING(sOndrop_id,          cx, "ondrop");
  SET_JSVAL_TO_STRING(sScrollIntoView_id,  cx, "scrollIntoView");
  SET_JSVAL_TO_STRING(sScrollX_id,         cx, "scrollX");
  SET_JSVAL_TO_STRING(sScrollY_id,         cx, "scrollY");
  SET_JSVAL_TO_STRING(sScrollMaxX_id,      cx, "scrollMaxX");
  SET_JSVAL_TO_STRING(sScrollMaxY_id,      cx, "scrollMaxY");
  SET_JSVAL_TO_STRING(sOpen_id,            cx, "open");
  SET_JSVAL_TO_STRING(sItem_id,            cx, "item");
  SET_JSVAL_TO_STRING(sNamedItem_id,       cx, "namedItem");
  SET_JSVAL_TO_STRING(sEnumerate_id,       cx, "enumerateProperties");
  SET_JSVAL_TO_STRING(sNavigator_id,       cx, "navigator");
  SET_JSVAL_TO_STRING(sDocument_id,        cx, "document");
  SET_JSVAL_TO_STRING(sWindow_id,          cx, "window");
  SET_JSVAL_TO_STRING(sFrames_id,          cx, "frames");
  SET_JSVAL_TO_STRING(sSelf_id,            cx, "self");
  SET_JSVAL_TO_STRING(sOpener_id,          cx, "opener");
  SET_JSVAL_TO_STRING(sAdd_id,             cx, "add");
  SET_JSVAL_TO_STRING(sAll_id,             cx, "all");
  SET_JSVAL_TO_STRING(sTags_id,            cx, "tags");
  SET_JSVAL_TO_STRING(sAddEventListener_id,cx, "addEventListener");
  SET_JSVAL_TO_STRING(sBaseURIObject_id,   cx, "baseURIObject");
  SET_JSVAL_TO_STRING(sNodePrincipal_id,   cx, "nodePrincipal");
  SET_JSVAL_TO_STRING(sDocumentURIObject_id,cx,"documentURIObject");
  SET_JSVAL_TO_STRING(sOncopy_id,          cx, "oncopy");
  SET_JSVAL_TO_STRING(sOncut_id,           cx, "oncut");
  SET_JSVAL_TO_STRING(sOnpaste_id,         cx, "onpaste");
  SET_JSVAL_TO_STRING(sJava_id,            cx, "java");
  SET_JSVAL_TO_STRING(sPackages_id,        cx, "Packages");

  return NS_OK;
}

static nsresult
CreateExceptionFromResult(JSContext *cx, nsresult aResult)
{
  nsCOMPtr<nsIExceptionService> xs =
    do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID);
  if (!xs) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIExceptionManager> xm;
  nsresult rv = xs->GetCurrentExceptionManager(getter_AddRefs(xm));
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIException> exception;
  rv = xm->GetExceptionFromProvider(aResult, 0, getter_AddRefs(exception));
  if (NS_FAILED(rv) || !exception) {
    return NS_ERROR_FAILURE;
  }

  jsval jv;
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  rv = nsDOMClassInfo::WrapNative(cx, ::JS_GetGlobalObject(cx), exception,
                                  &NS_GET_IID(nsIException), PR_FALSE, &jv,
                                  getter_AddRefs(holder));
  if (NS_FAILED(rv) || JSVAL_IS_NULL(jv)) {
    return NS_ERROR_FAILURE;
  }

  JS_SetPendingException(cx, jv);
  return NS_OK;
}


nsresult
nsDOMClassInfo::ThrowJSException(JSContext *cx, nsresult aResult)
{
  JSAutoRequest ar(cx);

  if (NS_SUCCEEDED(CreateExceptionFromResult(cx, aResult))) {
    return NS_OK;
  }

  
  
  JSString *str =
    JS_NewStringCopyZ(cx, "An error occured throwing an exception");
  if (!str) {
    
    return NS_OK; 
  }
  JS_SetPendingException(cx, STRING_TO_JSVAL(str));
  return NS_OK;
}

nsDOMClassInfo::nsDOMClassInfo(nsDOMClassInfoData* aData) : mData(aData)
{
}

nsDOMClassInfo::~nsDOMClassInfo()
{
  if (IS_EXTERNAL(mData->mCachedClassInfo)) {
    
    nsDOMClassInfoData* data = const_cast<nsDOMClassInfoData*>(mData);
    delete static_cast<nsExternalDOMClassInfoData*>(data);
  }
}

NS_IMPL_ADDREF(nsDOMClassInfo)
NS_IMPL_RELEASE(nsDOMClassInfo)

NS_INTERFACE_MAP_BEGIN(nsDOMClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPCScriptable)
NS_INTERFACE_MAP_END


static JSClass sDOMConstructorProtoClass = {
  "DOM Constructor.prototype", 0,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nsnull
};


static const char *
CutPrefix(const char *aName) {
  static const char prefix_nsIDOM[] = "nsIDOM";
  static const char prefix_nsI[]    = "nsI";

  if (strncmp(aName, prefix_nsIDOM, sizeof(prefix_nsIDOM) - 1) == 0) {
    return aName + sizeof(prefix_nsIDOM) - 1;
  }

  if (strncmp(aName, prefix_nsI, sizeof(prefix_nsI) - 1) == 0) {
    return aName + sizeof(prefix_nsI) - 1;
  }

  return aName;
}


nsresult
nsDOMClassInfo::RegisterClassName(PRInt32 aClassInfoID)
{
  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nameSpaceManager->RegisterClassName(sClassInfoData[aClassInfoID].mName,
                                      aClassInfoID,
                                      &sClassInfoData[aClassInfoID].mNameUTF16);

  return NS_OK;
}


nsresult
nsDOMClassInfo::RegisterClassProtos(PRInt32 aClassInfoID)
{
  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);
  PRBool found_old;

  const nsIID *primary_iid = sClassInfoData[aClassInfoID].mProtoChainInterface;

  if (!primary_iid || primary_iid == &NS_GET_IID(nsISupports)) {
    return NS_OK;
  }

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ENSURE_TRUE(iim, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIInterfaceInfo> if_info;
  PRBool first = PR_TRUE;

  iim->GetInfoForIID(primary_iid, getter_AddRefs(if_info));

  while (if_info) {
    const nsIID *iid = nsnull;

    if_info->GetIIDShared(&iid);
    NS_ENSURE_TRUE(iid, NS_ERROR_UNEXPECTED);

    if (iid->Equals(NS_GET_IID(nsISupports))) {
      break;
    }

    const char *name = nsnull;
    if_info->GetNameShared(&name);
    NS_ENSURE_TRUE(name, NS_ERROR_UNEXPECTED);

    nameSpaceManager->RegisterClassProto(CutPrefix(name), iid, &found_old);

    if (first) {
      first = PR_FALSE;
    } else if (found_old) {
      break;
    }

    nsCOMPtr<nsIInterfaceInfo> tmp(if_info);
    tmp->GetParent(getter_AddRefs(if_info));
  }

  return NS_OK;
}


nsresult
nsDOMClassInfo::RegisterExternalClasses()
{
  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> e;
  rv = cm->EnumerateCategory(JAVASCRIPT_DOM_CLASS, getter_AddRefs(e));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString contractId;
  nsCAutoString categoryEntry;
  nsCOMPtr<nsISupports> entry;

  while (NS_SUCCEEDED(e->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsCString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsCString!");
      continue;
    }

    rv = category->GetData(categoryEntry);

    cm->GetCategoryEntry(JAVASCRIPT_DOM_CLASS, categoryEntry.get(),
                         getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCID *cid;
    rv = registrar->ContractIDToCID(contractId, &cid);
    if (NS_FAILED(rv)) {
      NS_WARNING("Bad contract id registered with the script namespace manager");
      continue;
    }

    rv = nameSpaceManager->RegisterExternalClassName(categoryEntry.get(), *cid);
    nsMemory::Free(cid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return nameSpaceManager->RegisterExternalInterfaces(PR_TRUE);
}

#define _DOM_CLASSINFO_MAP_BEGIN(_class, _ifptr, _has_class_if)               \
  {                                                                           \
    nsDOMClassInfoData &d = sClassInfoData[eDOMClassInfo_##_class##_id];      \
    d.mProtoChainInterface = _ifptr;                                          \
    d.mHasClassInterface = _has_class_if;                                     \
    static const nsIID *interface_list[] = {

#define DOM_CLASSINFO_MAP_BEGIN(_class, _interface)                           \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), PR_TRUE)

#define DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(_class, _interface)               \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), PR_FALSE)

#define DOM_CLASSINFO_MAP_ENTRY(_if)                                          \
      &NS_GET_IID(_if),

#define DOM_CLASSINFO_MAP_END                                                 \
      nsnull                                                                  \
    };                                                                        \
                                                                              \
    d.mInterfaces = interface_list;                                           \
  }

#define DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES                                    \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSDocument)                                 \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentEvent)                              \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentStyle)                              \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSDocumentStyle)                            \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentView)                               \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentRange)                              \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentTraversal)                          \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentXBL)                                \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)                                \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Document)                                  \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)                                      \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathEvaluator)                             \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)


#define DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES                                \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLElement)                              \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElementCSSInlineStyle)                      \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)                                \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)                                      \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSElement)                                  \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)

#define DOM_CLASSINFO_EVENT_MAP_ENTRIES                                       \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEvent)                                      \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSEvent)                                    \

#define DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES                                    \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMUIEvent)                                    \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSUIEvent)                                  \
    DOM_CLASSINFO_EVENT_MAP_ENTRIES

nsresult
nsDOMClassInfo::Init()
{
  

  NS_ASSERTION(sizeof(PtrBits) == sizeof(void*),
               "BAD! You'll need to adjust the size of PtrBits to the size "
               "of a pointer on your platform.");

  NS_ENSURE_TRUE(!sIsInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = CallGetService(nsIXPConnect::GetCID(), &sXPConnect);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXPCFunctionThisTranslator> old;

  nsCOMPtr<nsIXPCFunctionThisTranslator> elt = new nsEventListenerThisTranslator();
  NS_ENSURE_TRUE(elt, NS_ERROR_OUT_OF_MEMORY);

  sXPConnect->SetFunctionThisTranslator(NS_GET_IID(nsIDOMEventListener),
                                        elt, getter_AddRefs(old));

  nsCOMPtr<nsIScriptSecurityManager> sm =
    do_GetService("@mozilla.org/scriptsecuritymanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sSecMan = sm;
  NS_ADDREF(sSecMan);

  nsCOMPtr<nsIThreadJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext *cx = nsnull;

  rv = stack->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  DOM_CLASSINFO_MAP_BEGIN(Window, nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowInternal)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMViewCSS)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMAbstractView)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageWindow)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WindowUtils, nsIDOMWindowUtils)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowUtils)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Location, nsIDOMLocation)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLocation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Navigator, nsIDOMNavigator)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNavigator)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSNavigator)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNavigatorGeolocation)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMClientInformation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Plugin, nsIDOMPlugin)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMPlugin)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(PluginArray, nsIDOMPluginArray)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMPluginArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MimeType, nsIDOMMimeType)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMimeType)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MimeTypeArray, nsIDOMMimeTypeArray)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMimeTypeArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BarProp, nsIDOMBarProp)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBarProp)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(History, nsIDOMHistory)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHistory)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Screen, nsIDOMScreen)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMScreen)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(DOMPrototype, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMConstructor, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XMLDocument, nsIDOMXMLDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXMLDocument)
    DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DocumentType, nsIDOMDocumentType)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentType)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMImplementation, nsIDOMDOMImplementation)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMImplementation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMException, nsIDOMDOMException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMTokenList, nsIDOMDOMTokenList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMTokenList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DocumentFragment, nsIDOMDocumentFragment)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocumentFragment)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Element, nsIDOMElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Attr, nsIDOMAttr)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMAttr)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Attr)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Text, nsIDOMText)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMText)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Text)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Comment, nsIDOMComment)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMComment)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CDATASection, nsIDOMCDATASection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCDATASection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Text)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(ProcessingInstruction, nsIDOMProcessingInstruction)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMProcessingInstruction)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Notation, nsIDOMNotation)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNotation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(NodeList, nsIDOMNodeList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(NamedNodeMap, nsIDOMNamedNodeMap)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNamedNodeMap)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Event, nsIDOMEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(PopupBlockedEvent, nsIDOMPopupBlockedEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMPopupBlockedEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(OrientationEvent, nsIDOMOrientationEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMOrientationEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SmartCardEvent, nsIDOMSmartCardEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSmartCardEvent)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(PageTransitionEvent, nsIDOMPageTransitionEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMPageTransitionEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MutationEvent, nsIDOMMutationEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMutationEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(UIEvent, nsIDOMUIEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END
  
  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(KeyboardEvent, nsIDOMKeyEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMKeyEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MouseEvent, nsIDOMMouseEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMouseEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSMouseEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MouseScrollEvent, nsIDOMMouseScrollEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMouseScrollEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMouseEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSMouseEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DragEvent, nsIDOMDragEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDragEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMouseEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSMouseEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLDocument, nsIDOMHTMLDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLDocument)
    DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLOptionsCollection, nsIDOMHTMLOptionsCollection)
    
    
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLOptionsCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLOptionCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCollection)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLFormControlCollection,
                                      nsIDOMHTMLCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFormControlList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLGenericCollection,
                                      nsIDOMHTMLCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCollection)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLAnchorElement, nsIDOMHTMLAnchorElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLAnchorElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLAnchorElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLAnchorElement2)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLAppletElement, nsIDOMHTMLAppletElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLAppletElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLAreaElement, nsIDOMHTMLAreaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLAreaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLAreaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLAreaElement2)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLBRElement, nsIDOMHTMLBRElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLBRElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLBaseElement, nsIDOMHTMLBaseElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLBaseElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLBaseFontElement, nsIDOMHTMLBaseFontElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLBaseFontElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLBodyElement, nsIDOMHTMLBodyElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLBodyElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLButtonElement, nsIDOMHTMLButtonElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLButtonElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLButtonElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLDListElement, nsIDOMHTMLDListElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLDListElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLDelElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLModElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLDirectoryElement, nsIDOMHTMLDirectoryElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLDirectoryElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLDivElement, nsIDOMHTMLDivElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLDivElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLEmbedElement, nsIDOMHTMLEmbedElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLEmbedElement)
#ifdef MOZ_SVG
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMGetSVGDocument)
#endif
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLFieldSetElement, nsIDOMHTMLFieldSetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFieldSetElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLFontElement, nsIDOMHTMLFontElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFontElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLFormElement, nsIDOMHTMLFormElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFormElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFormElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLFrameElement, nsIDOMHTMLFrameElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFrameElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFrameElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLFrameSetElement, nsIDOMHTMLFrameSetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLFrameSetElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLHRElement, nsIDOMHTMLHRElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLHRElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLHRElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLHeadElement, nsIDOMHTMLHeadElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLHeadElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLHeadingElement, nsIDOMHTMLHeadingElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLHeadingElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLHtmlElement, nsIDOMHTMLHtmlElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLHtmlElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLIFrameElement, nsIDOMHTMLIFrameElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLIFrameElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLFrameElement)
#ifdef MOZ_SVG
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMGetSVGDocument)
#endif
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLImageElement, nsIDOMHTMLImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLImageElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLInputElement, nsIDOMHTMLInputElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLInputElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLInputElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLInsElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLModElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLIsIndexElement, nsIDOMHTMLIsIndexElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLIsIndexElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLLIElement, nsIDOMHTMLLIElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLLIElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLLabelElement, nsIDOMHTMLLabelElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLLabelElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLLegendElement, nsIDOMHTMLLegendElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLLegendElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLLinkElement, nsIDOMHTMLLinkElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLLinkElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLinkStyle)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLMapElement, nsIDOMHTMLMapElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLMapElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLMenuElement, nsIDOMHTMLMenuElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLMenuElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLMetaElement, nsIDOMHTMLMetaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLMetaElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLOListElement, nsIDOMHTMLOListElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLOListElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLObjectElement, nsIDOMHTMLObjectElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLObjectElement)
#ifdef MOZ_SVG
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMGetSVGDocument)
#endif
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLOptGroupElement, nsIDOMHTMLOptGroupElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLOptGroupElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLOptionElement, nsIDOMHTMLOptionElement)
    
    
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLOptionElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLOptionElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLParagraphElement, nsIDOMHTMLParagraphElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLParagraphElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLParamElement, nsIDOMHTMLParamElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLParamElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLPreElement, nsIDOMHTMLPreElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLPreElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLQuoteElement, nsIDOMHTMLQuoteElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLQuoteElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLScriptElement, nsIDOMHTMLScriptElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLScriptElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLSelectElement, nsIDOMHTMLSelectElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLSelectElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLSelectElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLSpacerElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLSpanElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLStyleElement, nsIDOMHTMLStyleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLStyleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLinkStyle)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableCaptionElement,
                          nsIDOMHTMLTableCaptionElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableCaptionElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableCellElement, nsIDOMHTMLTableCellElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableCellElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableColElement, nsIDOMHTMLTableColElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableColElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableElement, nsIDOMHTMLTableElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableRowElement, nsIDOMHTMLTableRowElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableRowElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTableSectionElement,
                          nsIDOMHTMLTableSectionElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTableSectionElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTextAreaElement, nsIDOMHTMLTextAreaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTextAreaElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLTextAreaElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLTitleElement, nsIDOMHTMLTitleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLTitleElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLUListElement, nsIDOMHTMLUListElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLUListElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLUnknownElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(HTMLWBRElement, nsIDOMHTMLElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSStyleRule, nsIDOMCSSStyleRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSCharsetRule, nsIDOMCSSCharsetRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSCharsetRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSImportRule, nsIDOMCSSImportRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSImportRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSMediaRule, nsIDOMCSSMediaRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSMediaRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSNameSpaceRule, nsIDOMCSSRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSRuleList, nsIDOMCSSRuleList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRuleList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSGroupRuleRuleList, nsIDOMCSSRuleList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRuleList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MediaList, nsIDOMMediaList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMediaList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StyleSheetList, nsIDOMStyleSheetList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStyleSheetList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSStyleSheet, nsIDOMCSSStyleSheet)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleSheet)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSStyleDeclaration, nsIDOMCSSStyleDeclaration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSS2Properties)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSCSS2Properties)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ComputedCSSStyleDeclaration,
                                      nsIDOMCSSStyleDeclaration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSS2Properties)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSCSS2Properties)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ROCSSPrimitiveValue,
                                      nsIDOMCSSPrimitiveValue)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSPrimitiveValue)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSValueList, nsIDOMCSSValueList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSValueList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSRect, nsIDOMRect)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMRect)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSRGBColor, nsIDOMRGBColor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMRGBColor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSRGBAColor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Range, nsIDOMRange)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMRange)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSRange)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(NodeIterator, nsIDOMNodeIterator)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeIterator)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TreeWalker, nsIDOMTreeWalker)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMTreeWalker)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Selection, nsISelection)
    DOM_CLASSINFO_MAP_ENTRY(nsISelection)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(XULDocument, nsIDOMXULDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULDocument)
    DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XULElement, nsIDOMXULElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElementCSSInlineStyle)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XULCommandDispatcher, nsIDOMXULCommandDispatcher)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULCommandDispatcher)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULControllers, nsIControllers)
    DOM_CLASSINFO_MAP_ENTRY(nsIControllers)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BoxObject, nsIBoxObject)
    DOM_CLASSINFO_MAP_ENTRY(nsIBoxObject)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(TreeSelection, nsITreeSelection)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeSelection)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TreeContentView, nsITreeContentView)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeContentView)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeView)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(Crypto, nsIDOMCrypto)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCrypto)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CRMFObject, nsIDOMCRMFObject)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCRMFObject)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XMLStylesheetProcessingInstruction, nsIDOMProcessingInstruction)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMProcessingInstruction)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLinkStyle)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeWindow, nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowInternal)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMChromeWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMViewCSS)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMAbstractView)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(RangeException, nsIDOMRangeException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMRangeException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ContentList, nsIDOMHTMLCollection)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCollection)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(ImageDocument, nsIImageDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIImageDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSHTMLDocument)
    DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(XULTemplateBuilder, nsIXULTemplateBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsIXULTemplateBuilder)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XULTreeBuilder, nsIXULTreeBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsIXULTreeBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeView)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(DOMStringList, nsIDOMDOMStringList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMStringList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(NameList, nsIDOMNameList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNameList)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(TreeColumn, nsITreeColumn)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeColumn)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TreeColumns, nsITreeColumns)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeColumns)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(CSSMozDocumentRule, nsIDOMCSSMozDocumentRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSMozDocumentRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BeforeUnloadEvent, nsIDOMBeforeUnloadEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBeforeUnloadEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_SVG
#define DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGElement) \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSElement)  \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)      \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)

#define DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)        \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLocatable)       \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTransformable)   \
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)        \
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES

  
  
  
  

  

  DOM_CLASSINFO_MAP_BEGIN(SVGDocument, nsIDOMSVGDocument)
    
    
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDocument)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGDocument)
    DOM_CLASSINFO_DOCUMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  

  DOM_CLASSINFO_MAP_BEGIN(SVGAElement, nsIDOMSVGAElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_SMIL
  DOM_CLASSINFO_MAP_BEGIN(SVGAnimateElement, nsIDOMSVGAnimateElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimationElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimateElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElementTimeControl)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimateTransformElement,
                          nsIDOMSVGAnimateTransformElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimationElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimateTransformElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElementTimeControl)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGSetElement,
                          nsIDOMSVGSetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimationElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGSetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElementTimeControl)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END
#endif 

  DOM_CLASSINFO_MAP_BEGIN(SVGCircleElement, nsIDOMSVGCircleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGCircleElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGClipPathElement, nsIDOMSVGClipPathElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGClipPathElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLocatable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTransformable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGDefsElement, nsIDOMSVGDefsElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGDefsElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGDescElement, nsIDOMSVGDescElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGDescElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGEllipseElement, nsIDOMSVGEllipseElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGEllipseElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEBlendElement, nsIDOMSVGFEBlendElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEBlendElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEColorMatrixElement, nsIDOMSVGFEColorMatrixElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEColorMatrixElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEComponentTransferElement, nsIDOMSVGFEComponentTransferElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEComponentTransferElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFECompositeElement, nsIDOMSVGFECompositeElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFECompositeElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEConvolveMatrixElement, nsIDOMSVGFEConvolveMatrixElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEConvolveMatrixElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEDiffuseLightingElement, nsIDOMSVGFEDiffuseLightingElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEDiffuseLightingElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEDisplacementMapElement, nsIDOMSVGFEDisplacementMapElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEDisplacementMapElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEDistantLightElement, nsIDOMSVGFEDistantLightElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEDistantLightElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEFloodElement, nsIDOMSVGFEFloodElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEFloodElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEFuncAElement, nsIDOMSVGFEFuncAElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEFuncAElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEFuncBElement, nsIDOMSVGFEFuncBElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEFuncBElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEFuncGElement, nsIDOMSVGFEFuncGElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEFuncGElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEFuncRElement, nsIDOMSVGFEFuncRElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEFuncRElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEGaussianBlurElement, nsIDOMSVGFEGaussianBlurElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEGaussianBlurElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEImageElement, nsIDOMSVGFEImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEMergeElement, nsIDOMSVGFEMergeElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEMergeElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEMorphologyElement, nsIDOMSVGFEMorphologyElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEMorphologyElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEMergeNodeElement, nsIDOMSVGFEMergeNodeElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEMergeNodeElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEOffsetElement, nsIDOMSVGFEOffsetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEOffsetElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFEPointLightElement, nsIDOMSVGFEPointLightElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFEPointLightElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFESpecularLightingElement, nsIDOMSVGFESpecularLightingElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFESpecularLightingElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFESpotLightElement, nsIDOMSVGFESpotLightElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFESpotLightElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFETileElement, nsIDOMSVGFETileElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFETileElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFETurbulenceElement, nsIDOMSVGFETurbulenceElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFETurbulenceElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterPrimitiveStandardAttributes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGFilterElement, nsIDOMSVGFilterElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFilterElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGGElement, nsIDOMSVGGElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGGElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGImageElement, nsIDOMSVGImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGImageElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGLinearGradientElement, nsIDOMSVGLinearGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLinearGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGLineElement, nsIDOMSVGLineElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLineElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGMarkerElement, nsIDOMSVGMarkerElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGMarkerElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFitToViewBox)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGMaskElement, nsIDOMSVGMaskElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGMaskElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGMetadataElement, nsIDOMSVGMetadataElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGMetadataElement)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathElement, nsIDOMSVGPathElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedPathData)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPatternElement, nsIDOMSVGPatternElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPatternElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFitToViewBox)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPolygonElement, nsIDOMSVGPolygonElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPolygonElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedPoints)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPolylineElement, nsIDOMSVGPolylineElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPolylineElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedPoints)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGRadialGradientElement, nsIDOMSVGRadialGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGRadialGradientElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGRectElement, nsIDOMSVGRectElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGRectElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGScriptElement, nsIDOMSVGScriptElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGScriptElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGStopElement, nsIDOMSVGStopElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStopElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END
  
  DOM_CLASSINFO_MAP_BEGIN(SVGStyleElement, nsIDOMSVGStyleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStyleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLinkStyle)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGSVGElement, nsIDOMSVGSVGElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGSVGElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFitToViewBox)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLocatable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGZoomAndPan)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGSwitchElement, nsIDOMSVGSwitchElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGSwitchElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGSymbolElement, nsIDOMSVGSymbolElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGSymbolElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGFitToViewBox)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTextElement, nsIDOMSVGTextElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTextPositioningElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTextContentElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTextPathElement, nsIDOMSVGTextPathElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTextContentElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTitleElement, nsIDOMSVGTitleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTitleElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTSpanElement, nsIDOMSVGTSpanElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTextPositioningElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTextContentElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGStylable)
    DOM_CLASSINFO_SVG_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGUseElement, nsIDOMSVGUseElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUseElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGURIReference)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  

  DOM_CLASSINFO_MAP_BEGIN(SVGAngle, nsIDOMSVGAngle)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAngle)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedAngle, nsIDOMSVGAnimatedAngle)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedAngle)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedBoolean, nsIDOMSVGAnimatedBoolean)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedBoolean)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedEnumeration, nsIDOMSVGAnimatedEnumeration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedEnumeration)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedInteger, nsIDOMSVGAnimatedInteger)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedLength, nsIDOMSVGAnimatedLength)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedLength)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedLengthList, nsIDOMSVGAnimatedLengthList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedLengthList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedNumber, nsIDOMSVGAnimatedNumber)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedNumber)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedNumberList, nsIDOMSVGAnimatedNumberList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedNumberList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedPreserveAspectRatio, nsIDOMSVGAnimatedPreserveAspectRatio)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedPreserveAspectRatio)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedRect, nsIDOMSVGAnimatedRect)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedRect)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedString, nsIDOMSVGAnimatedString)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedString)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGAnimatedTransformList, nsIDOMSVGAnimatedTransformList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGAnimatedTransformList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGEvent, nsIDOMSVGEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGException, nsIDOMSVGException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGLength, nsIDOMSVGLength)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLength)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGLengthList, nsIDOMSVGLengthList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLengthList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGMatrix, nsIDOMSVGMatrix)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGMatrix)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGNumber, nsIDOMSVGNumber)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGNumber)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGNumberList, nsIDOMSVGNumberList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGNumberList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegArcAbs, nsIDOMSVGPathSegArcAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegArcAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegArcRel, nsIDOMSVGPathSegArcRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegArcRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegClosePath, nsIDOMSVGPathSegClosePath)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegClosePath)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoCubicAbs, nsIDOMSVGPathSegCurvetoCubicAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoCubicAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoCubicRel, nsIDOMSVGPathSegCurvetoCubicRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoCubicRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoCubicSmoothAbs, nsIDOMSVGPathSegCurvetoCubicSmoothAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoCubicSmoothAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoCubicSmoothRel, nsIDOMSVGPathSegCurvetoCubicSmoothRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoCubicSmoothRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoQuadraticAbs, nsIDOMSVGPathSegCurvetoQuadraticAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoQuadraticAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoQuadraticRel, nsIDOMSVGPathSegCurvetoQuadraticRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoQuadraticRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoQuadraticSmoothAbs, nsIDOMSVGPathSegCurvetoQuadraticSmoothAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoQuadraticSmoothAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegCurvetoQuadraticSmoothRel, nsIDOMSVGPathSegCurvetoQuadraticSmoothRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegCurvetoQuadraticSmoothRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoAbs, nsIDOMSVGPathSegLinetoAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoHorizontalAbs, nsIDOMSVGPathSegLinetoHorizontalAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoHorizontalAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoHorizontalRel, nsIDOMSVGPathSegLinetoHorizontalRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoHorizontalRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoRel, nsIDOMSVGPathSegLinetoRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoVerticalAbs, nsIDOMSVGPathSegLinetoVerticalAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoVerticalAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegLinetoVerticalRel, nsIDOMSVGPathSegLinetoVerticalRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegLinetoVerticalRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegList, nsIDOMSVGPathSegList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegMovetoAbs, nsIDOMSVGPathSegMovetoAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegMovetoAbs)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPathSegMovetoRel, nsIDOMSVGPathSegMovetoRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSegMovetoRel)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPathSeg)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPoint, nsIDOMSVGPoint)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPoint)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPointList, nsIDOMSVGPointList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPointList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGPreserveAspectRatio, nsIDOMSVGPreserveAspectRatio)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGPreserveAspectRatio)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGRect, nsIDOMSVGRect)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGRect)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTransform, nsIDOMSVGTransform)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTransform)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGTransformList, nsIDOMSVGTransformList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGTransformList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGUnitTypes, nsIDOMSVGUnitTypes)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGUnitTypes)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGZoomEvent, nsIDOMSVGZoomEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGZoomEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END
#endif 

  DOM_CLASSINFO_MAP_BEGIN(HTMLCanvasElement, nsIDOMHTMLCanvasElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLCanvasElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasRenderingContext2D, nsIDOMCanvasRenderingContext2D)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCanvasRenderingContext2D)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasGradient, nsIDOMCanvasGradient)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCanvasGradient)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasPattern, nsIDOMCanvasPattern)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCanvasPattern)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TextMetrics, nsIDOMTextMetrics)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMTextMetrics)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XSLTProcessor, nsIXSLTProcessor)
    DOM_CLASSINFO_MAP_ENTRY(nsIXSLTProcessor)
    DOM_CLASSINFO_MAP_ENTRY(nsIXSLTProcessorObsolete) 
    DOM_CLASSINFO_MAP_ENTRY(nsIXSLTProcessorPrivate)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathEvaluator, nsIDOMXPathEvaluator)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathEvaluator)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathException, nsIDOMXPathException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathExpression, nsIDOMXPathExpression)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathExpression)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSXPathExpression)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathNSResolver, nsIDOMXPathNSResolver)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathNSResolver)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathResult, nsIDOMXPathResult)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathResult)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StorageObsolete, nsIDOMStorageObsolete)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageObsolete)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Storage, nsIDOMStorage)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StorageList, nsIDOMStorageList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StorageItem, nsIDOMStorageItem)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageItem)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMToString)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StorageEvent, nsIDOMStorageEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageEvent)
  DOM_CLASSINFO_MAP_END

  
  
  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(WindowRoot, nsISupports)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(DOMParser, nsIDOMParser)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMParser)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMParserJS)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XMLSerializer, nsIDOMSerializer)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSerializer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XMLHttpRequest, nsIXMLHttpRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIXMLHttpRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIJSXMLHttpRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIXMLHttpRequestEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIInterfaceRequestor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XMLHttpProgressEvent, nsIDOMEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLSProgressEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMProgressEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_SVG
  DOM_CLASSINFO_MAP_BEGIN(SVGForeignObjectElement, nsIDOMSVGForeignObjectElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGForeignObjectElement)
    DOM_CLASSINFO_SVG_GRAPHIC_ELEMENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(XULCommandEvent, nsIDOMXULCommandEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULCommandEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CommandEvent, nsIDOMCommandEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCommandEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(OfflineResourceList, nsIDOMOfflineResourceList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMOfflineResourceList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(LoadStatus, nsIDOMLoadStatus)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLoadStatus)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(ClientRect, nsIDOMClientRect)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMClientRect)
   DOM_CLASSINFO_MAP_END
 
  DOM_CLASSINFO_MAP_BEGIN(ClientRectList, nsIDOMClientRectList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMClientRectList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(FileList, nsIDOMFileList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMFileList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(File, nsIDOMFile)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMFile)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(FileException, nsIDOMFileException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMFileException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ModalContentWindow, nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowInternal)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMViewCSS)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMAbstractView)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorageWindow)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMModalContentWindow)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DataContainerEvent, nsIDOMDataContainerEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDataContainerEvent)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MessageEvent, nsIDOMMessageEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMessageEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(GeoGeolocation, nsIDOMGeoGeolocation)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMGeoGeolocation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(GeoPosition, nsIDOMGeoPosition)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMGeoPosition)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(GeoPositionCoords, nsIDOMGeoPositionCoords)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMGeoPositionCoords)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(GeoPositionError, nsIDOMGeoPositionError)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMGeoPositionError)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSFontFaceRule, nsIDOMCSSFontFaceRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSFontFaceRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSFontFaceStyleDecl,
                                      nsIDOMCSSStyleDeclaration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLVideoElement, nsIDOMHTMLVideoElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLVideoElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLSourceElement, nsIDOMHTMLSourceElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLSourceElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLMediaError, nsIDOMHTMLMediaError)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLMediaError)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(HTMLAudioElement, nsIDOMHTMLAudioElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHTMLAudioElement)
    DOM_CLASSINFO_GENERIC_HTML_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(ProgressEvent, nsIDOMProgressEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMProgressEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XMLHttpRequestUpload, nsIXMLHttpRequestUpload)
    DOM_CLASSINFO_MAP_ENTRY(nsIXMLHttpRequestEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIXMLHttpRequestUpload)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DataTransfer, nsIDOMDataTransfer)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDataTransfer)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSDataTransfer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(NotifyPaintEvent, nsIDOMNotifyPaintEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNotifyPaintEvent)
    DOM_CLASSINFO_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SimpleGestureEvent, nsIDOMSimpleGestureEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSimpleGestureEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMouseEvent)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSMouseEvent)
    DOM_CLASSINFO_UI_EVENT_MAP_ENTRIES
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_MATHML
  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(MathMLElement, nsIDOMElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOM3Node)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNodeSelector)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(Worker, nsIWorker)
    DOM_CLASSINFO_MAP_ENTRY(nsIWorker)
    DOM_CLASSINFO_MAP_ENTRY(nsIAbstractWorker)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasRenderingContextWebGL, nsICanvasRenderingContextWebGL)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasRenderingContextWebGL)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLBuffer, nsIWebGLBuffer)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLBuffer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLTexture, nsIWebGLTexture)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLTexture)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLProgram, nsIWebGLProgram)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLProgram)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLShader, nsIWebGLShader)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLShader)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLFramebuffer, nsIWebGLFramebuffer)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLFramebuffer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WebGLRenderbuffer, nsIWebGLRenderbuffer)
    DOM_CLASSINFO_MAP_ENTRY(nsIWebGLRenderbuffer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasFloatArray, nsICanvasFloatArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasFloatArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasByteArray, nsICanvasByteArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasByteArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasUnsignedByteArray, nsICanvasUnsignedByteArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasUnsignedByteArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasShortArray, nsICanvasShortArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasShortArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasUnsignedShortArray, nsICanvasUnsignedShortArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasUnsignedShortArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasIntArray, nsICanvasIntArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasIntArray)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CanvasUnsignedIntArray, nsICanvasUnsignedIntArray)
    DOM_CLASSINFO_MAP_ENTRY(nsICanvasUnsignedIntArray)
  DOM_CLASSINFO_MAP_END

#ifdef NS_DEBUG
  {
    PRUint32 i = NS_ARRAY_LENGTH(sClassInfoData);

    if (i != eDOMClassInfoIDCount) {
      NS_ERROR("The number of items in sClassInfoData doesn't match the "
               "number of nsIDOMClassInfo ID's, this is bad! Fix it!");

      return NS_ERROR_NOT_INITIALIZED;
    }

    for (i = 0; i < eDOMClassInfoIDCount; i++) {
      if (!sClassInfoData[i].u.mConstructorFptr ||
          sClassInfoData[i].mDebugID != i) {
        NS_ERROR("Class info data out of sync, you forgot to update "
                 "nsDOMClassInfo.h and nsDOMClassInfo.cpp! Fix this, "
                 "mozilla will not work without this fixed!");

        return NS_ERROR_NOT_INITIALIZED;
      }
    }

    for (i = 0; i < eDOMClassInfoIDCount; i++) {
      if (!sClassInfoData[i].mInterfaces) {
        NS_ERROR("Class info data without an interface list! Fix this, "
                 "mozilla will not work without this fixed!");

        return NS_ERROR_NOT_INITIALIZED;
      }
    }
  }
#endif

  
  DefineStaticJSVals(cx);

  PRInt32 i;

  for (i = 0; i < eDOMClassInfoIDCount; ++i) {
    RegisterClassName(i);
  }

  for (i = 0; i < eDOMClassInfoIDCount; ++i) {
    RegisterClassProtos(i);
  }

  RegisterExternalClasses();

  sDisableDocumentAllSupport =
    nsContentUtils::GetBoolPref("browser.dom.document.all.disabled");

  sDisableGlobalScopePollutionSupport =
    nsContentUtils::GetBoolPref("browser.dom.global_scope_pollution.disabled");

  sIsInitialized = PR_TRUE;

  return NS_OK;
}


PRInt32
nsDOMClassInfo::GetArrayIndexFromId(JSContext *cx, jsval id, PRBool *aIsNumber)
{
  jsdouble array_index;

  if (aIsNumber) {
    *aIsNumber = PR_FALSE;
  }

  JSAutoRequest ar(cx);

  if (!::JS_ValueToNumber(cx, id, &array_index)) {
    return -1;
  }

  jsint i = -1;

  if (!JSDOUBLE_IS_INT(array_index, i)) {
    return -1;
  }

  if (aIsNumber) {
    *aIsNumber = PR_TRUE;
  }

  return i;
}

NS_IMETHODIMP
nsDOMClassInfo::GetInterfaces(PRUint32 *aCount, nsIID ***aArray)
{
  PRUint32 count = 0;

  while (mData->mInterfaces[count]) {
    count++;
  }

  *aCount = count;

  if (!count) {
    *aArray = nsnull;

    return NS_OK;
  }

  *aArray = static_cast<nsIID **>(nsMemory::Alloc(count * sizeof(nsIID *)));
  NS_ENSURE_TRUE(*aArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 i;
  for (i = 0; i < count; i++) {
    nsIID *iid = static_cast<nsIID *>(nsMemory::Clone(mData->mInterfaces[i],
                                                         sizeof(nsIID)));

    if (!iid) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, *aArray);

      return NS_ERROR_OUT_OF_MEMORY;
    }

    *((*aArray) + i) = iid;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  if (language == nsIProgrammingLanguage::JAVASCRIPT) {
    *_retval = static_cast<nsIXPCScriptable *>(this);

    NS_ADDREF(*_retval);
  } else {
    *_retval = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetContractID(char **aContractID)
{
  *aContractID = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassDescription(char **aClassDescription)
{
  return GetClassName(aClassDescription);
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassID(nsCID **aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassIDNoAlloc(nsCID *aClassID)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsDOMClassInfo::GetImplementationLanguage(PRUint32 *aImplLanguage)
{
  *aImplLanguage = nsIProgrammingLanguage::CPLUSPLUS;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetFlags(PRUint32 *aFlags)
{
  *aFlags = DOMCLASSINFO_STANDARD_FLAGS;

  return NS_OK;
}



NS_IMETHODIMP
nsDOMClassInfo::GetClassName(char **aClassName)
{
  *aClassName = NS_strdup(mData->mName);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetScriptableFlags(PRUint32 *aFlags)
{
  *aFlags = mData->mScriptableFlags;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::PreCreate(nsISupports *nativeObj, JSContext *cx,
                          JSObject *globalObj, JSObject **parentObj)
{
  *parentObj = globalObj;

  nsCOMPtr<nsPIDOMWindow> piwin = do_QueryWrapper(cx, globalObj);

  if (!piwin) {
    return NS_OK;
  }

  if (piwin->IsOuterWindow()) {
    *parentObj = ((nsGlobalWindow *)piwin.get())->
      GetCurrentInnerWindowInternal()->GetGlobalJSObject();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::Create(nsIXPConnectWrappedNative *wrapper,
                       JSContext *cx, JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Create Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::PostCreate(nsIXPConnectWrappedNative *wrapper,
                           JSContext *cx, JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::PostCreate Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::AddProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::DelProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::GetProperty Don't call me!");

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::SetProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRBool *_retval)
{
#ifdef DEBUG
  if (!sSecMan) {
    NS_ERROR("No security manager!!!");
    return NS_OK;
  }

  
  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, obj, mData->mName, sEnumerate_id,
                                 nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

  NS_ASSERTION(NS_SUCCEEDED(rv),
               "XOWs should have stopped us from getting here!!!");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                             JSContext *cx, JSObject *obj, PRUint32 enum_op,
                             jsval *statep, jsid *idp, PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::NewEnumerate Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

nsresult
nsDOMClassInfo::ResolveConstructor(JSContext *cx, JSObject *obj,
                                   JSObject **objp)
{
  JSObject *global = ::JS_GetGlobalForObject(cx, obj);

  jsval val;
  JSAutoRequest ar(cx);
  if (!::JS_LookupProperty(cx, global, mData->mName, &val)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!JSVAL_IS_PRIMITIVE(val)) {
    
    
    
    

    JSString *str = JSVAL_TO_STRING(sConstructor_id);
    if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                               ::JS_GetStringLength(str), val, nsnull, nsnull,
                               JSPROP_ENUMERATE)) {
      return NS_ERROR_UNEXPECTED;
    }

    *objp = obj;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsval id, PRUint32 flags,
                           JSObject **objp, PRBool *_retval)
{
  if (id == sConstructor_id && !(flags & JSRESOLVE_ASSIGNING)) {
    return ResolveConstructor(cx, obj, objp);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::Convert(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, PRUint32 type, jsval *vp,
                        PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Convert Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Finalize Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, PRUint32 mode,
                            jsval *vp, PRBool *_retval)
{
  PRUint32 mode_type = mode & JSACC_TYPEMASK;

  if ((mode_type == JSACC_WATCH ||
       mode_type == JSACC_PROTO ||
       mode_type == JSACC_PARENT) &&
      sSecMan) {

    nsresult rv;
    JSObject *real_obj;
    if (wrapper) {
      rv = wrapper->GetJSObject(&real_obj);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      real_obj = obj;
    }

    rv =
      sSecMan->CheckPropertyAccess(cx, real_obj, mData->mName, id,
                                   nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

    if (NS_FAILED(rv)) {
      
      *_retval = PR_FALSE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, PRUint32 argc, jsval *argv, jsval *vp,
                     PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Call Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRUint32 argc, jsval *argv,
                          jsval *vp, PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Construct Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval val, PRBool *bp,
                            PRBool *_retval)
{
  NS_WARNING("nsDOMClassInfo::HasInstance Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Trace(nsIXPConnectWrappedNative *wrapper, JSTracer *trc,
                      JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Trace Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Equality(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                         JSObject * obj, jsval val, PRBool *bp)
{
  NS_WARNING("nsDOMClassInfo::Equality Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                            JSObject * obj, JSObject * *_retval)
{
  NS_WARNING("nsDOMClassInfo::OuterObject Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::InnerObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                            JSObject * obj, JSObject * *_retval)
{
  NS_WARNING("nsDOMClassInfo::InnerObject Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

static nsresult
GetExternalClassInfo(nsScriptNameSpaceManager *aNameSpaceManager,
                     const nsString &aName,
                     const nsGlobalNameStruct *aStruct,
                     const nsGlobalNameStruct **aResult)
{
  NS_ASSERTION(aStruct->mType ==
                 nsGlobalNameStruct::eTypeExternalClassInfoCreator,
               "Wrong type!");

  nsresult rv;
  nsCOMPtr<nsIDOMCIExtension> creator(do_CreateInstance(aStruct->mCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMScriptObjectFactory> sof(do_GetService(kDOMSOF_CID));
  NS_ENSURE_TRUE(sof, NS_ERROR_FAILURE);

  rv = creator->RegisterDOMCI(NS_ConvertUTF16toUTF8(aName).get(), sof);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsGlobalNameStruct *name_struct;
  rv = aNameSpaceManager->LookupName(aName, &name_struct);
  if (NS_SUCCEEDED(rv) && name_struct &&
      name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    *aResult = name_struct;
  }
  else {
    NS_ERROR("Couldn't get the DOM ClassInfo data.");

    *aResult = nsnull;
  }

  return NS_OK;
}


static nsresult
ResolvePrototype(nsIXPConnect *aXPConnect, nsGlobalWindow *aWin, JSContext *cx,
                 JSObject *obj, const PRUnichar *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject *dot_prototype, PRBool install, PRBool *did_resolve);


NS_IMETHODIMP
nsDOMClassInfo::PostCreatePrototype(JSContext * cx, JSObject * proto)
{
  PRUint32 flags = (mData->mScriptableFlags & DONT_ENUM_STATIC_PROPS)
                   ? 0
                   : JSPROP_ENUMERATE;

  PRUint32 count = 0;
  while (mData->mInterfaces[count]) {
    count++;
  }

  if (!sXPConnect->DefineDOMQuickStubs(cx, proto, flags,
                                       count, mData->mInterfaces)) {
    JS_ClearPendingException(cx);
  }

  static const nsIID *sSupportsIID = &NS_GET_IID(nsISupports);

  
  if (mData->mProtoChainInterface == sSupportsIID ||
      !mData->mProtoChainInterface) {
    return NS_OK;
  }

  
  
  
  if (!sObjectClass) {
    FindObjectClass(proto);
    NS_ASSERTION(sObjectClass && !strcmp(sObjectClass->name, "Object"),
                 "Incorrect object class!");
  }

  NS_ASSERTION(::JS_GetPrototype(cx, proto) &&
               JS_GET_CLASS(cx, ::JS_GetPrototype(cx, proto)) == sObjectClass,
               "Hmm, somebody did something evil?");
 
#ifdef DEBUG
  if (mData->mHasClassInterface) {
    nsCOMPtr<nsIInterfaceInfoManager>
      iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));

    if (iim) {
      nsCOMPtr<nsIInterfaceInfo> if_info;
      iim->GetInfoForIID(mData->mProtoChainInterface,
                         getter_AddRefs(if_info));

      if (if_info) {
        nsXPIDLCString name;
        if_info->GetName(getter_Copies(name));
        NS_ASSERTION(nsCRT::strcmp(CutPrefix(name), mData->mName) == 0,
                     "Class name and proto chain interface name mismatch!");
      }
    }
  }
#endif

  
  
  
  
  JSObject *global = ::JS_GetGlobalForObject(cx, proto);

  
  
  nsISupports *globalNative = XPConnect()->GetNativeOfWrapper(cx, global);
  nsCOMPtr<nsPIDOMWindow> piwin = do_QueryInterface(globalNative);
  if(!piwin) {
    return NS_OK;
  }

  nsGlobalWindow *win = nsGlobalWindow::FromSupports(globalNative);
  if (win->IsOuterWindow()) {
    
    

    win = win->GetCurrentInnerWindowInternal();

    if (!win || !(global = win->GetGlobalJSObject())) {
      return NS_OK;
    }
  }

  
  JSBool found;
  if (!::JS_AlreadyHasOwnUCProperty(cx, global, reinterpret_cast<const jschar*>(mData->mNameUTF16),
                                    nsCRT::strlen(mData->mNameUTF16), &found)) {
    return NS_ERROR_FAILURE;
  }

  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_OK);

  PRBool unused;
  return ResolvePrototype(sXPConnect, win, cx, global, mData->mNameUTF16,
                          mData, nsnull, nameSpaceManager, proto, !found,
                          &unused);
}


nsIClassInfo *
NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID)
{
  if (aID >= eDOMClassInfoIDCount) {
    NS_ERROR("Bad ID!");

    return nsnull;
  }

  if (!nsDOMClassInfo::sIsInitialized) {
    nsresult rv = nsDOMClassInfo::Init();

    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  if (!sClassInfoData[aID].mCachedClassInfo) {
    nsDOMClassInfoData& data = sClassInfoData[aID];

    data.mCachedClassInfo = data.u.mConstructorFptr(&data);
    NS_ENSURE_TRUE(data.mCachedClassInfo, nsnull);

    NS_ADDREF(data.mCachedClassInfo);
  }

  NS_ASSERTION(!IS_EXTERNAL(sClassInfoData[aID].mCachedClassInfo),
               "This is bad, internal class marked as external!");

  return sClassInfoData[aID].mCachedClassInfo;
}


nsIClassInfo *
nsDOMClassInfo::GetClassInfoInstance(nsDOMClassInfoData* aData)
{
  NS_ASSERTION(IS_EXTERNAL(aData->mCachedClassInfo)
               || !aData->mCachedClassInfo,
               "This is bad, external class marked as internal!");

  if (!aData->mCachedClassInfo) {
    if (aData->u.mExternalConstructorFptr) {
      aData->mCachedClassInfo =
        aData->u.mExternalConstructorFptr(aData->mName);
    } else {
      aData->mCachedClassInfo = nsDOMGenericSH::doCreate(aData);
    }
    NS_ENSURE_TRUE(aData->mCachedClassInfo, nsnull);

    NS_ADDREF(aData->mCachedClassInfo);
    aData->mCachedClassInfo = MARK_EXTERNAL(aData->mCachedClassInfo);
  }

  return GET_CLEAN_CI_PTR(aData->mCachedClassInfo);
}


void
nsDOMClassInfo::PreserveNodeWrapper(nsIXPConnectWrappedNative *aWrapper)
{
  nsWrapperCache* cache = nsnull;
  CallQueryInterface(aWrapper->Native(), &cache);
  if (cache) {
    nsContentUtils::PreserveWrapper(aWrapper->Native(), cache);
  }
}



void
nsDOMClassInfo::ShutDown()
{
  if (sClassInfoData[0].u.mConstructorFptr) {
    PRUint32 i;

    for (i = 0; i < eDOMClassInfoIDCount; i++) {
      NS_IF_RELEASE(sClassInfoData[i].mCachedClassInfo);
    }
  }

  sTop_id             = JSVAL_VOID;
  sParent_id          = JSVAL_VOID;
  sScrollbars_id      = JSVAL_VOID;
  sLocation_id        = JSVAL_VOID;
  sConstructor_id     = JSVAL_VOID;
  s_content_id        = JSVAL_VOID;
  sContent_id         = JSVAL_VOID;
  sMenubar_id         = JSVAL_VOID;
  sToolbar_id         = JSVAL_VOID;
  sLocationbar_id     = JSVAL_VOID;
  sPersonalbar_id     = JSVAL_VOID;
  sStatusbar_id       = JSVAL_VOID;
  sDirectories_id     = JSVAL_VOID;
  sControllers_id     = JSVAL_VOID;
  sLength_id          = JSVAL_VOID;
  sInnerHeight_id     = JSVAL_VOID;
  sInnerWidth_id      = JSVAL_VOID;
  sOuterHeight_id     = JSVAL_VOID;
  sOuterWidth_id      = JSVAL_VOID;
  sScreenX_id         = JSVAL_VOID;
  sScreenY_id         = JSVAL_VOID;
  sStatus_id          = JSVAL_VOID;
  sName_id            = JSVAL_VOID;
  sOnmousedown_id     = JSVAL_VOID;
  sOnmouseup_id       = JSVAL_VOID;
  sOnclick_id         = JSVAL_VOID;
  sOndblclick_id      = JSVAL_VOID;
  sOncontextmenu_id   = JSVAL_VOID;
  sOnmouseover_id     = JSVAL_VOID;
  sOnmouseout_id      = JSVAL_VOID;
  sOnkeydown_id       = JSVAL_VOID;
  sOnkeyup_id         = JSVAL_VOID;
  sOnkeypress_id      = JSVAL_VOID;
  sOnmousemove_id     = JSVAL_VOID;
  sOnfocus_id         = JSVAL_VOID;
  sOnblur_id          = JSVAL_VOID;
  sOnsubmit_id        = JSVAL_VOID;
  sOnreset_id         = JSVAL_VOID;
  sOnchange_id        = JSVAL_VOID;
  sOnselect_id        = JSVAL_VOID;
  sOnload_id          = JSVAL_VOID;
  sOnbeforeunload_id  = JSVAL_VOID;
  sOnunload_id        = JSVAL_VOID;
  sOnhashchange_id    = JSVAL_VOID;
  sOnpageshow_id      = JSVAL_VOID;
  sOnpagehide_id      = JSVAL_VOID;
  sOnabort_id         = JSVAL_VOID;
  sOnerror_id         = JSVAL_VOID;
  sOnpaint_id         = JSVAL_VOID;
  sOnresize_id        = JSVAL_VOID;
  sOnscroll_id        = JSVAL_VOID;
  sOndrag_id          = JSVAL_VOID;
  sOndragend_id       = JSVAL_VOID;
  sOndragenter_id     = JSVAL_VOID;
  sOndragleave_id     = JSVAL_VOID;
  sOndragover_id      = JSVAL_VOID;
  sOndragstart_id     = JSVAL_VOID;
  sOndrop_id          = JSVAL_VOID;
  sScrollIntoView_id  = JSVAL_VOID;
  sScrollX_id         = JSVAL_VOID;
  sScrollY_id         = JSVAL_VOID;
  sScrollMaxX_id      = JSVAL_VOID;
  sScrollMaxY_id      = JSVAL_VOID;
  sOpen_id            = JSVAL_VOID;
  sItem_id            = JSVAL_VOID;
  sEnumerate_id       = JSVAL_VOID;
  sNavigator_id       = JSVAL_VOID;
  sDocument_id        = JSVAL_VOID;
  sWindow_id          = JSVAL_VOID;
  sFrames_id          = JSVAL_VOID;
  sSelf_id            = JSVAL_VOID;
  sOpener_id          = JSVAL_VOID;
  sAdd_id             = JSVAL_VOID;
  sAll_id             = JSVAL_VOID;
  sTags_id            = JSVAL_VOID;
  sAddEventListener_id= JSVAL_VOID;
  sBaseURIObject_id   = JSVAL_VOID;
  sNodePrincipal_id   = JSVAL_VOID;
  sDocumentURIObject_id=JSVAL_VOID;
  sOncopy_id          = JSVAL_VOID;
  sOncut_id           = JSVAL_VOID;
  sOnpaste_id         = JSVAL_VOID;
  sJava_id            = JSVAL_VOID;
  sPackages_id        = JSVAL_VOID;

  NS_IF_RELEASE(sXPConnect);
  NS_IF_RELEASE(sSecMan);
  sIsInitialized = PR_FALSE;
}



NS_IMETHODIMP
nsWindowSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                      JSObject *globalObj, JSObject **parentObj)
{
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeObj));
  NS_ASSERTION(sgo, "nativeObj not a global object!");

  if (sgo) {
    *parentObj = sgo->GetGlobalJSObject();

    if (*parentObj) {
      return NS_OK;
    }
  }

  
  
  

  *parentObj = globalObj;

  return NS_OK;
}




static JSClass sGlobalScopePolluterClass = {
  "Global Scope Polluter",
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_NEW_RESOLVE,
  nsWindowSH::SecurityCheckOnSetProp, nsWindowSH::SecurityCheckOnSetProp,
  nsWindowSH::GlobalScopePolluterGetProperty,
  nsWindowSH::SecurityCheckOnSetProp, JS_EnumerateStub,
  (JSResolveOp)nsWindowSH::GlobalScopePolluterNewResolve, JS_ConvertStub,
  nsHTMLDocumentSH::ReleaseDocument
};



JSBool
nsWindowSH::GlobalScopePolluterGetProperty(JSContext *cx, JSObject *obj,
                                           jsval id, jsval *vp)
{
  
  

  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, ::JS_GetGlobalForObject(cx, obj),
                                 "Window", id,
                                 nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

  if (NS_FAILED(rv)) {
    
    

    return JS_FALSE;
  }

  
  
  PrintWarningOnConsole(cx, "GlobalScopeElementReference");

  return JS_TRUE;
}


JSBool
nsWindowSH::SecurityCheckOnSetProp(JSContext *cx, JSObject *obj, jsval id,
                                   jsval *vp)
{
  
  

  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, ::JS_GetGlobalForObject(cx, obj),
                                 "Window", id,
                                 nsIXPCSecurityManager::ACCESS_SET_PROPERTY);

  
  
  return NS_SUCCEEDED(rv);
}


JSBool
nsWindowSH::GlobalScopePolluterNewResolve(JSContext *cx, JSObject *obj,
                                          jsval id, uintN flags,
                                          JSObject **objp)
{
  if (flags & (JSRESOLVE_ASSIGNING | JSRESOLVE_DECLARING |
               JSRESOLVE_CLASSNAME | JSRESOLVE_QUALIFIED) ||
      !JSVAL_IS_STRING(id)) {
    
    
    

    return JS_TRUE;
  }

  nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, obj);
  nsCOMPtr<nsIDocument> document(do_QueryInterface(doc));

  if (!document ||
      document->GetCompatibilityMode() != eCompatibility_NavQuirks) {
    
    

    return JS_TRUE;
  }

  JSObject *proto = ::JS_GetPrototype(cx, obj);
  JSString *jsstr = JSVAL_TO_STRING(id);
  JSBool hasProp;

  if (!proto || !::JS_HasUCProperty(cx, proto, ::JS_GetStringChars(jsstr),
                                    ::JS_GetStringLength(jsstr), &hasProp) ||
      hasProp) {
    
    

    return JS_TRUE;
  }

  nsDependentJSString str(jsstr);
  nsCOMPtr<nsISupports> result;

  {
    nsCOMPtr<nsIDOMDocument> dom_doc(do_QueryInterface(doc));
    nsCOMPtr<nsIDOMElement> element;

    dom_doc->GetElementById(str, getter_AddRefs(element));

    result = element;
  }

  if (!result) {
    doc->ResolveName(str, nsnull, getter_AddRefs(result));
  }

  if (result) {
    jsval v;
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, result, PR_TRUE, &v,
                             getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, JS_FALSE);

    if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(jsstr),
                               ::JS_GetStringLength(jsstr), v, nsnull, nsnull,
                               0)) {
      nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_UNEXPECTED);

      return JS_FALSE;
    }

    *objp = obj;
  }

  return JS_TRUE;
}


void
nsWindowSH::InvalidateGlobalScopePolluter(JSContext *cx, JSObject *obj)
{
  JSObject *proto;

  JSAutoRequest ar(cx);

  while ((proto = ::JS_GetPrototype(cx, obj))) {
    if (JS_GET_CLASS(cx, proto) == &sGlobalScopePolluterClass) {
      nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, proto);

      NS_IF_RELEASE(doc);

      ::JS_SetPrivate(cx, proto, nsnull);

      
      
      ::JS_SetPrototype(cx, obj, ::JS_GetPrototype(cx, proto));

      break;
    }

    obj = proto;
  }
}


nsresult
nsWindowSH::InstallGlobalScopePolluter(JSContext *cx, JSObject *obj,
                                       nsIHTMLDocument *doc)
{
  
  
  if (sDisableGlobalScopePollutionSupport || !doc) {
    return NS_OK;
  }

  JSAutoRequest ar(cx);

  JSObject *gsp = ::JS_NewObject(cx, &sGlobalScopePolluterClass, nsnull, obj);
  if (!gsp) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JSObject *o = obj, *proto;

  
  

  while ((proto = ::JS_GetPrototype(cx, o))) {
    if (JS_GET_CLASS(cx, proto) == sObjectClass) {
      
      if (!::JS_SetPrototype(cx, gsp, proto)) {
        return NS_ERROR_UNEXPECTED;
      }

      break;
    }

    o = proto;
  }

  
  
  if (!::JS_SetPrototype(cx, o, gsp)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!::JS_SetPrivate(cx, gsp, doc)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  NS_ADDREF(doc);

  return NS_OK;
}

static
already_AddRefed<nsIDOMWindow>
GetChildFrame(nsGlobalWindow *win, jsval id)
{
  nsCOMPtr<nsIDOMWindowCollection> frames;
  win->GetFrames(getter_AddRefs(frames));

  nsIDOMWindow *frame = nsnull;

  if (frames) {
    frames->Item(JSVAL_TO_INT(id), &frame);
  }

  return frame;
}

NS_IMETHODIMP
nsWindowSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

  JSAutoRequest ar(cx);

#ifdef DEBUG_SH_FORWARDING
  {
    JSString *jsstr = ::JS_ValueToString(cx, id);
    if (jsstr) {
      nsDependentJSString str(jsstr);

      if (win->IsInnerWindow()) {
#ifdef DEBUG_PRINT_INNER
        printf("Property '%s' get on inner window %p\n",
              NS_ConvertUTF16toUTF8(str).get(), (void *)win);
#endif
      } else {
        printf("Property '%s' get on outer window %p\n",
              NS_ConvertUTF16toUTF8(str).get(), (void *)win);
      }
    }
  }
#endif

  JSObject *realObj;
  wrapper->GetJSObject(&realObj);
  if (win->IsOuterWindow() && realObj == obj) {
    
    

    nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

    JSObject *innerObj;
    if (innerWin && (innerObj = innerWin->GetGlobalJSObject())) {
#ifdef DEBUG_SH_FORWARDING
      printf(" --- Forwarding get to inner window %p\n", (void *)innerWin);
#endif

      
      if (JSVAL_IS_STRING(id)) {
        JSString *str = JSVAL_TO_STRING(id);

        *_retval = ::JS_GetUCProperty(cx, innerObj, ::JS_GetStringChars(str),
                                      ::JS_GetStringLength(str), vp);
      } else if (JSVAL_IS_INT(id)) {
        *_retval = ::JS_GetElement(cx, innerObj, JSVAL_TO_INT(id), vp);
      } else {
        NS_ERROR("Write me!");

        return NS_ERROR_NOT_IMPLEMENTED;
      }

      return NS_OK;
    }
  }

  
  
  

  if (JSVAL_IS_INT(id)) {
    
    
    
    

    nsCOMPtr<nsIDOMWindow> frame = GetChildFrame(win, id);
    nsresult rv = NS_OK;

    if (frame) {
      
      
      

      nsGlobalWindow *frameWin = (nsGlobalWindow *)frame.get();

      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      rv = WrapNative(cx, frameWin->GetGlobalJSObject(), frame,
                      &NS_GET_IID(nsIDOMWindow), PR_TRUE, vp,
                      getter_AddRefs(holder));

      if (NS_SUCCEEDED(rv) && !win->IsChromeWindow()) {
        JSObject *scopeobj = JS_GetScopeChain(cx);
        if (!scopeobj) {
          *_retval = JS_FALSE;
          return NS_ERROR_FAILURE;
        }

        rv = sXPConnect->GetXOWForObject(cx, scopeobj, JSVAL_TO_OBJECT(*vp),
                                         vp);
      }
    }

    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }

  if (JSVAL_IS_STRING(id) && !JSVAL_IS_PRIMITIVE(*vp) &&
      ::JS_TypeOfValue(cx, *vp) != JSTYPE_FUNCTION) {
    
    
    
    
    
    
    
    

    const char *name = JS_GET_CLASS(cx, JSVAL_TO_OBJECT(*vp))->name;

    
    
    
    
    
    if ((*name == 'W' && strcmp(name, "Window") == 0) ||
        (*name == 'C' && strcmp(name, "ChromeWindow") == 0) ||
        (*name == 'M' && strcmp(name, "ModalContentWindow") == 0) ||
        (*name == 'X' && strcmp(name, "XPCCrossOriginWrapper") == 0)) {
      nsCOMPtr<nsIDOMWindow> window = do_QueryWrapper(cx, JSVAL_TO_OBJECT(*vp));

      if (window) {
        
        

        return NS_SUCCESS_I_DID_SOMETHING;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

#ifdef DEBUG_SH_FORWARDING
  {
    nsDependentJSString str(::JS_ValueToString(cx, id));

    if (win->IsInnerWindow()) {
#ifdef DEBUG_PRINT_INNER
      printf("Property '%s' set on inner window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
#endif
    } else {
      printf("Property '%s' set on outer window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
    }
  }
#endif

  JSObject *realObj;
  wrapper->GetJSObject(&realObj);
  if (win->IsOuterWindow() && obj == realObj) {
    
    

    nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

    JSObject *innerObj;
    if (innerWin && (innerObj = innerWin->GetGlobalJSObject())) {
#ifdef DEBUG_SH_FORWARDING
      printf(" --- Forwarding set to inner window %p\n", (void *)innerWin);
#endif

      
      if (JSVAL_IS_STRING(id)) {
        JSString *str = JSVAL_TO_STRING(id);

        *_retval = ::JS_SetUCProperty(cx, innerObj, ::JS_GetStringChars(str),
                                      ::JS_GetStringLength(str), vp);
      } else if (JSVAL_IS_INT(id)) {
        *_retval = ::JS_SetElement(cx, innerObj, JSVAL_TO_INT(id), vp);
      } else {
        NS_ERROR("Write me!");

        return NS_ERROR_NOT_IMPLEMENTED;
      }

      return NS_OK;
    }
  }

  if (id == sLocation_id) {
    JSAutoRequest ar(cx);

    JSString *val = ::JS_ValueToString(cx, *vp);
    NS_ENSURE_TRUE(val, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMWindowInternal> window(do_QueryWrappedNative(wrapper));
    NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMLocation> location;
    nsresult rv = window->GetLocation(getter_AddRefs(location));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && location, rv);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = WrapNative(cx, obj, location, &NS_GET_IID(nsIDOMLocation), PR_TRUE,
                    vp, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = location->SetHref(nsDependentJSString(val));

    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }

  return nsEventReceiverSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsWindowSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp,
                        PRBool *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

#ifdef DEBUG_SH_FORWARDING
  {
    nsDependentJSString str(::JS_ValueToString(cx, id));

    if (win->IsInnerWindow()) {
#ifdef DEBUG_PRINT_INNER
      printf("Property '%s' add on inner window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
#endif
    } else {
      printf("Property '%s' add on outer window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
    }
  }
#endif

  JSObject *realObj;
  wrapper->GetJSObject(&realObj);
  if (win->IsOuterWindow() && obj == realObj) {
    
    

    nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

    JSObject *innerObj;
    if (innerWin && (innerObj = innerWin->GetGlobalJSObject())) {
#ifdef DEBUG_SH_FORWARDING
      printf(" --- Forwarding add to inner window %p\n", (void *)innerWin);
#endif

      jsid interned_id;
      if (!JS_ValueToId(cx, id, &interned_id)) {
        *_retval = JS_FALSE;
        return NS_OK;
      }

      JSPropertyDescriptor desc;
      if (!JS_GetPropertyDescriptorById(cx, obj, interned_id,
                                        JSRESOLVE_QUALIFIED, &desc)) {
        *_retval = JS_FALSE;
        return NS_OK;
      }

      
      *_retval = JS_DefinePropertyById(cx, innerObj, interned_id, *vp,
                                       desc.getter, desc.setter,
                                       desc.attrs | JSPROP_ENUMERATE);
      return NS_OK;
    }
  }

  return nsEventReceiverSH::AddProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsWindowSH::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, jsval *vp,
                        PRBool *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

#ifdef DEBUG_SH_FORWARDING
  {
    nsDependentJSString str(::JS_ValueToString(cx, id));

    if (win->IsInnerWindow()) {
#ifdef DEBUG_PRINT_INNER
      printf("Property '%s' del on inner window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
#endif
    } else {
      printf("Property '%s' del on outer window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
    }
  }
#endif

  if (win->IsOuterWindow() && !ObjectIsNativeWrapper(cx, obj)) {
    
    

    nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

    JSObject *innerObj;
    if (innerWin && (innerObj = innerWin->GetGlobalJSObject())) {
#ifdef DEBUG_SH_FORWARDING
      printf(" --- Forwarding del to inner window %p\n", (void *)innerWin);
#endif

      
      jsid interned_id;
      *_retval = (JS_ValueToId(cx, id, &interned_id) &&
                  JS_DeletePropertyById(cx, innerObj, interned_id));

      return NS_OK;
    }
  }

  

  nsGlobalWindow *outerWin = win->GetOuterWindowInternal();
  if (outerWin) {
    nsCOMPtr<nsIXPConnectWrappedNative> wn;
    nsIXPConnect *xpc = nsContentUtils::XPConnect();
    nsresult rv =
      xpc->GetWrappedNativeOfJSObject(cx, outerWin->GetGlobalJSObject(),
                                      getter_AddRefs(wn));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = xpc->UpdateXOWs(cx, wn, nsIXPConnect::XPC_XOW_CLEARSCOPE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

static const char*
FindConstructorContractID(const nsDOMClassInfoData *aDOMClassInfoData)
{
  PRUint32 i;
  for (i = 0; i < NS_ARRAY_LENGTH(kConstructorMap); ++i) {
    if (&sClassInfoData[kConstructorMap[i].mDOMClassInfoID] ==
        aDOMClassInfoData) {
      return kConstructorMap[i].mContractID;
    }
  }
  return nsnull;
}

static nsDOMConstructorFunc
FindConstructorFunc(const nsDOMClassInfoData *aDOMClassInfoData)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(kConstructorFuncMap); ++i) {
    if (&sClassInfoData[kConstructorFuncMap[i].mDOMClassInfoID] ==
        aDOMClassInfoData) {
      return kConstructorFuncMap[i].mConstructorFunc;
    }
  }
  return nsnull;
}

static nsresult
BaseStubConstructor(nsIWeakReference* aWeakOwner,
                    const nsGlobalNameStruct *name_struct, JSContext *cx,
                    JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  nsresult rv;
  nsCOMPtr<nsISupports> native;
  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    const nsDOMClassInfoData* ci_data =
      &sClassInfoData[name_struct->mDOMClassInfoID];
    const char *contractid = FindConstructorContractID(ci_data);
    if (contractid) {
      native = do_CreateInstance(contractid, &rv);
    }
    else {
      nsDOMConstructorFunc func = FindConstructorFunc(ci_data);
      if (func) {
        rv = func(getter_AddRefs(native));
      }
    }
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    native = do_CreateInstance(name_struct->mCID, &rv);
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    native = do_CreateInstance(name_struct->mAlias->mCID, &rv);
  } else {
    native = do_CreateInstance(*name_struct->mData->mConstructorCID, &rv);
  }
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to create the object");
    return rv;
  }

  nsCOMPtr<nsIJSNativeInitializer> initializer(do_QueryInterface(native));
  if (initializer) {
    
    
    nsCOMPtr<nsPIDOMWindow> owner = do_QueryReferent(aWeakOwner);
    nsPIDOMWindow* outerWindow = owner ? owner->GetOuterWindow() : nsnull;
    nsPIDOMWindow* currentInner =
      outerWindow ? outerWindow->GetCurrentInnerWindow() : nsnull;
    if (!currentInner ||
        (owner != currentInner &&
         !nsContentUtils::CanCallerAccess(currentInner))) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    rv = initializer->Initialize(currentInner, cx, obj, argc, argv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(native));
  if (owner) {
    nsIScriptContext *context = nsJSUtils::GetStaticScriptContext(cx, obj);
    if (!context) {
      return NS_ERROR_UNEXPECTED;
    }

    JSObject* new_obj;
    rv = owner->GetScriptObject(context, (void**)&new_obj);

    if (NS_SUCCEEDED(rv)) {
      *rval = OBJECT_TO_JSVAL(new_obj);
    }

    return rv;
  }

  rv = nsDOMGenericSH::WrapNative(cx, obj, native, PR_TRUE, rval);

  return rv;
}

static nsresult
DefineInterfaceConstants(JSContext *cx, JSObject *obj, const nsIID *aIID)
{
  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ENSURE_TRUE(iim, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIInterfaceInfo> if_info;

  nsresult rv = iim->GetInfoForIID(aIID, getter_AddRefs(if_info));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && if_info, rv);

  PRUint16 constant_count;

  if_info->GetConstantCount(&constant_count);

  if (!constant_count) {
    return NS_OK;
  }

  nsCOMPtr<nsIInterfaceInfo> parent_if_info;

  rv = if_info->GetParent(getter_AddRefs(parent_if_info));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && parent_if_info, rv);

  PRUint16 parent_constant_count, i;
  parent_if_info->GetConstantCount(&parent_constant_count);

  for (i = parent_constant_count; i < constant_count; i++) {
    const nsXPTConstant *c = nsnull;

    rv = if_info->GetConstant(i, &c);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && c, rv);

    PRUint16 type = c->GetType().TagPart();

    jsval v;
    switch (type) {
      case nsXPTType::T_I8:
      case nsXPTType::T_U8:
      {
        v = INT_TO_JSVAL(c->GetValue()->val.u8);
        break;
      }
      case nsXPTType::T_I16:
      case nsXPTType::T_U16:
      {
        v = INT_TO_JSVAL(c->GetValue()->val.u16);
        break;
      }
      case nsXPTType::T_I32:
      {
        if (!JS_NewNumberValue(cx, c->GetValue()->val.i32, &v)) {
          return NS_ERROR_UNEXPECTED;
        }
        break;
      }
      case nsXPTType::T_U32:
      {
        if (!JS_NewNumberValue(cx, c->GetValue()->val.u32, &v)) {
          return NS_ERROR_UNEXPECTED;
        }
        break;
      }
      default:
      {
#ifdef NS_DEBUG
        NS_ERROR("Non-numeric constant found in interface.");
#endif
        continue;
      }
    }

    if (!::JS_DefineProperty(cx, obj, c->GetName(), v, nsnull, nsnull,
                             JSPROP_ENUMERATE)) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLBodyElementSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext
                                *cx, JSObject *obj, jsval id, PRUint32 flags,
                                JSObject **objp, PRBool *_retval)
{
  if (id == sOnhashchange_id) {
    
    jsid interned_id;

    if (!JS_ValueToId(cx, id, &interned_id) ||
        !JS_DefinePropertyById(cx, obj, interned_id, JSVAL_VOID,
                               nsnull, nsnull, JSPROP_ENUMERATE)) {
      *_retval = PR_FALSE;
      return NS_ERROR_FAILURE;
    }

    *objp = obj;
    return NS_OK;
  }

  return nsHTMLElementSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                     _retval);
}

NS_IMETHODIMP
nsHTMLBodyElementSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp, PRBool *_retval)
{
  if (id == sOnhashchange_id) {
    
    jsid interned_id;

    if (!JS_ValueToId(cx, id, &interned_id) ||
        !JS_GetPropertyById(cx, JS_GetGlobalForObject(cx, obj),
                            interned_id, vp)) {
      *_retval = PR_FALSE;
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  return nsHTMLElementSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsHTMLBodyElementSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj,
                                 jsval id, jsval *vp, PRBool *_retval)
{
  if (id == sOnhashchange_id) {
    
    jsid interned_id;

    if (!JS_ValueToId(cx, id, &interned_id) ||
        !JS_SetPropertyById(cx, JS_GetGlobalForObject(cx, obj),
                            interned_id, vp)) {
      *_retval = PR_FALSE;
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  return nsHTMLElementSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}

class nsDOMConstructor : public nsIDOMDOMConstructor
{
protected:
  nsDOMConstructor(const PRUnichar* aName,
                   PRBool aIsConstructable,
                   nsPIDOMWindow* aOwner)
    : mClassName(aName),
      mConstructable(aIsConstructable),
      mWeakOwner(do_GetWeakReference(aOwner))
  {
  }

public:

  static nsresult Create(const PRUnichar* aName,
                         const nsDOMClassInfoData* aData,
                         const nsGlobalNameStruct* aNameStruct,
                         nsPIDOMWindow* aOwner,
                         nsDOMConstructor** aResult);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMCONSTRUCTOR

  nsresult Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, PRUint32 argc, jsval *argv,
                     jsval *vp, PRBool *_retval);

  nsresult HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval val, PRBool *bp,
                       PRBool *_retval);

  nsresult Install(JSContext *cx, JSObject *target, jsval thisAsVal)
  {
    JSBool ok =
      ::JS_DefineUCProperty(cx, target,
                            reinterpret_cast<const jschar *>(mClassName),
                            nsCRT::strlen(mClassName), thisAsVal, nsnull,
                            nsnull, JSPROP_PERMANENT);

    return ok ? NS_OK : NS_ERROR_UNEXPECTED;
  }

private:
  const nsGlobalNameStruct *GetNameStruct()
  {
    if (!mClassName) {
      NS_ERROR("Can't get name");
      return nsnull;
    }

    const nsGlobalNameStruct *nameStruct;
#ifdef DEBUG
    nsresult rv =
#endif
      GetNameStruct(nsDependentString(mClassName), &nameStruct);

    NS_ASSERTION(NS_FAILED(rv) || nameStruct, "Name isn't in hash.");

    return nameStruct;
  }

  static nsresult GetNameStruct(const nsAString& aName,
                                const nsGlobalNameStruct **aNameStruct)
  {
    *aNameStruct = nsnull;

    nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
    if (!nameSpaceManager) {
      NS_ERROR("Can't get namespace manager.");
      return NS_ERROR_UNEXPECTED;
    }

    nameSpaceManager->LookupName(aName, aNameStruct);

    
    return NS_OK;
  }

  static PRBool IsConstructable(const nsDOMClassInfoData *aData)
  {
    if (IS_EXTERNAL(aData)) {
      const nsExternalDOMClassInfoData* data =
        static_cast<const nsExternalDOMClassInfoData*>(aData);
      return data->mConstructorCID != nsnull;
    }

    return FindConstructorContractID(aData) || FindConstructorFunc(aData);
  }
  static PRBool IsConstructable(const nsGlobalNameStruct *aNameStruct)
  {
    return
      (aNameStruct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
       IsConstructable(&sClassInfoData[aNameStruct->mDOMClassInfoID])) ||
      (aNameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo &&
       IsConstructable(aNameStruct->mData)) ||
      aNameStruct->mType == nsGlobalNameStruct::eTypeExternalConstructor ||
      aNameStruct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias;
  }

  const PRUnichar*   mClassName;
  const PRPackedBool mConstructable;
  nsWeakPtr          mWeakOwner;
};


nsresult
nsDOMConstructor::Create(const PRUnichar* aName,
                         const nsDOMClassInfoData* aData,
                         const nsGlobalNameStruct* aNameStruct,
                         nsPIDOMWindow* aOwner,
                         nsDOMConstructor** aResult)
{
  *aResult = nsnull;
  
  
  
  
  
  nsPIDOMWindow* outerWindow = aOwner->GetOuterWindow();
  nsPIDOMWindow* currentInner =
    outerWindow ? outerWindow->GetCurrentInnerWindow() : aOwner;
  if (!outerWindow ||
      (aOwner != currentInner &&
       !nsContentUtils::CanCallerAccess(currentInner) &&
       !(currentInner = aOwner)->IsInnerWindow())) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  PRBool constructable = aNameStruct ?
                         IsConstructable(aNameStruct) :
                         IsConstructable(aData);

  *aResult = new nsDOMConstructor(aName, constructable, currentInner);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMPL_ADDREF(nsDOMConstructor)
NS_IMPL_RELEASE(nsDOMConstructor)
NS_INTERFACE_MAP_BEGIN(nsDOMConstructor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMConstructor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
#ifdef DEBUG
    {
      const nsGlobalNameStruct *name_struct = GetNameStruct();
      NS_ASSERTION(!name_struct ||
                   mConstructable == IsConstructable(name_struct),
                   "Can't change constructability dynamically!");
    }
#endif
    foundInterface =
      NS_GetDOMClassInfoInstance(mConstructable ?
                                 eDOMClassInfo_DOMConstructor_id :
                                 eDOMClassInfo_DOMPrototype_id);
    if (!foundInterface) {
      *aInstancePtr = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else
NS_INTERFACE_MAP_END

nsresult
nsDOMConstructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                            JSObject * obj, PRUint32 argc, jsval * argv,
                            jsval * vp, PRBool *_retval)
{
  JSObject* class_obj = JSVAL_TO_OBJECT(argv[-2]);
  if (!class_obj) {
    NS_ERROR("nsDOMConstructor::Construct couldn't get constructor object.");
    return NS_ERROR_UNEXPECTED;
  }

  const nsGlobalNameStruct *name_struct = GetNameStruct();
  NS_ENSURE_TRUE(name_struct, NS_ERROR_FAILURE);

  if (!IsConstructable(name_struct)) {
    
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  return BaseStubConstructor(mWeakOwner, name_struct, cx, obj, argc, argv, vp);
}

nsresult
nsDOMConstructor::HasInstance(nsIXPConnectWrappedNative *wrapper,
                              JSContext * cx, JSObject * obj,
                              jsval v, PRBool *bp, PRBool *_retval)

{
  
  if (JSVAL_IS_PRIMITIVE(v)) {
    return NS_OK;
  }

  JSObject *dom_obj = JSVAL_TO_OBJECT(v);
  NS_ASSERTION(dom_obj, "nsDOMConstructor::HasInstance couldn't get object");

  
  
  
  JSObject *wrapped_obj;
  nsresult rv = nsContentUtils::XPConnect()->GetJSObjectOfWrapper(cx, dom_obj,
                                                                  &wrapped_obj);
  if (NS_SUCCEEDED(rv)) {
    dom_obj = wrapped_obj;
  }

  JSClass *dom_class = JS_GET_CLASS(cx, dom_obj);
  if (!dom_class) {
    NS_ERROR("nsDOMConstructor::HasInstance can't get class.");
    return NS_ERROR_UNEXPECTED;
  }

  const nsGlobalNameStruct *name_struct;
  rv = GetNameStruct(NS_ConvertASCIItoUTF16(dom_class->name), &name_struct);
  if (!name_struct) {
    return rv;
  }

  if (name_struct->mType != nsGlobalNameStruct::eTypeClassConstructor &&
      name_struct->mType != nsGlobalNameStruct::eTypeExternalClassInfo &&
      name_struct->mType != nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    
    return NS_OK;
  }

  const nsGlobalNameStruct *class_name_struct = GetNameStruct();
  NS_ENSURE_TRUE(class_name_struct, NS_ERROR_FAILURE);

  if (name_struct == class_name_struct) {
    *bp = JS_TRUE;

    return NS_OK;
  }

  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ASSERTION(nameSpaceManager, "Can't get namespace manager?");

  const nsIID *class_iid;
  if (class_name_struct->mType == nsGlobalNameStruct::eTypeInterface ||
      class_name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    class_iid = &class_name_struct->mIID;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    class_iid =
      sClassInfoData[class_name_struct->mDOMClassInfoID].mProtoChainInterface;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    class_iid = class_name_struct->mData->mProtoChainInterface;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    const nsGlobalNameStruct* alias_struct =
      nameSpaceManager->GetConstructorProto(class_name_struct);
    if (!alias_struct) {
      NS_ERROR("Couldn't get constructor prototype.");
      return NS_ERROR_UNEXPECTED;
    }

    if (alias_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      class_iid =
        sClassInfoData[alias_struct->mDOMClassInfoID].mProtoChainInterface;
    } else if (alias_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      class_iid = alias_struct->mData->mProtoChainInterface;
    } else {
      NS_ERROR("Expected eTypeClassConstructor or eTypeExternalClassInfo.");
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    *bp = JS_FALSE;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    name_struct = nameSpaceManager->GetConstructorProto(name_struct);
    if (!name_struct) {
      NS_ERROR("Couldn't get constructor prototype.");
      return NS_ERROR_UNEXPECTED;
    }
  }

  NS_ASSERTION(name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
               name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
               "The constructor was set up with a struct of the wrong type.");

  const nsDOMClassInfoData *ci_data = nsnull;
  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
      name_struct->mDOMClassInfoID >= 0) {
    ci_data = &sClassInfoData[name_struct->mDOMClassInfoID];
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    ci_data = name_struct->mData;
  }

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  if (!iim) {
    NS_ERROR("nsDOMConstructor::HasInstance can't get interface info mgr.");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIInterfaceInfo> if_info;
  PRUint32 count = 0;
  const nsIID* class_interface;
  while ((class_interface = ci_data->mInterfaces[count++])) {
    if (class_iid->Equals(*class_interface)) {
      *bp = JS_TRUE;

      return NS_OK;
    }

    iim->GetInfoForIID(class_interface, getter_AddRefs(if_info));
    if (!if_info) {
      NS_ERROR("nsDOMConstructor::HasInstance can't get interface info.");
      return NS_ERROR_UNEXPECTED;
    }

    if_info->HasAncestor(class_iid, bp);

    if (*bp) {
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructor::ToString(nsAString &aResult)
{
  aResult.AssignLiteral("[object ");
  aResult.Append(mClassName);
  aResult.Append(PRUnichar(']'));

  return NS_OK;
}


static nsresult
GetXPCProto(nsIXPConnect *aXPConnect, JSContext *cx, nsGlobalWindow *aWin,
            const nsGlobalNameStruct *aNameStruct,
            nsIXPConnectJSObjectHolder **aProto)
{
  NS_ASSERTION(aNameStruct->mType ==
                 nsGlobalNameStruct::eTypeClassConstructor ||
               aNameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
               "Wrong type!");

  nsCOMPtr<nsIClassInfo> ci;
  if (aNameStruct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    PRInt32 id = aNameStruct->mDOMClassInfoID;
    NS_ABORT_IF_FALSE(id >= 0, "Negative DOM classinfo?!?");

    nsDOMClassInfoID ci_id = (nsDOMClassInfoID)id;

    ci = NS_GetDOMClassInfoInstance(ci_id);

    
    
    
    
    
    
    
    if (ci_id == eDOMClassInfo_Window_id ||
        ci_id == eDOMClassInfo_ModalContentWindow_id ||
        ci_id == eDOMClassInfo_ChromeWindow_id) {
      nsGlobalWindow *scopeWindow = aWin->GetOuterWindowInternal();

      if (scopeWindow) {
        aWin = scopeWindow;
      }
    }
  }
  else {
    ci = nsDOMClassInfo::GetClassInfoInstance(aNameStruct->mData);
  }
  NS_ENSURE_TRUE(ci, NS_ERROR_UNEXPECTED);

  return aXPConnect->GetWrappedNativePrototype(cx, aWin->GetGlobalJSObject(),
                                               ci, aProto);
}



static nsresult
ResolvePrototype(nsIXPConnect *aXPConnect, nsGlobalWindow *aWin, JSContext *cx,
                 JSObject *obj, const PRUnichar *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject *dot_prototype, PRBool install, PRBool *did_resolve)
{
  NS_ASSERTION(ci_data ||
               (name_struct &&
                name_struct->mType == nsGlobalNameStruct::eTypeClassProto),
               "Wrong type or missing ci_data!");

  nsRefPtr<nsDOMConstructor> constructor;
  nsresult rv = nsDOMConstructor::Create(name, ci_data, name_struct, aWin,
                                         getter_AddRefs(constructor));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  jsval v;

  rv = nsDOMClassInfo::WrapNative(cx, obj, constructor,
                                  &NS_GET_IID(nsIDOMDOMConstructor),
                                  PR_FALSE, &v, getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  if (install) {
    rv = constructor->Install(cx, obj, v);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JSObject *class_obj;
  holder->GetJSObject(&class_obj);
  NS_ASSERTION(class_obj, "The return value lied");

  const nsIID *primary_iid = &NS_GET_IID(nsISupports);

  if (!ci_data) {
    primary_iid = &name_struct->mIID;
  }
  else if (ci_data->mProtoChainInterface) {
    primary_iid = ci_data->mProtoChainInterface;
  }

  nsCOMPtr<nsIInterfaceInfo> if_info;
  nsCOMPtr<nsIInterfaceInfo> parent;
  const char *class_parent_name = nsnull;

  if (!primary_iid->Equals(NS_GET_IID(nsISupports))) {
    rv = DefineInterfaceConstants(cx, class_obj, primary_iid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (primary_iid->Equals(NS_GET_IID(nsIDOMNode))) {
      rv = DefineInterfaceConstants(cx, class_obj,
                                    &NS_GET_IID(nsIDOM3Node));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    if (primary_iid->Equals(NS_GET_IID(nsIDOMEvent))) {
      rv = DefineInterfaceConstants(cx, class_obj,
                                    &NS_GET_IID(nsIDOMNSEvent));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIInterfaceInfoManager>
      iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
    NS_ENSURE_TRUE(iim, NS_ERROR_NOT_AVAILABLE);

    iim->GetInfoForIID(primary_iid, getter_AddRefs(if_info));
    NS_ENSURE_TRUE(if_info, NS_ERROR_UNEXPECTED);

    const nsIID *iid = nsnull;

    if (ci_data && !ci_data->mHasClassInterface) {
      if_info->GetIIDShared(&iid);
    } else {
      if_info->GetParent(getter_AddRefs(parent));
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

      parent->GetIIDShared(&iid);
    }

    if (iid) {
      if (!iid->Equals(NS_GET_IID(nsISupports))) {
        if (ci_data && !ci_data->mHasClassInterface) {
          
          
          

          if_info->GetNameShared(&class_parent_name);
        } else {
          
          
          
          

          NS_ASSERTION(parent, "Whoa, this is bad, null parent here!");

          parent->GetNameShared(&class_parent_name);
        }
      }
    }
  }

  JSObject *proto = nsnull;

  if (class_parent_name) {
    jsval val;

    if (!::JS_LookupProperty(cx, obj, CutPrefix(class_parent_name), &val)) {
      return NS_ERROR_UNEXPECTED;
    }

    JSObject *tmp = JSVAL_IS_OBJECT(val) ? JSVAL_TO_OBJECT(val) : nsnull;

    if (tmp) {
      if (!::JS_LookupProperty(cx, tmp, "prototype", &val)) {
        return NS_ERROR_UNEXPECTED;
      }

      if (JSVAL_IS_OBJECT(val)) {
        proto = JSVAL_TO_OBJECT(val);
      }
    }
  }

  if (dot_prototype) {
    JSObject *xpc_proto_proto = ::JS_GetPrototype(cx, dot_prototype);

    if (proto &&
        (!xpc_proto_proto ||
         JS_GET_CLASS(cx, xpc_proto_proto) == sObjectClass)) {
      if (!::JS_SetPrototype(cx, dot_prototype, proto)) {
        return NS_ERROR_UNEXPECTED;
      }
    }
  } else {
    dot_prototype = ::JS_NewObject(cx, &sDOMConstructorProtoClass, proto,
                                   obj);
    NS_ENSURE_TRUE(dot_prototype, NS_ERROR_OUT_OF_MEMORY);
  }

  v = OBJECT_TO_JSVAL(dot_prototype);

  
  if (!::JS_DefineProperty(cx, class_obj, "prototype", v, nsnull, nsnull,
                           JSPROP_PERMANENT | JSPROP_READONLY)) {
    return NS_ERROR_UNEXPECTED;
  }

  *did_resolve = PR_TRUE;

  return NS_OK;
}



nsresult
nsWindowSH::GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                          JSObject *obj, JSString *str, PRBool *did_resolve)
{
  *did_resolve = PR_FALSE;

  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsDependentJSString name(str);

  const nsGlobalNameStruct *name_struct = nsnull;
  const PRUnichar *class_name = nsnull;

  nameSpaceManager->LookupName(name, &name_struct, &class_name);

  if (!name_struct) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(class_name, NS_ERROR_UNEXPECTED);

  nsresult rv = NS_OK;

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
    rv = GetExternalClassInfo(nameSpaceManager, name, name_struct,
                              &name_struct);
    if (NS_FAILED(rv) || !name_struct) {
      return rv;
    }
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeInterface) {
    
    

    nsRefPtr<nsDOMConstructor> constructor;
    rv = nsDOMConstructor::Create(class_name,
                                  nsnull,
                                  name_struct,
                                  static_cast<nsPIDOMWindow*>(aWin),
                                  getter_AddRefs(constructor));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    jsval v;
    rv = WrapNative(cx, obj, constructor, &NS_GET_IID(nsIDOMDOMConstructor),
                    PR_FALSE, &v, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = constructor->Install(cx, obj, v);
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject *class_obj;
    holder->GetJSObject(&class_obj);
    NS_ASSERTION(class_obj, "The return value lied");

    
    

    rv = DefineInterfaceConstants(cx, class_obj, &name_struct->mIID);
    NS_ENSURE_SUCCESS(rv, rv);

    *did_resolve = PR_TRUE;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
      name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> proto_holder;
    rv = GetXPCProto(sXPConnect, cx, aWin, name_struct,
                     getter_AddRefs(proto_holder));

    if (NS_SUCCEEDED(rv) && obj != aWin->GetGlobalJSObject()) {
      JSObject* dot_prototype;
      rv = proto_holder->GetJSObject(&dot_prototype);
      NS_ENSURE_SUCCESS(rv, rv);

      const nsDOMClassInfoData *ci_data;
      if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
        ci_data = &sClassInfoData[name_struct->mDOMClassInfoID];
      } else {
        ci_data = name_struct->mData;
      }

      return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, ci_data,
                              name_struct, nameSpaceManager, dot_prototype,
                              PR_TRUE, did_resolve);
    }

    *did_resolve = NS_SUCCEEDED(rv);

    return rv;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    
    
    return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, nsnull,
                            name_struct, nameSpaceManager, nsnull, PR_TRUE,
                            did_resolve);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    const nsGlobalNameStruct *alias_struct =
      nameSpaceManager->GetConstructorProto(name_struct);
    NS_ENSURE_TRUE(alias_struct, NS_ERROR_UNEXPECTED);

    
    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> proto_holder;
    rv = GetXPCProto(sXPConnect, cx, aWin, alias_struct,
                     getter_AddRefs(proto_holder));
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject* dot_prototype;
    rv = proto_holder->GetJSObject(&dot_prototype);
    NS_ENSURE_SUCCESS(rv, rv);

    const nsDOMClassInfoData *ci_data;
    if (alias_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      ci_data = &sClassInfoData[alias_struct->mDOMClassInfoID];
    } else if (alias_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      ci_data = alias_struct->mData;
    } else {
      return NS_ERROR_UNEXPECTED;
    }

    return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, ci_data,
                            name_struct, nameSpaceManager, nsnull, PR_TRUE,
                            did_resolve);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    nsRefPtr<nsDOMConstructor> constructor;
    rv = nsDOMConstructor::Create(class_name, nsnull, name_struct,
                                  static_cast<nsPIDOMWindow*>(aWin),
                                  getter_AddRefs(constructor));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval val;
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = WrapNative(cx, obj, constructor, &NS_GET_IID(nsIDOMDOMConstructor),
                    PR_FALSE, &val, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = constructor->Install(cx, obj, val);
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject* class_obj;
    holder->GetJSObject(&class_obj);
    NS_ASSERTION(class_obj, "Why didn't we get a JSObject?");

    *did_resolve = PR_TRUE;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeProperty) {
    if (name_struct->mPrivilegedOnly && !nsContentUtils::IsCallerChrome())
      return NS_OK;

    nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval prop_val; 

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsCOMPtr<nsIScriptObjectOwner> owner(do_QueryInterface(native));
    if (owner) {
      nsIScriptContext *context = nsJSUtils::GetStaticScriptContext(cx, obj);
      NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

      JSObject *prop_obj = nsnull;
      rv = owner->GetScriptObject(context, (void**)&prop_obj);
      NS_ENSURE_TRUE(prop_obj, NS_ERROR_UNEXPECTED);

      prop_val = OBJECT_TO_JSVAL(prop_obj);
    } else {
      JSObject *scope;

      if (aWin->IsOuterWindow()) {
        nsGlobalWindow *inner = aWin->GetCurrentInnerWindowInternal();
        NS_ENSURE_TRUE(inner, NS_ERROR_UNEXPECTED);

        scope = inner->GetGlobalJSObject();
      } else {
        scope = aWin->GetGlobalJSObject();
      }

      rv = WrapNative(cx, scope, native, PR_TRUE, &prop_val,
                      getter_AddRefs(holder));
    }

    NS_ENSURE_SUCCESS(rv, rv);

    JSBool ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                      ::JS_GetStringLength(str),
                                      prop_val, nsnull, nsnull,
                                      JSPROP_ENUMERATE);

    *did_resolve = PR_TRUE;

    return ok ? NS_OK : NS_ERROR_FAILURE;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeDynamicNameSet) {
    nsCOMPtr<nsIScriptExternalNameSet> nameset =
      do_CreateInstance(name_struct->mCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIScriptContext *context = aWin->GetContext();
    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

    rv = nameset->InitializeNameSet(context);

    *did_resolve = PR_TRUE;
  }

  return rv;
}



static JSBool
ContentWindowGetter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                    jsval *rval)
{
  return ::JS_GetProperty(cx, obj, "content", rval);
}

NS_IMETHODIMP
nsWindowSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval id, PRUint32 flags,
                       JSObject **objp, PRBool *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

#ifdef DEBUG_SH_FORWARDING
  {
    nsDependentJSString str(::JS_ValueToString(cx, id));

    if (win->IsInnerWindow()) {
#ifdef DEBUG_PRINT_INNER
      printf("Property '%s' resolve on inner window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
#endif
    } else {
      printf("Property '%s' resolve on outer window %p\n",
             NS_ConvertUTF16toUTF8(str).get(), (void *)win);
    }
  }
#endif

  
  
  
  
  
  
  if (win->IsOuterWindow() && id != sLocation_id) {
    
    

    nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

    if ((!innerWin || !innerWin->GetExtantDocument()) &&
        !win->IsCreatingInnerWindow()) {
      
      
      
      
      
      
      
      
      
      
      nsIScriptContext *scx = win->GetContextInternal();

      if (scx && scx->IsContextInitialized()) {
        
        innerWin = win->EnsureInnerWindowInternal();

        if (!innerWin) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }

    JSObject *innerObj;
    JSObject *realObj;
    wrapper->GetJSObject(&realObj);
    if (realObj == obj &&
        innerWin && (innerObj = innerWin->GetGlobalJSObject())) {
#ifdef DEBUG_SH_FORWARDING
      printf(" --- Forwarding resolve to inner window %p\n", (void *)innerWin);
#endif

      jsid interned_id;
      JSObject *pobj = NULL;
      jsval val;

      *_retval = (::JS_ValueToId(cx, id, &interned_id) &&
                  ::JS_LookupPropertyWithFlagsById(cx, innerObj, interned_id,
                                                   flags, &pobj, &val));

      if (*_retval && pobj) {
#ifdef DEBUG_SH_FORWARDING
        printf(" --- Resolve on inner window found property.\n");
#endif
        *objp = pobj;
      }

      return NS_OK;
    }
  }

  if (!JSVAL_IS_STRING(id)) {
    if (JSVAL_IS_INT(id) && !(flags & JSRESOLVE_ASSIGNING)) {
      
      
      
      

      nsCOMPtr<nsIDOMWindow> frame = GetChildFrame(win, id);

      if (frame) {
        
        

        *_retval = ::JS_DefineElement(cx, obj, JSVAL_TO_INT(id), JSVAL_VOID,
                                      nsnull, nsnull, 0);

        if (*_retval) {
          *objp = obj;
        }
      }
    }

    return NS_OK;
  }

  nsIScriptContext *my_context = win->GetContextInternal();

  nsresult rv = NS_OK;

  
  
  
  
  
  

  JSBool did_resolve = JS_FALSE;
  JSContext *my_cx;

  if (!my_context) {
    my_cx = cx;
  } else {
    my_cx = (JSContext *)my_context->GetNativeContext();
  }

  JSBool ok;
  jsval exn;
  {
    JSAutoSuspendRequest asr(my_cx != cx ? cx : nsnull);
    {
      JSAutoRequest ar(my_cx);

      JSObject *realObj;
      wrapper->GetJSObject(&realObj);

      
      
      ok = obj == realObj ?
           ::JS_ResolveStandardClass(my_cx, obj, id, &did_resolve) :
           JS_TRUE;

      if (!ok) {
        
        

        if (!JS_GetPendingException(my_cx, &exn)) {
          return NS_ERROR_UNEXPECTED;
        }

        
        
        
        

        JS_ClearPendingException(my_cx);
      }
    }
  }

  if (!ok) {
    JS_SetPendingException(cx, exn);
    *_retval = JS_FALSE;
    return NS_OK;
  }

  if (did_resolve) {
    *objp = obj;

    return NS_OK;
  }

  if (!(flags & JSRESOLVE_ASSIGNING)) {
    
    
    
    if (id == sConstructor_id) {
      return ResolveConstructor(cx, obj, objp);
    }
  }

  if (!my_context || !my_context->IsContextInitialized()) {
    
    

    return NS_OK;
  }


  
  
  

  JSString *str = JSVAL_TO_STRING(id);

  
  if (!ObjectIsNativeWrapper(cx, obj)) {
    nsCOMPtr<nsIDocShellTreeNode> dsn(do_QueryInterface(win->GetDocShell()));

    PRInt32 count = 0;

    if (dsn) {
      dsn->GetChildCount(&count);
    }

    if (count > 0) {
      nsCOMPtr<nsIDocShellTreeItem> child;

      const jschar *chars = ::JS_GetStringChars(str);

      dsn->FindChildWithName(reinterpret_cast<const PRUnichar*>(chars),
                             PR_FALSE, PR_TRUE, nsnull, nsnull,
                             getter_AddRefs(child));

      nsCOMPtr<nsIDOMWindow> child_win(do_GetInterface(child));

      if (child_win) {
        
        
        

        JSObject *wrapperObj;
        wrapper->GetJSObject(&wrapperObj);

        jsval v;
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        rv = WrapNative(cx, wrapperObj, child_win,
                        &NS_GET_IID(nsIDOMWindowInternal), PR_TRUE, &v,
                        getter_AddRefs(holder));
        NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
        if (!win->IsChromeWindow()) {
          NS_ASSERTION(JSVAL_IS_OBJECT(v) &&
                       !strcmp(STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name,
                               "XPCCrossOriginWrapper"),
                       "Didn't wrap a window!");
        }
#endif

        JSAutoRequest ar(cx);

        PRBool ok = ::JS_DefineUCProperty(cx, obj, chars,
                                          ::JS_GetStringLength(str),
                                          v, nsnull, nsnull, 0);

        if (!ok) {
          return NS_ERROR_FAILURE;
        }

        *objp = obj;

        return NS_OK;
      }
    }
  }

  
  
  
  if (!(flags & JSRESOLVE_ASSIGNING)) {
    JSAutoRequest ar(cx);

    
    
    

    JSBool did_resolve = JS_FALSE;
    rv = GlobalResolve(win, cx, obj, str, &did_resolve);
    NS_ENSURE_SUCCESS(rv, rv);

    if (did_resolve) {
      
      *objp = obj;

      return NS_OK;
    }
  }

  if (id == s_content_id) {
    
    
    

    JSObject *windowObj = win->GetGlobalJSObject();

    JSAutoRequest ar(cx);

    JSFunction *fun = ::JS_NewFunction(cx, ContentWindowGetter, 0, 0,
                                       windowObj, "_content");
    if (!fun) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    JSObject *funObj = ::JS_GetFunctionObject(fun);

    nsAutoGCRoot root(&funObj, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!::JS_DefineUCProperty(cx, windowObj, ::JS_GetStringChars(str),
                               ::JS_GetStringLength(str), JSVAL_VOID,
                               JS_DATA_TO_FUNC_PTR(JSPropertyOp, funObj),
                               nsnull,
                               JSPROP_ENUMERATE | JSPROP_GETTER |
                               JSPROP_SHARED)) {
      return NS_ERROR_FAILURE;
    }

    *objp = obj;

    return NS_OK;
  }

  if (id == sLocation_id) {
    
    
    
    

    nsCOMPtr<nsIDOMLocation> location;
    rv = win->GetLocation(getter_AddRefs(location));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    JSObject *scope = nsnull;
    if (win->IsOuterWindow()) {
      nsGlobalWindow *innerWin = win->GetCurrentInnerWindowInternal();

      if (innerWin) {
        scope = innerWin->GetGlobalJSObject();
      }
    }

    if (!scope) {
      wrapper->GetJSObject(&scope);
    }

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    jsval v;
    rv = WrapNative(cx, scope, location, &NS_GET_IID(nsIDOMLocation), PR_TRUE,
                    &v, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
    if (!win->IsChromeWindow()) {
          NS_ASSERTION(JSVAL_IS_OBJECT(v) &&
                       !strcmp(STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v))->name,
                               "XPCCrossOriginWrapper"),
                       "Didn't wrap a location object!");
    }
#endif

    JSAutoRequest ar(cx);

    JSBool ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                      ::JS_GetStringLength(str), v, nsnull,
                                      nsnull, JSPROP_ENUMERATE);

    if (!ok) {
      return NS_ERROR_FAILURE;
    }

    *objp = obj;

    return NS_OK;
  }

  if (id == sOnhashchange_id) {
    
    jsid interned_id;

    if (!JS_ValueToId(cx, id, &interned_id) ||
        !JS_DefinePropertyById(cx, obj, interned_id, JSVAL_VOID,
                                nsnull, nsnull, JSPROP_ENUMERATE)) {
      *_retval = PR_FALSE;
      return NS_ERROR_FAILURE;
    }

    *objp = obj;
    return NS_OK;
  }

  if (flags & JSRESOLVE_ASSIGNING) {
    if (IsReadonlyReplaceable(id) ||
        (!(flags & JSRESOLVE_QUALIFIED) && IsWritableReplaceable(id))) {
      
      
      
      
      
      JSAutoRequest ar(cx);

      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str),
                                 JSVAL_VOID, JS_PropertyStub, JS_PropertyStub,
                                 JSPROP_ENUMERATE)) {
        return NS_ERROR_FAILURE;
      }
      *objp = obj;

      return NS_OK;
    }
  } else {
    if (id == sNavigator_id) {
      nsCOMPtr<nsIDOMNavigator> navigator;
      rv = win->GetNavigator(getter_AddRefs(navigator));
      NS_ENSURE_SUCCESS(rv, rv);

      jsval v;
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      rv = WrapNative(cx, obj, navigator, &NS_GET_IID(nsIDOMNavigator), PR_TRUE,
                      &v, getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      JSAutoRequest ar(cx);

      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), v, nsnull, nsnull,
                                 JSPROP_READONLY | JSPROP_PERMANENT |
                                 JSPROP_ENUMERATE)) {
        return NS_ERROR_FAILURE;
      }
      *objp = obj;

      return NS_OK;
    }

    if (id == sDocument_id) {
      nsCOMPtr<nsIDOMDocument> document;
      rv = win->GetDocument(getter_AddRefs(document));
      NS_ENSURE_SUCCESS(rv, rv);

      jsval v;
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      rv = WrapNative(cx, obj, document, &NS_GET_IID(nsIDOMDocument), PR_FALSE,
                      &v, getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      *objp = obj;

      return NS_OK;
    }

    if (id == sWindow_id) {
      
      nsGlobalWindow *oldWin = win;
      win = win->GetOuterWindowInternal();
      NS_ENSURE_TRUE(win, NS_ERROR_NOT_AVAILABLE);

      JSAutoRequest ar(cx);

      jsval winVal = OBJECT_TO_JSVAL(win->GetGlobalJSObject());
      if (!win->IsChromeWindow()) {
        JSObject *scope;
        nsGlobalWindow *innerWin;
        if (oldWin->IsInnerWindow()) {
          scope = oldWin->GetGlobalJSObject();
        } else if ((innerWin = oldWin->GetCurrentInnerWindowInternal())) {
          scope = innerWin->GetGlobalJSObject();
        } else {
          NS_ERROR("I don't know what scope to use!");
          scope = oldWin->GetGlobalJSObject();
        }

        rv = sXPConnect->GetXOWForObject(cx, scope, JSVAL_TO_OBJECT(winVal),
                                         &winVal);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      PRBool ok =
        ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                              ::JS_GetStringLength(str),
                              winVal, JS_PropertyStub, JS_PropertyStub,
                              JSPROP_READONLY | JSPROP_ENUMERATE);

      if (!ok) {
        return NS_ERROR_FAILURE;
      }
      *objp = obj;

      return NS_OK;
    }

    if (id == sJava_id || id == sPackages_id) {
      static PRBool isResolvingJavaProperties;

      if (!isResolvingJavaProperties) {
        isResolvingJavaProperties = PR_TRUE;

        
        
        

        win->InitJavaProperties(); 

        PRBool hasProp;
        PRBool ok = ::JS_HasProperty(cx, obj, ::JS_GetStringBytes(str),
                                     &hasProp);

        isResolvingJavaProperties = PR_FALSE;

        if (!ok) {
          return NS_ERROR_FAILURE;
        }

        if (hasProp) {
          *objp = obj;

          return NS_OK;
        }
      }
    }
  }

  JSObject *oldobj = *objp;
  rv = nsEventReceiverSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                     _retval);

  if (NS_FAILED(rv) || *objp != oldobj) {
    
    return rv;
  }

  
  
  
  
  if ((flags & JSRESOLVE_ASSIGNING) &&
      !(flags & JSRESOLVE_WITH) &&
      win->IsInnerWindow()) {
    JSObject *realObj;
    wrapper->GetJSObject(&realObj);

    if (obj == realObj) {
      JSObject *proto = STOBJ_GET_PROTO(obj);
      if (proto) {
        jsid interned_id;
        JSObject *pobj = NULL;
        jsval val;

        if (!::JS_ValueToId(cx, id, &interned_id) ||
            !::JS_LookupPropertyWithFlagsById(cx, proto, interned_id, flags,
                                              &pobj, &val)) {
          *_retval = JS_FALSE;

          return NS_OK;
        }

        if (pobj) {
          
          *objp = pobj;
          return NS_OK;
        }
      }

      
      
      
      
      
      
      
      
      
      
      

      JSString *str = JSVAL_TO_STRING(id);
      if (!::js_CheckUndeclaredVarAssignment(cx) ||
          !::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), JSVAL_VOID,
                                 JS_PropertyStub, JS_PropertyStub,
                                 JSPROP_ENUMERATE)) {
        *_retval = JS_FALSE;

        return NS_OK;
      }

      *objp = obj;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, PRUint32 enum_op, jsval *statep,
                         jsid *idp, PRBool *_retval)
{
  switch ((JSIterateOp)enum_op) {
    case JSENUMERATE_INIT:
    {
      
      
      nsDOMClassInfo::Enumerate(wrapper, cx, obj, _retval);
      if (!*_retval) {
        return NS_OK;
      }

      
      
      nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);
      JSObject *enumobj = win->GetGlobalJSObject();
      if (win->IsOuterWindow()) {
        nsGlobalWindow *inner = win->GetCurrentInnerWindowInternal();
        if (inner) {
          enumobj = inner->GetGlobalJSObject();
        }
      }

      
      JSObject *iterator = JS_NewPropertyIterator(cx, enumobj);
      if (!iterator) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      *statep = OBJECT_TO_JSVAL(iterator);
      if (idp) {
        
        
        *idp = JSVAL_ZERO;
      }
      break;
    }
    case JSENUMERATE_NEXT:
    {
      JSObject *iterator = (JSObject*)JSVAL_TO_OBJECT(*statep);
      if (!JS_NextProperty(cx, iterator, idp)) {
        return NS_ERROR_UNEXPECTED;
      }

      if (*idp != JSVAL_VOID) {
        break;
      }

      
    }
    case JSENUMERATE_DESTROY:
      
      *statep = JSVAL_NULL;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::Finalize(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(sgo, NS_ERROR_UNEXPECTED);

  sgo->OnFinalize(nsIProgrammingLanguage::JAVASCRIPT, obj);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::Equality(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                     JSObject * obj, jsval val, PRBool *bp)
{
  *bp = PR_FALSE;

  if (JSVAL_IS_PRIMITIVE(val)) {
    return NS_OK;
  }

  nsCOMPtr<nsIXPConnectWrappedNative> other_wrapper;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, JSVAL_TO_OBJECT(val),
                               getter_AddRefs(other_wrapper));
  if (!other_wrapper) {
    

    return NS_OK;
  }

  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

  NS_ASSERTION(win->IsOuterWindow(),
               "Inner window detected in Equality hook!");

  nsCOMPtr<nsPIDOMWindow> other = do_QueryWrappedNative(other_wrapper);

  if (other) {
    NS_ASSERTION(other->IsOuterWindow(),
                 "Inner window detected in Equality hook!");

    *bp = win->GetOuterWindow() == other->GetOuterWindow();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                        JSObject * obj, JSObject * *_retval)
{
  nsGlobalWindow *origWin = nsGlobalWindow::FromWrapper(wrapper);
  nsGlobalWindow *win = origWin->GetOuterWindowInternal();

  if (!win) {
    
    
    
    
    
    *_retval = nsnull;
    return NS_ERROR_UNEXPECTED;
  }

  JSObject *winObj = win->GetGlobalJSObject();
  if (!winObj) {
    NS_ASSERTION(origWin->IsOuterWindow(), "What window is this?");
    *_retval = obj;
    return NS_OK;
  }

  *_retval = winObj;
  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::InnerObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                        JSObject * obj, JSObject * *_retval)
{
  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);

  if (win->IsInnerWindow() || win->IsFrozen()) {
    
    

    *_retval = obj;
  } else {
    

    nsGlobalWindow *inner = win->GetCurrentInnerWindowInternal();
    if (!inner) {
      
      

      *_retval = nsnull;

      return NS_ERROR_UNEXPECTED;
    }

    *_retval = inner->GetGlobalJSObject();
  }

  return NS_OK;
}



NS_IMETHODIMP
nsLocationSH::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, PRUint32 mode,
                          jsval *vp, PRBool *_retval)
{
  if ((mode & JSACC_TYPEMASK) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    

    
    *_retval = PR_FALSE;

    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return nsDOMGenericSH::CheckAccess(wrapper, cx, obj, id, mode, vp, _retval);
}

NS_IMETHODIMP
nsLocationSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                        JSObject *globalObj, JSObject **parentObj)
{
  
  
  
  
  *parentObj = globalObj;

  nsCOMPtr<nsIDOMLocation> safeLoc(do_QueryInterface(nativeObj));
  if (!safeLoc) {
    
    
    
    return NS_OK;
  }

  nsLocation *loc = (nsLocation *)safeLoc.get();
  nsIDocShell *ds = loc->GetDocShell();
  if (!ds) {
    NS_WARNING("Refusing to create a location in the wrong scope");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_GetInterface(ds);

  if (sgo) {
    JSObject *global = sgo->GetGlobalJSObject();

    if (global) {
      *parentObj = global;
    }
  }

  return NS_OK;
}


nsresult
nsNavigatorSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                         JSObject *globalObj, JSObject **parentObj)
{
  
  
  
  
  
  *parentObj = globalObj;

  nsCOMPtr<nsIDOMNavigator> safeNav(do_QueryInterface(nativeObj));
  if (!safeNav) {
    
    
    
    return NS_OK;
  }

  nsNavigator *nav = (nsNavigator *)safeNav.get();
  nsIDocShell *ds = nav->GetDocShell();
  if (!ds) {
    NS_WARNING("Refusing to create a navigator in the wrong scope");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_GetInterface(ds);

  if (sgo) {
    JSObject *global = sgo->GetGlobalJSObject();

    if (global) {
      *parentObj = global;
    }
  }

  return NS_OK;
}



PRBool
nsNodeSH::IsCapabilityEnabled(const char* aCapability)
{
  PRBool enabled;
  return sSecMan &&
    NS_SUCCEEDED(sSecMan->IsCapabilityEnabled(aCapability, &enabled)) &&
    enabled;
}

nsresult
nsNodeSH::DefineVoidProp(JSContext* cx, JSObject* obj, jsval id,
                         JSObject** objp)
{
  NS_ASSERTION(JSVAL_IS_STRING(id), "id must be a string");

  JSString* str = JSVAL_TO_STRING(id);

  
  
  
  JSBool ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                    ::JS_GetStringLength(str), JSVAL_VOID,
                                    nsnull, nsnull, JSPROP_SHARED);

  if (!ok) {
    return NS_ERROR_FAILURE;
  }

  *objp = obj;
  return NS_OK;
}

NS_IMETHODIMP
nsNodeSH::PreCreate(nsISupports *nativeObj, JSContext *cx, JSObject *globalObj,
                    JSObject **parentObj)
{
  nsINode *node = static_cast<nsINode*>(nativeObj);
  
#ifdef DEBUG
  {
    nsCOMPtr<nsINode> node_qi(do_QueryInterface(nativeObj));

    
    
    
    NS_ASSERTION(node_qi == node, "Uh, fix QI!");
  }
#endif

  
  
  
  
  
  
  nsIDocument* doc = node->GetOwnerDoc();

  if (!doc) {
    
    

    *parentObj = globalObj;

    return node->IsInNativeAnonymousSubtree() ?
      NS_SUCCESS_CHROME_ACCESS_ONLY : NS_OK;
  }

  
  
  
  
  
  
  
  PRBool hasHadScriptHandlingObject = PR_FALSE;
  NS_ENSURE_STATE(doc->GetScriptHandlingObject(hasHadScriptHandlingObject) ||
                  hasHadScriptHandlingObject ||
                  IsPrivilegedScript());

  nsISupports *native_parent;

  PRBool slimWrappers = PR_TRUE;
  if (node->IsNodeOfType(nsINode::eELEMENT | nsINode::eXUL)) {
    
    native_parent = node->GetParent();

    if (!native_parent) {
      native_parent = doc;
    }
  } else if (!node->IsNodeOfType(nsINode::eDOCUMENT)) {
    NS_ASSERTION(node->IsNodeOfType(nsINode::eCONTENT) ||
                 node->IsNodeOfType(nsINode::eATTRIBUTE),
                 "Unexpected node type");
                 
    
    native_parent = doc;

    
    if (node->IsNodeOfType(nsINode::eELEMENT |
                           nsIContent::eHTML |
                           nsIContent::eHTML_FORM_CONTROL)) {
      nsCOMPtr<nsIFormControl> form_control(do_QueryInterface(node));

      if (form_control) {
        nsCOMPtr<nsIDOMHTMLFormElement> form;
        form_control->GetForm(getter_AddRefs(form));

        if (form) {
          
          native_parent = form;
        }
      }
    }
  } else {
    
    

    
    native_parent = doc->GetScopeObject();

    if (!native_parent) {
      
      

      *parentObj = globalObj;

      return node->IsInNativeAnonymousSubtree() ?
        NS_SUCCESS_CHROME_ACCESS_ONLY : NS_OK;
    }

    slimWrappers = PR_FALSE;
  }

  
  
  
  

  if (native_parent == doc && (*parentObj = doc->GetWrapper()))
    return node->IsInNativeAnonymousSubtree() ?
      NS_SUCCESS_CHROME_ACCESS_ONLY :
      (slimWrappers ? NS_SUCCESS_ALLOW_SLIM_WRAPPERS : NS_OK);

  jsval v;
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  nsresult rv = WrapNative(cx, globalObj, native_parent, PR_FALSE, &v,
                           getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  *parentObj = JSVAL_TO_OBJECT(v);

  return node->IsInNativeAnonymousSubtree() ?
    NS_SUCCESS_CHROME_ACCESS_ONLY :
    (slimWrappers ? NS_SUCCESS_ALLOW_SLIM_WRAPPERS : NS_OK);
}

NS_IMETHODIMP
nsNodeSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                      JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  nsINode* node = static_cast<nsINode*>(GetNative(wrapper, obj));
  nsContentUtils::PreserveWrapper(node, node);
  return nsEventReceiverSH::AddProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsNodeSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, jsval id, PRUint32 flags,
                     JSObject **objp, PRBool *_retval)
{
  if ((id == sBaseURIObject_id || id == sNodePrincipal_id) &&
      IsPrivilegedScript()) {
    return DefineVoidProp(cx, obj, id, objp);
  }

  if (id == sOnload_id || id == sOnerror_id) {
    
    
    nsINode* node = static_cast<nsINode*>(GetNative(wrapper, obj));
    nsContentUtils::PreserveWrapper(node, node);
  }

  return nsEventReceiverSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                       _retval);
}

NS_IMETHODIMP
nsNodeSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                      JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (id == sBaseURIObject_id && IsPrivilegedScript()) {
    
    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIContent> content = do_QueryWrappedNative(wrapper, obj);
    if (content) {
      uri = content->GetBaseURI();
      NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
    } else {
      nsCOMPtr<nsIDocument> doc = do_QueryWrappedNative(wrapper, obj);
      NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

      uri = doc->GetBaseURI();
      NS_ENSURE_TRUE(uri, NS_ERROR_NOT_AVAILABLE);
    }

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, uri, &NS_GET_IID(nsIURI), PR_TRUE, vp,
                             getter_AddRefs(holder));
    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }

  if (id == sNodePrincipal_id && IsPrivilegedScript()) {
    nsCOMPtr<nsINode> node = do_QueryWrappedNative(wrapper, obj);
    NS_ENSURE_TRUE(node, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, node->NodePrincipal(),
                             &NS_GET_IID(nsIPrincipal), PR_TRUE, vp,
                             getter_AddRefs(holder));
    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }    

  
  return NS_OK;
}

NS_IMETHODIMP
nsNodeSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                      JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if ((id == sBaseURIObject_id || id == sNodePrincipal_id) &&
      IsPrivilegedScript()) {
    
    
    
    
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  return nsEventReceiverSH::SetProperty(wrapper, cx, obj, id, vp,_retval);
}

NS_IMETHODIMP
nsNodeSH::GetFlags(PRUint32 *aFlags)
{
  *aFlags = DOMCLASSINFO_STANDARD_FLAGS | nsIClassInfo::CONTENT_NODE;

  return NS_OK;
}




PRBool
nsEventReceiverSH::ReallyIsEventName(jsval id, jschar aFirstChar)
{
  

  switch (aFirstChar) {
  case 'a' :
    return id == sOnabort_id;
  case 'b' :
    return (id == sOnbeforeunload_id ||
            id == sOnblur_id);
  case 'e' :
    return id == sOnerror_id;
  case 'f' :
    return id == sOnfocus_id;
  case 'c' :
    return (id == sOnchange_id       ||
            id == sOnclick_id        ||
            id == sOncontextmenu_id  ||
            id == sOncopy_id         ||
            id == sOncut_id);
  case 'd' :
    return (id == sOndblclick_id     || 
            id == sOndrag_id         ||
            id == sOndragend_id      ||
            id == sOndragenter_id    ||
            id == sOndragleave_id    ||
            id == sOndragover_id     ||
            id == sOndragstart_id    ||
            id == sOndrop_id);
  case 'h' :
    return id == sOnhashchange_id;
  case 'l' :
    return id == sOnload_id;
  case 'p' :
    return (id == sOnpaint_id        ||
            id == sOnpageshow_id     ||
            id == sOnpagehide_id     ||
            id == sOnpaste_id);
  case 'k' :
    return (id == sOnkeydown_id      ||
            id == sOnkeypress_id     ||
            id == sOnkeyup_id);
  case 'u' :
    return id == sOnunload_id;
  case 'm' :
    return (id == sOnmousemove_id    ||
            id == sOnmouseout_id     ||
            id == sOnmouseover_id    ||
            id == sOnmouseup_id      ||
            id == sOnmousedown_id);
  case 'r' :
    return (id == sOnreset_id        ||
            id == sOnresize_id);
  case 's' :
    return (id == sOnscroll_id       ||
            id == sOnselect_id       ||
            id == sOnsubmit_id);
  }

  return PR_FALSE;
}


JSBool
nsEventReceiverSH::AddEventListenerHelper(JSContext *cx, JSObject *obj,
                                          uintN argc, jsval *argv, jsval *rval)
{
  if (argc < 3 || argc > 4) {
    ThrowJSException(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);

    return JS_FALSE;
  }

  OBJ_TO_INNER_OBJECT(cx, obj);

  nsresult rv = sXPConnect->GetJSObjectOfWrapper(cx, obj, &obj);
  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);

    return JS_FALSE;
  }

  
  if (NS_FAILED(sSecMan->CheckPropertyAccess(cx, obj,
                                             JS_GET_CLASS(cx, obj)->name,
                                             sAddEventListener_id,
                                             nsIXPCSecurityManager::ACCESS_GET_PROPERTY)) ||
      NS_FAILED(sSecMan->CheckPropertyAccess(cx, obj,
                                             JS_GET_CLASS(cx, obj)->name,
                                             sAddEventListener_id,
                                             nsIXPCSecurityManager::ACCESS_CALL_METHOD))) {
    
    
    

    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(argv[1])) {
    
    
    ThrowJSException(cx, NS_ERROR_XPC_BAD_CONVERT_JS);

    return JS_FALSE;
  }

  JSString* jsstr = JS_ValueToString(cx, argv[0]);
  if (!jsstr) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_OUT_OF_MEMORY);

    return JS_FALSE;
  }

  nsDependentJSString type(jsstr);

  nsCOMPtr<nsIDOMEventListener> listener;

  {
    nsCOMPtr<nsISupports> tmp;
    sXPConnect->WrapJS(cx, JSVAL_TO_OBJECT(argv[1]),
                       NS_GET_IID(nsIDOMEventListener), getter_AddRefs(tmp));

    listener = do_QueryInterface(tmp, &rv);
    if (NS_FAILED(rv)) {
      ThrowJSException(cx, rv);

      return JS_FALSE;
    }
  }

  JSBool useCapture;
  JS_ValueToBoolean(cx, argv[2], &useCapture);

  if (argc == 4) {
    JSBool wantsUntrusted;
    JS_ValueToBoolean(cx, argv[3], &wantsUntrusted);

    nsCOMPtr<nsIDOMNSEventTarget> eventTarget = do_QueryWrapper(cx, obj, &rv);
    if (NS_FAILED(rv)) {
      ThrowJSException(cx, rv);

      return JS_FALSE;
    }

    rv = eventTarget->AddEventListener(type, listener, useCapture,
                                       wantsUntrusted);
    if (NS_FAILED(rv)) {
      ThrowJSException(cx, rv);

      return JS_FALSE;
    }
  } else {
    nsCOMPtr<nsIDOMEventTarget> eventTarget = do_QueryWrapper(cx, obj, &rv);
    if (NS_FAILED(rv)) {
      ThrowJSException(cx, rv);

      return JS_FALSE;
    }

    rv = eventTarget->AddEventListener(type, listener, useCapture);
    if (NS_FAILED(rv)) {
      ThrowJSException(cx, rv);

      return JS_FALSE;
    }
  }
  
  return JS_TRUE;
}

nsresult
nsEventReceiverSH::RegisterCompileHandler(nsIXPConnectWrappedNative *wrapper,
                                          JSContext *cx, JSObject *obj,
                                          jsval id, PRBool compile,
                                          PRBool remove,
                                          PRBool *did_define)
{
  NS_PRECONDITION(!compile || !remove,
                  "Can't both compile and remove at the same time");
  *did_define = PR_FALSE;

  if (!IsEventName(id)) {
    return NS_OK;
  }

  if (ObjectIsNativeWrapper(cx, obj)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsIScriptContext *script_cx = nsJSUtils::GetStaticScriptContext(cx, obj);
  NS_ENSURE_TRUE(script_cx, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsPIDOMEventTarget> piTarget =
    do_QueryWrappedNative(wrapper, obj);
  if (!piTarget) {
    
    NS_WARNING("Doesn't QI to nsPIDOMEventTarget?");
    return NS_OK;
  }
  
  nsIEventListenerManager* manager = piTarget->GetListenerManager(PR_TRUE);
  NS_ENSURE_TRUE(manager, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIAtom> atom(do_GetAtom(nsDependentJSString(id)));
  NS_ENSURE_TRUE(atom, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  JSObject *scope = ::JS_GetGlobalForObject(cx, obj);

  if (compile) {
    rv = manager->CompileScriptEventListener(script_cx, scope, piTarget, atom,
                                             did_define);
  } else if (remove) {
    rv = manager->RemoveScriptEventListener(atom);
  } else {
    rv = manager->RegisterScriptEventListener(script_cx, scope, piTarget,
                                              atom);
  }

  return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
}

nsresult
nsEventReceiverSH::DefineAddEventListener(JSContext *cx, JSObject *obj,
                                          jsval id, JSObject **objp)
{
  NS_ASSERTION(id == sAddEventListener_id, "Wrong call?!?");
  JSString *str = JSVAL_TO_STRING(id);
  
  JSFunction *fnc =
    ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                        nsEventReceiverSH::AddEventListenerHelper, 3,
                        JSPROP_ENUMERATE);
  *objp = obj;
  return fnc ? NS_OK : NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsEventReceiverSH::NewResolve(nsIXPConnectWrappedNative *wrapper,
                              JSContext *cx, JSObject *obj, jsval id,
                              PRUint32 flags, JSObject **objp, PRBool *_retval)
{
  if (!JSVAL_IS_STRING(id)) {
    return NS_OK;
  }

  if (flags & JSRESOLVE_ASSIGNING) {
    if (!IsEventName(id)) {
      
      return NS_OK;
    }

    
    
    
    
    JSString* str = JSVAL_TO_STRING(id);
    JSAutoRequest ar(cx);

    JSObject *proto = ::JS_GetPrototype(cx, obj);
    PRBool ok = PR_TRUE, hasProp = PR_FALSE;
    if (!proto || ((ok = ::JS_HasUCProperty(cx, proto, ::JS_GetStringChars(str),
                                            ::JS_GetStringLength(str),
                                            &hasProp)) &&
                   !hasProp)) {
      
      
      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), JSVAL_NULL,
                                 nsnull, nsnull,
                                 JSPROP_ENUMERATE | JSPROP_PERMANENT)) {
        return NS_ERROR_FAILURE;
      }

      *objp = obj;
      return NS_OK;
    }

    return ok ? NS_OK : NS_ERROR_FAILURE;
  }

  if (id == sAddEventListener_id) {
    return nsEventReceiverSH::DefineAddEventListener(cx, obj, id, objp);
  }

  PRBool did_define = PR_FALSE;
  nsresult rv = RegisterCompileHandler(wrapper, cx, obj, id, PR_TRUE, PR_FALSE,
                                       &did_define);
  NS_ENSURE_SUCCESS(rv, rv);

  if (did_define) {
    *objp = obj;
  }

  return nsDOMGenericSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                    _retval);
}

NS_IMETHODIMP
nsEventReceiverSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj, jsval id,
                               jsval *vp, PRBool *_retval)
{
  JSAutoRequest ar(cx);

  if ((::JS_TypeOfValue(cx, *vp) != JSTYPE_FUNCTION && !JSVAL_IS_NULL(*vp)) ||
      !JSVAL_IS_STRING(id) || id == sAddEventListener_id) {
    return NS_OK;
  }

  PRBool did_compile; 

  return RegisterCompileHandler(wrapper, cx, obj, id, PR_FALSE,
                                JSVAL_IS_NULL(*vp), &did_compile);
}

NS_IMETHODIMP
nsEventReceiverSH::AddProperty(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj, jsval id,
                               jsval *vp, PRBool *_retval)
{
  return nsEventReceiverSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}



NS_IMETHODIMP
nsEventTargetSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                           JSObject *globalObj, JSObject **parentObj)
{
  nsXHREventTarget *target = nsXHREventTarget::FromSupports(nativeObj);

  nsCOMPtr<nsIScriptGlobalObject> native_parent;
  target->GetParentObject(getter_AddRefs(native_parent));

  *parentObj = native_parent ? native_parent->GetGlobalJSObject() : globalObj;

  return NS_OK;
}

NS_IMETHODIMP
nsEventTargetSH::NewResolve(nsIXPConnectWrappedNative *wrapper,
                            JSContext *cx, JSObject *obj, jsval id,
                            PRUint32 flags, JSObject **objp, PRBool *_retval)
{
  if ((flags & JSRESOLVE_ASSIGNING) || !JSVAL_IS_STRING(id)) {
    return NS_OK;
  }
  if (id == sAddEventListener_id) {
    return nsEventReceiverSH::DefineAddEventListener(cx, obj, id, objp);
  }
  return nsDOMGenericSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                    _retval);
}

NS_IMETHODIMP
nsEventTargetSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (id == sAddEventListener_id) {
    return NS_OK;
  }

  nsISupports *target = GetNative(wrapper, obj);
  nsContentUtils::PreserveWrapper(target,
                                  nsXHREventTarget::FromSupports(target));

  return NS_OK;
}




static PRBool
GetBindingURL(nsIContent *aContent, nsIDocument *aDocument,
              nsCSSValue::URL **aResult)
{
  
  nsIPresShell *shell = aDocument->GetPrimaryShell();
  nsIFrame *frame;
  if (!shell ||
      (frame = shell->GetPrimaryFrameFor(aContent))) {
    *aResult = nsnull;

    return PR_TRUE;
  }

  
  nsPresContext *pctx = shell->GetPresContext();
  NS_ENSURE_TRUE(pctx, PR_FALSE);

  nsRefPtr<nsStyleContext> sc = pctx->StyleSet()->ResolveStyleFor(aContent,
                                                                  nsnull);
  NS_ENSURE_TRUE(sc, PR_FALSE);

  *aResult = sc->GetStyleDisplay()->mBinding;

  return PR_TRUE;
}

NS_IMETHODIMP
nsElementSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj)
{
  nsresult rv = nsNodeSH::PreCreate(nativeObj, cx, globalObj, parentObj);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent *content = static_cast<nsIContent*>(nativeObj);

#ifdef DEBUG
  {
    nsCOMPtr<nsIContent> content_qi(do_QueryInterface(nativeObj));

    
    
    
    NS_ASSERTION(content_qi == content, "Uh, fix QI!");
  }
#endif

  nsIDocument *doc = content->HasFlag(NODE_FORCE_XBL_BINDINGS) ?
                     content->GetOwnerDoc() :
                     content->GetCurrentDoc();

  if (!doc) {
    return rv;
  }

  if (content->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR) &&
      doc->BindingManager()->GetBinding(content)) {
    
    return rv == NS_SUCCESS_ALLOW_SLIM_WRAPPERS ? NS_OK : rv;
  }

  nsCSSValue::URL *bindingURL;
  PRBool ok = GetBindingURL(content, doc, &bindingURL);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  
  if (!bindingURL) {
    return rv;
  }

  content->SetFlags(NODE_ATTACH_BINDING_ON_POSTCREATE);

  return rv == NS_SUCCESS_ALLOW_SLIM_WRAPPERS ? NS_OK : rv;
}

NS_IMETHODIMP
nsElementSH::PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj)
{
  nsIContent *content = static_cast<nsIContent*>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIContent> content_qi(do_QueryWrappedNative(wrapper));

    
    
    
    NS_ASSERTION(content_qi == content, "Uh, fix QI!");
  }
#endif

  nsIDocument* doc;
  if (content->HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    doc = content->GetOwnerDoc();
  }
  else {
    doc = content->GetCurrentDoc();
  }

  if (!doc) {
    
    

    return NS_OK;
  }

  
  

  if (!content->HasFlag(NODE_ATTACH_BINDING_ON_POSTCREATE)) {
    
    

    
    
    
    

    return NS_OK;
  }

  content->UnsetFlags(NODE_ATTACH_BINDING_ON_POSTCREATE);

  
  
  nsCSSValue::URL *bindingURL;
  PRBool ok = GetBindingURL(content, doc, &bindingURL);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  if (!bindingURL) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri = bindingURL->mURI;
  nsCOMPtr<nsIPrincipal> principal = bindingURL->mOriginPrincipal;

  
  PRBool dummy;

  nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));
  NS_ENSURE_TRUE(xblService, NS_ERROR_NOT_AVAILABLE);

  nsRefPtr<nsXBLBinding> binding;
  xblService->LoadBindings(content, uri, principal, PR_FALSE,
                           getter_AddRefs(binding), &dummy);
  
  if (binding) {
    if (nsContentUtils::IsSafeToRunScript()) {
      binding->ExecuteAttachedHandler();
    }
    else {
      nsContentUtils::AddScriptRunner(new nsRunnableMethod<nsXBLBinding>(
        binding, &nsXBLBinding::ExecuteAttachedHandler));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsElementSH::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, PRBool *_retval)
{
  
  nsCOMPtr<nsIContent> content(do_QueryWrappedNative(wrapper, obj));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);

  nsIDocument* doc = content->GetOwnerDoc();
  if (!doc) {
    
    return NS_OK;
  }

  nsRefPtr<nsXBLBinding> binding = doc->BindingManager()->GetBinding(content);
  if (!binding) {
    
    return NS_OK;
  }

  *_retval = binding->ResolveAllFields(cx, obj);
  
  return NS_OK;
}
  



NS_IMETHODIMP
nsGenericArraySH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, PRUint32 flags,
                             JSObject **objp, PRBool *_retval)
{
  if (id == sLength_id) {
    
    return NS_OK;
  }
  
  PRBool is_number = PR_FALSE;
  PRInt32 n = GetArrayIndexFromId(cx, id, &is_number);

  if (is_number && n >= 0) {
    
    
    
    

    PRUint32 length;
    nsresult rv = GetLength(wrapper, cx, obj, &length);
    NS_ENSURE_SUCCESS(rv, rv);

    if ((PRUint32)n < length) {
      *_retval = ::JS_DefineElement(cx, obj, n, JSVAL_VOID, nsnull, nsnull,
                                    JSPROP_ENUMERATE | JSPROP_SHARED);
      *objp = obj;
    }
  }

  return NS_OK;
}

nsresult
nsGenericArraySH::GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, PRUint32 *length)
{
  *length = 0;

  jsval lenval;
  if (!JS_GetProperty(cx, obj, "length", &lenval)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!JSVAL_IS_INT(lenval)) {
    
    
    return NS_OK;
  }

  PRInt32 slen = JSVAL_TO_INT(lenval);
  if (slen < 0) {
    return NS_OK;
  }

  *length = (PRUint32)slen;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericArraySH::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, PRBool *_retval)
{
  
  
  

  static PRBool sCurrentlyEnumerating;

  if (sCurrentlyEnumerating) {
    
    return NS_OK;
  }

  sCurrentlyEnumerating = PR_TRUE;

  jsval len_val;
  JSAutoRequest ar(cx);
  JSBool ok = ::JS_GetProperty(cx, obj, "length", &len_val);

  if (ok && JSVAL_IS_INT(len_val)) {
    PRInt32 length = JSVAL_TO_INT(len_val);

    for (PRInt32 i = 0; ok && i < length; ++i) {
      ok = ::JS_DefineElement(cx, obj, i, JSVAL_VOID, nsnull, nsnull,
                              JSPROP_ENUMERATE | JSPROP_SHARED);
    }
  }

  sCurrentlyEnumerating = PR_FALSE;

  return ok ? NS_OK : NS_ERROR_UNEXPECTED;
}



NS_IMETHODIMP
nsArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  PRBool is_number = PR_FALSE;
  PRInt32 n = GetArrayIndexFromId(cx, id, &is_number);

  nsresult rv = NS_OK;

  if (is_number) {
    if (n < 0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    
    
    rv = NS_OK;
    nsISupports* array_item = GetItemAt(GetNative(wrapper, obj), n, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (array_item) {
      rv = WrapNative(cx, obj, array_item, PR_TRUE, vp);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_SUCCESS_I_DID_SOMETHING;
    }
  }

  return rv;
}




nsresult
nsNodeListSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                        JSObject *globalObj, JSObject **parentObj)
{
  nsWrapperCache* cache = nsnull;
  CallQueryInterface(nativeObj, &cache);
  if (!cache) {
    *parentObj = globalObj;
    return NS_OK;
  }

  
  
  nsChildContentList *list = nsChildContentList::FromSupports(nativeObj);
  nsISupports *native_parent = list->GetParentObject();
  if (!native_parent) {
    return NS_ERROR_FAILURE;
  }

  jsval v;
  nsresult rv = WrapNative(cx, globalObj, native_parent, PR_FALSE, &v);
  NS_ENSURE_SUCCESS(rv, rv);

  *parentObj = JSVAL_TO_OBJECT(v);

  return NS_SUCCESS_ALLOW_SLIM_WRAPPERS;
}

nsresult
nsNodeListSH::GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, PRUint32 *length)
{
  nsINodeList* list = static_cast<nsINodeList*>(GetNative(wrapper, obj));
#ifdef DEBUG
  {
    nsCOMPtr<nsINodeList> list_qi = do_QueryWrappedNative(wrapper, obj);

    
    
    
    NS_ASSERTION(list_qi == list, "Uh, fix QI!");
  }
#endif

  return list->GetLength(length);
}

nsISupports*
nsNodeListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                        nsresult *aResult)
{
  nsINodeList* list = static_cast<nsINodeList*>(aNative);
#ifdef DEBUG
  {
    nsCOMPtr<nsINodeList> list_qi = do_QueryInterface(aNative);

    
    
    
    NS_ASSERTION(list_qi == list, "Uh, fix QI!");
  }
#endif

  return list->GetNodeAt(aIndex);
}




nsresult
nsStringListSH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                            nsAString& aResult)
{
  nsCOMPtr<nsIDOMDOMStringList> list(do_QueryInterface(aNative));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  return list->Item(aIndex, aResult);
}




nsresult
nsDOMTokenListSH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                              nsAString& aResult)
{
  nsCOMPtr<nsIDOMDOMTokenList> list(do_QueryInterface(aNative));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  return list->Item(aIndex, aResult);
}




NS_IMETHODIMP
nsNamedArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, jsval *vp,
                            PRBool *_retval)
{
  if (JSVAL_IS_STRING(id) && !ObjectIsNativeWrapper(cx, obj)) {
    nsresult rv = NS_OK;
    nsISupports* item = GetNamedItem(GetNative(wrapper, obj),
                                     nsDependentJSString(id),
                                     &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (item) {
      rv = WrapNative(cx, obj, item, PR_TRUE, vp);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_SUCCESS_I_DID_SOMETHING;
    }

    
    return rv;
  }

  return nsArraySH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}




nsISupports*
nsNamedNodeMapSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                            nsresult *aResult)
{
  nsDOMAttributeMap* map = nsDOMAttributeMap::FromSupports(aNative);

  return map->GetItemAt(aIndex, aResult);
}

nsISupports*
nsNamedNodeMapSH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                               nsresult *aResult)
{
  nsDOMAttributeMap* map = nsDOMAttributeMap::FromSupports(aNative);

  return map->GetNamedItem(aName, aResult);
}




nsresult
nsHTMLCollectionSH::GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                              JSObject *obj, PRUint32 *length)
{
  nsIHTMLCollection* collection =
    static_cast<nsIHTMLCollection*>(GetNative(wrapper, obj));
#ifdef DEBUG
  {
    nsCOMPtr<nsIHTMLCollection> collection_qi =
      do_QueryWrappedNative(wrapper, obj);

    
    
    
    NS_ASSERTION(collection_qi == collection, "Uh, fix QI!");
  }
#endif

  return collection->GetLength(length);
}

nsISupports*
nsHTMLCollectionSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                              nsresult *aResult)
{
  nsIHTMLCollection* collection = static_cast<nsIHTMLCollection*>(aNative);
#ifdef DEBUG
  {
    nsCOMPtr<nsIHTMLCollection> collection_qi = do_QueryInterface(aNative);

    
    
    
    NS_ASSERTION(collection_qi == collection, "Uh, fix QI!");
  }
#endif

  return collection->GetNodeAt(aIndex, aResult);
}

nsISupports*
nsHTMLCollectionSH::GetNamedItem(nsISupports *aNative,
                                 const nsAString& aName,
                                 nsresult *aResult)
{
  nsIHTMLCollection* collection = static_cast<nsIHTMLCollection*>(aNative);
#ifdef DEBUG
  {
    nsCOMPtr<nsIHTMLCollection> collection_qi = do_QueryInterface(aNative);

    
    
    
    NS_ASSERTION(collection_qi == collection, "Uh, fix QI!");
  }
#endif

  return collection->GetNamedItem(aName, aResult);
}



nsresult
nsContentListSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                           JSObject *globalObj, JSObject **parentObj)
{
  nsContentList *contentList = nsContentList::FromSupports(nativeObj);
  nsISupports *native_parent = contentList->GetParentObject();

  if (!native_parent) {
    return NS_ERROR_FAILURE;
  }

  jsval v;
  nsresult rv = WrapNative(cx, globalObj, native_parent, PR_FALSE, &v);
  NS_ENSURE_SUCCESS(rv, rv);

  *parentObj = JSVAL_TO_OBJECT(v);

  return NS_SUCCESS_ALLOW_SLIM_WRAPPERS;
}

nsresult
nsContentListSH::GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, PRUint32 *length)
{
  nsContentList *list =
    nsContentList::FromSupports(GetNative(wrapper, obj));

  return list->GetLength(length);
}

nsISupports*
nsContentListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                           nsresult *aResult)
{
  nsContentList *list = nsContentList::FromSupports(aNative);

  return list->GetNodeAt(aIndex, aResult);
}

nsISupports*
nsContentListSH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                              nsresult *aResult)
{
  nsContentList *list = nsContentList::FromSupports(aNative);

  return list->GetNamedItem(aName, aResult);
}

NS_IMETHODIMP
nsDocumentSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, PRUint32 flags,
                         JSObject **objp, PRBool *_retval)
{
  nsresult rv;

  if (id == sLocation_id) {
    
    

    nsCOMPtr<nsIDOMNSDocument> doc(do_QueryWrappedNative(wrapper, obj));
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMLocation> location;
    rv = doc->GetLocation(getter_AddRefs(location));
    NS_ENSURE_SUCCESS(rv, rv);

    jsval v;

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = WrapNative(cx, obj, location, &NS_GET_IID(nsIDOMLocation), PR_TRUE,
                    &v, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    JSAutoRequest ar(cx);

    JSString *str = JSVAL_TO_STRING(id);
    JSBool ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                      ::JS_GetStringLength(str), v, nsnull,
                                      nsnull, JSPROP_ENUMERATE);

    if (!ok) {
      return NS_ERROR_FAILURE;
    }

    *objp = obj;

    return NS_OK;
  }

  if (id == sDocumentURIObject_id && IsPrivilegedScript()) {
    return DefineVoidProp(cx, obj, id, objp);
  } 

  return nsNodeSH::NewResolve(wrapper, cx, obj, id, flags, objp, _retval);
}

NS_IMETHODIMP
nsDocumentSH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (id == sDocumentURIObject_id && IsPrivilegedScript()) {
    nsCOMPtr<nsIDocument> doc = do_QueryWrappedNative(wrapper);
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    nsIURI* uri = doc->GetDocumentURI();
    NS_ENSURE_TRUE(uri, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, uri, &NS_GET_IID(nsIURI), PR_TRUE, vp,
                             getter_AddRefs(holder));

    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }

  return nsNodeSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsDocumentSH::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  if (id == sLocation_id) {
    nsCOMPtr<nsIDOMNSDocument> doc(do_QueryWrappedNative(wrapper));
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMLocation> location;

    nsresult rv = doc->GetLocation(getter_AddRefs(location));
    NS_ENSURE_SUCCESS(rv, rv);

    if (location) {
      JSAutoRequest ar(cx);

      JSString *val = ::JS_ValueToString(cx, *vp);
      NS_ENSURE_TRUE(val, NS_ERROR_UNEXPECTED);

      rv = location->SetHref(nsDependentJSString(val));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      rv = WrapNative(cx, obj, location, &NS_GET_IID(nsIDOMLocation), PR_TRUE,
                      vp, getter_AddRefs(holder));
      return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
    }
  }

  if (id == sDocumentURIObject_id && IsPrivilegedScript()) {
    
    
    
    
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }
  
  return nsNodeSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsDocumentSH::GetFlags(PRUint32* aFlags)
{
  *aFlags = DOMCLASSINFO_STANDARD_FLAGS;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentSH::PostCreate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj)
{
  
  
  
  
  nsCOMPtr<nsIDocument> doc = do_QueryWrappedNative(wrapper);
  if (!doc) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIScriptGlobalObject *sgo = doc->GetScriptGlobalObject();
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(sgo);
  if (!win) {
    
    return NS_OK;
  }

  nsIDOMDocument* currentDoc = win->GetExtantDocument();

  if (SameCOMIdentity(doc, currentDoc)) {
    jsval winVal;

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, win, &NS_GET_IID(nsIDOMWindow), PR_FALSE,
                             &winVal, getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_NAMED_LITERAL_STRING(doc_str, "document");

    if (!::JS_DefineUCProperty(cx, JSVAL_TO_OBJECT(winVal),
                               reinterpret_cast<const jschar *>
                                               (doc_str.get()),
                               doc_str.Length(), OBJECT_TO_JSVAL(obj),
                               JS_PropertyStub, JS_PropertyStub,
                               JSPROP_READONLY | JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}




nsresult
nsHTMLDocumentSH::ResolveImpl(JSContext *cx,
                              nsIXPConnectWrappedNative *wrapper, jsval id,
                              nsISupports **result)
{
  nsHTMLDocument *doc =
    static_cast<nsHTMLDocument*>(static_cast<nsINode*>
                                 (wrapper->Native()));

  
  
  
  JSString *str = JS_ValueToString(cx, id);
  NS_ENSURE_TRUE(str, NS_ERROR_UNEXPECTED);

  return doc->ResolveName(nsDependentJSString(str), nsnull, result);
}


JSBool
nsHTMLDocumentSH::DocumentOpen(JSContext *cx, JSObject *obj, uintN argc,
                               jsval *argv, jsval *rval)
{
  if (argc > 2) {
    JSObject *global = ::JS_GetGlobalForObject(cx, obj);

    
    

    return ::JS_CallFunctionName(cx, global, "open", argc, argv, rval);
  }

  nsCOMPtr<nsISupports> native = do_QueryWrapper(cx, obj);
  if (!native) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_FAILURE);

    return JS_FALSE;
  }

  nsCOMPtr<nsIDOMNSHTMLDocument> doc = do_QueryInterface(native);
  NS_ENSURE_TRUE(doc, JS_FALSE);

  nsCAutoString contentType("text/html");
  if (argc > 0) {
    JSString* jsstr = JS_ValueToString(cx, argv[0]);
    if (!jsstr) {
      nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_OUT_OF_MEMORY);
      return JS_FALSE;
    }
    nsAutoString type;
    type.Assign(nsDependentJSString(jsstr));
    ToLowerCase(type);
    nsCAutoString actualType, dummy;
    NS_ParseContentType(NS_ConvertUTF16toUTF8(type), actualType, dummy);
    if (!actualType.EqualsLiteral("text/html") &&
        !type.EqualsLiteral("replace")) {
      contentType = "text/plain";
    }
  }
  
  PRBool replace = PR_FALSE;
  if (argc > 1) {
    JSString* jsstr = JS_ValueToString(cx, argv[1]);
    if (!jsstr) {
      nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_OUT_OF_MEMORY);
      return JS_FALSE;
    }

    replace = NS_LITERAL_STRING("replace").
      Equals(reinterpret_cast<const PRUnichar*>
                             (::JS_GetStringChars(jsstr)));
  }

  nsCOMPtr<nsIDOMDocument> retval;
  nsresult rv = doc->Open(contentType, replace, getter_AddRefs(retval));
  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);

    return JS_FALSE;
  }

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  rv = WrapNative(cx, obj, retval, &NS_GET_IID(nsIDOMDocument), PR_FALSE, rval,
                  getter_AddRefs(holder));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to wrap native!");

  return NS_SUCCEEDED(rv);
}


static JSClass sHTMLDocumentAllClass = {
  "HTML document.all class",
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_NEW_RESOLVE |
  JSCLASS_HAS_RESERVED_SLOTS(1),
  JS_PropertyStub, JS_PropertyStub, nsHTMLDocumentSH::DocumentAllGetProperty,
  JS_PropertyStub, JS_EnumerateStub,
  (JSResolveOp)nsHTMLDocumentSH::DocumentAllNewResolve, JS_ConvertStub,
  nsHTMLDocumentSH::ReleaseDocument, nsnull, nsnull,
  nsHTMLDocumentSH::CallToGetPropMapper
};


static JSClass sHTMLDocumentAllHelperClass = {
  "HTML document.all helper class", JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,
  JS_PropertyStub, JS_PropertyStub,
  nsHTMLDocumentSH::DocumentAllHelperGetProperty,
  JS_PropertyStub, JS_EnumerateStub,
  (JSResolveOp)nsHTMLDocumentSH::DocumentAllHelperNewResolve, JS_ConvertStub,
  nsnull
};


static JSClass sHTMLDocumentAllTagsClass = {
  "HTML document.all.tags class",
  JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_PRIVATE_IS_NSISUPPORTS,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, (JSResolveOp)nsHTMLDocumentSH::DocumentAllTagsNewResolve,
  JS_ConvertStub, nsHTMLDocumentSH::ReleaseDocument, nsnull, nsnull,
  nsHTMLDocumentSH::CallToGetPropMapper
};


JSBool
nsHTMLDocumentSH::GetDocumentAllNodeList(JSContext *cx, JSObject *obj,
                                         nsIDOMDocument *domdoc,
                                         nsIDOMNodeList **nodeList)
{
  
  
  
  
  
  
  jsval collection;
  nsresult rv = NS_OK;

  if (!JS_GetReservedSlot(cx, obj, 0, &collection)) {
    return JS_FALSE;
  }

  if (!JSVAL_IS_PRIMITIVE(collection)) {
    

    nsISupports *native =
      sXPConnect->GetNativeOfWrapper(cx, JSVAL_TO_OBJECT(collection));
    if (native) {
      CallQueryInterface(native, nodeList);
    }
    else {
      rv = NS_ERROR_FAILURE;
    }
  } else {
    

    rv |= domdoc->GetElementsByTagName(NS_LITERAL_STRING("*"), nodeList);

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv |= nsDOMClassInfo::WrapNative(cx, obj, *nodeList, PR_FALSE, &collection,
                                     getter_AddRefs(holder));

    
    if (!JS_SetReservedSlot(cx, obj, 0, collection)) {
      return JS_FALSE;
    }
  }

  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);

    return JS_FALSE;
  }

  return *nodeList != nsnull;
}

JSBool
nsHTMLDocumentSH::DocumentAllGetProperty(JSContext *cx, JSObject *obj,
                                         jsval id, jsval *vp)
{
  
  
  
  
  if (id == sItem_id || id == sNamedItem_id) {
    return JS_TRUE;
  }

  while (STOBJ_GET_CLASS(obj) != &sHTMLDocumentAllClass) {
    obj = STOBJ_GET_PROTO(obj);

    if (!obj) {
      NS_ERROR("The JS engine lies!");

      return JS_TRUE;
    }
  }

  nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, obj);
  nsCOMPtr<nsIDOMHTMLDocument> domdoc(do_QueryInterface(doc));
  nsCOMPtr<nsISupports> result;
  nsresult rv = NS_OK;

  if (JSVAL_IS_STRING(id)) {
    if (id == sLength_id) {
      
      
      

      nsCOMPtr<nsIDOMNodeList> nodeList;
      if (!GetDocumentAllNodeList(cx, obj, domdoc, getter_AddRefs(nodeList))) {
        return JS_FALSE;
      }

      PRUint32 length;
      rv = nodeList->GetLength(&length);

      if (NS_FAILED(rv)) {
        nsDOMClassInfo::ThrowJSException(cx, rv);

        return JS_FALSE;
      }

      *vp = INT_TO_JSVAL(length);

      return JS_TRUE;
    } else if (id != sTags_id) {
      

      nsDependentJSString str(id);

      rv = doc->GetDocumentAllResult(str, getter_AddRefs(result));

      if (NS_FAILED(rv)) {
        nsDOMClassInfo::ThrowJSException(cx, rv);

        return JS_FALSE;
      }
    }
  } else if (JSVAL_IS_INT(id) && JSVAL_TO_INT(id) >= 0) {
    
    

    nsCOMPtr<nsIDOMNodeList> nodeList;
    if (!GetDocumentAllNodeList(cx, obj, domdoc, getter_AddRefs(nodeList))) {
      return JS_FALSE;
    }

    nsCOMPtr<nsIDOMNode> node;
    nodeList->Item(JSVAL_TO_INT(id), getter_AddRefs(node));

    result = node;
  }

  if (result) {
    rv = WrapNative(cx, obj, result, PR_TRUE, vp);
    if (NS_FAILED(rv)) {
      nsDOMClassInfo::ThrowJSException(cx, rv);

      return JS_FALSE;
    }
  } else {
    *vp = JSVAL_VOID;
  }

  return JS_TRUE;
}

JSBool
nsHTMLDocumentSH::DocumentAllNewResolve(JSContext *cx, JSObject *obj, jsval id,
                                        uintN flags, JSObject **objp)
{
  if (flags & JSRESOLVE_ASSIGNING) {
    

    return JS_TRUE;
  }

  jsval v = JSVAL_VOID;

  if (id == sItem_id || id == sNamedItem_id) {
    

    JSFunction *fnc =
      ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(JSVAL_TO_STRING(id)),
                          CallToGetPropMapper, 0, JSPROP_ENUMERATE);

    *objp = obj;

    return fnc != nsnull;
  }

  if (id == sLength_id) {
    
    
    
    

    v = JSVAL_ONE;
  } else if (id == sTags_id) {
    nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, obj);

    JSObject *tags = ::JS_NewObject(cx, &sHTMLDocumentAllTagsClass, nsnull,
                                    ::JS_GetGlobalForObject(cx, obj));
    if (!tags) {
      return JS_FALSE;
    }

    if (!::JS_SetPrivate(cx, tags, doc)) {
      return JS_FALSE;
    }

    
    NS_ADDREF(doc);

    v = OBJECT_TO_JSVAL(tags);
  } else {
    if (!DocumentAllGetProperty(cx, obj, id, &v)) {
      return JS_FALSE;
    }
  }

  JSBool ok = JS_TRUE;

  if (v != JSVAL_VOID) {
    if (JSVAL_IS_STRING(id)) {
      JSString *str = JSVAL_TO_STRING(id);

      ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), v, nsnull, nsnull,
                                 0);
    } else {
      ok = ::JS_DefineElement(cx, obj, JSVAL_TO_INT(id), v, nsnull, nsnull, 0);
    }

    *objp = obj;
  }

  return ok;
}




void
nsHTMLDocumentSH::ReleaseDocument(JSContext *cx, JSObject *obj)
{
  nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, obj);

  NS_IF_RELEASE(doc);
}

JSBool
nsHTMLDocumentSH::CallToGetPropMapper(JSContext *cx, JSObject *obj, uintN argc,
                                      jsval *argv, jsval *rval)
{
  

  if (argc != 1) {
    
    
    
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_INVALID_ARG);

    return JS_FALSE;
  }

  
  JSString *str = ::JS_ValueToString(cx, argv[0]);
  if (!str) {
    return JS_FALSE;
  }

  JSObject *self;

  if (::JS_TypeOfValue(cx, argv[-2]) == JSTYPE_FUNCTION) {
    
    
    

    self = obj;
  } else {
    
    

    self = JSVAL_TO_OBJECT(argv[-2]);
  }

  return ::JS_GetUCProperty(cx, self, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), rval);
}


static inline JSObject *
GetDocumentAllHelper(JSContext *cx, JSObject *obj)
{
  while (obj && JS_GET_CLASS(cx, obj) != &sHTMLDocumentAllHelperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  return obj;
}

JSBool
nsHTMLDocumentSH::DocumentAllHelperGetProperty(JSContext *cx, JSObject *obj,
                                               jsval id, jsval *vp)
{
  if (id != nsDOMClassInfo::sAll_id) {
    return JS_TRUE;
  }

  JSObject *helper = GetDocumentAllHelper(cx, obj);

  if (!helper) {
    NS_ERROR("Uh, how'd we get here?");

    

    return JS_TRUE;
  }

  PRUint32 flags = JSVAL_TO_INT(PRIVATE_TO_JSVAL(::JS_GetPrivate(cx, helper)));

  if (flags & JSRESOLVE_DETECTING || !(flags & JSRESOLVE_QUALIFIED)) {
    
    
    

    *vp = JSVAL_VOID;
  } else {
    
    

    if (!JSVAL_IS_OBJECT(*vp)) {
      
      
      nsresult rv;
      nsCOMPtr<nsIHTMLDocument> doc = do_QueryWrapper(cx, obj, &rv);
      if (NS_FAILED(rv)) {
        nsDOMClassInfo::ThrowJSException(cx, rv);

        return JS_FALSE;
      }

      JSObject *all = ::JS_NewObject(cx, &sHTMLDocumentAllClass, nsnull,
                                     ::JS_GetGlobalForObject(cx, obj));
      if (!all) {
        return JS_FALSE;
      }

      
      if (!::JS_SetPrivate(cx, all, doc)) {
        return JS_FALSE;
      }

      doc.forget();

      *vp = OBJECT_TO_JSVAL(all);
    }
  }

  return JS_TRUE;
}

JSBool
nsHTMLDocumentSH::DocumentAllHelperNewResolve(JSContext *cx, JSObject *obj,
                                              jsval id, uintN flags,
                                              JSObject **objp)
{
  if (id == nsDOMClassInfo::sAll_id) {
    
    JSObject *helper = GetDocumentAllHelper(cx, obj);

    if (helper) {
      if (!::JS_DefineProperty(cx, helper, "all", JSVAL_VOID, nsnull, nsnull,
                               JSPROP_ENUMERATE)) {
        return JS_FALSE;
      }

      *objp = helper;
    }
  }

  return JS_TRUE;
}


JSBool
nsHTMLDocumentSH::DocumentAllTagsNewResolve(JSContext *cx, JSObject *obj,
                                            jsval id, uintN flags,
                                            JSObject **objp)
{
  if (JSVAL_IS_STRING(id)) {
    nsIHTMLDocument *doc = (nsIHTMLDocument *)::JS_GetPrivate(cx, obj);

    JSString *str = JSVAL_TO_STRING(id);

    JSObject *proto = ::JS_GetPrototype(cx, obj);
    if (NS_UNLIKELY(!proto)) {
      return JS_TRUE;
    }

    JSBool found;
    if (!::JS_HasUCProperty(cx, proto,
                            ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), &found)) {
      return JS_FALSE;
    }

    if (found) {
      return JS_TRUE;
    }

    nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(doc));

    nsCOMPtr<nsIDOMNodeList> tags;
    domdoc->GetElementsByTagName(nsDependentJSString(str),
                                 getter_AddRefs(tags));

    if (tags) {
      jsval v;
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      nsresult rv = nsDOMClassInfo::WrapNative(cx, obj, tags, PR_TRUE, &v,
                                               getter_AddRefs(holder));
      if (NS_FAILED(rv)) {
        nsDOMClassInfo::ThrowJSException(cx, rv);

        return JS_FALSE;
      }

      if (!::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), v, nsnull, nsnull,
                                 0)) {
        return JS_FALSE;
      }

      *objp = obj;
    }
  }

  return JS_TRUE;
}


NS_IMETHODIMP
nsHTMLDocumentSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, PRUint32 flags,
                             JSObject **objp, PRBool *_retval)
{
  
  
  
  
  

  if (!(flags & JSRESOLVE_ASSIGNING)) {
    

    JSAutoRequest ar(cx);

    if (!ObjectIsNativeWrapper(cx, obj)) {
      nsCOMPtr<nsISupports> result;

      nsresult rv = ResolveImpl(cx, wrapper, id, getter_AddRefs(result));
      NS_ENSURE_SUCCESS(rv, rv);

      if (result) {
        JSString *str = JS_ValueToString(cx, id);

        JSBool ok = *_retval =
          ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                ::JS_GetStringLength(str), JSVAL_VOID, nsnull,
                                nsnull, 0);
        *objp = obj;

        return ok ? NS_OK : NS_ERROR_FAILURE;
      }
    }

    if (id == sOpen_id) {
      JSString *str = JSVAL_TO_STRING(id);
      JSFunction *fnc =
        ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                            DocumentOpen, 0, JSPROP_ENUMERATE);

      *objp = obj;

      return fnc ? NS_OK : NS_ERROR_UNEXPECTED;
    }

    if (id == sAll_id && !sDisableDocumentAllSupport &&
        !ObjectIsNativeWrapper(cx, obj)) {
      nsIDocument *doc = static_cast<nsIDocument*>(wrapper->Native());

      if (doc->GetCompatibilityMode() == eCompatibility_NavQuirks) {
        JSObject *helper =
          GetDocumentAllHelper(cx, ::JS_GetPrototype(cx, obj));

        JSObject *proto = ::JS_GetPrototype(cx, helper ? helper : obj);

        
        
        

        JSBool hasAll = JS_FALSE;
        if (proto && !JS_HasProperty(cx, proto, "all", &hasAll)) {
          return NS_ERROR_UNEXPECTED;
        }

        if (hasAll && helper) {
          
          
          
          JSObject *tmp = obj, *tmpProto;

          while ((tmpProto = ::JS_GetPrototype(cx, tmp)) != helper) {
            tmp = tmpProto;
          }

          ::JS_SetPrototype(cx, tmp, proto);
        }

        
        
        
        
        if (!helper && flags & JSRESOLVE_QUALIFIED &&
            !(flags & JSRESOLVE_DETECTING) && !hasAll) {
          
          PrintWarningOnConsole(cx, "DocumentAllUsed");

          helper = ::JS_NewObject(cx, &sHTMLDocumentAllHelperClass,
                                  ::JS_GetPrototype(cx, obj),
                                  ::JS_GetGlobalForObject(cx, obj));

          if (!helper) {
            return NS_ERROR_OUT_OF_MEMORY;
          }

          
          
          if (!::JS_SetPrototype(cx, obj, helper)) {
            nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_UNEXPECTED);

            return NS_ERROR_UNEXPECTED;
          }
        }

        
        
        if (helper &&
            !::JS_SetPrivate(cx, helper,
                             JSVAL_TO_PRIVATE(INT_TO_JSVAL(flags)))) {
          nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_UNEXPECTED);

          return NS_ERROR_UNEXPECTED;
        }
      }

      return NS_OK;
    }
  }

  return nsDocumentSH::NewResolve(wrapper, cx, obj, id, flags, objp, _retval);
}

NS_IMETHODIMP
nsHTMLDocumentSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                              JSContext *cx, JSObject *obj, jsval id,
                              jsval *vp, PRBool *_retval)
{
  
  if (!ObjectIsNativeWrapper(cx, obj)) {
    nsCOMPtr<nsISupports> result;

    JSAutoRequest ar(cx);

    nsresult rv = ResolveImpl(cx, wrapper, id, getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    if (result) {
      rv = WrapNative(cx, obj, result, PR_TRUE, vp);
      if (NS_SUCCEEDED(rv)) {
        rv = NS_SUCCESS_I_DID_SOMETHING;
      }
      return rv;
    }
  }

  return nsDocumentSH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}




JSBool
nsHTMLElementSH::ScrollIntoView(JSContext *cx, JSObject *obj, uintN argc,
                                jsval *argv, jsval *rval)
{
  nsresult rv;
  nsCOMPtr<nsIDOMNSHTMLElement> element = do_QueryWrapper(cx, obj, &rv);
  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);

    return JS_FALSE;
  }

  JSBool top = JS_TRUE;

  if (argc > 0) {
    ::JS_ValueToBoolean(cx, argv[0], &top);
  }

  rv = element->ScrollIntoView(top);

  *rval = JSVAL_VOID;

  return NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
nsHTMLElementSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsval id, PRUint32 flags,
                            JSObject **objp, PRBool *_retval)
{
  if (id == sScrollIntoView_id && !(JSRESOLVE_ASSIGNING & flags)) {
    JSString *str = JSVAL_TO_STRING(id);
    JSAutoRequest ar(cx);
    JSFunction *cfnc =
      ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str), ScrollIntoView,
                          0, 0);

    *objp = obj;

    return cfnc ? NS_OK : NS_ERROR_UNEXPECTED;
  }

  return nsElementSH::NewResolve(wrapper, cx, obj, id, flags, objp, _retval);
}




nsresult
nsHTMLFormElementSH::FindNamedItem(nsIForm *aForm, JSString *str,
                                   nsISupports **aResult)
{
  nsDependentJSString name(str);

  *aResult = aForm->ResolveName(name).get();

  if (!*aResult) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aForm));
    nsCOMPtr<nsIDOMHTMLFormElement> form_element(do_QueryInterface(aForm));

    nsCOMPtr<nsIHTMLDocument> html_doc =
      do_QueryInterface(content->GetDocument());

    if (html_doc && form_element) {
      html_doc->ResolveName(name, form_element, aResult);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFormElementSH::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                JSContext *cx, JSObject *obj, jsval id,
                                PRUint32 flags, JSObject **objp,
                                PRBool *_retval)
{
  
  if ((!(JSRESOLVE_ASSIGNING & flags)) && JSVAL_IS_STRING(id) &&
      !ObjectIsNativeWrapper(cx, obj)) {
    nsCOMPtr<nsIForm> form(do_QueryWrappedNative(wrapper, obj));
    nsCOMPtr<nsISupports> result;

    JSString *str = JSVAL_TO_STRING(id);
    FindNamedItem(form, str, getter_AddRefs(result));

    if (result) {
      JSAutoRequest ar(cx);
      *_retval = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                       ::JS_GetStringLength(str),
                                       JSVAL_VOID, nsnull, nsnull, 0);

      *objp = obj;

      return *_retval ? NS_OK : NS_ERROR_FAILURE;
    }
  }

  return nsHTMLElementSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                     _retval);
}


NS_IMETHODIMP
nsHTMLFormElementSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                 JSContext *cx, JSObject *obj, jsval id,
                                 jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIForm> form(do_QueryWrappedNative(wrapper, obj));

  if (JSVAL_IS_STRING(id)) {
    
    if (!ObjectIsNativeWrapper(cx, obj)) {
      nsCOMPtr<nsISupports> result;

      JSString *str = JSVAL_TO_STRING(id);
      FindNamedItem(form, str, getter_AddRefs(result));

      if (result) {
        
        
        nsresult rv = WrapNative(cx, obj, result, PR_TRUE, vp);
        return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
      }
    }
  } else {
    PRInt32 n = GetArrayIndexFromId(cx, id);

    if (n >= 0) {
      nsCOMPtr<nsIFormControl> control;
      form->GetElementAt(n, getter_AddRefs(control));

      if (control) {
        nsresult rv = WrapNative(cx, obj, control, PR_TRUE, vp);
        return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
      }
    }
  }

  return nsHTMLElementSH::GetProperty(wrapper, cx, obj, id, vp, _retval);;
}

NS_IMETHODIMP
nsHTMLFormElementSH::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                                  JSContext *cx, JSObject *obj,
                                  PRUint32 enum_op, jsval *statep,
                                  jsid *idp, PRBool *_retval)
{
  switch (enum_op) {
  case JSENUMERATE_INIT:
    {
      nsCOMPtr<nsIForm> form(do_QueryWrappedNative(wrapper, obj));

      if (!form) {
        *statep = JSVAL_NULL;
        return NS_ERROR_UNEXPECTED;
      }

      *statep = INT_TO_JSVAL(0);

      if (idp) {
        PRUint32 count = form->GetElementCount();

        *idp = INT_TO_JSVAL(count);
      }

      break;
    }
  case JSENUMERATE_NEXT:
    {
      nsCOMPtr<nsIForm> form(do_QueryWrappedNative(wrapper, obj));
      NS_ENSURE_TRUE(form, NS_ERROR_FAILURE);

      PRInt32 index = (PRInt32)JSVAL_TO_INT(*statep);

      PRUint32 count = form->GetElementCount();

      if ((PRUint32)index < count) {
        nsCOMPtr<nsIFormControl> controlNode;
        form->GetElementAt(index, getter_AddRefs(controlNode));
        NS_ENSURE_TRUE(controlNode, NS_ERROR_FAILURE);

        nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(controlNode);
        NS_ENSURE_TRUE(domElement, NS_ERROR_FAILURE);

        nsAutoString attr;
        domElement->GetAttribute(NS_LITERAL_STRING("name"), attr);
        if (attr.IsEmpty()) {
          
          attr.AppendInt(index);
        }

        JSAutoRequest ar(cx);

        JSString *jsname =
          JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                                  (attr.get()),
                              attr.Length());
        NS_ENSURE_TRUE(jsname, NS_ERROR_OUT_OF_MEMORY);

        JS_ValueToId(cx, STRING_TO_JSVAL(jsname), idp);

        *statep = INT_TO_JSVAL(++index);
      } else {
        *statep = JSVAL_NULL;
      }

      break;
    }
  case JSENUMERATE_DESTROY:
    *statep = JSVAL_NULL;

    break;
  }

  return NS_OK;
}




NS_IMETHODIMP
nsHTMLSelectElementSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                   JSContext *cx, JSObject *obj, jsval id,
                                   jsval *vp, PRBool *_retval)
{
  PRInt32 n = GetArrayIndexFromId(cx, id);

  nsresult rv = NS_OK;
  if (n >= 0) {
    nsCOMPtr<nsIDOMHTMLSelectElement> s = do_QueryWrappedNative(wrapper, obj);

    nsCOMPtr<nsIDOMHTMLOptionsCollection> options;
    s->GetOptions(getter_AddRefs(options));

    if (options) {
      nsCOMPtr<nsIDOMNode> node;

      options->Item(n, getter_AddRefs(node));

      rv = WrapNative(cx, obj, node, &NS_GET_IID(nsIDOMNode), PR_TRUE, vp);
      if (NS_SUCCEEDED(rv)) {
        rv = NS_SUCCESS_I_DID_SOMETHING;
      }
      return rv;
    }
  }

  return nsHTMLElementSH::GetProperty(wrapper, cx, obj, id, vp, _retval);;
}


nsresult
nsHTMLSelectElementSH::SetOption(JSContext *cx, jsval *vp, PRUint32 aIndex,
                                 nsIDOMNSHTMLOptionCollection *aOptCollection)
{
  JSAutoRequest ar(cx);

  
  if (!JSVAL_IS_OBJECT(*vp) && !::JS_ConvertValue(cx, *vp, JSTYPE_OBJECT, vp)) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIDOMHTMLOptionElement> new_option;

  if (!JSVAL_IS_NULL(*vp)) {
    new_option = do_QueryWrapper(cx, JSVAL_TO_OBJECT(*vp));
    if (!new_option) {
      

      return NS_ERROR_UNEXPECTED;
    }
  }

  return aOptCollection->SetOption(aIndex, new_option);
}

NS_IMETHODIMP
nsHTMLSelectElementSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                                   JSContext *cx, JSObject *obj, jsval id,
                                   jsval *vp, PRBool *_retval)
{
  PRInt32 n = GetArrayIndexFromId(cx, id);

  if (n >= 0) {
    nsCOMPtr<nsIDOMHTMLSelectElement> select =
      do_QueryWrappedNative(wrapper, obj);
    NS_ENSURE_TRUE(select, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIDOMHTMLOptionsCollection> options;
    select->GetOptions(getter_AddRefs(options));

    nsCOMPtr<nsIDOMNSHTMLOptionCollection> oc(do_QueryInterface(options));
    NS_ENSURE_TRUE(oc, NS_ERROR_UNEXPECTED);

    nsresult rv = SetOption(cx, vp, n, oc);
    return NS_FAILED(rv) ? rv : NS_SUCCESS_I_DID_SOMETHING;
  }

  return nsElementSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}





nsresult
nsHTMLPluginObjElementSH::GetPluginInstanceIfSafe(nsIXPConnectWrappedNative *wrapper,
                                                  JSObject *obj,
                                                  nsIPluginInstance **_result)
{
  *_result = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryWrappedNative(wrapper, obj));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIObjectLoadingContent> objlc(do_QueryInterface(content));
  NS_ASSERTION(objlc, "Object nodes must implement nsIObjectLoadingContent");

  
  
  if (!nsContentUtils::IsSafeToRunScript()) {
    return objlc->GetPluginInstance(_result);
  }

  
  return objlc->EnsureInstantiation(_result);
}



static PRBool
IsObjInProtoChain(JSContext *cx, JSObject *obj, JSObject *proto)
{
  JSObject *o = obj;

  JSAutoRequest ar(cx);

  while (o) {
    JSObject *p = ::JS_GetPrototype(cx, o);

    if (p == proto) {
      return PR_TRUE;
    }

    o = p;
  }

  return PR_FALSE;
}

class nsPluginProtoChainInstallRunner : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  nsPluginProtoChainInstallRunner(nsIXPConnectWrappedNative* wrapper,
                                  nsIScriptContext* scriptContext)
    : mWrapper(wrapper),
      mContext(scriptContext)
  {
  }

  NS_IMETHOD Run()
  {
    JSContext* cx = nsnull;
    if (mContext) {
      cx = (JSContext*)mContext->GetNativeContext();
    } else {
      nsCOMPtr<nsIThreadJSContextStack> stack =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1");
      NS_ENSURE_TRUE(stack, NS_OK);

      stack->GetSafeJSContext(&cx);
      NS_ENSURE_TRUE(cx, NS_OK);
    }

    JSObject* obj = nsnull;
    mWrapper->GetJSObject(&obj);
    NS_ASSERTION(obj, "Should never be null");
    nsHTMLPluginObjElementSH::SetupProtoChain(mWrapper, cx, obj);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIXPConnectWrappedNative> mWrapper;
  nsCOMPtr<nsIScriptContext> mContext;
};

NS_IMPL_ISUPPORTS1(nsPluginProtoChainInstallRunner, nsIRunnable)


nsresult
nsHTMLPluginObjElementSH::SetupProtoChain(nsIXPConnectWrappedNative *wrapper,
                                          JSContext *cx,
                                          JSObject *obj)
{
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Shouldn't have gotten in here");

  nsCxPusher cxPusher;
  if (!cxPusher.Push(cx)) {
    return NS_OK;
  }

  nsCOMPtr<nsIPluginInstance> pi;
  nsresult rv = GetPluginInstanceIfSafe(wrapper, obj, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!pi) {
    

    return NS_OK;
  }

  JSObject *pi_obj = nsnull; 
  JSObject *pi_proto = nsnull; 

  rv = GetPluginJSObject(cx, obj, pi, &pi_obj, &pi_proto);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!pi_obj) {
    

    return NS_OK;
  }

  if (IsObjInProtoChain(cx, obj, pi_obj)) {
    
    
    
    

    return NS_OK;
  }


  
  

  JSObject *my_proto = nsnull;

  
  rv = wrapper->GetJSObjectPrototype(&my_proto);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  
  if (!::JS_SetPrototype(cx, obj, pi_obj)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (pi_proto && JS_GET_CLASS(cx, pi_proto) != sObjectClass) {
    
    
    if (pi_proto != my_proto && !::JS_SetPrototype(cx, pi_proto, my_proto)) {
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    
    
    
    if (!::JS_SetPrototype(cx, pi_obj, my_proto)) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLPluginObjElementSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                                    JSObject *globalObj, JSObject **parentObj)
{
  nsresult rv = nsHTMLElementSH::PreCreate(nativeObj, cx, globalObj, parentObj);

  
  return rv == NS_SUCCESS_ALLOW_SLIM_WRAPPERS ? NS_OK : rv;
}

NS_IMETHODIMP
nsHTMLPluginObjElementSH::PostCreate(nsIXPConnectWrappedNative *wrapper,
                                     JSContext *cx, JSObject *obj)
{
  if (nsContentUtils::IsSafeToRunScript()) {
    nsresult rv = SetupProtoChain(wrapper, cx, obj);

    
    
    
    
    
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetupProtoChain failed!");
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIScriptContext> scriptContext = GetScriptContextFromJSContext(cx);

  nsRefPtr<nsPluginProtoChainInstallRunner> runner =
    new nsPluginProtoChainInstallRunner(wrapper, scriptContext);
  nsContentUtils::AddScriptRunner(runner);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLPluginObjElementSH::GetProperty(nsIXPConnectWrappedNative *wrapper,
                                      JSContext *cx, JSObject *obj, jsval id,
                                      jsval *vp, PRBool *_retval)
{
  JSAutoRequest ar(cx);

  JSObject *pi_obj = ::JS_GetPrototype(cx, obj);
  if (NS_UNLIKELY(!pi_obj)) {
    return NS_OK;
  }

  const jschar *id_chars = nsnull;
  size_t id_length = 0;

  JSBool found = PR_FALSE;

  if (!ObjectIsNativeWrapper(cx, obj)) {
    if (JSVAL_IS_STRING(id)) {
      JSString *id_str = JSVAL_TO_STRING(id);

      id_chars = ::JS_GetStringChars(id_str);
      id_length = ::JS_GetStringLength(id_str);

      *_retval = ::JS_HasUCProperty(cx, pi_obj, id_chars, id_length, &found);
    } else {
      *_retval = JS_HasElement(cx, pi_obj, JSVAL_TO_INT(id), &found);
    }

    if (!*_retval) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  if (found) {
    if (JSVAL_IS_STRING(id)) {
      *_retval = ::JS_GetUCProperty(cx, pi_obj, id_chars, id_length, vp);
    } else {
      *_retval = ::JS_GetElement(cx, pi_obj, JSVAL_TO_INT(id), vp);
    }

    return *_retval ? NS_SUCCESS_I_DID_SOMETHING : NS_ERROR_FAILURE;
  }

  return nsHTMLElementSH::GetProperty(wrapper, cx, obj, id, vp, _retval);;
}

NS_IMETHODIMP
nsHTMLPluginObjElementSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                                      JSContext *cx, JSObject *obj, jsval id,
                                      jsval *vp, PRBool *_retval)
{
  JSAutoRequest ar(cx);

  JSObject *pi_obj = ::JS_GetPrototype(cx, obj);
  if (NS_UNLIKELY(!pi_obj)) {
    return NS_OK;
  }

  const jschar *id_chars = nsnull;
  size_t id_length = 0;

  JSBool found = PR_FALSE;

  if (!ObjectIsNativeWrapper(cx, obj)) {
    if (JSVAL_IS_STRING(id)) {
      JSString *id_str = JSVAL_TO_STRING(id);

      id_chars = ::JS_GetStringChars(id_str);
      id_length = ::JS_GetStringLength(id_str);

      *_retval = ::JS_HasUCProperty(cx, pi_obj, id_chars, id_length, &found);
    } else {
      *_retval = JS_HasElement(cx, pi_obj, JSVAL_TO_INT(id), &found);
    }

    if (!*_retval) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  if (found) {
    if (JSVAL_IS_STRING(id)) {
      *_retval = ::JS_SetUCProperty(cx, pi_obj, id_chars, id_length, vp);
    } else {
      *_retval = ::JS_SetElement(cx, pi_obj, JSVAL_TO_INT(id), vp);
    }

    return *_retval ? NS_SUCCESS_I_DID_SOMETHING : NS_ERROR_FAILURE;
  }

  return nsHTMLElementSH::SetProperty(wrapper, cx, obj, id, vp, _retval);
}

NS_IMETHODIMP
nsHTMLPluginObjElementSH::Call(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj, PRUint32 argc,
                               jsval *argv, jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIPluginInstance> pi;
  nsresult rv = GetPluginInstanceIfSafe(wrapper, obj, getter_AddRefs(pi));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!ObjectIsNativeWrapper(cx, obj) || !pi) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  JSObject *pi_obj = nsnull;
  JSObject *pi_proto = nsnull;

  rv = GetPluginJSObject(cx, obj, pi, &pi_obj, &pi_proto);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!pi) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  JSAutoRequest ar(cx);
  *_retval = ::JS_CallFunctionValue(cx, JSVAL_TO_OBJECT(argv[-1]),
                                    OBJECT_TO_JSVAL(pi_obj), argc, argv, vp);

  return NS_OK;
}


nsresult
nsHTMLPluginObjElementSH::GetPluginJSObject(JSContext *cx, JSObject *obj,
                                            nsIPluginInstance *plugin_inst,
                                            JSObject **plugin_obj,
                                            JSObject **plugin_proto)
{
  *plugin_obj = nsnull;
  *plugin_proto = nsnull;

  JSAutoRequest ar(cx);

  if (plugin_inst) {
    plugin_inst->GetJSObject(cx, plugin_obj);
    if (*plugin_obj) {
      *plugin_proto = ::JS_GetPrototype(cx, *plugin_obj);
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsHTMLOptionsCollectionSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                                       JSContext *cx, JSObject *obj, jsval id,
                                       jsval *vp, PRBool *_retval)
{
  PRInt32 n = GetArrayIndexFromId(cx, id);

  if (n < 0) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNSHTMLOptionCollection> oc =
    do_QueryWrappedNative(wrapper, obj);
  NS_ENSURE_TRUE(oc, NS_ERROR_UNEXPECTED);

  nsresult rv = nsHTMLSelectElementSH::SetOption(cx, vp, n, oc);
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLOptionsCollectionSH::NewResolve(nsIXPConnectWrappedNative *wrapper,
                                      JSContext *cx, JSObject *obj, 
                                      jsval id, PRUint32 flags, 
                                      JSObject **objp, PRBool *_retval)
{
  if (id == sAdd_id) {
    JSString *str = JSVAL_TO_STRING(id);

    JSAutoRequest ar(cx);

    JSFunction *fnc =
      ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                          Add, 0, JSPROP_ENUMERATE);
    
    *objp = obj;
    
    return fnc ? NS_OK : NS_ERROR_UNEXPECTED;
  }

  return nsHTMLCollectionSH::NewResolve(wrapper, cx, obj, id, flags, objp, _retval);
}

JSBool
nsHTMLOptionsCollectionSH::Add(JSContext *cx, JSObject *obj, uintN argc,
                               jsval *argv, jsval *rval)
{
  *rval = JSVAL_VOID;

  nsresult rv;
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = do_QueryWrapper(cx, obj, &rv);
  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);
    return JS_FALSE;
  }

  if (argc < 1) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_XPC_NOT_ENOUGH_ARGS);
    return JS_FALSE;
  }

  if (JSVAL_IS_PRIMITIVE(argv[0])) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_WRONG_TYPE_ERR);
    return JS_FALSE;
  }

  nsCOMPtr<nsIDOMHTMLOptionElement> newOption =
    do_QueryWrapper(cx, JSVAL_TO_OBJECT(argv[0]));
  if (!newOption) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_WRONG_TYPE_ERR);
    return JS_FALSE;
  }

  int32 index = -1;
  if (argc > 1) {
    if (!JS_ValueToInt32(cx, argv[1], &index)) {
      return JS_FALSE;
    }
  }

  if (index < -1) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_INDEX_SIZE_ERR);
    return JS_FALSE;
  }

  PRUint32 length;
  options->GetLength(&length);

  if (index == -1 || index > (int32)length) {
    
    index = length;
  }

  nsCOMPtr<nsIDOMNode> beforeNode;
  options->Item(index, getter_AddRefs(beforeNode));
  
  nsCOMPtr<nsIDOMHTMLOptionElement> beforeElement(do_QueryInterface(beforeNode));

  nsCOMPtr<nsIDOMNSHTMLOptionCollection> nsoptions(do_QueryInterface(options));

  nsCOMPtr<nsIDOMHTMLSelectElement> select;
  nsoptions->GetSelect(getter_AddRefs(select));
                             
  rv = select->Add(newOption, beforeElement);

  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);
  }

  return NS_SUCCEEDED(rv);
}




nsISupports*
nsPluginSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                      nsresult *aResult)
{
  nsPluginElement* plugin = nsPluginElement::FromSupports(aNative);

  return plugin->GetItemAt(aIndex, aResult);
}

nsISupports*
nsPluginSH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                         nsresult *aResult)
{
  nsPluginElement* plugin = nsPluginElement::FromSupports(aNative);

  return plugin->GetNamedItem(aName, aResult);
}




nsISupports*
nsPluginArraySH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                           nsresult *aResult)
{
  nsPluginArray* array = nsPluginArray::FromSupports(aNative);

  return array->GetItemAt(aIndex, aResult);
}

nsISupports*
nsPluginArraySH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                              nsresult *aResult)
{
  nsPluginArray* array = nsPluginArray::FromSupports(aNative);

  return array->GetNamedItem(aName, aResult);
}




nsISupports*
nsMimeTypeArraySH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                             nsresult *aResult)
{
  nsMimeTypeArray* array = nsMimeTypeArray::FromSupports(aNative);

  return array->GetItemAt(aIndex, aResult);
}

nsISupports*
nsMimeTypeArraySH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                                nsresult *aResult)
{
  nsMimeTypeArray* array = nsMimeTypeArray::FromSupports(aNative);

  return array->GetNamedItem(aName, aResult);
}




NS_IMETHODIMP
nsStringArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  PRBool is_number = PR_FALSE;
  PRInt32 n = GetArrayIndexFromId(cx, id, &is_number);

  if (!is_number) {
    return NS_OK;
  }

  nsAutoString val;

  nsresult rv = GetStringAt(GetNative(wrapper, obj), n, val);
  NS_ENSURE_SUCCESS(rv, rv);

  

  JSAutoRequest ar(cx);

  JSString *str =
    ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>(val.get()),
                          val.Length());
  NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

  *vp = STRING_TO_JSVAL(str);

  return NS_SUCCESS_I_DID_SOMETHING;
}




NS_IMETHODIMP
nsHistorySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  PRBool is_number = PR_FALSE;
  GetArrayIndexFromId(cx, id, &is_number);

  if (!is_number) {
    return NS_OK;
  }

  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, obj, mData->mName, sItem_id,
                                 nsIXPCSecurityManager::ACCESS_CALL_METHOD);

  if (NS_FAILED(rv)) {
    
    *_retval = PR_FALSE;

    return NS_OK;
  }

  

  return nsStringArraySH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

nsresult
nsHistorySH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                         nsAString& aResult)
{
  if (aIndex < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHistory> history(do_QueryInterface(aNative));

  return history->Item(aIndex, aResult);
}




nsresult
nsMediaListSH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                           nsAString& aResult)
{
  if (aIndex < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMMediaList> media_list(do_QueryInterface(aNative));

  return media_list->Item(PRUint32(aIndex), aResult);
}




nsISupports*
nsStyleSheetListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                              nsresult *rv)
{
  nsDOMStyleSheetList* list = nsDOMStyleSheetList::FromSupports(aNative);

  return list->GetItemAt(aIndex);
}




nsISupports*
nsCSSValueListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                            nsresult *aResult)
{
  nsDOMCSSValueList* list = nsDOMCSSValueList::FromSupports(aNative);

  return list->GetItemAt(aIndex);
}




NS_IMETHODIMP
nsCSSStyleDeclSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                            JSObject *globalObj, JSObject **parentObj)
{
  nsWrapperCache* cache = nsnull;
  CallQueryInterface(nativeObj, &cache);
  if (!cache) {
    return nsDOMClassInfo::PreCreate(nativeObj, cx, globalObj, parentObj);
  }

  nsICSSDeclaration *declaration = static_cast<nsICSSDeclaration*>(nativeObj);
  nsISupports *native_parent = declaration->GetParentObject();
  if (!native_parent) {
    return NS_ERROR_FAILURE;
  }

  jsval v;
  nsresult rv = WrapNative(cx, globalObj, native_parent, PR_FALSE, &v);
  NS_ENSURE_SUCCESS(rv, rv);

  *parentObj = JSVAL_TO_OBJECT(v);

  return NS_SUCCESS_ALLOW_SLIM_WRAPPERS;
}

nsresult
nsCSSStyleDeclSH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                              nsAString& aResult)
{
  if (aIndex < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> style_decl(do_QueryInterface(aNative));

  return style_decl->Item(PRUint32(aIndex), aResult);
}




nsISupports*
nsCSSRuleListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                           nsresult *aResult)
{
  nsICSSRuleList* list = static_cast<nsICSSRuleList*>(aNative);
#ifdef DEBUG
  {
    nsCOMPtr<nsICSSRuleList> list_qi = do_QueryInterface(aNative);

    
    
    
    NS_ASSERTION(list_qi == list, "Uh, fix QI!");
  }
#endif

  return list->GetItemAt(aIndex, aResult);
}



nsISupports*
nsClientRectListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                              nsresult *aResult)
{
  nsClientRectList* list = nsClientRectList::FromSupports(aNative);

  return list->GetItemAt(aIndex);
}

#ifdef MOZ_XUL


nsISupports*
nsTreeColumnsSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                           nsresult *aResult)
{
  nsTreeColumns* columns = nsTreeColumns::FromSupports(aNative);

  return columns->GetColumnAt(aIndex);
}

nsISupports*
nsTreeColumnsSH::GetNamedItem(nsISupports *aNative,
                              const nsAString& aName,
                              nsresult *aResult)
{
  nsTreeColumns* columns = nsTreeColumns::FromSupports(aNative);

  return columns->GetNamedColumn(aName);
}
#endif









NS_IMETHODIMP
nsStorageSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, jsval id, PRUint32 flags,
                        JSObject **objp, PRBool *_retval)
{
  JSObject *realObj;
  wrapper->GetJSObject(&realObj);

  
  

  JSString *jsstr = JS_ValueToString(cx, id);
  if (!jsstr) {
    return JS_FALSE;
  }

  JSObject *proto = ::JS_GetPrototype(cx, realObj);
  JSBool hasProp;

  if (proto &&
      (::JS_HasUCProperty(cx, proto, ::JS_GetStringChars(jsstr),
                          ::JS_GetStringLength(jsstr), &hasProp) &&
       hasProp)) {
    
    

    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMStorageObsolete> storage(do_QueryWrappedNative(wrapper));

  
  
  nsCOMPtr<nsIDOMStorageItem> item;
  nsresult rv = storage->GetItem(nsDependentJSString(jsstr),
                                 getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  if (item) {
    if (!::JS_DefineUCProperty(cx, realObj, ::JS_GetStringChars(jsstr),
                               ::JS_GetStringLength(jsstr), JSVAL_VOID, nsnull,
                               nsnull, 0)) {
      return NS_ERROR_FAILURE;
    }

    *objp = realObj;
  }

  return NS_OK;
}

nsISupports*
nsStorageSH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                          nsresult *aResult)
{
  nsDOMStorage* storage = nsDOMStorage::FromSupports(aNative);

  return storage->GetNamedItem(aName, aResult);
}

NS_IMETHODIMP
nsStorageSH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx, JSObject *obj, jsval id,
                         jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIDOMStorageObsolete> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = ::JS_ValueToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  JSString *value = ::JS_ValueToString(cx, *vp);
  NS_ENSURE_TRUE(value, NS_ERROR_UNEXPECTED);

  nsresult rv = storage->SetItem(nsDependentJSString(key),
                                 nsDependentJSString(value));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }

  return rv;
}

NS_IMETHODIMP
nsStorageSH::DelProperty(nsIXPConnectWrappedNative *wrapper,
                         JSContext *cx, JSObject *obj, jsval id,
                         jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIDOMStorageObsolete> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = ::JS_ValueToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  nsresult rv = storage->RemoveItem(nsDependentJSString(key));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }

  return rv;
}


NS_IMETHODIMP
nsStorageSH::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, PRUint32 enum_op, jsval *statep,
                          jsid *idp, PRBool *_retval)
{
  nsTArray<nsString> *keys =
    (nsTArray<nsString> *)JSVAL_TO_PRIVATE(*statep);

  switch (enum_op) {
    case JSENUMERATE_INIT:
    {
      nsCOMPtr<nsPIDOMStorage> storage(do_QueryWrappedNative(wrapper));

      
      keys = storage->GetKeys();
      NS_ENSURE_TRUE(keys, NS_ERROR_OUT_OF_MEMORY);

      *statep = PRIVATE_TO_JSVAL(keys);

      if (idp) {
        *idp = INT_TO_JSVAL(keys->Length());
      }
      break;
    }
    case JSENUMERATE_NEXT:
      if (keys->Length() != 0) {
        nsString& key = keys->ElementAt(0);
        JSString *str =
          JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                                  (key.get()),
                              key.Length());
        NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

        JS_ValueToId(cx, STRING_TO_JSVAL(str), idp);

        keys->RemoveElementAt(0);

        break;
      }

      
    case JSENUMERATE_DESTROY:
      delete keys;

      *statep = JSVAL_NULL;

      break;
    default:
      NS_NOTREACHED("Bad call from the JS engine");

      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}









NS_IMETHODIMP
nsStorage2SH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsval id, PRUint32 flags,
                         JSObject **objp, PRBool *_retval)
{
  JSObject *realObj;
  wrapper->GetJSObject(&realObj);

  
  

  JSString *jsstr = JS_ValueToString(cx, id);
  if (!jsstr) {
    return JS_FALSE;
  }

  JSObject *proto = ::JS_GetPrototype(cx, realObj);
  JSBool hasProp;

  if (proto &&
      (::JS_HasUCProperty(cx, proto, ::JS_GetStringChars(jsstr),
                          ::JS_GetStringLength(jsstr), &hasProp) &&
       hasProp)) {
    
    

    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));

  
  
  nsAutoString data;
  nsresult rv = storage->GetItem(nsDependentJSString(jsstr), data);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!DOMStringIsNull(data)) {
    if (!::JS_DefineUCProperty(cx, realObj, ::JS_GetStringChars(jsstr),
                               ::JS_GetStringLength(jsstr), JSVAL_VOID, nsnull,
                               nsnull, 0)) {
      return NS_ERROR_FAILURE;
    }

    *objp = realObj;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsStorage2SH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsval id, jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  nsAutoString val;
  nsresult rv = NS_OK;

  if (JSVAL_IS_STRING(id)) {
    
    if (ObjectIsNativeWrapper(cx, obj)) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    rv = storage->GetItem(nsDependentJSString(id), val);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    PRInt32 n = GetArrayIndexFromId(cx, id);
    NS_ENSURE_TRUE(n >= 0, NS_ERROR_NOT_AVAILABLE);

    rv = storage->Key(n, val);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JSAutoRequest ar(cx);

  if (DOMStringIsNull(val)) {
      *vp = JSVAL_NULL;
  }
  else {
      JSString *str =
        ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>(val.get()),
                              val.Length());
      NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

      *vp = STRING_TO_JSVAL(str);
  }

  return NS_SUCCESS_I_DID_SOMETHING;
}

NS_IMETHODIMP
nsStorage2SH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj, jsval id,
                          jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = ::JS_ValueToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  JSString *value = ::JS_ValueToString(cx, *vp);
  NS_ENSURE_TRUE(value, NS_ERROR_UNEXPECTED);

  nsresult rv = storage->SetItem(nsDependentJSString(key),
                                 nsDependentJSString(value));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }

  return rv;
}

NS_IMETHODIMP
nsStorage2SH::DelProperty(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj, jsval id,
                          jsval *vp, PRBool *_retval)
{
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = ::JS_ValueToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  nsresult rv = storage->RemoveItem(nsDependentJSString(key));
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }

  return rv;
}


NS_IMETHODIMP
nsStorage2SH::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, PRUint32 enum_op, jsval *statep,
                           jsid *idp, PRBool *_retval)
{
  nsTArray<nsString> *keys =
    (nsTArray<nsString> *)JSVAL_TO_PRIVATE(*statep);

  switch (enum_op) {
    case JSENUMERATE_INIT:
    {
      nsCOMPtr<nsPIDOMStorage> storage(do_QueryWrappedNative(wrapper));

      
      keys = storage->GetKeys();
      NS_ENSURE_TRUE(keys, NS_ERROR_OUT_OF_MEMORY);

      *statep = PRIVATE_TO_JSVAL(keys);

      if (idp) {
        *idp = INT_TO_JSVAL(keys->Length());
      }
      break;
    }
    case JSENUMERATE_NEXT:
      if (keys->Length() != 0) {
        nsString& key = keys->ElementAt(0);
        JSString *str =
          JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                                  (key.get()),
                              key.Length());
        NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

        JS_ValueToId(cx, STRING_TO_JSVAL(str), idp);

        keys->RemoveElementAt(0);

        break;
      }

      
    case JSENUMERATE_DESTROY:
      delete keys;

      *statep = JSVAL_NULL;

      break;
    default:
      NS_NOTREACHED("Bad call from the JS engine");

      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}



nsISupports*
nsStorageListSH::GetNamedItem(nsISupports *aNative, const nsAString& aName,
                              nsresult *aResult)
{
  nsDOMStorageList* storagelist = static_cast<nsDOMStorageList*>(aNative);

  return storagelist->GetNamedItem(aName, aResult);
}




NS_INTERFACE_MAP_BEGIN(nsEventListenerThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsIXPCFunctionThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsEventListenerThisTranslator)
NS_IMPL_RELEASE(nsEventListenerThisTranslator)


NS_IMETHODIMP
nsEventListenerThisTranslator::TranslateThis(nsISupports *aInitialThis,
                                             nsIInterfaceInfo *aInterfaceInfo,
                                             PRUint16 aMethodIndex,
                                             PRBool *aHideFirstParamFromJS,
                                             nsIID * *aIIDOfResult,
                                             nsISupports **_retval)
{
  *aHideFirstParamFromJS = PR_FALSE;
  *aIIDOfResult = nsnull;

  nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(aInitialThis));
  NS_ENSURE_TRUE(event, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMEventTarget> target;
  event->GetCurrentTarget(getter_AddRefs(target));

  *_retval = target;
  NS_IF_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructorSH::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, PRUint32 argc, jsval *argv, jsval *vp,
                         PRBool *_retval)
{
  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->Construct(wrapper, cx, obj, argc, argv, vp, _retval);
}

NS_IMETHODIMP
nsDOMConstructorSH::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                              JSObject *obj, PRUint32 argc, jsval *argv,
                              jsval *vp, PRBool *_retval)
{
  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->Construct(wrapper, cx, obj, argc, argv, vp, _retval);
}

NS_IMETHODIMP
nsDOMConstructorSH::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                JSContext *cx, JSObject *obj, jsval val,
                                PRBool *bp, PRBool *_retval)
{
  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->HasInstance(wrapper, cx, obj, val, bp, _retval);
}

NS_IMETHODIMP
nsNonDOMObjectSH::GetFlags(PRUint32 *aFlags)
{
  
  
  
  *aFlags = nsIClassInfo::MAIN_THREAD_ONLY;
  return NS_OK;
}

NS_IMETHODIMP
nsAttributeSH::GetFlags(PRUint32 *aFlags)
{
  
  *aFlags = DOMCLASSINFO_STANDARD_FLAGS;

  return NS_OK;
}


nsresult
nsOfflineResourceListSH::GetStringAt(nsISupports *aNative, PRInt32 aIndex,
                                     nsAString& aResult)
{
  nsCOMPtr<nsIDOMOfflineResourceList> list(do_QueryInterface(aNative));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  return list->MozItem(aIndex, aResult);
}


nsISupports*
nsFileListSH::GetItemAt(nsISupports *aNative, PRUint32 aIndex,
                        nsresult *aResult)
{
  nsDOMFileList* list = nsDOMFileList::FromSupports(aNative);

  return list->GetItemAt(aIndex);
}
