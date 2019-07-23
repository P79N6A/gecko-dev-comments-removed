





































function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  gDialog = {
    inputName:      document.getElementById( "InputName" ),
    inputDisabled:  document.getElementById( "InputDisabled" ),
    inputTabIndex:  document.getElementById( "InputTabIndex" )
  };

  ImageStartup();

  
  var tagName = "input";
  try {
    imageElement = editor.getSelectedElement(tagName);
  } catch (e) {}

  if (imageElement)
  {
    
    gInsertNewImage = false;
  }
  else
  {
    gInsertNewImage = true;

    
    
    try {
      imageElement = editor.createElementWithDefaults(tagName);
    } catch(e) {}

    if (!imageElement )
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
    var imgElement;
    try {
      imgElement = editor.getSelectedElement("img");
    } catch(e) {}

    if (imgElement)
    {
      
      var attributes = ["src", "alt", "width", "height", "hspace", "vspace", "border", "align", "usemap", "ismap"];
      for (i in attributes)
        imageElement.setAttribute(attributes[i], imgElement.getAttribute(attributes[i]));
    }
  }

  
  globalElement = imageElement.cloneNode(false);

  
  gHaveDocumentUrl = GetDocumentBaseUrl();

  InitDialog();

  
  gOriginalSrc = gDialog.srcInput.value;

  
  gDialog.constrainCheckbox.checked =
    gDialog.widthUnitsMenulist.selectedIndex == 0 &&
    gDialog.heightUnitsMenulist.selectedIndex == 0;

  SetTextboxFocus(gDialog.inputName);

  SetWindowLocation();
}

function InitDialog()
{
  InitImage();
  gDialog.inputName.value = globalElement.getAttribute("name");
  gDialog.inputDisabled.setAttribute("checked", globalElement.hasAttribute("disabled"));
  gDialog.inputTabIndex.value = globalElement.getAttribute("tabindex");
}

function ValidateData()
{
  if (!ValidateImage())
    return false;
  if (gDialog.inputName.value)
    globalElement.setAttribute("name", gDialog.inputName.value);
  else
    globalElement.removeAttribute("name");
  if (gDialog.inputTabIndex.value)
    globalElement.setAttribute("tabindex", gDialog.inputTabIndex.value);
  else
    globalElement.removeAttribute("tabindex");
  if (gDialog.inputDisabled.checked)
    globalElement.setAttribute("disabled", "");
  else
    globalElement.removeAttribute("disabled");
  globalElement.setAttribute("type", "image");
  return true;
}

function onAccept()
{
  
  
  
  
  gDoAltTextError = true;

  if (ValidateData())
  {

    var editor = GetCurrentEditor();
    editor.beginTransaction();

    try {
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

      if (gInsertNewImage)
      {
        
        
        editor.insertElementAtSelection(imageElement, true);
      }
      editor.cloneAttributes(imageElement, globalElement);

      
      
      if (gImageMap && gInsertNewIMap)
      {
        
        var body = editor.rootElement;
        editor.setShouldTxnSetSelection(false);
        editor.insertNode(gImageMap, body, 0);
        editor.setShouldTxnSetSelection(true);
      }
    } catch (e) {}

    editor.endTransaction();

    SaveWindowLocation();

    return true;
  }
  return false;
}

