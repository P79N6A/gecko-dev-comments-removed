






































var gActiveEditor;
var anchorElement = null;
var imageElement = null;
var insertNew = false;
var replaceExistingLink = false;
var insertLinkAtCaret;
var needLinkText = false;
var href;
var newLinkText;
var gHNodeArray = {};
var gHaveNamedAnchors = false;
var gHaveHeadings = false;
var gCanChangeHeadingSelected = true;
var gCanChangeAnchorSelected = true;
var gHaveDocumentUrl = false;



var tagName = "href";


function Startup()
{
  gActiveEditor = GetCurrentEditor();
  if (!gActiveEditor)
  {
    dump("Failed to get active editor!\n");
    window.close();
    return;
  }
  
  gDialog.linkTextCaption     = document.getElementById("linkTextCaption");
  gDialog.linkTextMessage     = document.getElementById("linkTextMessage");
  gDialog.linkTextInput       = document.getElementById("linkTextInput");
  gDialog.hrefInput           = document.getElementById("hrefInput");
  gDialog.makeRelativeLink    = document.getElementById("MakeRelativeLink");
  gDialog.AdvancedEditSection = document.getElementById("AdvancedEdit");

  
  imageElement = gActiveEditor.getSelectedElement("img");

  if (imageElement)
  {
    
    anchorElement = gActiveEditor.getElementOrParentByTagName("href", imageElement);
    if (anchorElement)
    {
      if (anchorElement.childNodes.length > 1)
      {
        
        
        
        anchorElement = anchorElement.cloneNode(false);
        
        replaceExistingLink = true;
      }
    }
  }
  else
  {
    
    
    anchorElement = gActiveEditor.getSelectedElement(tagName);

    if (anchorElement)
    {
      
      gActiveEditor.selectElement(anchorElement);
    }
    else
    {
      
      
      
      
      
      

      anchorElement = gActiveEditor.getElementOrParentByTagName("href", gActiveEditor.selection.anchorNode);
      if (!anchorElement)
        anchorElement = gActiveEditor.getElementOrParentByTagName("href", gActiveEditor.selection.focusNode);

      if (anchorElement)
      {
        
        
        anchorElement = anchorElement.cloneNode(false);
        
        replaceExistingLink = true;
      }
    }
  }

  if(!anchorElement)
  {
    
    anchorElement = gActiveEditor.createElementWithDefaults(tagName);
    insertNew = true;
    
    
  }
  if(!anchorElement)
  {
    dump("Failed to get selected element or create a new one!\n");
    window.close();
    return;
  } 

  
  insertLinkAtCaret = gActiveEditor.selection.isCollapsed;
  
  var selectedText;
  if (insertLinkAtCaret)
  {
    
    gDialog.linkTextCaption.setAttribute("label", GetString("LinkText"));

    
    gDialog.linkTextMessage.setAttribute("value", GetString("EnterLinkText"));
    gDialog.linkTextMessage.setAttribute("accesskey", GetString("EnterLinkTextAccessKey"));
  }
  else
  {
    if (!imageElement)
    {
      
      
      selectedText = GetSelectionAsText();
      if (!selectedText) 
      {
        
        var children = anchorElement.childNodes;
        if (children)
        {
          for(var i=0; i < children.length; i++) 
          {
            var nodeName = children.item(i).nodeName.toLowerCase();
            if (nodeName == "img")
            {
              imageElement = children.item(i);
              break;
            }
          }
        }
      }
    }
    
    if (imageElement)
    {
      gDialog.linkTextCaption.setAttribute("label", GetString("LinkImage"));
      
      
      gDialog.linkTextMessage.setAttribute("value", imageElement.src);
    } else {
      gDialog.linkTextCaption.setAttribute("label", GetString("LinkText"));
      if (selectedText) 
      {
        
        gDialog.linkTextMessage.setAttribute("value", TruncateStringAtWordEnd(ReplaceWhitespace(selectedText, " "), 60, true));
      } else {
        gDialog.linkTextMessage.setAttribute("value", GetString("MixedSelection"));
      }
    }
  }

  
  globalElement = anchorElement.cloneNode(false);

  
  FillLinkMenulist(gDialog.hrefInput, gHNodeArray);

  
  gHaveDocumentUrl = GetDocumentBaseUrl();

  
  InitDialog();
  
  
  
  selectedText = TrimString(selectedText); 
  if (!gDialog.hrefInput.value && TextIsURI(selectedText))
      gDialog.hrefInput.value = selectedText;

  
  if (insertLinkAtCaret) {
    
    SetTextboxFocus(gDialog.linkTextInput);
  } else {
    SetTextboxFocus(gDialog.hrefInput);

    
    gDialog.linkTextInput.hidden = true;
    gDialog.linkTextInput = null;
  }
    
  
  doEnabling();

  SetWindowLocation();
}




function InitDialog()
{
  
  
  gDialog.hrefInput.value = globalElement.getAttribute("href");

  
  SetRelativeCheckbox(gDialog.makeRelativeLink);
}

function doEnabling()
{
  
  var enable = insertNew ? (TrimString(gDialog.hrefInput.value).length > 0) : true;
  
  
  var dialogNode = document.getElementById("linkDlg");
  dialogNode.getButton("accept").disabled = !enable;

  SetElementEnabledById( "AdvancedEditButton1", enable);
}

function ChangeLinkLocation()
{
  SetRelativeCheckbox();
  
  doEnabling();
}



function ValidateData()
{
  href = TrimString(gDialog.hrefInput.value);
  if (href)
  {
    
    
    globalElement.setAttribute("href",href);
  }
  else if (insertNew)
  {
    
    
    ShowInputErrorMessage(GetString("EmptyHREFError"));
    return false;
  }
  if (gDialog.linkTextInput)
  {
    
    
    newLinkText = TrimString(gDialog.linkTextInput.value);
    if (!newLinkText)
    {
      if (href)
        newLinkText = href
      else
      {
        ShowInputErrorMessage(GetString("EmptyLinkTextError"));
        SetTextboxFocus(gDialog.linkTextInput);
        return false;
      }
    }
  }
  return true;
}

function onAccept()
{
  if (ValidateData())
  {
    if (href.length > 0)
    {
      
      gActiveEditor.cloneAttributes(anchorElement, globalElement);

      
      gActiveEditor.beginTransaction();

      
      if (insertLinkAtCaret)
      {
        
        
        var textNode = gActiveEditor.document.createTextNode(newLinkText);
        if (textNode)
          anchorElement.appendChild(textNode);
        try {
          gActiveEditor.insertElementAtSelection(anchorElement, false);
        } catch (e) {
          dump("Exception occured in InsertElementAtSelection\n");
          return true;
        }
      } else if (insertNew || replaceExistingLink)
      {
        
        
        
        try {
          gActiveEditor.insertLinkAroundSelection(anchorElement);
        } catch (e) {
          dump("Exception occured in InsertElementAtSelection\n");
          return true;
        }
      }
      
      if (href in gHNodeArray)
      {
        var anchorNode = gActiveEditor.createElementWithDefaults("a");
        if (anchorNode)
        {
          anchorNode.name = href.substr(1);

          
          
          gActiveEditor.setShouldTxnSetSelection(false);
          gActiveEditor.insertNode(anchorNode, gHNodeArray[href], 0);
          gActiveEditor.setShouldTxnSetSelection(true);
        }
      }
      gActiveEditor.endTransaction();
    } 
    else if (!insertNew)
    {
      
      EditorRemoveTextProperty("href", "");
    }
    SaveWindowLocation();
    return true;
  }
  return false;
}
