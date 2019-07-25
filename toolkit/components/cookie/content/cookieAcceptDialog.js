





const nsICookieAcceptDialog = Components.interfaces.nsICookieAcceptDialog;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;
const nsICookie = Components.interfaces.nsICookie;
const nsICookiePromptService = Components.interfaces.nsICookiePromptService;

var params; 
var cookieBundle;
var gDateService = null;

var showDetails = "";
var hideDetails = "";
var detailsAccessKey = "";

function onload()
{
  doSetOKCancel(cookieAcceptNormal, cookieDeny, cookieAcceptSession);

  var dialog = document.documentElement;

  document.getElementById("Button2").collapsed = false;
  
  document.getElementById("ok").label = dialog.getAttribute("acceptLabel");
  document.getElementById("ok").accessKey = dialog.getAttribute("acceptKey");
  document.getElementById("Button2").label = dialog.getAttribute("extra1Label");
  document.getElementById("Button2").accessKey = dialog.getAttribute("extra1Key");
  document.getElementById("cancel").label = dialog.getAttribute("cancelLabel");
  document.getElementById("cancel").accessKey = dialog.getAttribute("cancelKey");

  
  document.getElementById("ok").setAttribute("icon","accept");
  document.getElementById("cancel").setAttribute("icon","cancel");
  document.getElementById("disclosureButton").setAttribute("icon","properties");

  if (!gDateService) {
    const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
    const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
    gDateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
                             .getService(nsIScriptableDateFormat);
  }

  cookieBundle = document.getElementById("cookieBundle");

  
  if (!showDetails) {
    showDetails = cookieBundle.getString('showDetails');
  }
  if (!hideDetails) {
    hideDetails = cookieBundle.getString('hideDetails');
  }
  detailsAccessKey = cookieBundle.getString('detailsAccessKey');

  if (document.getElementById('infobox').hidden) {
    document.getElementById('disclosureButton').setAttribute("label",showDetails);
  } else {
    document.getElementById('disclosureButton').setAttribute("label",hideDetails);
  }
  document.getElementById('disclosureButton').setAttribute("accesskey",detailsAccessKey);

  if ("arguments" in window && window.arguments.length >= 1 && window.arguments[0]) {
    try {
      params = window.arguments[0].QueryInterface(nsIDialogParamBlock);
      var objects = params.objects;
      var cookie = params.objects.queryElementAt(0,nsICookie);
      
      var cookiesFromHost = params.GetInt(nsICookieAcceptDialog.COOKIESFROMHOST);

      var messageFormat;
      if (params.GetInt(nsICookieAcceptDialog.CHANGINGCOOKIE))
        messageFormat = 'permissionToModifyCookie';
      else if (cookiesFromHost > 1)
        messageFormat = 'permissionToSetAnotherCookie';
      else if (cookiesFromHost == 1)
        messageFormat = 'permissionToSetSecondCookie';
      else
        messageFormat = 'permissionToSetACookie';

      var hostname = params.GetString(nsICookieAcceptDialog.HOSTNAME);

      var messageText;
      if (cookie)
        messageText = cookieBundle.getFormattedString(messageFormat,[hostname, cookiesFromHost]);
      else
        
        
        messageText = cookieBundle.getFormattedString(messageFormat,["",cookiesFromHost]);

      var messageParent = document.getElementById("dialogtextbox");
      var messageParagraphs = messageText.split("\n");

      
      var headerNode = document.getElementById("dialog-header");
      headerNode.setAttribute("value",messageParagraphs[0]);

      
      for (var i = 1; i < messageParagraphs.length; i++) {
        var descriptionNode = document.createElement("description");
        text = document.createTextNode(messageParagraphs[i]);
        descriptionNode.appendChild(text);
        messageParent.appendChild(descriptionNode);
      }

      if (cookie) {
        document.getElementById('ifl_name').setAttribute("value",cookie.name);
        document.getElementById('ifl_value').setAttribute("value",cookie.value);
        document.getElementById('ifl_host').setAttribute("value",cookie.host);
        document.getElementById('ifl_path').setAttribute("value",cookie.path);
        document.getElementById('ifl_isSecure').setAttribute("value",
                                                                 cookie.isSecure ?
                                                                    cookieBundle.getString("forSecureOnly") : cookieBundle.getString("forAnyConnection")
                                                          );
        document.getElementById('ifl_expires').setAttribute("value",GetExpiresString(cookie.expires));
        document.getElementById('ifl_isDomain').setAttribute("value",
                                                                 cookie.isDomain ?
                                                                    cookieBundle.getString("domainColon") : cookieBundle.getString("hostColon")
                                                            );
      }
      
      params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, 0);
      
      params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, 0);
    } catch (e) {
    }
  }

  
  try {
    var pb = Components.classes["@mozilla.org/privatebrowsing;1"].
             getService(Components.interfaces.nsIPrivateBrowsingService);
    if (pb.privateBrowsingEnabled) {
      var persistCheckbox = document.getElementById("persistDomainAcceptance");
      persistCheckbox.removeAttribute("checked");
      persistCheckbox.setAttribute("disabled", "true");
    }
  } catch (ex) {}
}

function showhideinfo()
{
  var infobox=document.getElementById('infobox');

  if (infobox.hidden) {
    infobox.setAttribute("hidden","false");
    document.getElementById('disclosureButton').setAttribute("label",hideDetails);
  } else {
    infobox.setAttribute("hidden","true");
    document.getElementById('disclosureButton').setAttribute("label",showDetails);
  }
  sizeToContent();
}

function cookieAcceptNormal()
{
  
  params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, nsICookiePromptService.ACCEPT_COOKIE); 
  
  params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, document.getElementById('persistDomainAcceptance').checked);
  window.close();
}

function cookieAcceptSession()
{
  
  params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, nsICookiePromptService.ACCEPT_SESSION_COOKIE);
  
  params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, document.getElementById('persistDomainAcceptance').checked);
  window.close();
}

function cookieDeny()
{
  
  params.SetInt(nsICookieAcceptDialog.ACCEPT_COOKIE, nsICookiePromptService.DENY_COOKIE); 
  
  params.SetInt(nsICookieAcceptDialog.REMEMBER_DECISION, document.getElementById('persistDomainAcceptance').checked);
  window.close();
}

function GetExpiresString(secondsUntilExpires) {
  if (secondsUntilExpires) {
    var date = new Date(1000*secondsUntilExpires);

    
    
    
    var expiry = "";
    try {
      expiry = gDateService.FormatDateTime("", gDateService.dateFormatLong,
                                           gDateService.timeFormatSeconds, 
                                           date.getFullYear(), date.getMonth()+1, 
                                           date.getDate(), date.getHours(),
                                           date.getMinutes(), date.getSeconds());
    } catch(ex) {
      
    }
    return expiry;
  }
  return cookieBundle.getString("expireAtEndOfSession");
}
