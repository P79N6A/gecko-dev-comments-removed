




































var gInsertNew = true;
var gAnchorElement = null;
var gOriginalName = "";
const kTagName = "anchor";


function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  gDialog.OkButton  = document.documentElement.getButton("accept");
  gDialog.NameInput = document.getElementById("nameInput");

  
  gAnchorElement = editor.getSelectedElement(kTagName);

  if (gAnchorElement) {
    
    gInsertNew = false;

    
    globalElement = gAnchorElement.cloneNode(false);
    gOriginalName = ConvertToCDATAString(gAnchorElement.name);
  } else {
    gInsertNew = true;
    
    
    gAnchorElement = editor.createElementWithDefaults(kTagName);
    if (gAnchorElement) {
      
      var name = GetSelectionAsText();
      
      
      name = ConvertToCDATAString(TruncateStringAtWordEnd(name, 40, false));
      
      if (AnchorNameExists(name))
        name += "_"

      
      globalElement = gAnchorElement.cloneNode(false);
      globalElement.setAttribute("name",name);
    }
  }
  if(!gAnchorElement)
  {
    dump("Failed to get selected element or create a new one!\n");
    window.close();
    return;
  }

  InitDialog();
  
  DoEnabling();
  SetTextboxFocus(gDialog.NameInput);
  SetWindowLocation();
}

function InitDialog()
{
  gDialog.NameInput.value = globalElement.getAttribute("name");
}

function ChangeName()
{
  if (gDialog.NameInput.value.length > 0)
  {
    
    
    
    gDialog.NameInput.value = ConvertToCDATAString(gDialog.NameInput.value);
  }
  DoEnabling();
}

function DoEnabling()
{
  var enable = gDialog.NameInput.value.length > 0;
  SetElementEnabled(gDialog.OkButton,  enable);
  SetElementEnabledById("AdvancedEditButton1", enable);
}

function AnchorNameExists(name)
{
  var anchorList;
  try {
    anchorList = GetCurrentEditor().document.anchors;
  } catch (e) {}

  if (anchorList) {
    for (var i = 0; i < anchorList.length; i++) {
      if (anchorList[i].name == name)
        return true;
    }
  }
  return false;
}



function ValidateData()
{
  var name = TrimString(gDialog.NameInput.value);
  if (!name)
  {
      ShowInputErrorMessage(GetString("MissingAnchorNameError"));
      SetTextboxFocus(gDialog.NameInput);
      return false;
  } else {
    
    
    
    name = ConvertToCDATAString(name);

    if (gOriginalName != name && AnchorNameExists(name))
    {
      ShowInputErrorMessage(GetString("DuplicateAnchorNameError").replace(/%name%/,name));            
      SetTextboxFocus(gDialog.NameInput);
      return false;
    }
    globalElement.name = name;
  }
  return true;
}

function onAccept()
{
  if (ValidateData())
  {
    if (gOriginalName != globalElement.name)
    {
      var editor = GetCurrentEditor();
      editor.beginTransaction();

      try {
        
        if (gInsertNew)
        {
          
          
          gAnchorElement.name = globalElement.name;
          editor.insertElementAtSelection(gAnchorElement, false);
        }

        
        editor.cloneAttributes(gAnchorElement, globalElement);

      } catch (e) {}

      editor.endTransaction();
    }
    SaveWindowLocation();
    return true;
  }
  return false;
}
