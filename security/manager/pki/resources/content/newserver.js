








































const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsIX509Cert         = Components.interfaces.nsIX509Cert;

var dialogParams;
var pkiParams;
var cert;

function onLoad()
{
  pkiParams    = window.arguments[0].QueryInterface(nsIPKIParamBlock);
  dialogParams = pkiParams.QueryInterface(nsIDialogParamBlock);

  var isupport = pkiParams.getISupportAtIndex(1);
  cert = isupport.QueryInterface(nsIX509Cert);

  var bundle = document.getElementById("newserver_bundle");

  var intro = bundle.getFormattedString(
    "newserver.intro", [cert.commonName]);

  var reason3 = bundle.getFormattedString(
    "newserver.reason3", [cert.commonName]);

  var question = bundle.getFormattedString(
    "newserver.question", [cert.commonName]);

  setText("intro", intro);
  setText("reason3", reason3);
  setText("question", question);
}

function doOK()
{
  dialogParams.SetInt(1, 1);
  var selectedItem = document.getElementById("whatnow").selectedItem;
  dialogParams.SetInt(2, parseInt(selectedItem.value));
  return true;
}

function doCancel()
{
  dialogParams.SetInt(1,0);
  return true;
}

function viewCert()
{
  viewCertHelper(window, cert);
}
