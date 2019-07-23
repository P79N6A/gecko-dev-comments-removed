








































var gTableElement;
var gCellElement;
var gTableCaptionElement;
var globalCellElement;
var globalTableElement
var gValidateTab;
const defHAlign =   "left";
const centerStr =   "center";  
const rightStr =    "right";   
const justifyStr =  "justify"; 
const charStr =     "char";    
const defVAlign =   "middle";
const topStr =      "top";
const bottomStr =   "bottom";
const bgcolor = "bgcolor";
var gTableColor;
var gCellColor;

const cssBackgroundColorStr = "background-color";

var gRowCount = 1;
var gColCount = 1;
var gLastRowIndex;
var gLastColIndex;
var gNewRowCount;
var gNewColCount;
var gCurRowIndex;
var gCurColIndex;
var gCurColSpan;
var gSelectedCellsType = 1;
const SELECT_CELL = 1;
const SELECT_ROW = 2;
const SELECT_COLUMN = 3;
const RESET_SELECTION = 0;
var gCellData = { value:null, startRowIndex:0, startColIndex:0, rowSpan:0, colSpan:0,
                 actualRowSpan:0, actualColSpan:0, isSelected:false
               };
var gAdvancedEditUsed;
var gAlignWasChar = false;












var gSelectedCellCount = 0;
var gApplyUsed = false;
var gSelection;
var gCellDataChanged = false;
var gCanDelete = false;
var gPrefs = GetPrefs();
var gUseCSS = true;
var gActiveEditor;


function Startup()
{
  gActiveEditor = GetCurrentTableEditor();
  if (!gActiveEditor)
  {
    window.close();
    return;
  }

  try {
    gSelection = gActiveEditor.selection;
  } catch (e) {}
  if (!gSelection) return;

  
  gDialog.TableRowsInput = document.getElementById("TableRowsInput");
  gDialog.TableColumnsInput = document.getElementById("TableColumnsInput");
  gDialog.TableWidthInput = document.getElementById("TableWidthInput");
  gDialog.TableWidthUnits = document.getElementById("TableWidthUnits");
  gDialog.TableHeightInput = document.getElementById("TableHeightInput");
  gDialog.TableHeightUnits = document.getElementById("TableHeightUnits");
  try {
    if (!gPrefs.getBoolPref("editor.use_css") || (gActiveEditor.flags & 1))
    {
      gUseCSS = false;
      var tableHeightLabel = document.getElementById("TableHeightLabel");
      tableHeightLabel.parentNode.removeChild(tableHeightLabel);
      gDialog.TableHeightInput.parentNode.removeChild(gDialog.TableHeightInput);
      gDialog.TableHeightUnits.parentNode.removeChild(gDialog.TableHeightUnits);
    }
  } catch (e) {}
  gDialog.BorderWidthInput = document.getElementById("BorderWidthInput");
  gDialog.SpacingInput = document.getElementById("SpacingInput");
  gDialog.PaddingInput = document.getElementById("PaddingInput");
  gDialog.TableAlignList = document.getElementById("TableAlignList");
  gDialog.TableCaptionList = document.getElementById("TableCaptionList");
  gDialog.TableInheritColor = document.getElementById("TableInheritColor");
  gDialog.TabBox =  document.getElementById("TabBox");

  
  gDialog.SelectionList = document.getElementById("SelectionList");
  gDialog.PreviousButton = document.getElementById("PreviousButton");
  gDialog.NextButton = document.getElementById("NextButton");
  
  
  
  

  gDialog.CellHeightInput = document.getElementById("CellHeightInput");
  gDialog.CellHeightUnits = document.getElementById("CellHeightUnits");
  gDialog.CellWidthInput = document.getElementById("CellWidthInput");
  gDialog.CellWidthUnits = document.getElementById("CellWidthUnits");
  gDialog.CellHAlignList = document.getElementById("CellHAlignList");
  gDialog.CellVAlignList = document.getElementById("CellVAlignList");
  gDialog.CellInheritColor = document.getElementById("CellInheritColor");
  gDialog.CellStyleList = document.getElementById("CellStyleList");
  gDialog.TextWrapList = document.getElementById("TextWrapList");

  
  
  
  gDialog.CellHeightCheckbox = document.getElementById("CellHeightCheckbox");
  gDialog.CellWidthCheckbox = document.getElementById("CellWidthCheckbox");
  gDialog.CellHAlignCheckbox = document.getElementById("CellHAlignCheckbox");
  gDialog.CellVAlignCheckbox = document.getElementById("CellVAlignCheckbox");
  gDialog.CellStyleCheckbox = document.getElementById("CellStyleCheckbox");
  gDialog.TextWrapCheckbox = document.getElementById("TextWrapCheckbox");
  gDialog.CellColorCheckbox = document.getElementById("CellColorCheckbox");
  gDialog.TableTab = document.getElementById("TableTab");
  gDialog.CellTab = document.getElementById("CellTab");
  gDialog.AdvancedEditCell = document.getElementById("AdvancedEditButton2");
  
  gDialog.AdvancedEditCellToolTipText = gDialog.AdvancedEditCell.getAttribute("tooltiptext");

  try {
    gTableElement = gActiveEditor.getElementOrParentByTagName("table", null);
  } catch (e) {}
  if(!gTableElement)
  {
    dump("Failed to get table element!\n");
    window.close();
    return;
  }
  globalTableElement = gTableElement.cloneNode(false);

  var tagNameObj = { value: "" };
  var countObj = { value : 0 };
  var tableOrCellElement;
  try {
   tableOrCellElement = gActiveEditor.getSelectedOrParentTableElement(tagNameObj, countObj);
  } catch (e) {}

  if (tagNameObj.value == "td")
  {
    
    gSelectedCellCount = countObj.value;
    gCellElement = tableOrCellElement;
    globalCellElement = gCellElement.cloneNode(false);

    
    try {
      gSelectedCellsType = gActiveEditor.getSelectedCellsType(gTableElement);
    } catch (e) {}

    
    if (gSelectedCellsType < SELECT_CELL || gSelectedCellsType > SELECT_COLUMN)
      gSelectedCellsType = SELECT_CELL;

    
    
    if (gSelectedCellCount == 0)
      DoCellSelection();

    
    var rowIndexObj = { value: 0 };
    var colIndexObj = { value: 0 };
    try {
      gActiveEditor.getCellIndexes(gCellElement, rowIndexObj, colIndexObj);
    } catch (e) {}
    gCurRowIndex = rowIndexObj.value;
    gCurColIndex = colIndexObj.value;

    
    
    if (GetCellData(gCurRowIndex, gCurColIndex))
      gCurColSpan = gCellData.colSpan;

    
    if (window.arguments[1] == "CellPanel")
      gDialog.TabBox.selectedTab = gDialog.CellTab;
  }

  if (gDialog.TabBox.selectedTab == gDialog.TableTab)
  {
    
    
    if(!gCellElement)
    {
      
      
      
      gDialog.CellTab.parentNode.removeChild(gDialog.CellTab);
    }
  }

  
  
  
  var rowCountObj = { value: 0 };
  var colCountObj = { value: 0 };
  try {
    gActiveEditor.getTableSize(gTableElement, rowCountObj, colCountObj);
  } catch (e) {}

  gRowCount = rowCountObj.value;
  gLastRowIndex = gRowCount-1;
  gColCount = colCountObj.value;
  gLastColIndex = gColCount-1;


  
  SetSelectionButtons();

  
  if (gRowCount == 1 && gColCount == 1)
    gDialog.SelectionList.setAttribute("disabled", "true");

  
  gNewRowCount = gRowCount;
  gNewColCount = gColCount;

  
  
  
  gAdvancedEditUsed = false;
  InitDialog();
  gAdvancedEditUsed = true;

  
  gCellDataChanged = false;

  if (gDialog.TabBox.selectedTab == gDialog.CellTab)
    setTimeout("gDialog.SelectionList.focus()", 0);
  else
    SetTextboxFocus(gDialog.TableRowsInput);

  SetWindowLocation();
}


function InitDialog()
{
  
  gDialog.TableRowsInput.value = gRowCount;
  gDialog.TableColumnsInput.value = gColCount;
  gDialog.TableWidthInput.value = InitPixelOrPercentMenulist(globalTableElement, gTableElement, "width", "TableWidthUnits", gPercent);
  if (gUseCSS) {
    gDialog.TableHeightInput.value = InitPixelOrPercentMenulist(globalTableElement, gTableElement, "height",
                                                                "TableHeightUnits", gPercent);
  }
  gDialog.BorderWidthInput.value = globalTableElement.border;
  gDialog.SpacingInput.value = globalTableElement.cellSpacing;
  gDialog.PaddingInput.value = globalTableElement.cellPadding;

  var marginLeft  = GetHTMLOrCSSStyleValue(globalTableElement, "align", "margin-left");
  var marginRight = GetHTMLOrCSSStyleValue(globalTableElement, "align", "margin-right");
  var halign = marginLeft.toLowerCase() + " " + marginRight.toLowerCase();
  if (halign == "center center" || halign == "auto auto")
    gDialog.TableAlignList.value = "center";
  else if (halign == "right right" || halign == "auto 0px")
    gDialog.TableAlignList.value = "right";
  else 
    gDialog.TableAlignList.value = "left";

  
  gTableCaptionElement = gTableElement.caption;
  if (gTableCaptionElement)
  {
    var align = GetHTMLOrCSSStyleValue(gTableCaptionElement, "align", "caption-side");
    if (align != "bottom" && align != "left" && align != "right")
      align = "top";
    gDialog.TableCaptionList.value = align;
  }

  gTableColor = GetHTMLOrCSSStyleValue(globalTableElement, bgcolor, cssBackgroundColorStr);
  gTableColor = ConvertRGBColorIntoHEXColor(gTableColor);
  SetColor("tableBackgroundCW", gTableColor);

  InitCellPanel();
}

function InitCellPanel()
{
  
  if (globalCellElement)
  {
    
    gDialog.SelectionList.value = gSelectedCellsType;

    var previousValue = gDialog.CellHeightInput.value;
    gDialog.CellHeightInput.value = InitPixelOrPercentMenulist(globalCellElement, gCellElement, "height", "CellHeightUnits", gPixel);
    gDialog.CellHeightCheckbox.checked = gAdvancedEditUsed && previousValue != gDialog.CellHeightInput.value;

    previousValue= gDialog.CellWidthInput.value;
    gDialog.CellWidthInput.value = InitPixelOrPercentMenulist(globalCellElement, gCellElement, "width", "CellWidthUnits", gPixel);
    gDialog.CellWidthCheckbox.checked = gAdvancedEditUsed && previousValue != gDialog.CellWidthInput.value;

    var previousIndex = gDialog.CellVAlignList.selectedIndex;
    var valign = GetHTMLOrCSSStyleValue(globalCellElement, "valign", "vertical-align").toLowerCase();
    if (valign == topStr || valign == bottomStr)
      gDialog.CellVAlignList.value = valign;
    else 
      gDialog.CellVAlignList.value = defVAlign;

    gDialog.CellVAlignCheckbox.checked = gAdvancedEditUsed && previousIndex != gDialog.CellVAlignList.selectedIndex;

    previousIndex = gDialog.CellHAlignList.selectedIndex;

    gAlignWasChar = false;

    var halign = GetHTMLOrCSSStyleValue(globalCellElement, "align", "text-align").toLowerCase();
    switch (halign)
    {
      case centerStr:
      case rightStr:
      case justifyStr:
        gDialog.CellHAlignList.value = halign;
        break;
      case charStr:
        
        
        
        gAlignWasChar = true;
        
      default:
        
        gDialog.CellHAlignList.value =
          (globalCellElement.nodeName.toLowerCase() == "th") ? "center" : "left";
        break;
    }

    gDialog.CellHAlignCheckbox.checked = gAdvancedEditUsed &&
      previousIndex != gDialog.CellHAlignList.selectedIndex;

    previousIndex = gDialog.CellStyleList.selectedIndex;
    gDialog.CellStyleList.value = globalCellElement.nodeName.toLowerCase();
    gDialog.CellStyleCheckbox.checked = gAdvancedEditUsed && previousIndex != gDialog.CellStyleList.selectedIndex;

    previousIndex = gDialog.TextWrapList.selectedIndex;
    if (GetHTMLOrCSSStyleValue(globalCellElement, "nowrap", "white-space") == "nowrap")
      gDialog.TextWrapList.value = "nowrap";
    else
      gDialog.TextWrapList.value = "wrap";
    gDialog.TextWrapCheckbox.checked = gAdvancedEditUsed && previousIndex != gDialog.TextWrapList.selectedIndex;

    previousValue = gCellColor;
    gCellColor = GetHTMLOrCSSStyleValue(globalCellElement, bgcolor, cssBackgroundColorStr);
    gCellColor = ConvertRGBColorIntoHEXColor(gCellColor);
    SetColor("cellBackgroundCW", gCellColor);
    gDialog.CellColorCheckbox.checked = gAdvancedEditUsed && previousValue != gCellColor;

    
    
    gCellDataChanged = true;
  }
}

function GetCellData(rowIndex, colIndex)
{
  
  var startRowIndexObj = { value: 0 };
  var startColIndexObj = { value: 0 };
  var rowSpanObj = { value: 0 };
  var colSpanObj = { value: 0 };
  var actualRowSpanObj = { value: 0 };
  var actualColSpanObj = { value: 0 };
  var isSelectedObj = { value: false };

  try {
    gActiveEditor.getCellDataAt(gTableElement, rowIndex, colIndex,
                         gCellData,
                         startRowIndexObj, startColIndexObj,
                         rowSpanObj, colSpanObj,
                         actualRowSpanObj, actualColSpanObj, isSelectedObj);
    
    if (!gCellData.value) return false;
  }
  catch(ex) {
    return false;
  }

  gCellData.startRowIndex = startRowIndexObj.value;
  gCellData.startColIndex = startColIndexObj.value;
  gCellData.rowSpan = rowSpanObj.value;
  gCellData.colSpan = colSpanObj.value;
  gCellData.actualRowSpan = actualRowSpanObj.value;
  gCellData.actualColSpan = actualColSpanObj.value;
  gCellData.isSelected = isSelectedObj.value;
  return true;
}

function SelectCellHAlign()
{
  SetCheckbox("CellHAlignCheckbox");
  
  
  gAlignWasChar = false;
}

function GetColorAndUpdate(ColorWellID)
{
  var colorWell = document.getElementById(ColorWellID);
  if (!colorWell) return;

  var colorObj = { Type:"", TableColor:0, CellColor:0, NoDefault:false, Cancel:false, BackgroundColor:0 };

  switch( ColorWellID )
  {
    case "tableBackgroundCW":
      colorObj.Type = "Table";
      colorObj.TableColor = gTableColor;
      break;
    case "cellBackgroundCW":
      colorObj.Type = "Cell";
      colorObj.CellColor = gCellColor;
      break;
  }
  window.openDialog("chrome://editor/content/EdColorPicker.xul", "_blank", "chrome,close,titlebar,modal", "", colorObj);

  
  if (colorObj.Cancel)
    return;

  switch( ColorWellID )
  {
    case "tableBackgroundCW":
      gTableColor = colorObj.BackgroundColor;
      SetColor(ColorWellID, gTableColor);
      break;
    case "cellBackgroundCW":
      gCellColor = colorObj.BackgroundColor;
      SetColor(ColorWellID, gCellColor);
      SetCheckbox('CellColorCheckbox');
      break;
  }
}

function SetColor(ColorWellID, color)
{
  
  if (ColorWellID == "cellBackgroundCW")
  {
    if (color)
    {
      try {
        gActiveEditor.setAttributeOrEquivalent(globalCellElement, bgcolor,
                                               color, true);
      } catch(e) {}
      gDialog.CellInheritColor.collapsed = true;
    }
    else
    {
      try {
        gActiveEditor.removeAttributeOrEquivalent(globalCellElement, bgcolor, true);
      } catch(e) {}
      
      gDialog.CellInheritColor.collapsed = false;
    }
  }
  else
  {
    if (color)
    {
      try {
        gActiveEditor.setAttributeOrEquivalent(globalTableElement, bgcolor,
                                               color, true);
      } catch(e) {}
      gDialog.TableInheritColor.collapsed = true;
    }
    else
    {
      try {
        gActiveEditor.removeAttributeOrEquivalent(globalTableElement, bgcolor, true);
      } catch(e) {}
      gDialog.TableInheritColor.collapsed = false;
    }
    SetCheckbox('CellColorCheckbox');
  }

  setColorWell(ColorWellID, color);
}

function ChangeSelectionToFirstCell()
{
  if (!GetCellData(0,0))
  {
    dump("Can't find first cell in table!\n");
    return;
  }
  gCellElement = gCellData.value;
  globalCellElement = gCellElement;

  gCurRowIndex = 0;
  gCurColIndex = 0;
  ChangeSelection(RESET_SELECTION);
}

function ChangeSelection(newType)
{
  newType = Number(newType);

  if (gSelectedCellsType == newType)
    return;

  if (newType == RESET_SELECTION)
    
    gSelection.collapse(gCellElement,0);
  else
    gSelectedCellsType = newType;

  
  DoCellSelection();
  SetSelectionButtons();

  
}

function MoveSelection(forward)
{
  var newRowIndex = gCurRowIndex;
  var newColIndex = gCurColIndex;
  var focusCell;
  var inRow = false;

  if (gSelectedCellsType == SELECT_ROW)
  {
    newRowIndex += (forward ? 1 : -1);

    
    if (newRowIndex < 0)
      newRowIndex = gLastRowIndex;
    else if (newRowIndex > gLastRowIndex)
      newRowIndex = 0;
    inRow = true;

    
    newColIndex = 0;
  }
  else
  {
    
    if (!forward)
      newColIndex--;

    if (gSelectedCellsType == SELECT_CELL)
    {
      
      if (forward)
        newColIndex += gCurColSpan;
    }
    else  
    {
      
      newRowIndex = 0;

      
      
      if (forward)
        newColIndex++;
    }

    if (newColIndex < 0)
    {
      

      
      newColIndex = gLastColIndex;

      if (gSelectedCellsType == SELECT_CELL)
      {
        
        if (newRowIndex > 0)
          newRowIndex -= 1;
        else
          
          newRowIndex = gLastRowIndex;

        inRow = true;
      }
    }
    else if (newColIndex > gLastColIndex)
    {
      

      
      newColIndex = 0;

      if (gSelectedCellsType == SELECT_CELL)
      {
        
        if (newRowIndex < gLastRowIndex)
          newRowIndex++;
        else
          
          newRowIndex = 0;

        inRow = true;
      }
    }
  }

  
  do {
    if (!GetCellData(newRowIndex, newColIndex))
    {
      dump("MoveSelection: CELL NOT FOUND\n");
      return;
    }
    if (inRow)
    {
      if (gCellData.startRowIndex == newRowIndex)
        break;
      else
        
        newRowIndex += gCellData.actualRowSpan;
    }
    else
    {
      if (gCellData.startColIndex == newColIndex)
        break;
      else
        
        newColIndex += gCellData.actualColSpan;
    }
  }
  while(true);

  
  if (gCellDataChanged) 
  {
    if (!ValidateCellData())
      return;

    gActiveEditor.beginTransaction();
    
    ApplyCellAttributes();
    gActiveEditor.endTransaction();

    SetCloseButton();
  }

  
  gCellElement = gCellData.value;

  
  gCurRowIndex = gCellData.startRowIndex;
  gCurColIndex = gCellData.startColIndex;
  gCurColSpan = gCellData.actualColSpan;

  
  globalCellElement = gCellElement.cloneNode(false);

  
  DoCellSelection();

  
  
  
  try {
    var selectionController = gActiveEditor.selectionController;
    selectionController.scrollSelectionIntoView(selectionController.SELECTION_NORMAL, selectionController.SELECTION_ANCHOR_REGION, true);
  } catch (e) {}

  

  
  gAdvancedEditUsed = false;
  InitCellPanel();
  gAdvancedEditUsed = true;
}


function DoCellSelection()
{
  
  
  gSelection.collapse(gCellElement, 0);

  var tagNameObj = { value: "" };
  var countObj = { value: 0 };
  try {
    switch (gSelectedCellsType)
    {
      case SELECT_CELL:
        gActiveEditor.selectTableCell();
        break
      case SELECT_ROW:
        gActiveEditor.selectTableRow();
        break;
      default:
        gActiveEditor.selectTableColumn();
        break;
    }
    
    var tableOrCellElement = gActiveEditor.getSelectedOrParentTableElement(tagNameObj, countObj);
  } catch (e) {}

  if (tagNameObj.value == "td")
    gSelectedCellCount = countObj.value;
  else
    gSelectedCellCount = 0;

  
  
  SetElementEnabled(gDialog.AdvancedEditCell, gSelectedCellCount == 1);

  gDialog.AdvancedEditCell.setAttribute("tooltiptext", 
    gSelectedCellCount > 1 ? GetString("AdvancedEditForCellMsg") :
                             gDialog.AdvancedEditCellToolTipText);
}

function SetSelectionButtons()
{
  if (gSelectedCellsType == SELECT_ROW)
  {
    
    gDialog.PreviousButton.setAttribute("type","row");
    gDialog.NextButton.setAttribute("type","row");
  }
  else
  {
    
    gDialog.PreviousButton.setAttribute("type","col");
    gDialog.NextButton.setAttribute("type","col");
  }
  DisableSelectionButtons((gSelectedCellsType == SELECT_ROW && gRowCount == 1) ||
                          (gSelectedCellsType == SELECT_COLUMN && gColCount == 1) ||
                          (gRowCount == 1 && gColCount == 1));
}

function DisableSelectionButtons( disable )
{
  gDialog.PreviousButton.setAttribute("disabled", disable ? "true" : "false");
  gDialog.NextButton.setAttribute("disabled", disable ? "true" : "false");
}

function SwitchToValidatePanel()
{
  if (gDialog.TabBox.selectedTab != gValidateTab)
    gDialog.TabBox.selectedTab = gValidateTab;
}

function SetAlign(listID, defaultValue, element, attName)
{
  var value = document.getElementById(listID).value;
  if (value == defaultValue)
  {
    try {
      gActiveEditor.removeAttributeOrEquivalent(element, attName, true);
    } catch(e) {}
  }
  else
  {
    try {
      gActiveEditor.setAttributeOrEquivalent(element, attName, value, true);
    } catch(e) {}
  }
}

function ValidateTableData()
{
  gValidateTab = gDialog.TableTab;
  gNewRowCount = Number(ValidateNumber(gDialog.TableRowsInput, null, 1, gMaxRows, null, true, true));
  if (gValidationError) return false;

  gNewColCount = Number(ValidateNumber(gDialog.TableColumnsInput, null, 1, gMaxColumns, null, true, true));
  if (gValidationError) return false;

  
  
  if ( !gCanDelete &&
        (gNewRowCount < gRowCount ||
         gNewColCount < gColCount) ) 
  {
    if (ConfirmWithTitle(GetString("DeleteTableTitle"), 
                         GetString("DeleteTableMsg"),
                         GetString("DeleteCells")) )
    {
      gCanDelete = true;
    }
    else
    {
      SetTextboxFocus(gNewRowCount < gRowCount ? gDialog.TableRowsInput : gDialog.TableColumnsInput);
      return false;
    }
  }

  ValidateNumber(gDialog.TableWidthInput, gDialog.TableWidthUnits,
                 1, gMaxTableSize, globalTableElement, "width");
  if (gValidationError) return false;

  if (gUseCSS) {
    ValidateNumber(gDialog.TableHeightInput, gDialog.TableHeightUnits,
                   1, gMaxTableSize, globalTableElement, "height");
    if (gValidationError) return false;
  }

  var border = ValidateNumber(gDialog.BorderWidthInput, null, 0, gMaxPixels, globalTableElement, "border");
  
  if (gValidationError) return false;

  ValidateNumber(gDialog.SpacingInput, null, 0, gMaxPixels, globalTableElement, "cellspacing");
  if (gValidationError) return false;

  ValidateNumber(gDialog.PaddingInput, null, 0, gMaxPixels, globalTableElement, "cellpadding");
  if (gValidationError) return false;

  SetAlign("TableAlignList", defHAlign, globalTableElement, "align");

  
  return true;
}

function ValidateCellData()
{

  gValidateTab = gDialog.CellTab;

  if (gDialog.CellHeightCheckbox.checked)
  {
    ValidateNumber(gDialog.CellHeightInput, gDialog.CellHeightUnits,
                    1, gMaxTableSize, globalCellElement, "height");
    if (gValidationError) return false;
  }

  if (gDialog.CellWidthCheckbox.checked)
  {
    ValidateNumber(gDialog.CellWidthInput, gDialog.CellWidthUnits,
                   1, gMaxTableSize, globalCellElement, "width");
    if (gValidationError) return false;
  }

  if (gDialog.CellHAlignCheckbox.checked)
  {
    var hAlign = gDialog.CellHAlignList.value;

    
    
    if (!gAlignWasChar)
    {
      globalCellElement.removeAttribute(charStr);

      
      
      
      globalCellElement.setAttribute("align", hAlign);
    }
  }

  if (gDialog.CellVAlignCheckbox.checked)
  {
    
    
    
    SetAlign("CellVAlignList", "", globalCellElement, "valign");
  }

  if (gDialog.TextWrapCheckbox.checked)
  {
    if (gDialog.TextWrapList.value == "nowrap")
      try {
        gActiveEditor.setAttributeOrEquivalent(globalCellElement, "nowrap",
                                               "nowrap", true);
      } catch(e) {}
    else
      try {
        gActiveEditor.removeAttributeOrEquivalent(globalCellElement, "nowrap", true);
      } catch(e) {}
  }

  return true;
}

function ValidateData()
{
  var result;

  
  if (gDialog.TabBox.selectedTab == gDialog.TableTab)
  {
    result = ValidateTableData();
    if (result)
      result = ValidateCellData();
  } else {
    result = ValidateCellData();
    if (result)
      result = ValidateTableData();
  }
  if(!result) return false;

  
  if(gDialog.TabBox.selectedTab == gDialog.TableTab)
    globalElement = globalTableElement;
  else
    globalElement = globalCellElement;

  return true;
}

function ChangeCellTextbox(textboxID)
{
  
  forceInteger(textboxID);

  if (gDialog.TabBox.selectedTab == gDialog.CellTab)
    gCellDataChanged = true;
}



function SetCheckbox(checkboxID)
{
  if (checkboxID && checkboxID.length > 0)
  {
    
    document.getElementById(checkboxID).checked = true;
  }
  gCellDataChanged = true;
}

function ChangeIntTextbox(textboxID, checkboxID)
{
  
  forceInteger(textboxID);

  
  SetCheckbox(checkboxID);
}

function CloneAttribute(destElement, srcElement, attr)
{
  var value = srcElement.getAttribute(attr);
  
  
  
  try {
    if (!value || value.length == 0)
      gActiveEditor.removeAttributeOrEquivalent(destElement, attr, false);
    else
      gActiveEditor.setAttributeOrEquivalent(destElement, attr, value, false);
  } catch(e) {}
}

function ApplyTableAttributes()
{
  var newAlign = gDialog.TableCaptionList.value;
  if (!newAlign) newAlign = "";

  if (gTableCaptionElement)
  {
    
    var align = GetHTMLOrCSSStyleValue(gTableCaptionElement, "align", "caption-side").toLowerCase();
    
    if (!align) align = "top";

    if (newAlign == "")
    {
      
      try {
        gActiveEditor.deleteNode(gTableCaptionElement);
      } catch(e) {}
      gTableCaptionElement = null;
    }
    else if(newAlign != align)
    {
      try {
        if (newAlign == "top") 
          gActiveEditor.removeAttributeOrEquivalent(gTableCaptionElement, "align", false);
        else
          gActiveEditor.setAttributeOrEquivalent(gTableCaptionElement, "align", newAlign, false);
      } catch(e) {}
    }
  }
  else if (newAlign != "")
  {
    
    try {
      gTableCaptionElement = gActiveEditor.createElementWithDefaults("caption");
    } catch (e) {}
    if (gTableCaptionElement)
    {
      if (newAlign != "top")
        gTableCaptionElement.setAttribute("align", newAlign);

      
      try {
        gActiveEditor.insertNode(gTableCaptionElement, gTableElement, 0);
      } catch(e) {}

      
      ChangeSelection(RESET_SELECTION);
    }
  }

  var countDelta;
  var foundCell;
  var i;

  if (gNewRowCount != gRowCount)
  {
    countDelta = gNewRowCount - gRowCount;
    if (gNewRowCount > gRowCount)
    {
      
      
      if(GetCellData(gLastRowIndex, 0))
      {
        try {
          
          gSelection.collapse(gCellData.value,0);
          
          gActiveEditor.insertTableRow(countDelta, true);
          gRowCount = gNewRowCount;
          gLastRowIndex = gRowCount - 1;
          
          ChangeSelection(RESET_SELECTION);
        }
        catch(ex) {
          dump("FAILED TO FIND FIRST CELL IN LAST ROW\n");
        }
      }
    }
    else
    {
      
      if (gCanDelete)
      {
        
        var firstDeleteRow = gRowCount + countDelta;
        foundCell = false;
        for ( i = 0; i <= gLastColIndex; i++)
        {
          if (!GetCellData(firstDeleteRow, i))
            break; 

          if (gCellData.startRowIndex == firstDeleteRow)
          {
            foundCell = true;
            break;
          }
        };
        if (foundCell)
        {
          try {
            
            gSelection.collapse(gCellData.value, 0);
            gActiveEditor.deleteTableRow(-countDelta);
            gRowCount = gNewRowCount;
            gLastRowIndex = gRowCount - 1;
            if (gCurRowIndex > gLastRowIndex)
              
              
              ChangeSelectionToFirstCell()
            else
              
              ChangeSelection(RESET_SELECTION);
          }
          catch(ex) {
            dump("FAILED TO FIND FIRST CELL IN LAST ROW\n");
          }
        }
      }
    }
  }

  if (gNewColCount != gColCount)
  {
    countDelta = gNewColCount - gColCount;

    if (gNewColCount > gColCount)
    {
      
      
      if(GetCellData(0, gLastColIndex))
      {
        try {
          
          gSelection.collapse(gCellData.value,0);
          gActiveEditor.insertTableColumn(countDelta, true);
          gColCount = gNewColCount;
          gLastColIndex = gColCount-1;
          
          ChangeSelection(RESET_SELECTION);
        }
        catch(ex) {
          dump("FAILED TO FIND FIRST CELL IN LAST COLUMN\n");
        }
      }
    }
    else
    {
      
      if (gCanDelete)
      {
        var firstDeleteCol = gColCount + countDelta;
        foundCell = false;
        for ( i = 0; i <= gLastRowIndex; i++)
        {
          
          if (!GetCellData(i, firstDeleteCol))
            break; 

          if (gCellData.startColIndex == firstDeleteCol)
          {
            foundCell = true;
            break;
          }
        };
        if (foundCell)
        {
          try {
            
            gSelection.collapse(gCellData.value, 0);
            gActiveEditor.deleteTableColumn(-countDelta);
            gColCount = gNewColCount;
            gLastColIndex = gColCount-1;
            if (gCurColIndex > gLastColIndex)
              ChangeSelectionToFirstCell()
            else
              ChangeSelection(RESET_SELECTION);
          }
          catch(ex) {
            dump("FAILED TO FIND FIRST CELL IN LAST ROW\n");
          }
        }
      }
    }
  }

  
  
  try {
    gActiveEditor.cloneAttributes(gTableElement, globalTableElement);
  } catch(e) {}
}

function ApplyCellAttributes()
{
  var rangeObj = { value: null };
  var selectedCell;
  try {
    selectedCell = gActiveEditor.getFirstSelectedCell(rangeObj);
  } catch(e) {}

  if (!selectedCell)
    return;

  if (gSelectedCellCount == 1)
  {
    
    
    try {
      gActiveEditor.cloneAttributes(selectedCell, globalCellElement);
    } catch(e) {}

    if (gDialog.CellStyleCheckbox.checked)
    {
      var currentStyleIndex = (selectedCell.nodeName.toLowerCase() == "th") ? 1 : 0;
      if (gDialog.CellStyleList.selectedIndex != currentStyleIndex)
      {
        
        
        try {
          selectedCell = gActiveEditor.switchTableCellHeaderType(selectedCell);
        } catch(e) {}
      }
    }
  }
  else
  {
    
    
    try {
      while (selectedCell)
      {
        ApplyAttributesToOneCell(selectedCell);
        selectedCell = gActiveEditor.getNextSelectedCell(rangeObj);
      }
    } catch(e) {}
  }
  gCellDataChanged = false;
}

function ApplyAttributesToOneCell(destElement)
{
  if (gDialog.CellHeightCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "height");

  if (gDialog.CellWidthCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "width");

  if (gDialog.CellHAlignCheckbox.checked)
  {
    CloneAttribute(destElement, globalCellElement, "align");
    CloneAttribute(destElement, globalCellElement, charStr);
  }

  if (gDialog.CellVAlignCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "valign");

  if (gDialog.TextWrapCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "nowrap");

  if (gDialog.CellStyleCheckbox.checked)
  {
    var newStyleIndex = gDialog.CellStyleList.selectedIndex;
    var currentStyleIndex = (destElement.nodeName.toLowerCase() == "th") ? 1 : 0;

    if (newStyleIndex != currentStyleIndex)
    {
      
      
      try {
        destElement = gActiveEditor.switchTableCellHeaderType(destElement);
      } catch(e) {}
    }
  }

  if (gDialog.CellColorCheckbox.checked)
    CloneAttribute(destElement, globalCellElement, "bgcolor");
}

function SetCloseButton()
{
  
  if (!gApplyUsed)
  {
    document.documentElement.setAttribute("buttonlabelcancel",
      document.documentElement.getAttribute("buttonlabelclose"));
    gApplyUsed = true;
  }
}

function Apply()
{
  if (ValidateData())
  {
    gActiveEditor.beginTransaction();

    ApplyTableAttributes();

    
    if (globalCellElement)
      ApplyCellAttributes();

    gActiveEditor.endTransaction();

    SetCloseButton();
    return true;
  }
  return false;
}

function doHelpButton()
{
  openHelp("table_properties");
}

function onAccept()
{
  
  var retVal = Apply();
  if (retVal)
    SaveWindowLocation();

  return retVal;
}
