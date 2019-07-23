




































const nsIX509CertDB = Components.interfaces.nsIX509CertDB;
const nsX509CertDB = "@mozilla.org/security/x509certdb;1";
const nsIOCSPResponder = Components.interfaces.nsIOCSPResponder;
const nsISupportsArray = Components.interfaces.nsISupportsArray;

var certdb;
var ocspResponders;
var cacheRadio = 0;

function onLoad()
{
  var ocspEntry;
  var i;

  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  ocspResponders = certdb.getOCSPResponders();

  var signersMenu = document.getElementById("signingCA");
  var signersURL = document.getElementById("serviceURL");
  for (i=0; i<ocspResponders.length; i++) {
    ocspEntry = ocspResponders.queryElementAt(i, nsIOCSPResponder);
    var menuItemNode = document.createElement("menuitem");
    menuItemNode.setAttribute("value", ocspEntry.responseSigner);
    menuItemNode.setAttribute("label", ocspEntry.responseSigner);
    signersMenu.firstChild.appendChild(menuItemNode);
  }

  parent.initPanel('chrome://pippki/content/pref-validation.xul');

  doEnabling(0);
}

function doEnabling(called_by)
{
  var signingCA = document.getElementById("signingCA");
  var serviceURL = document.getElementById("serviceURL");
  var securityOCSPEnabled = document.getElementById("securityOCSPEnabled");
  var requireWorkingOCSP = document.getElementById("requireWorkingOCSP");
  var enableOCSPBox = document.getElementById("enableOCSPBox");
  var certOCSP = document.getElementById("certOCSP");
  var proxyOCSP = document.getElementById("proxyOCSP");

  var OCSPPrefValue = parseInt(securityOCSPEnabled.value);

  if (called_by == 0) {
    
    enableOCSPBox.checked = (OCSPPrefValue != 0);
  }
  else {
    
    var new_val = 0;
    if (enableOCSPBox.checked) {
      
      
      new_val = (cacheRadio > 0) ? cacheRadio : 1;
    }
    else {
      
      cacheRadio = OCSPPrefValue;
    }
    securityOCSPEnabled.value = OCSPPrefValue = new_val;
  }

  certOCSP.disabled = (OCSPPrefValue == 0);
  proxyOCSP.disabled = (OCSPPrefValue == 0);
  signingCA.disabled = serviceURL.disabled = OCSPPrefValue == 0 || OCSPPrefValue == 1;
  requireWorkingOCSP.disabled = (OCSPPrefValue == 0);
}

function changeURL()
{
  var signersMenu = document.getElementById("signingCA");
  var signersURL = document.getElementById("serviceURL");
  var CA = signersMenu.getAttribute("value");
  var i;
  var ocspEntry;

  for (i=0; i < ocspResponders.length; i++) {
    ocspEntry = ocspResponders.queryElementAt(i, nsIOCSPResponder);
    if (CA == ocspEntry.responseSigner) {
      signersURL.setAttribute("value", ocspEntry.serviceURL);
      break;
    }
  }
}

function openCrlManager()
{
    window.open('chrome://pippki/content/crlManager.xul',  "",
                'chrome,centerscreen,resizable');
}

