







































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

  document.title = dialogParams.GetString(2);
  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var message1 = dialogParams.GetString(1);
  
  const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
  var dateService = Components.classes["@mozilla.org/intl/scriptabledateformat;1"].getService(nsIScriptableDateFormat);
  var date = new Date(); 
  var dateStr = dateService.FormatDateTime("", dateService.dateFormatShort, dateService.timeFormatNoSeconds, 
                                           date.getFullYear(), date.getMonth()+1, date.getDate(), 
                                           date.getHours(), date.getMinutes(), date.getSeconds());

  var message2 = bundle.formatStringFromName("serverCertExpiredMsg2", 
                                             [ dateStr ],
                                              1);
  setText("message1", message1);
  setText("message2", message2);
  document.documentElement.getButton("accept").focus();
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
  if (cert == null) {
    var isupport = pkiParams.getISupportAtIndex(1);
    cert = isupport.QueryInterface(nsIX509Cert);
  }
  viewCertHelper(window, cert);
}
