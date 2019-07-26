





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
  eHTMLLiType,
  eHTMLSelectListType,
  eHTMLMediaType,
  eHTMLRadioButtonType,
  eHTMLRangeType,
  eHTMLSpinnerType,
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
  eHTMLOptGroupType,
  eImageMapType,
  eMenuPopupType,
  eProgressType,
  eRootType,
  eXULLabelType,
  eXULListItemType,
  eXULTabpanelsType,
  eXULTreeType,

  eLastAccType = eXULTreeType
};





enum AccGenericType {
  eAutoComplete = 1 << 0,
  eAutoCompletePopup = 1 << 1,
  eButton = 1 << 2,
  eCombobox = 1 << 3,
  eDocument = 1 << 4,
  eHyperText = 1 << 5,
  eList = 1 << 6,
  eListControl = 1 << 7,
  eMenuButton = 1 << 8,
  eSelect = 1 << 9,
  eTable = 1 << 10,
  eTableCell = 1 << 11,
  eTableRow = 1 << 12,

  eLastAccGenericType = eTableRow
};

} 
} 

#endif 
