





#ifndef mozilla_a11y_AccTypes_h
#define mozilla_a11y_AccTypes_h

namespace mozilla {
namespace a11y {




enum AccType {
  



  eNoType,
  eHTMLBRType,
  eHTMLButtonType,
  eHTMLCanvasType,
  eHTMLCaptionType,
  eHTMLCheckboxType,
  eHTMLComboboxType,
  eHTMLFileInputType,
  eHTMLGroupboxType,
  eHTMLHRType,
  eHTMLImageMapType,
  eHTMLLabelType,
  eHTMLLiType,
  eHTMLSelectListType,
  eHTMLMediaType,
  eHTMLRadioButtonType,
  eHTMLTableType,
  eHTMLTableCellType,
  eHTMLTableRowType,
  eHTMLTextFieldType,
  eHyperTextType,
  eImageType,
  eOuterDocType,
  ePluginType,
  eTextLeafType,

  


  eApplicationType,
  eImageMapType,
  eMenuPopupType,
  eProgressType,
  eRootType,
  eXULLabelType,
  eXULTabpanelsType,
  eXULTreeType,

  eLastAccType = eXULTreeType
};





enum AccGenericType {
  eAutoComplete = 1 << 0,
  eAutoCompletePopup = 1 << 1,
  eCombobox = 1 << 2,
  eDocument = 1 << 3,
  eHyperText = 1 << 4,
  eList = 1 << 5,
  eListControl = 1 << 6,
  eMenuButton = 1 << 7,
  eSelect = 1 << 8,
  eTable = 1 << 9,
  eTableCell = 1 << 10,
  eTableRow = 1 << 11,

  eLastAccGenericType = eTableRow
};

} 
} 

#endif 
