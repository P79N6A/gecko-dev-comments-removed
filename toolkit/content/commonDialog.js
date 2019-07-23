








































const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;


var gCommonDialogParam = 
  window.arguments[0].QueryInterface(Ci.nsIDialogParamBlock);
  
function showControls()
{
  
  
  
  
  
  var nTextBoxes = gCommonDialogParam.GetInt(3);
  if (nTextBoxes == 2) {
    if (gCommonDialogParam.GetInt(4) == 1) {
      initTextbox("password1", 4, 6, false);
      initTextbox("password2", 5, 7, false);
    }
    else {
      initTextbox("login", 4, 6, false);
      initTextbox("password1", 5, 7, false);
    }
  } else if (nTextBoxes == 1) {
    if (gCommonDialogParam.GetInt(4) == 1)
      initTextbox("password1", -1, 6, true);
    else
      initTextbox("login", 4, 6, true);
  }
}

function setLabelForNode(aNode, aLabel, aIsLabelFlag)
{
  
  
  
  
  
  

  
  
  var accessKey = null;
  if (/ *\(\&([^&])\)(:)?$/.test(aLabel)) {
    aLabel = RegExp.leftContext + RegExp.$2;
    accessKey = RegExp.$1;
  } else if (/^(.*[^&])?\&(([^&]).*$)/.test(aLabel)) {
    aLabel = RegExp.$1 + RegExp.$2;
    accessKey = RegExp.$3;
  }

  
  aLabel = aLabel.replace(/\&\&/g, "&");
  if (aIsLabelFlag) {    
    aNode.setAttribute("value", aLabel);
  } else {    
    aNode.label = aLabel;
  }

  
  
  if (accessKey)
    aNode.accessKey = accessKey;
}

var softkbObserver = {
 QueryInterface: function (aIID) {
    if (aIID.equals(Ci.nsISupports) ||
        aIID.equals(Ci.nsIObserver))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
 observe: function(subject, topic, data) {
    if (topic === "softkb-change") {
      var rect = JSON.parse(data);
      if (rect) {
        var height = rect.bottom - rect.top;
        var width = rect.right - rect.left;
        var top = (rect.top + (height - window.innerHeight) / 2);
        var left = (rect.left + (width - window.innerWidth) / 2);
        window.moveTo(left, top);
      }
    }
  }
};

function commonDialogOnLoad()
{
  
  document.getElementById("filler").maxWidth = screen.availWidth;

  
#ifdef XP_MACOSX
  setElementText("info.title", gCommonDialogParam.GetString(12), true);
#else
  document.title = gCommonDialogParam.GetString(12);
#endif

  var observerService = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  observerService.addObserver(softkbObserver, "softkb-change", false);

  
  var nButtons = gCommonDialogParam.GetInt(2);
  var dialog = document.documentElement;
  switch (nButtons) {
    case 1:
      dialog.getButton("cancel").hidden = true;
      break;
    case 4:
      dialog.getButton("extra2").hidden = false;
    case 3:
      dialog.getButton("extra1").hidden = false;
  }

  
  
  var croppedMessage = gCommonDialogParam.GetString(0).substr(0, 10000);
  setElementText("info.body", croppedMessage, true);

  setElementText("info.header", gCommonDialogParam.GetString(3), true);

  
  var iconElement = document.getElementById("info.icon");
  var iconClass = gCommonDialogParam.GetString(2);
  if (!iconClass)
    iconClass = "message-icon";
  iconElement.setAttribute("class", iconElement.getAttribute("class") + " " + iconClass);

  switch (nButtons) {
    case 4:
      setLabelForNode(document.documentElement.getButton("extra2"), gCommonDialogParam.GetString(11));
      
    case 3:
      setLabelForNode(document.documentElement.getButton("extra1"), gCommonDialogParam.GetString(10));
      
    default:
    case 2:
      var string = gCommonDialogParam.GetString(9);
      if (string)
        setLabelForNode(document.documentElement.getButton("cancel"), string);
      
    case 1:
      string = gCommonDialogParam.GetString(8);
      if (string)
        setLabelForNode(document.documentElement.getButton("accept"), string);
      break;
  }

  
  gCommonDialogParam.SetInt(0, 1); 

  
  setCheckbox(gCommonDialogParam.GetString(1), gCommonDialogParam.GetInt(1));

  if (gCommonDialogParam.GetInt(3) == 0) 
  {
    var dlgButtons = ['accept', 'cancel', 'extra1', 'extra2'];

    
    var dButton = dlgButtons[gCommonDialogParam.GetInt(5)];
    document.documentElement.defaultButton = dButton;
#ifndef XP_MACOSX
    document.documentElement.getButton(dButton).focus();
#endif
  }
  else {
    if (gCommonDialogParam.GetInt(4) == 1)
      document.getElementById("password1Textbox").select();
    else
      document.getElementById("loginTextbox").select();
  }

  if (gCommonDialogParam.GetInt(6) != 0) 
  {
    var delayInterval = 2000;
    try {
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      delayInterval = prefs.getIntPref("security.dialog_enable_delay");
    } catch (e) {}

    document.documentElement.getButton("accept").disabled = true;
    document.documentElement.getButton("extra1").disabled = true;
    document.documentElement.getButton("extra2").disabled = true;

    setTimeout(commonDialogReenableButtons, delayInterval);
    
    addEventListener("blur", commonDialogBlur, false);
    addEventListener("focus", commonDialogFocus, false);
  }

  getAttention();

  
  try {
    var sound = gCommonDialogParam.GetInt(7);
    if (sound) {
      Cc["@mozilla.org/sound;1"]
        .createInstance(Ci.nsISound)
        .playEventSound(sound);
    }
  } catch (e) { }
}

function commonDialogOnUnload(){
  var observerService = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  observerService.removeObserver(softkbObserver, "softkb-change");
}

var gDelayExpired = false;
var gBlurred = false;

function commonDialogBlur(aEvent)
{
  if (aEvent.target != document)
    return;
  gBlurred = true;
  document.documentElement.getButton("accept").disabled = true;
  document.documentElement.getButton("extra1").disabled = true;
  document.documentElement.getButton("extra2").disabled = true;
}

function commonDialogFocus(aEvent)
{
  if (aEvent.target != document)
    return;
  gBlurred = false;
  
  
  if (gDelayExpired) {
    var script = "document.documentElement.getButton('accept').disabled = false; ";
    script += "document.documentElement.getButton('extra1').disabled = false; ";
    script += "document.documentElement.getButton('extra2').disabled = false;";
    setTimeout(script, 250);
  }
}

function commonDialogReenableButtons()
{
  
  if (!gBlurred) {
    document.documentElement.getButton("accept").disabled = false;
    document.documentElement.getButton("extra1").disabled = false;
    document.documentElement.getButton("extra2").disabled = false;
  }
  gDelayExpired = true;
}

function initTextbox(aName, aLabelIndex, aValueIndex, aAlwaysLabel)
{
  unHideElementById(aName+"Container");

  var label = aLabelIndex < 0 ? "" : gCommonDialogParam.GetString(aLabelIndex);
  if (label || aAlwaysLabel && !label)
    setElementText(aName+"Label", label);
    
  var value = aValueIndex < 0 ? "" : gCommonDialogParam.GetString(aValueIndex);
  var textbox = document.getElementById(aName + "Textbox");
  textbox.setAttribute("value", value);
}

function setElementText(aElementID, aValue, aChildNodeFlag)
{
  var element = document.getElementById(aElementID);
  if (!aChildNodeFlag && element) {
    setLabelForNode(element, aValue, true);
  } else if (aChildNodeFlag && element) {
    element.appendChild(document.createTextNode(aValue));
  }
}

function setCheckbox(aChkMsg, aChkValue)
{
  if (aChkMsg) {
    unHideElementById("checkboxContainer");
    
    var checkboxElement = document.getElementById("checkbox");
    setLabelForNode(checkboxElement, aChkMsg);
    checkboxElement.checked = aChkValue > 0;
  }
}

function unHideElementById(aElementID)
{
  var element = document.getElementById(aElementID);
  element.hidden = false;
}

function hideElementById(aElementID)
{
  var element = document.getElementById(aElementID)
  element.hidden = true;
}

function isVisible(aElementId)
{
  return document.getElementById(aElementId).hasAttribute("hidden");
}

function onCheckboxClick(aCheckboxElement)
{
  gCommonDialogParam.SetInt(1, aCheckboxElement.checked);
}

function commonDialogOnAccept()
{
  gCommonDialogParam.SetInt(0, 0); 

  var numTextBoxes = gCommonDialogParam.GetInt(3);
  var textboxIsPassword1 = gCommonDialogParam.GetInt(4) == 1;
  
  if (numTextBoxes >= 1) {
    var editField1;
    if (textboxIsPassword1)
      editField1 = document.getElementById("password1Textbox");
    else
      editField1 = document.getElementById("loginTextbox");
    gCommonDialogParam.SetString(6, editField1.value);
  }

  if (numTextBoxes == 2) {
    var editField2;
    if (textboxIsPassword1)
      
      editField2 = document.getElementById("password2Textbox");
    else
      
      editField2 = document.getElementById("password1Textbox");
    gCommonDialogParam.SetString(7, editField2.value);
  }
}

function commonDialogOnExtra1()
{
  gCommonDialogParam.SetInt(0, 2);
  window.close();
}

function commonDialogOnExtra2()
{
  gCommonDialogParam.SetInt(0, 3);
  window.close();
}
