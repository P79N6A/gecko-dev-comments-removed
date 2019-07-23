







































const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsIX509Cert         = Components.interfaces.nsIX509Cert;

var dialogParams;
var pkiParams;
var cert=null;

function onLoad()
{
  pkiParams    = window.arguments[0].QueryInterface(nsIPKIParamBlock);
  dialogParams = pkiParams.QueryInterface(nsIDialogParamBlock);
 
  var isupports = pkiParams.getISupportAtIndex(1);
  cert = isupports.QueryInterface(nsIX509Cert); 
  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var dispName = cert.commonName;
  if (dispName == null)
    dispName = cert.windowTitle;

  var msg = bundle.formatStringFromName("escrowFinalMessage",
                                        [dispName], 1);
  setText("message1",msg);
}

function doOK()
{
  dialogParams.SetInt(1,1);
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
