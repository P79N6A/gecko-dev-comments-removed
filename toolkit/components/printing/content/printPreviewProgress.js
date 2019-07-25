






var dialog;


var printProgress = null; 


var targetFile;

var docTitle = "";
var docURL   = "";
var progressParams = null;

function ellipseString(aStr, doFront)
{
  if (aStr.length > 3 && (aStr.substr(0, 3) == "..." || aStr.substr(aStr.length-4, 3) == "..."))
    return aStr;

  var fixedLen = 64;
  if (aStr.length <= fixedLen)
    return aStr;

  if (doFront)
    return "..." + aStr.substr(aStr.length-fixedLen, fixedLen);
  
  return aStr.substr(0, fixedLen) + "...";
}


var progressListener = {

  onStateChange: function (aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
      window.close();
  },
  
  onProgressChange: function (aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {
    if (!progressParams)
      return;
    var docTitleStr = ellipseString(progressParams.docTitle, false);
    if (docTitleStr != docTitle) {
      docTitle = docTitleStr;
      dialog.title.value = docTitle;
    }
    var docURLStr = ellipseString(progressParams.docURL, true);
    if (docURLStr != docURL && dialog.title != null) {
      docURL = docURLStr;
      if (docTitle == "")
        dialog.title.value = docURLStr;
    }
  },

  onLocationChange: function (aWebProgress, aRequest, aLocation, aFlags) {},
  onSecurityChange: function (aWebProgress, aRequest, state) {},

  onStatusChange: function (aWebProgress, aRequest, aStatus, aMessage)
  {
    if (aMessage)
      dialog.title.setAttribute("value", aMessage);
  },

  QueryInterface: function (iid)
  {
    if (iid.equals(Components.interfaces.nsIWebProgressListener) || iid.equals(Components.interfaces.nsISupportsWeakReference))
      return this;   
    throw Components.results.NS_NOINTERFACE;
  }
}

function onLoad() {
  
  printProgress = window.arguments[0];
  if (window.arguments[1]) {
    progressParams = window.arguments[1].QueryInterface(Components.interfaces.nsIPrintProgressParams)
    if (progressParams) {
      docTitle = ellipseString(progressParams.docTitle, false);
      docURL   = ellipseString(progressParams.docURL, true);
    }
  }

  if (!printProgress) {
    dump( "Invalid argument to printPreviewProgress.xul\n" );
    window.close()
    return;
  }

  dialog         = new Object;
  dialog.strings = new Array;
  dialog.title   = document.getElementById("dialog.title");
  dialog.titleLabel = document.getElementById("dialog.titleLabel");

  dialog.title.value = docTitle;

  
  printProgress.registerListener(progressListener);
  moveToAlertPosition();

  
  window.setTimeout(doneIniting, 100);
}

function onUnload() 
{
  if (!printProgress)
    return;
  try {
    printProgress.unregisterListener(progressListener);
    printProgress = null;
  }
  catch(e) {}
}

function getString (stringId) {
  
  if (!(stringId in dialog.strings)) {
    
    var elem = document.getElementById( "dialog.strings."+stringId);
    try {
      if (elem && elem.childNodes && elem.childNodes[0] &&
          elem.childNodes[0].nodeValue)
        dialog.strings[stringId] = elem.childNodes[0].nodeValue;
      
      else
        dialog.strings[stringId] = "";
    } catch (e) { dialog.strings[stringId] = ""; }
  }
  return dialog.strings[stringId];
}


function onCancel () 
{
  
  try {
    printProgress.processCanceledByUser = true;
  }
  catch(e) {return true;}
    
  
  return false;
}

function doneIniting() 
{
  
  printProgress.doneIniting();
}
