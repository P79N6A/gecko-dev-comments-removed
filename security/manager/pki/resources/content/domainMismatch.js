







































const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIX509Cert         = Components.interfaces.nsIX509Cert;

var pkiParams;
var dialogParams;
var cert;

function onLoad()
{
  pkiParams = window.arguments[0].QueryInterface(nsIPKIParamBlock);  
  var isupport = pkiParams.getISupportAtIndex(1);
  cert     = isupport.QueryInterface(nsIX509Cert);
  dialogParams = pkiParams.QueryInterface(nsIDialogParamBlock);
  var connectURL = dialogParams.GetString(1); 

  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");

  var message1 = bundle.formatStringFromName("mismatchDomainMsg1", 
                                             [ connectURL, cert.commonName ],
                                             2);
  var message2 = bundle.formatStringFromName("mismatchDomainMsg2", 
                                             [ connectURL ],
                                              1);
  setText("message1", message1);
  setText("message2", message2);
}

function viewCert()
{
  viewCertHelper(window, cert);
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
