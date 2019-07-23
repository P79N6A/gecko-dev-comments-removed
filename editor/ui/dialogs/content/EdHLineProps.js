




































var tagName = "hr";
var gHLineElement;
var width;
var height;
var align;
var shading;
const gMaxHRSize = 1000; 


function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }
  try {
    
    gHLineElement = editor.getSelectedElement(tagName);
  } catch (e) {}

  if (!gHLineElement) {
    
    window.close();
    return;
  }
  gDialog.heightInput = document.getElementById("height");
  gDialog.widthInput = document.getElementById("width");
  gDialog.leftAlign = document.getElementById("leftAlign");
  gDialog.centerAlign = document.getElementById("centerAlign");
  gDialog.rightAlign = document.getElementById("rightAlign");
  gDialog.alignGroup = gDialog.rightAlign.radioGroup;
  gDialog.shading = document.getElementById("3dShading");
  gDialog.pixelOrPercentMenulist = document.getElementById("pixelOrPercentMenulist");

  
  globalElement = gHLineElement.cloneNode(false);

  
  InitDialog()

  
  SetTextboxFocus(gDialog.widthInput);

  
  window.sizeToContent();

  SetWindowLocation();
}




function InitDialog()
{
  
  
  var height = GetHTMLOrCSSStyleValue(globalElement, "size", "height")
  if (/px/.test(height)) {
    height = RegExp.leftContext;
  }
  if(!height) {
    height = 2; 
  }

  
  gDialog.heightInput.value = height;

  
  
  gDialog.widthInput.value = InitPixelOrPercentMenulist(globalElement, gHLineElement, "width","pixelOrPercentMenulist");

  var marginLeft  = GetHTMLOrCSSStyleValue(globalElement, "align", "margin-left").toLowerCase();
  var marginRight = GetHTMLOrCSSStyleValue(globalElement, "align", "margin-right").toLowerCase();
  align = marginLeft + " " + marginRight;
  gDialog.leftAlign.checked   = (align == "left left"     || align == "0px auto");
  gDialog.centerAlign.checked = (align == "center center" || align == "auto auto" || align == " ");
  gDialog.rightAlign.checked  = (align == "right right"   || align == "auto 0px");

  if (gDialog.centerAlign.checked) {
    gDialog.alignGroup.selectedItem = gDialog.centerAlign;
  }
  else if (gDialog.rightAlign.checked) {
    gDialog.alignGroup.selectedItem = gDialog.rightAlign;
  }
  else {
    gDialog.alignGroup.selectedItem = gDialog.leftAlign;
  }

  gDialog.shading.checked = !globalElement.hasAttribute("noshade");
}

function onSaveDefault()
{
  
  
  if (ValidateData()) {
    var prefs = GetPrefs();
    if (prefs) {

      var alignInt;
      if (align == "left") {
        alignInt = 0;
      } else if (align == "right") {
        alignInt = 2;
      } else {
        alignInt = 1;
      }
      prefs.setIntPref("editor.hrule.align", alignInt);

      var percent;
      var widthInt;
      var heightInt;

      if (width)
      {
        if (/%/.test(width)) {
          percent = true;
          widthInt = Number(RegExp.leftContext);
        } else {
          percent = false;
          widthInt = Number(width);
        }
      }
      else
      {
        percent = true;
        widthInt = Number(100);
      }

      heightInt = height ? Number(height) : 2;

      prefs.setIntPref("editor.hrule.width", widthInt);
      prefs.setBoolPref("editor.hrule.width_percent", percent);
      prefs.setIntPref("editor.hrule.height", heightInt);
      prefs.setBoolPref("editor.hrule.shading", shading);

      
      var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                                  .getService(Components.interfaces.nsIPrefService);
      prefService.savePrefFile(null);
    }
	}
}



function ValidateData()
{
  
  height = ValidateNumber(gDialog.heightInput, null, 1, gMaxHRSize,
                          globalElement, "size", false);
  if (gValidationError)
    return false;

  width = ValidateNumber(gDialog.widthInput, gDialog.pixelOrPercentMenulist, 1, gMaxPixels, 
                         globalElement, "width", false);
  if (gValidationError)
    return false;

  align = "left";
  if (gDialog.centerAlign.selected) {
    
    align = "";
  } else if (gDialog.rightAlign.selected) {
    align = "right";
  }
  if (align)
    globalElement.setAttribute("align", align);
  else
    try {
      GetCurrentEditor().removeAttributeOrEquivalent(globalElement, "align", true);
    } catch (e) {}

  if (gDialog.shading.checked) {
    shading = true;
    globalElement.removeAttribute("noshade");
  } else {
    shading = false;
    globalElement.setAttribute("noshade", "noshade");
  }
  return true;
}

function onAccept()
{
  if (ValidateData())
  {
    
    try {
      GetCurrentEditor().cloneAttributes(gHLineElement, globalElement);
    } catch (e) {}
    return true;
  }
  return false;
}
