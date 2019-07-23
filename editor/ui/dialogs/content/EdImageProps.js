









































var gAnchorElement = null;
var gOriginalHref = "";
var gHNodeArray = {};



function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  ImageStartup();
  gDialog.hrefInput        = document.getElementById("hrefInput");
  gDialog.makeRelativeLink = document.getElementById("MakeRelativeLink");
  gDialog.showLinkBorder   = document.getElementById("showLinkBorder");
  gDialog.linkTab          = document.getElementById("imageLinkTab");

  
  var tagName = "img";
  if ("arguments" in window && window.arguments[0])
  {
    imageElement = window.arguments[0];
    
    gDialog.linkTab.parentNode.removeChild(gDialog.linkTab);
    gDialog.linkTab = null;
  }
  else
  {
    
    try {
      imageElement = editor.getSelectedElement("input");

      if (!imageElement || imageElement.getAttribute("type") != "image") {
        
        imageElement = editor.getSelectedElement(tagName);
        if (imageElement)
          gAnchorElement = editor.getElementOrParentByTagName("href", imageElement);
      }
    } catch (e) {}

  }

  if (imageElement)
  {
    
    if (imageElement.hasAttribute("src"))
    {
      gInsertNewImage = false;
      gActualWidth  = imageElement.naturalWidth;
      gActualHeight = imageElement.naturalHeight;
    }
  }
  else
  {
    gInsertNewImage = true;

    
    
    try {
      imageElement = editor.createElementWithDefaults(tagName);
    } catch(e) {}

    if (!imageElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
    try {
      gAnchorElement = editor.getSelectedElement("href");
    } catch (e) {}
  }

  
  globalElement = imageElement.cloneNode(false);

  
  gHaveDocumentUrl = GetDocumentBaseUrl();

  InitDialog();
  if (gAnchorElement)
    gOriginalHref = gAnchorElement.getAttribute("href");
  gDialog.hrefInput.value = gOriginalHref;

  FillLinkMenulist(gDialog.hrefInput, gHNodeArray);
  ChangeLinkLocation();

  
  gOriginalSrc = gDialog.srcInput.value;

  
  gDialog.constrainCheckbox.checked =
    gDialog.widthUnitsMenulist.selectedIndex == 0 &&
    gDialog.heightUnitsMenulist.selectedIndex == 0;

  
  if (gDialog.linkTab && "arguments" in window && window.arguments[1])
  {
    document.getElementById("TabBox").selectedTab = gDialog.linkTab;
    SetTextboxFocus(gDialog.hrefInput);
  }
  else
    SetTextboxFocus(gDialog.srcInput);

  SetWindowLocation();
}




function InitDialog()
{
  InitImage();
  var border = TrimString(gDialog.border.value);
  gDialog.showLinkBorder.checked = border != "" && border > 0;
}

function ChangeLinkLocation()
{
  SetRelativeCheckbox(gDialog.makeRelativeLink);
  gDialog.showLinkBorder.disabled = !TrimString(gDialog.hrefInput.value);
}

function ToggleShowLinkBorder()
{
  if (gDialog.showLinkBorder.checked)
  {
    var border = TrimString(gDialog.border.value);
    if (!border || border == "0")
      gDialog.border.value = "2";
  }
  else
  {
    gDialog.border.value = "0";
  }
}



function ValidateData()
{
  return ValidateImage();
}

function onAccept()
{
  
  gDoAltTextError = true;

  if (ValidateData())
  {
    if ("arguments" in window && window.arguments[0])
    {
      SaveWindowLocation();
      return true;
    }

    var editor = GetCurrentEditor();

    editor.beginTransaction();

    try
    {
      if (gRemoveImageMap)
      {
        globalElement.removeAttribute("usemap");
        if (gImageMap)
        {
          editor.deleteNode(gImageMap);
          gInsertNewIMap = true;
          gImageMap = null;
        }
      }
      else if (gImageMap)
      {
        
        









        
        var mapName = gImageMap.getAttribute("name");
        if (mapName != "")
        {
          globalElement.setAttribute("usemap", ("#"+mapName));
          if (globalElement.getAttribute("border") == "")
            globalElement.setAttribute("border", 0);
        }
      }

      
      var href = gDialog.hrefInput.value;
      if (href != gOriginalHref)
      {
        if (href && !gInsertNewImage)
          EditorSetTextProperty("a", "href", href);
        else
          EditorRemoveTextProperty("href", "");
      }

      
      if (href)
      {
        if (gDialog.showLinkBorder.checked)
        {
          
          if (!globalElement.hasAttribute("border"))
            globalElement.setAttribute("border", "2");
        }
        else
          globalElement.setAttribute("border", "0");
      }

      if (gInsertNewImage)
      {
        if (href) {
          var linkElement = editor.createElementWithDefaults("a");
          linkElement.setAttribute("href", href);
          linkElement.appendChild(imageElement);
          editor.insertElementAtSelection(linkElement, true);
        }
        else
          
          editor.insertElementAtSelection(imageElement, true);
      }

      
      
      if (href in gHNodeArray)
      {
        var anchorNode = editor.createElementWithDefaults("a");
        if (anchorNode)
        {
          anchorNode.name = href.substr(1);
          
          editor.insertNode(anchorNode, gHNodeArray[href], 0, false);
        }
      }
      
      
      editor.cloneAttributes(imageElement, globalElement);

      
      
      if (gImageMap && gInsertNewIMap)
      {
        
        var body = editor.rootElement;
        editor.setShouldTxnSetSelection(false);
        editor.insertNode(gImageMap, body, 0);
        editor.setShouldTxnSetSelection(true);
      }
    }
    catch (e)
    {
      dump(e);
    }

    editor.endTransaction();

    SaveWindowLocation();
    return true;
  }

  gDoAltTextError = false;

  return false;
}
