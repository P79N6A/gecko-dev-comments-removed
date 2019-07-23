







































const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsIPKIParamBlock    = Components.interfaces.nsIPKIParamBlock;
const nsIX509Cert         = Components.interfaces.nsIX509Cert;

var dialogParams;
var pkiParams;
var bundle;

function onLoad()
{
  pkiParams    = window.arguments[0].QueryInterface(nsIPKIParamBlock);
  dialogParams = pkiParams.QueryInterface(nsIDialogParamBlock);
  var isupport = pkiParams.getISupportAtIndex(1);
  var cert = isupport.QueryInterface(nsIX509Cert);
  var connectURL = dialogParams.GetString(1); 
  var gBundleBrand = srGetStrBundle("chrome://branding/locale/brand.properties");
  var brandName = gBundleBrand.GetStringFromName("brandShortName");

  bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
 
  var message1 = bundle.formatStringFromName("crlNextUpdateMsg1", 
                                             [ brandName, connectURL ],
                                             2);
  var message2 = bundle.formatStringFromName("crlNextUpdateMsg2", 
                                             [ cert.issuerOrganization ],
                                              1);
  setText("message1", message1);
  setText("message2", message2);
}
