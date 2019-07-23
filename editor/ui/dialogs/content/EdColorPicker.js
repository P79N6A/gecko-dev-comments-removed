








































var insertNew = true;
var tagname = "TAG NAME"
var gColor = "";
var LastPickedColor = "";
var ColorType = "Text";
var TextType = false;
var HighlightType = false;
var TableOrCell = false;
var LastPickedIsDefault = true;
var NoDefault = false;
var gColorObj;


function Startup()
{ 
  if (!window.arguments[1])
  {
    dump("EdColorPicker: Missing color object param\n");
    return;
  }

  
  gColorObj = window.arguments[1];
  gColorObj.Cancel = false;

  gDialog.ColorPicker      = document.getElementById("ColorPicker");
  gDialog.ColorInput       = document.getElementById("ColorInput");
  gDialog.LastPickedButton = document.getElementById("LastPickedButton");
  gDialog.LastPickedColor  = document.getElementById("LastPickedColor");
  gDialog.CellOrTableGroup = document.getElementById("CellOrTableGroup");
  gDialog.TableRadio       = document.getElementById("TableRadio");
  gDialog.CellRadio        = document.getElementById("CellRadio");
  gDialog.ColorSwatch      = document.getElementById("ColorPickerSwatch");
  gDialog.Ok               = document.documentElement.getButton("accept");

  
  
  
  if (gColorObj.Type)
  {
    ColorType = gColorObj.Type;
    
    
    var prefs = GetPrefs();
    var IsCSSPrefChecked = prefs.getBoolPref("editor.use_css");

    if (GetCurrentEditor())
    {
      if (ColorType == "Page" && IsCSSPrefChecked && IsHTMLEditor())
        document.title = GetString("BlockColor");
      else
        document.title = GetString(ColorType + "Color");
    }
  }

  gDialog.ColorInput.value = "";
  var tmpColor;
  var haveTableRadio = false;

  switch (ColorType)
  {
    case "Page":
      tmpColor = gColorObj.PageColor;
      if (tmpColor && tmpColor.toLowerCase() != "window")
        gColor = tmpColor;
      break;
    case "Table":
      if (gColorObj.TableColor)
        gColor = gColorObj.TableColor;
      break;
    case "Cell":
      if (gColorObj.CellColor)
        gColor = gColorObj.CellColor;
      break;
    case "TableOrCell":
      TableOrCell = true;
      document.getElementById("TableOrCellGroup").collapsed = false;
      haveTableRadio = true;
      if (gColorObj.SelectedType == "Cell")
      {
        gColor = gColorObj.CellColor;
        gDialog.CellOrTableGroup.selectedItem = gDialog.CellRadio;
        gDialog.CellRadio.focus();
      }
      else
      {
        gColor = gColorObj.TableColor;
        gDialog.CellOrTableGroup.selectedItem = gDialog.TableRadio;
        gDialog.TableRadio.focus();
      }
      break;
    case "Highlight":
      HighlightType = true;
      if (gColorObj.HighlightColor)
        gColor = gColorObj.HighlightColor;
      break;
    default:
      
      TextType = true;
      tmpColor = gColorObj.TextColor;
      if (tmpColor && tmpColor.toLowerCase() != "windowtext")
        gColor = gColorObj.TextColor;
      break;
  }

  
  SetCurrentColor(gColor);
  gDialog.ColorPicker.initColor(gColor);

  
  if (TextType)
  {
    if ( !("LastTextColor" in gColorObj) || !gColorObj.LastTextColor)
      gColorObj.LastTextColor = gDialog.LastPickedColor.getAttribute("LastTextColor");
    LastPickedColor = gColorObj.LastTextColor;
  }
  else if (HighlightType)
  {
    if ( !("LastHighlightColor" in gColorObj) || !gColorObj.LastHighlightColor)
      gColorObj.LastHighlightColor = gDialog.LastPickedColor.getAttribute("LastHighlightColor");
    LastPickedColor = gColorObj.LastHighlightColor;
  }
  else
  {
    if ( !("LastBackgroundColor" in gColorObj) || !gColorObj.LastBackgroundColor)
      gColorObj.LastBackgroundColor = gDialog.LastPickedColor.getAttribute("LastBackgroundColor");
    LastPickedColor = gColorObj.LastBackgroundColor;
  }
  gDialog.LastPickedColor.setAttribute("style","background-color: "+LastPickedColor);

  
  
  gDialog.Ok.setAttribute("onclick", "SetDefaultToOk()");

  
  
  gDialog.Ok.removeAttribute("default");
  gDialog.LastPickedButton.setAttribute("default","true");

  
  NoDefault = gColorObj.NoDefault;
  if (NoDefault)
  {
    
    document.getElementById("DefaultColorButton").collapsed = true;
  }

  
  if (!haveTableRadio)
    gDialog.ColorPicker.focus();

  SetWindowLocation();
}

function ChangePalette(palette)
{
  gDialog.ColorPicker.setAttribute("palettename", palette);
  window.sizeToContent();
}

function SelectColor()
{
  var color = gDialog.ColorPicker.color;
  if (color)
    SetCurrentColor(color);
}

function RemoveColor()
{
  SetCurrentColor("");
  gDialog.ColorInput.focus();
  SetDefaultToOk();
}

function SelectColorByKeypress(aEvent)
{
  if (aEvent.charCode == aEvent.DOM_VK_SPACE)
  {
    SelectColor();
    SetDefaultToOk();
  }
}

function SelectLastPickedColor()
{
  SetCurrentColor(LastPickedColor);
  if ( onAccept() )
    
    return true;

  return false;
}

function SetCurrentColor(color)
{
  
  if(!color) color = "";
  gColor = TrimString(color).toLowerCase();
  if (gColor == "mixed")
    gColor = "";
  gDialog.ColorInput.value = gColor;
  SetColorSwatch();
}

function SetColorSwatch()
{
  
  var color = TrimString(gDialog.ColorInput.value);
  if (color)
  {
    gDialog.ColorSwatch.setAttribute("style",("background-color:"+color));
    gDialog.ColorSwatch.removeAttribute("default");
  }
  else
  {
    gDialog.ColorSwatch.setAttribute("style",("background-color:inherit"));
    gDialog.ColorSwatch.setAttribute("default","true");
  }
}

function SetDefaultToOk()
{
  gDialog.LastPickedButton.removeAttribute("default");
  gDialog.Ok.setAttribute("default","true");
  LastPickedIsDefault = false;
}

function ValidateData()
{
  if (LastPickedIsDefault)
    gColor = LastPickedColor;
  else
    gColor = gDialog.ColorInput.value;
  
  gColor = TrimString(gColor).toLowerCase();

  

  if (NoDefault && !gColor)
  {
    ShowInputErrorMessage(GetString("NoColorError"));
    SetTextboxFocus(gDialog.ColorInput);
    return false;   
  }
  return true;
}

function onAccept()
{
  if (!ValidateData())
    return false;

  
  if (TextType)
  {
    gColorObj.TextColor = gColor;
    if (gColor.length > 0)
    {
      gDialog.LastPickedColor.setAttribute("LastTextColor", gColor);
      gColorObj.LastTextColor = gColor;
    }
  }
  else if (HighlightType)
  {
    gColorObj.HighlightColor = gColor;
    if (gColor.length > 0)
    {
      gDialog.LastPickedColor.setAttribute("LastHighlightColor", gColor);
      gColorObj.LastHighlightColor = gColor;
    }
  }
  else
  {
    gColorObj.BackgroundColor = gColor;
    if (gColor.length > 0)
    {
      gDialog.LastPickedColor.setAttribute("LastBackgroundColor", gColor);
      gColorObj.LastBackgroundColor = gColor;
    }
    
    if (TableOrCell && gDialog.TableRadio.selected)
      gColorObj.Type = "Table";
  }
  SaveWindowLocation();

  return true; 
}

function onCancelColor()
{
  
  gColorObj.Cancel = true;
  SaveWindowLocation();
  return true;
}
