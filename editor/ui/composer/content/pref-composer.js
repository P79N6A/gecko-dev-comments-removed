






































var defaultTextColor = "#000000";
var defaultLinkColor = "#000099";
var defaultActiveColor = "#000099";
var defaultVisitedColor = "#990099";
var defaultBackgroundColor = "#FFFFFF";

var customTextColor;
var customLinkColor;
var customActiveColor;
var customVisitedColor;
var customBackgroundColor;
var previewBGColor;
var backgroundImage = "";


const styleStr =       "style";
const textStr =        "text";
const linkStr =        "link";
const vlinkStr =       "vlink";
const alinkStr =       "alink";
const bgcolorStr =     "bgcolor";
const backgroundStr =  "background";
const colorStyle =     "color: ";
const backColorStyle = "background-color: ";
const backImageStyle = "; background-image: url(";

var browserColors;
var dialog;

function Startup()
{
  gDialog.ColorPreview = document.getElementById("ColorPreview");
  gDialog.NormalText = document.getElementById("NormalText");
  gDialog.LinkText = document.getElementById("LinkText");
  gDialog.ActiveLinkText = document.getElementById("ActiveLinkText");
  gDialog.VisitedLinkText = document.getElementById("VisitedLinkText");
  gDialog.DefaultColorsRadio = document.getElementById("DefaultColorsRadio");
  gDialog.CustomColorsRadio = document.getElementById("CustomColorsRadio");
  gDialog.BackgroundImageInput = document.getElementById("BackgroundImageInput");

  
  gDialog.NormalData = document.getElementById("textData");
  gDialog.LinkData = document.getElementById("linkData");
  gDialog.ActiveLinkData = document.getElementById("aLinkData");
  gDialog.VisitedLinkData = document.getElementById("fLinkData");
  gDialog.BackgroundColorData = document.getElementById("backgroundColorData");
  gDialog.BackgroundImageData = document.getElementById("backgroundImageData");

  browserColors = GetDefaultBrowserColors();

  
  defaultTextColor = browserColors.TextColor;
  defaultLinkColor = browserColors.LinkColor;
  defaultActiveColor = browserColors.ActiveLinkColor;
  defaultVisitedColor =  browserColors.VisitedLinkColor;
  defaultBackgroundColor=  browserColors.BackgroundColor;

  
  customTextColor = gDialog.NormalData.getAttribute("value"); 
  customLinkColor = gDialog.LinkData.getAttribute("value");
  customActiveColor = gDialog.ActiveLinkData.getAttribute("value");
  customVisitedColor = gDialog.VisitedLinkData.getAttribute("value");
  customBackgroundColor = gDialog.BackgroundColorData.getAttribute("value");
  backgroundImage = gDialog.BackgroundImageData.getAttribute("value");
  if (backgroundImage)
    gDialog.BackgroundImageInput.value = backgroundImage;

  
  if( document.getElementById( "useCustomColors" ).value == "true" )
    UseCustomColors();
  else
    UseDefaultColors();

  return true;
}

function GetColorAndUpdate(ColorWellID)
{
  
  if (!gDialog.CustomColorsRadio.selected) return;

  var colorWell = document.getElementById(ColorWellID);
  if (!colorWell) return;

  
  var colorObj = { NoDefault:true, Type:"", TextColor:0, PageColor:0, Cancel:false };

  switch( ColorWellID )
  {
    case "textCW":
      colorObj.Type = "Text";
      colorObj.TextColor = customTextColor;
      break;
    case "linkCW":
      colorObj.Type = "Link";
      colorObj.TextColor = customLinkColor;
      break;
    case "activeCW":
      colorObj.Type = "ActiveLink";
      colorObj.TextColor = customActiveColor;
      break;
    case "visitedCW":
      colorObj.Type = "VisitedLink";
      colorObj.TextColor = customVisitedColor;
      break;
    case "backgroundCW":
      colorObj.Type = "Page";
      colorObj.PageColor = customBackgroundColor;
      break;
  }

  window.openDialog("chrome://editor/content/EdColorPicker.xul", "_blank", "chrome,close,titlebar,modal", "", colorObj);

  
  if (colorObj.Cancel)
    return;

  var color = "";
  switch( ColorWellID )
  {
    case "textCW":
      color = customTextColor = colorObj.TextColor;
      gDialog.NormalData.setAttribute("value", color); 
      break;
    case "linkCW":
      color = customLinkColor = colorObj.TextColor;
      gDialog.LinkData.setAttribute("value", color);
      break;
    case "activeCW":
      color = customActiveColor = colorObj.TextColor;
      gDialog.ActiveLinkData.setAttribute("value", color);
      break;
    case "visitedCW":
      color = customVisitedColor = colorObj.TextColor;
      gDialog.VisitedLinkData.setAttribute("value", color);
      break;
    case "backgroundCW":
      color = customBackgroundColor = colorObj.BackgroundColor;
      gDialog.BackgroundColorData.setAttribute("value", color);
      break;
  }
  setColorWell(ColorWellID, color); 
  SetColorPreview(ColorWellID, color);
}

function SetColorPreview(ColorWellID, color)
{
  switch( ColorWellID )
  {
    case "textCW":
      gDialog.NormalText.setAttribute(styleStr,colorStyle+color);
      break;
    case "linkCW":
      gDialog.LinkText.setAttribute(styleStr,colorStyle+color);
      break;
    case "activeCW":
      gDialog.ActiveLinkText.setAttribute(styleStr,colorStyle+color);
      break;
    case "visitedCW":
      gDialog.VisitedLinkText.setAttribute(styleStr,colorStyle+color);
      break;
    case "backgroundCW":
      
      var styleValue = backColorStyle+color;
      if (backgroundImage)
        styleValue += ";"+backImageStyle+backgroundImage+");";

      gDialog.ColorPreview.setAttribute(styleStr,styleValue);
      previewBGColor = color;
      break;
  }
}

function UseCustomColors()
{
  SetElementEnabledById("TextButton", true);
  SetElementEnabledById("LinkButton", true);
  SetElementEnabledById("ActiveLinkButton", true);
  SetElementEnabledById("VisitedLinkButton", true);
  SetElementEnabledById("BackgroundButton", true);
  SetElementEnabledById("Text", true);
  SetElementEnabledById("Link", true);
  SetElementEnabledById("Active", true);
  SetElementEnabledById("Visited", true);
  SetElementEnabledById("Background", true);

  SetColorPreview("textCW",       customTextColor);
  SetColorPreview("linkCW",       customLinkColor);
  SetColorPreview("activeCW",     customActiveColor);
  SetColorPreview("visitedCW",    customVisitedColor);
  SetColorPreview("backgroundCW", customBackgroundColor);

  setColorWell("textCW",          customTextColor);
  setColorWell("linkCW",          customLinkColor);
  setColorWell("activeCW",        customActiveColor);
  setColorWell("visitedCW",       customVisitedColor);
  setColorWell("backgroundCW",    customBackgroundColor);

  gDialog.NormalData.setAttribute("value",          customTextColor); 
  gDialog.LinkData.setAttribute("value",            customLinkColor);
  gDialog.ActiveLinkData.setAttribute("value",      customActiveColor);
  gDialog.VisitedLinkData.setAttribute("value",     customVisitedColor);
  gDialog.BackgroundColorData.setAttribute("value", customBackgroundColor);
}

function UseDefaultColors()
{
  SetColorPreview("textCW",       defaultTextColor);
  SetColorPreview("linkCW",       defaultLinkColor);
  SetColorPreview("activeCW",     defaultActiveColor);
  SetColorPreview("visitedCW",    defaultVisitedColor);
  SetColorPreview("backgroundCW", defaultBackgroundColor);

  
  setColorWell("textCW",       "");
  setColorWell("linkCW",       "");
  setColorWell("activeCW",     "");
  setColorWell("visitedCW",    "");
  setColorWell("backgroundCW", "");

  
  SetElementEnabledById("TextButton", false);
  SetElementEnabledById("LinkButton", false);
  SetElementEnabledById("ActiveLinkButton", false);
  SetElementEnabledById("VisitedLinkButton", false);
  SetElementEnabledById("BackgroundButton", false);
  SetElementEnabledById("Text", false);
  SetElementEnabledById("Link", false);
  SetElementEnabledById("Active", false);
  SetElementEnabledById("Visited", false);
  SetElementEnabledById("Background", false);
  
  
  
}
  
function ChooseImageFile()
{
  
  var fileName = GetLocalFileURL("img");
  if (fileName)
  {
    gDialog.BackgroundImageInput.value = fileName;
    ValidateAndPreviewImage(true);
  }
  SetTextboxFocus(gDialog.BackgroundImageInput);
}

function ChangeBackgroundImage()
{
  
  ValidateAndPreviewImage(false);
}

function ValidateAndPreviewImage(ShowErrorMessage)
{
  
  var styleValue = backColorStyle+previewBGColor+";";

  var image = TrimString(gDialog.BackgroundImageInput.value);
  if (image)
  {
    backgroundImage = image;
    
    styleValue += backImageStyle+backgroundImage+");";
  }
  else
    backgroundImage = "";

  
  gDialog.ColorPreview.setAttribute(styleStr, styleValue);
  
  
  gDialog.BackgroundImageData.setAttribute("value", backgroundImage ? backgroundImage : "");
}

