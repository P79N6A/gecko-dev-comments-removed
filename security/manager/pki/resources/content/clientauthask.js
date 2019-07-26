






const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;

var dialogParams;
var itemCount = 0;
var rememberBox;

function onLoad()
{
    var cn;
    var org;
    var issuer;

    dialogParams = window.arguments[0].QueryInterface(nsIDialogParamBlock);
    cn = dialogParams.GetString(0);
    org = dialogParams.GetString(1);
    issuer = dialogParams.GetString(2);

    
    var capsBundle = document.getElementById("caps_bundle");
    var rememberString = capsBundle.getString("CheckMessage");
    var rememberSetting = true;

    var pref = Components.classes['@mozilla.org/preferences-service;1']
	       .getService(Components.interfaces.nsIPrefService);
    if (pref) {
      pref = pref.getBranch(null);
      try {
	rememberSetting = 
	  pref.getBoolPref("security.remember_cert_checkbox_default_setting");
      }
      catch(e) {
	
      }
    }

    rememberBox = document.getElementById("rememberBox");
    rememberBox.label = rememberString;
    rememberBox.checked = rememberSetting;

    var bundle = document.getElementById("pippki_bundle");
    var message1 = bundle.getFormattedString("clientAuthMessage1", [org]);
    var message2 = bundle.getFormattedString("clientAuthMessage2", [issuer]);
    setText("hostname", cn);
    setText("organization", message1);
    setText("issuer", message2);

    var selectElement = document.getElementById("nicknames");
    itemCount = dialogParams.GetInt(0);
    for (var i=0; i < itemCount; i++) {
        var menuItemNode = document.createElement("menuitem");
        var nick = dialogParams.GetString(i+3);
        menuItemNode.setAttribute("value", i);
        menuItemNode.setAttribute("label", nick); 
        selectElement.firstChild.appendChild(menuItemNode);
        if (i == 0) {
            selectElement.selectedItem = menuItemNode;
        }
    }

    setDetails();
}

function setDetails()
{
  var index = parseInt(document.getElementById("nicknames").value);
  var details = dialogParams.GetString(index+itemCount+3);
  document.getElementById("details").value = details;
}

function onCertSelected()
{
  setDetails();
}

function doOK()
{
  dialogParams.SetInt(0,1);
  var index = parseInt(document.getElementById("nicknames").value);
  dialogParams.SetInt(1, index);
  dialogParams.SetInt(2, rememberBox.checked);
  return true;
}

function doCancel()
{
  dialogParams.SetInt(0,0);
  dialogParams.SetInt(1, -1); 
  dialogParams.SetInt(2, rememberBox.checked);
  return true;
}
