

















































var gInsertNewImage = true;
var gInsertNewIMap = true;
var gDoAltTextError = false;
var gConstrainOn = false;


var gConstrainWidth  = 0;
var gConstrainHeight = 0;
var imageElement;
var gImageMap = 0;
var gCanRemoveImageMap = false;
var gRemoveImageMap = false;
var gImageMapDisabled = false;
var gActualWidth = "";
var gActualHeight = "";
var gOriginalSrc = "";
var gHaveDocumentUrl = false;
var gTimerID;
var gValidateTab;



var gPreviewImageWidth = 80;
var gPreviewImageHeight = 50;



function ImageStartup()
{
  gDialog.tabBox            = document.getElementById( "TabBox" );
  gDialog.tabLocation       = document.getElementById( "imageLocationTab" );
  gDialog.tabDimensions     = document.getElementById( "imageDimensionsTab" );
  gDialog.tabBorder         = document.getElementById( "imageBorderTab" );
  gDialog.srcInput          = document.getElementById( "srcInput" );
  gDialog.titleInput        = document.getElementById( "titleInput" );
  gDialog.altTextInput      = document.getElementById( "altTextInput" );
  gDialog.altTextRadioGroup = document.getElementById( "altTextRadioGroup" );
  gDialog.altTextRadio      = document.getElementById( "altTextRadio" );
  gDialog.noAltTextRadio    = document.getElementById( "noAltTextRadio" );
  gDialog.customSizeRadio   = document.getElementById( "customSizeRadio" );
  gDialog.actualSizeRadio   = document.getElementById( "actualSizeRadio" );
  gDialog.constrainCheckbox = document.getElementById( "constrainCheckbox" );
  gDialog.widthInput        = document.getElementById( "widthInput" );
  gDialog.heightInput       = document.getElementById( "heightInput" );
  gDialog.widthUnitsMenulist   = document.getElementById( "widthUnitsMenulist" );
  gDialog.heightUnitsMenulist  = document.getElementById( "heightUnitsMenulist" );
  gDialog.imagelrInput      = document.getElementById( "imageleftrightInput" );
  gDialog.imagetbInput      = document.getElementById( "imagetopbottomInput" );
  gDialog.border            = document.getElementById( "border" );
  gDialog.alignTypeSelect   = document.getElementById( "alignTypeSelect" );
  gDialog.ImageHolder       = document.getElementById( "preview-image-holder" );
  gDialog.PreviewWidth      = document.getElementById( "PreviewWidth" );
  gDialog.PreviewHeight     = document.getElementById( "PreviewHeight" );
  gDialog.PreviewSize       = document.getElementById( "PreviewSize" );
  gDialog.PreviewImage      = null;
  gDialog.OkButton          = document.documentElement.getButton("accept");
}




function InitImage()
{
  
  gDialog.srcInput.value = globalElement.getAttribute("src");

  
  SetRelativeCheckbox();

  
  LoadPreviewImage();

  if (globalElement.hasAttribute("title"))
    gDialog.titleInput.value = globalElement.getAttribute("title");

  var hasAltText = globalElement.hasAttribute("alt");
  var altText;
  if (hasAltText)
  {
    altText = globalElement.getAttribute("alt");
    gDialog.altTextInput.value = altText;
  }

  
  
  
  if (!gDialog.altTextRadioGroup.selectedItem || altText)
  {
    if (gInsertNewImage || !hasAltText || (hasAltText && gDialog.altTextInput.value))
    {
      SetAltTextDisabled(false);
      gDialog.altTextRadioGroup.selectedItem = gDialog.altTextRadio;
    }
    else
    {
      SetAltTextDisabled(true);
      gDialog.altTextRadioGroup.selectedItem = gDialog.noAltTextRadio;
    }
  }

  
  var width = InitPixelOrPercentMenulist(globalElement,
                    gInsertNewImage ? null : imageElement,
                    "width", "widthUnitsMenulist", gPixel);
  var height = InitPixelOrPercentMenulist(globalElement,
                    gInsertNewImage ? null : imageElement,
                    "height", "heightUnitsMenulist", gPixel);

  
  SetSizeWidgets(width, height);

  gDialog.widthInput.value  = gConstrainWidth = width ? width : (gActualWidth ? gActualWidth : "");
  gDialog.heightInput.value = gConstrainHeight = height ? height : (gActualHeight ? gActualHeight : "");

  
  gDialog.imagelrInput.value = globalElement.getAttribute("hspace");
  gDialog.imagetbInput.value = globalElement.getAttribute("vspace");

  
  var bv = GetHTMLOrCSSStyleValue(globalElement, "border", "border-top-width");
  if (/px/.test(bv))
  {
    
    bv = RegExp.leftContext;
  }
  else if (bv == "thin")
  {
    bv = "1";
  }
  else if (bv == "medium")
  {
    bv = "3";
  }
  else if (bv == "thick")
  {
    bv = "5";
  }
  gDialog.border.value = bv;

  
  var align = globalElement.getAttribute("align");
  if (align)
    align = align.toLowerCase();

  var imgClass;
  var textID;

  switch ( align )
  {
    case "top":
    case "middle":
    case "right":
    case "left":
      gDialog.alignTypeSelect.value = align;
      break;
    default:  
      gDialog.alignTypeSelect.value = "bottom";
  }

  
  gImageMap = GetImageMap();

  doOverallEnabling();
  doDimensionEnabling();
}

function  SetSizeWidgets(width, height)
{
  if (!(width || height) || (gActualWidth && gActualHeight && width == gActualWidth && height == gActualHeight))
    gDialog.actualSizeRadio.radioGroup.selectedItem = gDialog.actualSizeRadio;

  if (!gDialog.actualSizeRadio.selected)
  {
    gDialog.actualSizeRadio.radioGroup.selectedItem = gDialog.customSizeRadio;

    
    if (gActualWidth && gActualHeight)
    {
      if (gActualWidth > gActualHeight)
        gDialog.constrainCheckbox.checked = (Math.round(gActualHeight * width / gActualWidth) == height);
      else
        gDialog.constrainCheckbox.checked = (Math.round(gActualWidth * height / gActualHeight) == width);
    }
  }
}


function SetAltTextDisabled(disable)
{
  gDialog.altTextInput.disabled = disable;
}

function GetImageMap()
{
  var usemap = globalElement.getAttribute("usemap");
  if (usemap)
  {
    gCanRemoveImageMap = true;
    var mapname = usemap.substring(1, usemap.length);
    var mapCollection;
    try {
      mapCollection = GetCurrentEditor().document.getElementsByName(mapname);
    } catch (e) {}
    if (mapCollection && mapCollection[0] != null)
    {
      gInsertNewIMap = false;
      return mapCollection[0];
    }
  }
  else
  {
    gCanRemoveImageMap = false;
  }

  gInsertNewIMap = true;
  return null;
}

function chooseFile()
{
  if (gTimerID)
    clearTimeout(gTimerID);
  
  var fileName = GetLocalFileURL("img");
  if (fileName)
  {
    
    if (gHaveDocumentUrl)
      fileName = MakeRelativeUrl(fileName);

    gDialog.srcInput.value = fileName;

    SetRelativeCheckbox();
    doOverallEnabling();
  }
  LoadPreviewImage();

  
  SetTextboxFocus(gDialog.srcInput);
}

function PreviewImageLoaded()
{
  if (gDialog.PreviewImage)
  {
    
    gActualWidth  = gDialog.PreviewImage.naturalWidth;
    gActualHeight = gDialog.PreviewImage.naturalHeight;

    if (gActualWidth && gActualHeight)
    {
      
      var width = gActualWidth;
      var height = gActualHeight;
      if (gActualWidth > gPreviewImageWidth)
      {
          width = gPreviewImageWidth;
          height = gActualHeight * (gPreviewImageWidth / gActualWidth);
      }
      if (height > gPreviewImageHeight)
      {
        height = gPreviewImageHeight;
        width = gActualWidth * (gPreviewImageHeight / gActualHeight);
      }
      gDialog.PreviewImage.width = width;
      gDialog.PreviewImage.height = height;

      gDialog.PreviewWidth.setAttribute("value", gActualWidth);
      gDialog.PreviewHeight.setAttribute("value", gActualHeight);

      gDialog.PreviewSize.collapsed = false;
      gDialog.ImageHolder.collapsed = false;

      SetSizeWidgets(gDialog.widthInput.value, gDialog.heightInput.value);
    }

    if (gDialog.actualSizeRadio.selected)
      SetActualSize();
  }
}

function LoadPreviewImage()
{
  gDialog.PreviewSize.collapsed = true;
  
  gDialog.ImageHolder.collapsed = true;

  var imageSrc = TrimString(gDialog.srcInput.value);
  if (!imageSrc)
    return;

  try {
    
    
    
    
    var IOService = GetIOService();
    if (IOService)
    {
      
      imageSrc = MakeAbsoluteUrl(imageSrc);

      if (GetScheme(imageSrc))
      {
        var uri = IOService.newURI(imageSrc, null, null);
        if (uri)
        {
          var imgCacheService = Components.classes["@mozilla.org/image/cache;1"].getService();
          var imgCache = imgCacheService.QueryInterface(Components.interfaces.imgICache);

          
          imgCache.removeEntry(uri);
        }
      }
    }
  } catch(e) {}

  if (gDialog.PreviewImage)
    removeEventListener("load", PreviewImageLoaded, true);

  if (gDialog.ImageHolder.firstChild)
    gDialog.ImageHolder.removeChild(gDialog.ImageHolder.firstChild);
    
  gDialog.PreviewImage = document.createElementNS("http://www.w3.org/1999/xhtml", "html:img");
  if (gDialog.PreviewImage)
  {
    
    
    
    gDialog.PreviewImage.addEventListener("load", PreviewImageLoaded, true);
    gDialog.PreviewImage.src = imageSrc;
    gDialog.ImageHolder.appendChild(gDialog.PreviewImage);
  }
}

function SetActualSize()
{
  gDialog.widthInput.value = gActualWidth ? gActualWidth : "";
  gDialog.widthUnitsMenulist.selectedIndex = 0;
  gDialog.heightInput.value = gActualHeight ? gActualHeight : "";
  gDialog.heightUnitsMenulist.selectedIndex = 0;
  doDimensionEnabling();
}

function ChangeImageSrc()
{
  if (gTimerID)
    clearTimeout(gTimerID);

  gTimerID = setTimeout("LoadPreviewImage()", 800);

  SetRelativeCheckbox();
  doOverallEnabling();
}

function doDimensionEnabling()
{
  
  var enable = (gDialog.customSizeRadio.selected);

  
  
  
  SetElementEnabledById( "heightInput", enable );
  SetElementEnabledById( "heightLabel", enable );
  SetElementEnabledById( "heightUnitsMenulist", enable );

  SetElementEnabledById( "widthInput", enable );
  SetElementEnabledById( "widthLabel", enable);
  SetElementEnabledById( "widthUnitsMenulist", enable );

  var constrainEnable = enable
         && ( gDialog.widthUnitsMenulist.selectedIndex == 0 )
         && ( gDialog.heightUnitsMenulist.selectedIndex == 0 );

  SetElementEnabledById( "constrainCheckbox", constrainEnable );
}

function doOverallEnabling()
{
  var enabled = TrimString(gDialog.srcInput.value) != "";

  SetElementEnabled(gDialog.OkButton, enabled);
  SetElementEnabledById("AdvancedEditButton1", enabled);
  SetElementEnabledById("imagemapLabel", enabled);

  
  
  SetElementEnabledById("removeImageMap", gCanRemoveImageMap);
}

function ToggleConstrain()
{
  
  
  if (gDialog.constrainCheckbox.checked && !gDialog.constrainCheckbox.disabled
     && (gDialog.widthUnitsMenulist.selectedIndex == 0)
     && (gDialog.heightUnitsMenulist.selectedIndex == 0))
  {
    gConstrainWidth = Number(TrimString(gDialog.widthInput.value));
    gConstrainHeight = Number(TrimString(gDialog.heightInput.value));
  }
}

function constrainProportions( srcID, destID )
{
  var srcElement = document.getElementById(srcID);
  if (!srcElement)
    return;

  var destElement = document.getElementById(destID);
  if (!destElement)
    return;

  
  forceInteger(srcID);

  if (!gActualWidth || !gActualHeight ||
      !(gDialog.constrainCheckbox.checked && !gDialog.constrainCheckbox.disabled))
    return;

  
  if ( (gDialog.widthUnitsMenulist.selectedIndex != 0)
     || (gDialog.heightUnitsMenulist.selectedIndex != 0) )
    return;

  
  
  
  
  if (srcID == "widthInput")
    destElement.value = Math.round( srcElement.value * gActualHeight / gActualWidth );
  else
    destElement.value = Math.round( srcElement.value * gActualWidth / gActualHeight );









}

function editImageMap()
{
  
  if (gInsertNewIMap)
  {
    try {
      gImageMap = GetCurrentEditor().createElementWithDefaults("map");
    } catch (e) {}
  }

  
  window.openDialog("chrome://editor/content/EdImageMap.xul", "_blank", "chrome,close,titlebar,modal", globalElement, gImageMap);
}

function removeImageMap()
{
  gRemoveImageMap = true;
  gCanRemoveImageMap = false;
  SetElementEnabledById("removeImageMap", false);
}

function SwitchToValidatePanel()
{
  if (gDialog.tabBox && gValidateTab && gDialog.tabBox.selectedTab != gValidateTab)
    gDialog.tabBox.selectedTab = gValidateTab;
}



function ValidateImage()
{
  var editor = GetCurrentEditor();
  if (!editor)
    return false;

  gValidateTab = gDialog.tabLocation;
  if (!gDialog.srcInput.value)
  {
    AlertWithTitle(null, GetString("MissingImageError"));
    SwitchToValidatePanel();
    gDialog.srcInput.focus();
    return false;
  }

  
  var src = TrimString(gDialog.srcInput.value);
  var checkbox = document.getElementById("MakeRelativeCheckbox");
  try
  {
    if (checkbox && !checkbox.checked)
    {
      var URIFixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                               .getService(Components.interfaces.nsIURIFixup);
      src = URIFixup.createFixupURI(src, Components.interfaces.nsIURIFixup.FIXUP_FLAG_NONE).spec;
    }
  } catch (e) { }

  globalElement.setAttribute("src", src);

  var title = TrimString(gDialog.titleInput.value);
  if (title)
    globalElement.setAttribute("title", title);
  else
    globalElement.removeAttribute("title");

  
  
  var alt = "";
  var useAlt = gDialog.altTextRadioGroup.selectedItem == gDialog.altTextRadio;
  if (useAlt)
    alt = TrimString(gDialog.altTextInput.value);

  if (gDoAltTextError && useAlt && !alt)
  {
    AlertWithTitle(null, GetString("NoAltText"));
    SwitchToValidatePanel();
    gDialog.altTextInput.focus();
    return false;
  }
  globalElement.setAttribute("alt", alt);

  var width = "";
  var height = "";

  gValidateTab = gDialog.tabDimensions;
  if (!gDialog.actualSizeRadio.selected)
  {
    
    width = ValidateNumber(gDialog.widthInput, gDialog.widthUnitsMenulist, 1, gMaxPixels, 
                           globalElement, "width", false, true);
    if (gValidationError)
      return false;

    height = ValidateNumber(gDialog.heightInput, gDialog.heightUnitsMenulist, 1, gMaxPixels, 
                            globalElement, "height", false, true);
    if (gValidationError)
      return false;
  }

  
  
  if (!width)
    width = gActualWidth;
  if (!height)
    height = gActualHeight;

  
  
  var srcChanged = (src != gOriginalSrc);
  if (width)
    editor.setAttributeOrEquivalent(globalElement, "width", width, true);
  else if (srcChanged)
    editor.removeAttributeOrEquivalent(globalElement, "width", true);

  if (height)
    editor.setAttributeOrEquivalent(globalElement, "height", height, true);
  else if (srcChanged) 
    editor.removeAttributeOrEquivalent(globalElement, "height", true);

  
  gValidateTab = gDialog.tabBorder;
  ValidateNumber(gDialog.imagelrInput, null, 0, gMaxPixels, 
                 globalElement, "hspace", false, true, true);
  if (gValidationError)
    return false;

  ValidateNumber(gDialog.imagetbInput, null, 0, gMaxPixels, 
                 globalElement, "vspace", false, true);
  if (gValidationError)
    return false;

  
  ValidateNumber(gDialog.border, null, 0, gMaxPixels, 
                 globalElement, "border", false, true);
  if (gValidationError)
    return false;

  
  
  
  
  switch ( gDialog.alignTypeSelect.value )
  {
    case "top":
    case "middle":
    case "right":
    case "left":
      editor.setAttributeOrEquivalent( globalElement, "align",
                                       gDialog.alignTypeSelect.value , true);
      break;
    default:
      try {
        editor.removeAttributeOrEquivalent(globalElement, "align", true);
      } catch (e) {}
  }

  return true;
}
