











































var gDialog = {};

var gValidationError = false;


const gPixel = 0;
const gPercent = 1;

const gMaxPixels  = 100000; 


const gMaxRows    = 1000;
const gMaxColumns = 1000;
const gMaxTableSize = 1000000; 


var SeeMore = false;



var gLocation;


var globalElement;























function ValidateNumber(inputWidget, listWidget, minVal, maxVal, element, attName, mustHaveValue, mustShowMoreSection)
{
  if (!inputWidget)
  {
    gValidationError = true;
    return "";
  }

  
  gValidationError = false;
  var maxLimit = maxVal;
  var isPercent = false;

  var numString = TrimString(inputWidget.value);
  if (numString || mustHaveValue)
  {
    if (listWidget)
      isPercent = (listWidget.selectedIndex == 1);
    if (isPercent)
      maxLimit = 100;

    
    numString = ValidateNumberRange(numString, minVal, maxLimit, mustHaveValue);
    if(!numString)
    {
      
      SwitchToValidatePanel();

      
      if ("dialog" in window && dialog && 
           "MoreSection" in gDialog && gDialog.MoreSection)
      {
        if ( !SeeMore )
          onMoreFewer();
      }

      
      SetTextboxFocus(inputWidget);
      gValidationError = true;
    }
    else
    {
      if (isPercent)
        numString += "%";
      if (element)
        GetCurrentEditor().setAttributeOrEquivalent(element, attName, numString, true);
    }
  } else if (element) {
    GetCurrentEditor().removeAttributeOrEquivalent(element, attName, true)
  }
  return numString;
}

















function ValidateNumberRange(value, minValue, maxValue, mustHaveValue)
{
  
  gValidationError = false;
  value = TrimString(String(value));

  
  if (!value && !mustHaveValue)
    return "";

  var numberStr = "";

  if (value.length > 0)
  {
    
    var number = Number(value.replace(/\D+/g, ""));
    if (number >= minValue && number <= maxValue )
    {
      
      return String(number);
    }
    numberStr = String(number);
  }

  var message = "";

  if (numberStr.length > 0)
  {
    
    message = GetString( "ValidateRangeMsg");
    message = message.replace(/%n%/, numberStr);
    message += "\n ";
  }
  message += GetString( "ValidateNumberMsg");

  
  message = message.replace(/%min%/, minValue).replace(/%max%/, maxValue);
  ShowInputErrorMessage(message);

  
  gValidationError = true;
  return "";
}

function SetTextboxFocusById(id)
{
  SetTextboxFocus(document.getElementById(id));
}

function SetTextboxFocus(textbox)
{
  if (textbox)
  {
    
    
    setTimeout( function(textbox) { textbox.focus(); textbox.select(); }, 0, textbox );
  }
}

function ShowInputErrorMessage(message)
{
  AlertWithTitle(GetString("InputError"), message);
  window.focus();
}






function GetAppropriatePercentString(elementForAtt, elementInDoc)
{
  var editor = GetCurrentEditor();
  try {
    var name = elementForAtt.nodeName.toLowerCase();
    if ( name == "td" || name == "th")
      return GetString("PercentOfTable");

    
    if (editor.getElementOrParentByTagName("td", elementInDoc))
      return GetString("PercentOfCell");
    else
      return GetString("PercentOfWindow");
  } catch (e) { return "";}
}

function ClearListbox(listbox)
{
  if (listbox)
  {
    listbox.clearSelection();
    while (listbox.firstChild)
      listbox.removeChild(listbox.firstChild);
  }
}

function forceInteger(elementID)
{
  var editField = document.getElementById( elementID );
  if ( !editField )
    return;

  var stringIn = editField.value;
  if (stringIn && stringIn.length > 0)
  {
    
    stringIn = stringIn.replace(/\D+/g,"");
    if (!stringIn) stringIn = "";

    
    if (stringIn != editField.value)
      editField.value = stringIn;
  }
}

function LimitStringLength(elementID, length)
{
  var editField = document.getElementById( elementID );
  if ( !editField )
    return;

  var stringIn = editField.value;
  if (stringIn && stringIn.length > length)
    editField.value = stringIn.slice(0,length);
}

function InitPixelOrPercentMenulist(elementForAtt, elementInDoc, attribute, menulistID, defaultIndex)
{
  if (!defaultIndex) defaultIndex = gPixel;

  
  var size = GetHTMLOrCSSStyleValue(elementForAtt, attribute, attribute)
  var menulist = document.getElementById(menulistID);
  var pixelItem;
  var percentItem;

  if (!menulist)
  {
    dump("NO MENULIST found for ID="+menulistID+"\n");
    return size;
  }

  menulist.removeAllItems();
  pixelItem = menulist.appendItem(GetString("Pixels"));

  if (!pixelItem) return 0;

  percentItem = menulist.appendItem(GetAppropriatePercentString(elementForAtt, elementInDoc));
  if (size && size.length > 0)
  {
    
    if (/%/.test(size))
    {
      
      size = RegExp.leftContext;
      if (percentItem)
        menulist.selectedItem = percentItem;
    }
    else
    {
      if (/px/.test(size))
        
        size = RegExp.leftContext;
      menulist.selectedItem = pixelItem;
    }
  }
  else
    menulist.selectedIndex = defaultIndex;

  return size;
}

function onAdvancedEdit()
{
  
  if (ValidateData())
  {
    
    window.AdvancedEditOK = false;
    
    
    window.openDialog("chrome://editor/content/EdAdvancedEdit.xul", "_blank", "chrome,close,titlebar,modal,resizable=yes", "", globalElement);
    window.focus();
    if (window.AdvancedEditOK)
    {
      
      InitDialog();
    }
  }
}

function getColor(ColorPickerID)
{
  var colorPicker = document.getElementById(ColorPickerID);
  var color;
  if (colorPicker)
  {
    
    color = colorPicker.getAttribute("color");
    if (color && color == "")
      return null;
    
    
    colorPicker.setAttribute("color","");
  }

  return color;
}

function setColorWell(ColorWellID, color)
{
  var colorWell = document.getElementById(ColorWellID);
  if (colorWell)
  {
    if (!color || color == "")
    {
      
      
      colorWell.setAttribute("default","true");
      
      
      colorWell.removeAttribute("style");
    }
    else
    {
      colorWell.removeAttribute("default");
      
      colorWell.setAttribute("style", "background-color:"+color);
    }
  }
}

function getColorAndSetColorWell(ColorPickerID, ColorWellID)
{
  var color = getColor(ColorPickerID);
  setColorWell(ColorWellID, color);
  return color;
}

function InitMoreFewer()
{
  
  
  
  
  SeeMore = (gDialog.MoreFewerButton.getAttribute("more") != "1");
  onMoreFewer();
  gDialog.MoreFewerButton.setAttribute("accesskey",GetString("PropertiesAccessKey"));
}

function onMoreFewer()
{
  if (SeeMore)
  {
    gDialog.MoreSection.collapsed = true;
    gDialog.MoreFewerButton.setAttribute("more","0");
    gDialog.MoreFewerButton.setAttribute("label",GetString("MoreProperties"));
    SeeMore = false;
  }
  else
  {
    gDialog.MoreSection.collapsed = false;
    gDialog.MoreFewerButton.setAttribute("more","1");
    gDialog.MoreFewerButton.setAttribute("label",GetString("FewerProperties"));
    SeeMore = true;
  }
  window.sizeToContent();
}

function SwitchToValidatePanel()
{
  
  
}

const nsIFilePicker = Components.interfaces.nsIFilePicker;

function GetLocalFileURL(filterType)
{
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  var fileType = "html";

  if (filterType == "img")
  {
    fp.init(window, GetString("SelectImageFile"), nsIFilePicker.modeOpen);
    fp.appendFilters(nsIFilePicker.filterImages);
    fileType = "image";
  }
  
  
  else if (filterType.indexOf("html") == 0)
  {
    fp.init(window, GetString("OpenHTMLFile"), nsIFilePicker.modeOpen);

    
    
    fp.appendFilters(nsIFilePicker.filterHTML);
    fp.appendFilters(nsIFilePicker.filterText);

    
    if (filterType.indexOf("img") > 0)
      fp.appendFilters(nsIFilePicker.filterImages);

  }
  
  fp.appendFilters(nsIFilePicker.filterAll);

  
  SetFilePickerDirectory(fp, fileType);


  
  try {
    var ret = fp.show();
    if (ret == nsIFilePicker.returnCancel)
      return null;
  }
  catch (ex) {
    dump("filePicker.chooseInputFile threw an exception\n");
    return null;
  }
  SaveFilePickerDirectory(fp, fileType);
  
  var fileHandler = GetFileProtocolHandler();
  return fp.file ? fileHandler.getURLSpecFromFile(fp.file) : null;
}

function GetMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var editor = GetCurrentEditor();
      try {
        var metaNodes = editor.document.getElementsByTagName("meta");
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode && metaNode.getAttribute("name") == name)
            return metaNode;
        }
      } catch (e) {}
    }
  }
  return null;
}

function CreateMetaElement(name)
{
  var editor = GetCurrentEditor();
  try {
    var metaElement = editor.createElementWithDefaults("meta");
    metaElement.setAttribute("name", name);
    return metaElement;
  } catch (e) {}

  return null;
}

function GetHTTPEquivMetaElement(name)
{
  if (name)
  {
    name = name.toLowerCase();
    if (name != "")
    {
      var editor = GetCurrentEditor();
      try {
        var metaNodes = editor.document.getElementsByTagName("meta");
        for (var i = 0; i < metaNodes.length; i++)
        {
          var metaNode = metaNodes.item(i);
          if (metaNode)
          {
            var httpEquiv = metaNode.getAttribute("http-equiv");
            if (httpEquiv && httpEquiv.toLowerCase() == name)
              return metaNode;
          }
        }
      } catch (e) {}
    }
  }
  return null;
}

function CreateHTTPEquivMetaElement(name)
{
  var editor = GetCurrentEditor();
  try {
    var metaElement = editor.createElementWithDefaults("meta");
    metaElement.setAttribute("http-equiv", name);
    return metaElement;
  } catch (e) {}

  return null;
}

function CreateHTTPEquivElement(name)
{
  var editor = GetCurrentEditor();
  try {
    var metaElement = editor.createElementWithDefaults("meta");
    metaElement.setAttribute("http-equiv", name);
    return metaElement;
  } catch (e) {}

  return null;
}




function SetMetaElementContent(metaElement, content, insertNew, prepend)
{
  if (metaElement)
  {
    var editor = GetCurrentEditor();
    try {
      if(!content || content == "")
      {
        if (!insertNew)
          editor.deleteNode(metaElement);
      }
      else
      {
        if (insertNew)
        {
          metaElement.setAttribute("content", content);
          if (prepend)
            PrependHeadElement(metaElement);
          else
            AppendHeadElement(metaElement);
        }
        else
          editor.setAttribute(metaElement, "content", content);
      }
    } catch (e) {}
  }
}

function GetHeadElement()
{
  var editor = GetCurrentEditor();
  try {
    var headList = editor.document.getElementsByTagName("head");
    return headList.item(0);
  } catch (e) {}

  return null;
}

function PrependHeadElement(element)
{
  var head = GetHeadElement();
  if (head)
  {
    var editor = GetCurrentEditor();
    try {
      
      
      editor.insertNode(element, head, 0, true);
    } catch (e) {}
  }
}

function AppendHeadElement(element)
{
  var head = GetHeadElement();
  if (head)
  {
    var position = 0;
    if (head.hasChildNodes())
      position = head.childNodes.length;

    var editor = GetCurrentEditor();
    try {
      
      
      editor.insertNode(element, head, position, true);
    } catch (e) {}
  }
}

function SetWindowLocation()
{
  gLocation = document.getElementById("location");
  if (gLocation)
  {
    window.screenX = Math.max(0, Math.min(window.opener.screenX + Number(gLocation.getAttribute("offsetX")),
                                          screen.availWidth - window.outerWidth));
    window.screenY = Math.max(0, Math.min(window.opener.screenY + Number(gLocation.getAttribute("offsetY")),
                                          screen.availHeight - window.outerHeight));
  }
}

function SaveWindowLocation()
{
  if (gLocation)
  {
    var newOffsetX = window.screenX - window.opener.screenX;
    var newOffsetY = window.screenY - window.opener.screenY;
    gLocation.setAttribute("offsetX", window.screenX - window.opener.screenX);
    gLocation.setAttribute("offsetY", window.screenY - window.opener.screenY);
  }
}

function onCancel()
{
  SaveWindowLocation();
  
  return true;
}

function SetRelativeCheckbox(checkbox)
{
  if (!checkbox) {
    checkbox = document.getElementById("MakeRelativeCheckbox");
    if (!checkbox)
      return;
  }

  var editor = GetCurrentEditor();
  
  if (editor && (editor.flags & Components.interfaces.nsIPlaintextEditor.eEditorMailMask))
  {
    checkbox.collapsed = true;
    return;
  }

  var input =  document.getElementById(checkbox.getAttribute("for"));
  if (!input)
    return;

  var url = TrimString(input.value);
  var urlScheme = GetScheme(url);

  
  checkbox.checked = url.length > 0 && !urlScheme;

  
  var enable = false;

  var docUrl = GetDocumentBaseUrl();
  var docScheme = GetScheme(docUrl);

  if (url && docUrl && docScheme)
  {
    if (urlScheme)
    {
      
      
      
      enable = (GetScheme(MakeRelativeUrl(url)).length == 0);
    }
    else
    {
      
      
      
      
      
      if (url[0] == "#")
      {
        var docFilename = GetFilename(docUrl);
        enable = docFilename.length > 0;
      }
      else
      {
        
        
        enable = true;
      }
    }
  }

  SetElementEnabled(checkbox, enable);
}


function MakeInputValueRelativeOrAbsolute(checkbox)
{
  var input =  document.getElementById(checkbox.getAttribute("for"));
  if (!input)
    return;

  var docUrl = GetDocumentBaseUrl();
  if (!docUrl)
  {
    
    
    AlertWithTitle("", GetString("SaveToUseRelativeUrl"));
    window.focus();
  }
  else 
  {
    
    
    if (checkbox.checked)
      input.value = MakeRelativeUrl(input.value);
    else
      input.value = MakeAbsoluteUrl(input.value);

    
    SetRelativeCheckbox(checkbox);
  }
}

var IsBlockParent = {
  APPLET: true,
  BLOCKQUOTE: true,
  BODY: true,
  CENTER: true,
  DD: true,
  DIV: true,
  FORM: true,
  LI: true,
  NOSCRIPT: true,
  OBJECT: true,
  TD: true,
  TH: true
};

var NotAnInlineParent = {
  COL: true,
  COLGROUP: true,
  DL: true,
  DIR: true,
  MENU: true,
  OL: true,
  TABLE: true,
  TBODY: true,
  TFOOT: true,
  THEAD: true,
  TR: true,
  UL: true
};

function nodeIsBreak(editor, node)
{
  return !node || node.localName == 'BR' || editor.nodeIsBlock(node);
}

function InsertElementAroundSelection(element)
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();

  try {
    
    var range, start, end, offset;
    var count = editor.selection.rangeCount;
    if (count == 1)
      range = editor.selection.getRangeAt(0).cloneRange();
    else
    {
      range = editor.document.createRange();
      start = editor.selection.getRangeAt(0)
      range.setStart(start.startContainer, start.startOffset);
      end = editor.selection.getRangeAt(--count);
      range.setEnd(end.endContainer, end.endOffset);
    }

    
    while (range.startContainer != range.commonAncestorContainer)
      range.setStartBefore(range.startContainer);
    while (range.endContainer != range.commonAncestorContainer)
      range.setEndAfter(range.endContainer);

    if (editor.nodeIsBlock(element))
      
      while (!(range.commonAncestorContainer.localName in IsBlockParent))
        range.selectNode(range.commonAncestorContainer);
    else
    {
      
      if (!nodeIsBreak(editor, range.commonAncestorContainer))
        return false;
      else if (range.commonAncestorContainer.localName in NotAnInlineParent)
        
        do range.selectNode(range.commonAncestorContainer);
        while (range.commonAncestorContainer.localName in NotAnInlineParent);
      else
        
        for (var i = range.startOffset; ; i++)
          if (i == range.endOffset)
            return false;
          else if (nodeIsBreak(editor, range.commonAncestorContainer.childNodes[i]))
            break;
    }

    
    offset = range.startOffset;
    start = range.startContainer.childNodes[offset];
    if (!nodeIsBreak(editor, start))
    {
      while (!nodeIsBreak(editor, start.previousSibling))
      {
        start = start.previousSibling;
        offset--;
      }
    }
    end = range.endContainer.childNodes[range.endOffset];
    if (end && !nodeIsBreak(editor, end.previousSibling))
    {
      while (!nodeIsBreak(editor, end))
        end = end.nextSibling;
    }

    
    editor.insertNode(element, range.commonAncestorContainer, offset, true);
    offset = element.childNodes.length;
    if (!editor.nodeIsBlock(element))
      editor.setShouldTxnSetSelection(false);

    
    var empty = true;
    while (start != end)
    {
      var next = start.nextSibling;
      editor.deleteNode(start);
      editor.insertNode(start, element, element.childNodes.length);
      empty = false;
      start = next;
    }
    if (!editor.nodeIsBlock(element))
      editor.setShouldTxnSetSelection(true);
    else
    {
      
      if (start && start.localName == 'BR')
      {
        editor.deleteNode(start);
        editor.insertNode(start, element, element.childNodes.length);
        empty = false;
      }
      
      if (empty)
        editor.insertNode(editor.createElementWithDefaults("br"), element, element.childNodes.length);

      
      editor.insertNode(editor.document.createTextNode(""), element, offset);
    }
  }
  finally {
    editor.endTransaction();
  }

  return true;
}

function nodeIsBlank(node)
{
  return node && node.nodeType == Node.TEXT_NODE && !/\S/.test(node.data);
}

function nodeBeginsBlock(editor, node)
{
  while (nodeIsBlank(node))
    node = node.nextSibling;
  return nodeIsBreak(editor, node);
}

function nodeEndsBlock(editor, node)
{
  while (nodeIsBlank(node))
    node = node.previousSibling;
  return nodeIsBreak(editor, node);
}


function RemoveBlockContainer(element)
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();

  try {
    var range = editor.document.createRange();
    range.selectNode(element);
    var offset = range.startOffset;
    var parent = element.parentNode;

    
    if (!nodeBeginsBlock(editor, element.nextSibling) &&
        !nodeEndsBlock(editor, element.lastChild))
      editor.insertNode(editor.createElementWithDefaults("br"), parent, range.endOffset);

    
    if (!nodeEndsBlock(editor, element.previousSibling) &&
        !nodeBeginsBlock(editor, element.firstChild || element.nextSibling))
      editor.insertNode(editor.createElementWithDefaults("br"), parent, offset++);

    
    editor.deleteNode(element);

    
    for (var i = 0; i < element.childNodes.length; i++)
      editor.insertNode(element.childNodes[i].cloneNode(true), parent, offset++);
  }
  finally {
    editor.endTransaction();
  }
}


function RemoveContainer(element)
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();

  try {
    var range = editor.document.createRange();
    var parent = element.parentNode;
    
    
    
    for (var i = 0; i < element.childNodes.length; i++) {
      range.selectNode(element);
      editor.insertNode(element.childNodes[i].cloneNode(true), parent, range.startOffset);
    }
    
    editor.deleteNode(element);
  }
  finally {
    editor.endTransaction();
  }
}

function FillLinkMenulist(linkMenulist, headingsArray)
{
  var menupopup = linkMenulist.firstChild;
  var editor = GetCurrentEditor();
  try {
    var treeWalker = editor.document.createTreeWalker(editor.document, 1, null, true);
    var headingList = [];
    var anchorList = []; 
    var anchorMap  = {}; 
    var anchor;
    var i;
    for (var element = treeWalker.nextNode(); element; element = treeWalker.nextNode())
    {
      
      
      
      
      if (element instanceof HTMLHeadingElement && element.textContent &&
          !(element.firstChild instanceof HTMLAnchorElement && element.firstChild.name))
        headingList.push(element);

      
      if (element instanceof HTMLAnchorElement && element.name)
      {
        anchor = '#' + element.name;
        if (!(anchor in anchorMap))
        {
          anchorList.push({anchor: anchor, sortkey: anchor.toLowerCase()});
          anchorMap[anchor] = true;
        }
      }
      
      
      if (element.id)
      {
        anchor = '#' + element.id;
        if (!(anchor in anchorMap))
        {
          anchorList.push({anchor: anchor, sortkey: anchor.toLowerCase()});
          anchorMap[anchor] = true;
        }
      }
    }
    
    for (i = 0; i < headingList.length; i++)
    {
      var heading = headingList[i];

      
      
      anchor = '#' + ConvertToCDATAString(TruncateStringAtWordEnd(heading.textContent, 40, false));

      
      while (anchor in anchorMap)
        anchor += "_";
      anchorList.push({anchor: anchor, sortkey: anchor.toLowerCase()});
      anchorMap[anchor] = true;

      
      headingsArray[anchor] = heading;
    }
    if (anchorList.length)
    {
      
      function compare(a, b)
      {
        if(a.sortkey < b.sortkey) return -1;
        if(a.sortkey > b.sortkey) return 1;
        return 0;
      }
      anchorList.sort(compare);

      for (i = 0; i < anchorList.length; i++)
        createMenuItem(menupopup,anchorList[i].anchor);
    }
    else
    {
      var item = createMenuItem(menupopup, GetString("NoNamedAnchorsOrHeadings"));
      item.setAttribute("disabled", "true");
    }
  } catch (e) {}
}

function createMenuItem(aMenuPopup, aLabel)
{
  var menuitem = document.createElement("menuitem");
  menuitem.setAttribute("label", aLabel);
  aMenuPopup.appendChild(menuitem);
  return menuitem;
}


function chooseLinkFile()
{
  
  var fileName = GetLocalFileURL("html, img");
  if (fileName) 
  {
    
    if (gHaveDocumentUrl)
      fileName = MakeRelativeUrl(fileName);

    gDialog.hrefInput.value = fileName;

    
    
    ChangeLinkLocation();
  }
  
  SetTextboxFocus(gDialog.hrefInput);
}

