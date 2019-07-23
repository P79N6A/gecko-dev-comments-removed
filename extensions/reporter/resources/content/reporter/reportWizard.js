












































const gParamLanguage = window.navigator.language;
const gRMOvers = "0.3"; 
const gParamURL = window.arguments[0];
const gParamUserAgent = navigator.userAgent;
const gParamOSCPU = navigator.oscpu;
const gParamPlatform = navigator.platform;
const gCharset = window.arguments[1];


var gParamDescription;
var gParamProblemType;
var gParamBehindLogin;
var gParamEmail;
var gParamBuildConfig;
var gParamGecko;

var gPrefBranch;
var gStatusIndicator;

function getReporterPrefBranch() {
  if (!gPrefBranch) {
    gPrefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefService)
                            .getBranch("extensions.reporter.");
  }
  return gPrefBranch;
}

function getBoolPref(prefname, aDefault) {
  try {
    var prefs = getReporterPrefBranch();
    return prefs.getBoolPref(prefname);
  }
  catch(ex) {
    return aDefault;
  }
}

function getCharPref(prefname, aDefault) {
  try {
    var prefs = getReporterPrefBranch();
    return prefs.getCharPref(prefname);
  }
  catch(ex) {
    return aDefault;
  }
}

function initPrivacyNotice() {
  var reportWizard = document.getElementById('reportWizard');
  
  if (getBoolPref("hidePrivacyStatement", false)) {
    reportWizard.advance();
  } else {
    
    reportWizard.canRewind = false;
    reportWizard.canAdvance = false;

    
    var privacyURL = getCharPref("privacyURL", "http://reporter.mozilla.org/privacy/");
    document.getElementById("privacyStatement").setAttribute("src", privacyURL+"?plain");
  }
}

function privacyPolicyCheckbox() {
  
  
  var canAdvance = document.getElementById('dontShowPrivacyStatement').checked;
  document.getElementById('reportWizard').canAdvance = canAdvance;
}

function setPrivacyPref(){
  if (document.getElementById('dontShowPrivacyStatement').checked){
    var prefs = getReporterPrefBranch();
    prefs.setBoolPref("hidePrivacyStatement", true);
  }
}

function initForm() {
  var strbundle=document.getElementById("strings");
  var reportWizard = document.getElementById('reportWizard');

  reportWizard.canRewind = false;
  document.getElementById('url').value = gParamURL;

  
  reportWizard.getButton('next').label = strbundle.getString("submitReport");
  reportWizard.getButton('next').setAttribute("accesskey",
                                              strbundle.getString("submitReport.accesskey"));


  
  var url = getCharPref("privacyURL", "http://reporter.mozilla.org/privacy/");
  var privacyLink = document.getElementById("privacyPolicy");
  privacyLink.setAttribute("href", url);

  
  reportWizard.canAdvance = false;

  document.getElementById("problem_type").focus();

}

function validateForm() {
  var canAdvance = document.getElementById('problem_type').value != "0";
  document.getElementById('reportWizard').canAdvance = canAdvance;
}

function registerSysID() {
  var param = {
    'method':             'submitRegister',
    'language':           gParamLanguage
  };

  
  sendReporterServerData(param, onRegisterSysIDLoad);
}

function onRegisterSysIDLoad(req) {
  if (req.status == 200) {
    var paramSysID = req.responseXML.getElementsByTagName('result').item(0);

    
    if (paramSysID) {
      var prefs = getReporterPrefBranch();
      prefs.setCharPref("sysid", paramSysID.textContent);

      
      sendReport();
      return;
    }
    
    
    var strbundle = document.getElementById("strings");
    showError(strbundle.getString("invalidResponse"));

    return;
  }

  
  var errorStr = extractError(req);
  showError(errorStr);
}


function sendReport() {
  
  var sysId = getCharPref("sysid", "");
  if (sysId == ""){
    registerSysID();
    return;
  }

  
  var reportWizard = document.getElementById('reportWizard');

  reportWizard.canRewind = false;
  reportWizard.canAdvance = false;
  
  reportWizard.getButton("cancel").disabled = true;

  var strbundle=document.getElementById("strings");
  var statusDescription = document.getElementById('sendReportProgressDescription');
  gStatusIndicator = document.getElementById('sendReportProgressIndicator');

  
  gParamDescription = document.getElementById('description').value;
  gParamProblemType = document.getElementById('problem_type').value;
  gParamBehindLogin = document.getElementById('behind_login').checked;
  gParamEmail = document.getElementById('email').value;

  gParamBuildConfig = getBuildConfig();
  gParamGecko = getGecko();

  
  var param = {
    'method':           'submitReport',
    'rmoVers':          gRMOvers,
    'url':              gParamURL,
    'problem_type':     gParamProblemType,
    'description':      gParamDescription,
    'behind_login':     gParamBehindLogin,
    'platform':         gParamPlatform,
    'oscpu':            gParamOSCPU,
    'gecko':            gParamGecko,
    'product':          getProduct(),
    'useragent':        gParamUserAgent,
    'buildconfig':      gParamBuildConfig,
    'language':         gParamLanguage,
    'email':            gParamEmail,
    'charset':          gCharset,
    'sysid':            sysId
  };

  gStatusIndicator.value = "5%";
  statusDescription.value = strbundle.getString("sendingReport");

  sendReporterServerData(param, onSendReportDataLoad);
}

function onSendReportDataProgress(e) {
  gStatusIndicator.value = (e.position / e.totalSize)*100;
}

function sendReporterServerData(params, callback) {
  var serviceURL = getCharPref("serviceURL", "http://reporter.mozilla.org/service/0.3/");

  params = serializeParams(params);

  var request = new XMLHttpRequest();
  request.onprogress = onSendReportDataProgress;
  request.open("POST", serviceURL, true);

  request.onreadystatechange = function () {
    if (request.readyState == 4)
      callback(request);
  };

  request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  request.setRequestHeader("Content-length", params.length);
  request.setRequestHeader("Connection", "close");
  request.send(params);
}

function serializeParams(params) {
  var str = '';
  for (var key in params) {
    str += key + '=' + encodeURIComponent(params[key]) + '&';
  }
  return str.slice(0, -1);
}

function onSendReportDataLoad(req) {
  if (req.status != 200) {
    var errorStr = extractError(req);
    showError(errorStr);
    return;
  }

  var reportWizard = document.getElementById('reportWizard');

  var finishSummary = document.getElementById('finishSummary');
  var finishExtendedFailed = document.getElementById('finishExtendedFailed');
  var finishExtendedSuccess = document.getElementById('finishExtendedSuccess');
  var statusDescription = document.getElementById('sendReportProgressDescription');

  var strbundle = document.getElementById("strings");

  
  finishExtendedFailed.hidden = true;

  statusDescription.value = strbundle.getString("reportSent");

  reportWizard.canAdvance = true;
  gStatusIndicator.value = "100%";

  
  reportWizard.advance();

  
  var reportId = req.responseXML.getElementsByTagName('reportId').item(0).firstChild.data;
  finishSummary.value = strbundle.getString("successfullyCreatedReport") + " " + reportId;

  finishExtendedDoc = finishExtendedSuccess.contentDocument;
  finishExtendedDoc.getElementById('urlStri').textContent         = gParamURL;
  finishExtendedDoc.getElementById('problemTypeStri').textContent = document.getElementById('problem_type').label;
  finishExtendedDoc.getElementById('descriptionStri').textContent = gParamDescription;
  finishExtendedDoc.getElementById('platformStri').textContent    = gParamPlatform;
  finishExtendedDoc.getElementById('oscpuStri').textContent       = gParamOSCPU;
  finishExtendedDoc.getElementById('productStri').textContent     = getProduct();
  finishExtendedDoc.getElementById('geckoStri').textContent       = gParamGecko;
  finishExtendedDoc.getElementById('buildConfigStri').textContent = gParamBuildConfig;
  finishExtendedDoc.getElementById('userAgentStri').textContent   = gParamUserAgent;
  finishExtendedDoc.getElementById('langStri').textContent        = gParamLanguage;
  finishExtendedDoc.getElementById('emailStri').textContent       = gParamEmail;
  finishExtendedDoc.getElementById('charsetStri').textContent       = gCharset;

  reportWizard.canRewind = false;

  document.getElementById('finishExtendedFrame').collapsed = true;
  reportWizard.getButton("cancel").disabled = true;
  return;
}

function extractError(req){
  var error = req.responseXML.getElementsByTagName('errorString').item(0)
  if (error) {
    return error.textContent;
  }

  
  var strbundle = document.getElementById("strings");
  return strbundle.getString("defaultError");
}

function showError(errorStr){
  var strbundle = document.getElementById("strings");
  var finishSummary = document.getElementById('finishSummary');
  var finishExtendedSuccess = document.getElementById('finishExtendedSuccess');
  var finishExtendedFailed = document.getElementById('finishExtendedFailed');

  
  finishExtendedSuccess.hidden = true;

  
  var finishPage = document.getElementById('finish');
  finishPage.setAttribute("label",strbundle.getString("finishError"));

  var reportWizard = document.getElementById('reportWizard');
  reportWizard.canAdvance = true;
  reportWizard.advance();

  finishSummary.value = strbundle.getString("failedCreatingReport");

  finishExtendedDoc = finishExtendedFailed.contentDocument;
  finishExtendedDoc.getElementById('faultMessage').textContent = errorStr;

  document.getElementById('finishExtendedFrame').collapsed = true;
  reportWizard.getButton("cancel").disabled = true;
}

function showDetail() {
  var hideDetail = document.getElementById('showDetail').checked ? false : true;
  document.getElementById('finishExtendedFrame').collapsed = hideDetail;
}

function getBuildConfig() {
  
  try {
    var ioservice =
      Components.classes["@mozilla.org/network/io-service;1"]
                .getService(Components.interfaces.nsIIOService);
    var channel = ioservice.newChannel("chrome://global/content/buildconfig.html", null, null);
    var stream = channel.open();
    var scriptableInputStream =
      Components.classes["@mozilla.org/scriptableinputstream;1"]
                .createInstance(Components.interfaces.nsIScriptableInputStream);
    scriptableInputStream.init(stream);
    var data = "";
    var curBit = scriptableInputStream.read(4096);
    while (curBit.length) {
      data += curBit;
      curBit = scriptableInputStream.read(4096);
    }
    
    data = data.replace(/^<!DOCTYPE[^>]*>/, "");
    
    data = data.replace(/^<html>/, "<html xmlns='http://www.w3.org/1999/xhtml'>");
    var parser = new DOMParser();
    var buildconfig = parser.parseFromString(data, "application/xhtml+xml");
    var text = buildconfig.getElementsByTagName("body")[0].textContent;
    var start= text.indexOf('Configure arguments')+19;
    return text.substring(start);
  } catch(ex) {
    dump(ex);
    return "Unknown";
  }
}

function getProduct() {
  try {
    
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                            .getService(Components.interfaces.nsIXULAppInfo);
    
    return appInfo.name+"/"+appInfo.version;
  }
  catch(ex) {}
  
  if ('nsIChromeRegistrySea' in Components.interfaces) {
    return 'SeaMonkey/'+
    Components.classes['@mozilla.org/network/io-service;1']
              .getService(Components.interfaces.nsIIOService)
              .getProtocolHandler('http')
              .QueryInterface(Components.interfaces.nsIHttpProtocolHandler).misc.substring(3);
  }
  
  else if (navigator.vendor != ''){
    return window.navigator.vendor+'/'+window.navigator.vendorSub;
  }
  else {
    return "Unknown";
  }
}

function getGecko() {
  try {
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                            .getService(Components.interfaces.nsIXULAppInfo);
    
    return appInfo.platformBuildID;
  }
  catch(ex) {
    return "00000000"; 
  }
}
