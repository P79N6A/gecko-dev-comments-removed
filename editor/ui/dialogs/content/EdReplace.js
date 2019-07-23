








































var gReplaceDialog;      
var gFindInst;           
var gFindService;        
var gEditor;             

function initDialogObject()
{
  
  gReplaceDialog = {};
  gReplaceDialog.findInput       = document.getElementById("dialog.findInput");
  gReplaceDialog.replaceInput    = document.getElementById("dialog.replaceInput");
  gReplaceDialog.caseSensitive   = document.getElementById("dialog.caseSensitive");
  gReplaceDialog.wrap            = document.getElementById("dialog.wrap");
  gReplaceDialog.searchBackwards = document.getElementById("dialog.searchBackwards");
  gReplaceDialog.findNext        = document.getElementById("findNext");
  gReplaceDialog.replace         = document.getElementById("replace");
  gReplaceDialog.replaceAndFind  = document.getElementById("replaceAndFind");
  gReplaceDialog.replaceAll      = document.getElementById("replaceAll");
}

function loadDialog()
{
  
  
  
  gReplaceDialog.findInput.value         = (gFindInst.searchString
                                            ? gFindInst.searchString
                                            : gFindService.searchString);
  gReplaceDialog.replaceInput.value = gFindService.replaceString;
  gReplaceDialog.caseSensitive.checked   = (gFindInst.matchCase
                                            ? gFindInst.matchCase
                                            : gFindService.matchCase);
  gReplaceDialog.wrap.checked            = (gFindInst.wrapFind
                                            ? gFindInst.wrapFind
                                            : gFindService.wrapFind);
  gReplaceDialog.searchBackwards.checked = (gFindInst.findBackwards
                                            ? gFindInst.findBackwards
                                            : gFindService.findBackwards);

  doEnabling();
}

function onLoad()
{
  
  var editorElement = window.arguments[0];

  
  gEditor = editorElement.getEditor(editorElement.contentWindow);
  if (!gEditor)
  {
    window.close();
    return;
  }

  
  gFindInst = editorElement.webBrowserFind;

  try {
  
    gFindService = Components.classes["@mozilla.org/find/find_service;1"]
                         .getService(Components.interfaces.nsIFindService);
  } catch(e) { dump("No find service!\n"); gFindService = 0; }

  
  initDialogObject();

  
  

  
  loadDialog();

  if (gReplaceDialog.findInput.value)
    gReplaceDialog.findInput.select();
  else
    gReplaceDialog.findInput.focus();
}

function onUnload() {
  
  gFindReplaceData.replaceDialog = null;
}

function saveFindData()
{
  
  if (gFindService)
  {
    gFindService.searchString  = gReplaceDialog.findInput.value;
    gFindService.matchCase     = gReplaceDialog.caseSensitive.checked;
    gFindService.wrapFind      = gReplaceDialog.wrap.checked;
    gFindService.findBackwards = gReplaceDialog.searchBackwards.checked;
  }
}

function setUpFindInst()
{
  gFindInst.searchString  = gReplaceDialog.findInput.value;
  gFindInst.matchCase     = gReplaceDialog.caseSensitive.checked;
  gFindInst.wrapFind      = gReplaceDialog.wrap.checked;
  gFindInst.findBackwards = gReplaceDialog.searchBackwards.checked;
}

function onFindNext()
{
  
  saveFindData();
  
  setUpFindInst();

  
  var result = gFindInst.findNext();

  if (!result)
  {
    var bundle = document.getElementById("findBundle");
    AlertWithTitle(null, bundle.getString("notFoundWarning"));
    SetTextboxFocus(gReplaceDialog.findInput);
    gReplaceDialog.findInput.select();
    gReplaceDialog.findInput.focus();
    return false;
  } 
  return true;
}

function onReplace()
{
  if (!gEditor)
    return false;

  
  var selection = gEditor.selection;

  var selStr = selection.toString();
  var specStr = gReplaceDialog.findInput.value;
  if (!gReplaceDialog.caseSensitive.checked)
  {
    selStr = selStr.toLowerCase();
    specStr = specStr.toLowerCase();
  }
  
  
  
  var matches = true;
  var specLen = specStr.length;
  var selLen = selStr.length;
  if (selLen < specLen)
    matches = false;
  else
  {
    var specArray = specStr.match(/\S+|\s+/g);
    var selArray = selStr.match(/\S+|\s+/g);
    if ( specArray.length != selArray.length)
      matches = false;
    else
    {
      for (var i=0; i<selArray.length; i++)
      {
        if (selArray[i] != specArray[i])
        {
          if ( /\S/.test(selArray[i][0]) || /\S/.test(specArray[i][0]) )
          {
            
            matches = false;
            break;
          }
          else if ( selArray[i].length < specArray[i].length )
          {
            
            
            matches = false;
            break;
          }
        }
      }
    }
  }

  
  
  
  
  if (!matches)
    return false;

  
  saveFindData();

  
  
  var newRange;
  if (gReplaceDialog.searchBackwards.checked && selection.rangeCount > 0)
  {
    newRange = selection.getRangeAt(0).cloneRange();
    newRange.collapse(true);
  }

  
  
  var replStr = gReplaceDialog.replaceInput.value;
  if (replStr == "")
    gEditor.deleteSelection(0);
  else
    gEditor.insertText(replStr);

  
  if (gReplaceDialog.searchBackwards.checked && newRange)
  {
    gEditor.selection.removeAllRanges();
    gEditor.selection.addRange(newRange);
  }

  return true;
}

function onReplaceAll()
{
  if (!gEditor)
    return;

  var findStr = gReplaceDialog.findInput.value;
  var repStr = gReplaceDialog.replaceInput.value;

  
  saveFindData();

  var finder = Components.classes["@mozilla.org/embedcomp/rangefind;1"].createInstance().QueryInterface(Components.interfaces.nsIFind);

  finder.caseSensitive = gReplaceDialog.caseSensitive.checked;
  finder.findBackwards = gReplaceDialog.searchBackwards.checked;

  
  
  gEditor.beginTransaction();

  
  try {
    
    
    var selection = gEditor.selection;
    var selecRange;
    if (selection.rangeCount > 0)
      selecRange = selection.getRangeAt(0);
    var origRange = selecRange.cloneRange();

    
    var wholeDocRange = gEditor.document.createRange();
    var rootNode = gEditor.rootElement.QueryInterface(Components.interfaces.nsIDOMNode);
    wholeDocRange.selectNodeContents(rootNode);

    
    var endPt = gEditor.document.createRange();

    if (gReplaceDialog.searchBackwards.checked)
    {
      endPt.setStart(wholeDocRange.startContainer, wholeDocRange.startOffset);
      endPt.setEnd(wholeDocRange.startContainer, wholeDocRange.startOffset);
    }
    else
    {
      endPt.setStart(wholeDocRange.endContainer, wholeDocRange.endOffset);
      endPt.setEnd(wholeDocRange.endContainer, wholeDocRange.endOffset);
    }

    
    var foundRange;
    var searchRange = wholeDocRange.cloneRange();
    while ((foundRange = finder.Find(findStr, searchRange,
                                     selecRange, endPt)) != null)
    {
      gEditor.selection.removeAllRanges();
      gEditor.selection.addRange(foundRange);

      
      
      
      if (gReplaceDialog.searchBackwards.checked)
      {
        selecRange = foundRange.cloneRange();
        selecRange.setEnd(selecRange.startContainer, selecRange.startOffset);
      }

      
      
      if (repStr == "")
        gEditor.deleteSelection(0);
      else
        gEditor.insertText(repStr);

      
      if (!gReplaceDialog.searchBackwards.checked)
      {
        selection = gEditor.selection;
        if (selection.rangeCount <= 0) {
          gEditor.endTransaction();
          return;
        }
        selecRange = selection.getRangeAt(0).cloneRange();
      }
    }

    
    if (!gReplaceDialog.wrap.checked) {
      gEditor.endTransaction();
      return;
    }

    
    if (gReplaceDialog.searchBackwards.checked)
    {
      
      origRange.setStart(origRange.endContainer, origRange.endOffset);
      
      selecRange.setEnd(wholeDocRange.endContainer, wholeDocRange.endOffset);
      selecRange.setStart(wholeDocRange.endContainer, wholeDocRange.endOffset);
    }
    else
    {
      
      origRange.setEnd(origRange.startContainer, origRange.startOffset);
      
      selecRange.setStart(wholeDocRange.startContainer,
                          wholeDocRange.startOffset);
      selecRange.setEnd(wholeDocRange.startContainer, wholeDocRange.startOffset);
    }

    while ((foundRange = finder.Find(findStr, wholeDocRange,
                                     selecRange, origRange)) != null)
    {
      gEditor.selection.removeAllRanges();
      gEditor.selection.addRange(foundRange);

      
      if (gReplaceDialog.searchBackwards.checked)
      {
        selecRange = foundRange.cloneRange();
        selecRange.setEnd(selecRange.startContainer, selecRange.startOffset);
      }

      
      
      if (repStr == "")
        gEditor.deleteSelection(0);
      else
        gEditor.insertText(repStr);

      
      if (!gReplaceDialog.searchBackwards.checked)
      {
        selection = gEditor.selection;
        if (selection.rangeCount <= 0) {
          gEditor.endTransaction();
          return;
        }
        selecRange = selection.getRangeAt(0);
      }
    }
  } 
  catch (e) { }

  gEditor.endTransaction();
}

function doEnabling()
{
  var findStr = gReplaceDialog.findInput.value;
  var repStr = gReplaceDialog.replaceInput.value;
  gReplaceDialog.enabled = findStr;
  gReplaceDialog.findNext.disabled = !findStr;
  gReplaceDialog.replace.disabled = !findStr;
  gReplaceDialog.replaceAndFind.disabled = !findStr;
  gReplaceDialog.replaceAll.disabled = !findStr;
}
