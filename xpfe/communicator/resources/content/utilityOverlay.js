



















































const kIOServiceProgID = "@mozilla.org/network/io-service;1";
const kObserverServiceProgID = "@mozilla.org/observer-service;1";
const kProxyManual = ["network.proxy.ftp",
                      "network.proxy.gopher",
                      "network.proxy.http",
                      "network.proxy.socks",
                      "network.proxy.ssl"];
var gShowBiDi = false;

function toggleOfflineStatus()
{
  var checkfunc;
  try {
    checkfunc = document.getElementById("offline-status").getAttribute('checkfunc');
  }
  catch (ex) {
    checkfunc = null;
  }

  var ioService = Components.classes[kIOServiceProgID]
                            .getService(Components.interfaces.nsIIOService2);
  if (checkfunc) {
    if (!eval(checkfunc)) {
      
      return;
    }
  }
  ioService.manageOfflineStatus = false;
  ioService.offline = !ioService.offline;
}

function setNetworkStatus(networkProxyType)
{
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);
  try {
    prefBranch.setIntPref("network.proxy.type", networkProxyType);
  }
  catch (ex) {}
}

function InitProxyMenu()
{
  var networkProxyNo = document.getElementById("network-proxy-no");
  var networkProxyManual = document.getElementById("network-proxy-manual");
  var networkProxyPac = document.getElementById("network-proxy-pac");
  var networkProxyWpad = document.getElementById("network-proxy-wpad");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);

  var proxyLocked = prefBranch.prefIsLocked("network.proxy.type");
  if (proxyLocked) {
    networkProxyNo.setAttribute("disabled", "true");
    networkProxyWpad.setAttribute("disabled", "true");
  }
  else {
    networkProxyNo.removeAttribute("disabled");
    networkProxyWpad.removeAttribute("disabled");
  }

  
  
  var proxyManuallyConfigured = false;
  for (var i = 0; i < kProxyManual.length; i++) {
    if (GetStringPref(kProxyManual[i]) != "") {
      proxyManuallyConfigured = true;
      break;
    }
  }

  if (proxyManuallyConfigured && !proxyLocked) {
    networkProxyManual.removeAttribute("disabled");
  }
  else {
    networkProxyManual.setAttribute("disabled", "true");
  }

  
  var proxyAutoConfigured = false;
  if (GetStringPref("network.proxy.autoconfig_url") != "")
    proxyAutoConfigured = true;

  if (proxyAutoConfigured && !proxyLocked) {
    networkProxyPac.removeAttribute("disabled");
  }
  else {
    networkProxyPac.setAttribute("disabled", "true");
  }

  var networkProxyType;
  try {
    networkProxyType = prefBranch.getIntPref("network.proxy.type");
  } catch(e) {}

  
  
  var networkProxyStatus = [networkProxyNo, networkProxyManual, networkProxyPac,
                            networkProxyNo, networkProxyWpad];
  networkProxyStatus[networkProxyType].setAttribute("checked", "true");
}

function setProxyTypeUI()
{
  var panel = document.getElementById("offline-status");
  if (!panel)
    return;

  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);

  try {
    var networkProxyType = prefBranch.getIntPref("network.proxy.type");
  } catch(e) {}

  var onlineTooltip = "onlineTooltip" + networkProxyType;
  var bundle = srGetStrBundle("chrome://communicator/locale/utilityOverlay.properties");
  panel.setAttribute("tooltiptext", bundle.GetStringFromName(onlineTooltip));
}

function GetStringPref(name)
{
  try {
    return pref.getComplexValue(name, Components.interfaces.nsISupportsString).data;
  } catch (e) {}
  return "";
}

function setOfflineUI(offline)
{
  var broadcaster = document.getElementById("Communicator:WorkMode");
  var panel = document.getElementById("offline-status");
  if (!broadcaster || !panel) return;

  
  
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);
  
  var offlineLocked = prefBranch.prefIsLocked("network.online"); 
  
  if (offlineLocked ) {
      broadcaster.setAttribute("disabled","true");
  }

  var bundle = srGetStrBundle("chrome://communicator/locale/utilityOverlay.properties");

  if (offline)
    {
      broadcaster.setAttribute("offline", "true");
      broadcaster.setAttribute("checked", "true");
      panel.removeAttribute("context");
      panel.setAttribute("tooltiptext", bundle.GetStringFromName("offlineTooltip"));
    }
  else
    {
      broadcaster.removeAttribute("offline");
      broadcaster.removeAttribute("checked");
      panel.setAttribute("context", "networkProperties");
      try {
        var networkProxyType = prefBranch.getIntPref("network.proxy.type");
      } catch(e) {}
      var onlineTooltip = "onlineTooltip" + networkProxyType;
      panel.setAttribute("tooltiptext", bundle.GetStringFromName(onlineTooltip));
    }
}

var goPrefWindow = 0;

function getBrowserURL() {

  try {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    var url = prefs.getCharPref("browser.chromeURL");
    if (url)
      return url;
  } catch(e) {
  }
  return "chrome://navigator/content/navigator.xul";
}

function goPreferences(containerID, paneURL, itemID)
{
  var resizable;
  var pref = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);
  try {
    
    
    
    resizable = pref.getBoolPref("xul.debug.box");
  }
  catch (e) {
    resizable = false;
  }

  
  const kWindowMediatorContractID = "@mozilla.org/appshell/window-mediator;1";
  const kWindowMediatorIID = Components.interfaces.nsIWindowMediator;
  const kWindowMediator = Components.classes[kWindowMediatorContractID].getService(kWindowMediatorIID);
  var lastPrefWindow = kWindowMediator.getMostRecentWindow("mozilla:preferences");
  if (lastPrefWindow)
    lastPrefWindow.focus();
  else {
    var resizability = resizable ? "yes" : "no";
    var features = "chrome,titlebar,resizable=" + resizability;
    openDialog("chrome://communicator/content/pref/pref.xul","PrefWindow", 
               features, paneURL, containerID, itemID);
  }
}

function goToggleToolbar( id, elementID )
{
  var toolbar = document.getElementById( id );
  var element = document.getElementById( elementID );
  if ( toolbar )
  {
    var attribValue = toolbar.getAttribute("hidden") ;

    if ( attribValue == "true" )
    {
      toolbar.setAttribute("hidden", "false" );
      if ( element )
        element.setAttribute("checked","true")
    }
    else
    {
      toolbar.setAttribute("hidden", true );
      if ( element )
        element.setAttribute("checked","false")
    }
    document.persist(id, 'hidden');
    document.persist(elementID, 'checked');
  }
}


function goClickThrobber( urlPref )
{
  var url;
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    url = pref.getComplexValue(urlPref, Components.interfaces.nsIPrefLocalizedString).data;
  }

  catch(e) {
    url = null;
  }

  if ( url )
    openTopWin(url);
}



function goHelpMenu( url )
{
  
  
  window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
}

function getTopWin()
{
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var topWindowOfType = windowManagerInterface.getMostRecentWindow( "navigator:browser" );

    if (topWindowOfType) {
        return topWindowOfType;
    }
    return null;
}

function openTopWin( url )
{
    



    

    if ((url == null) || (url == "")) return null;

    
    if (url.indexOf("urn:") == 0)
    {
        url = xlateURL(url);        
    }

    
    if (url == "") {
        url = "about:blank";
    }

    var topWindowOfType = getTopWin();
    if ( topWindowOfType )
    {
        topWindowOfType.focus();
        topWindowOfType.loadURI(url);
        return topWindowOfType;
    }
    return window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", url );
}

function goAboutDialog()
{
  var defaultAboutState = false;
  try {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    defaultAboutState = pref.getBoolPref("browser.show_about_as_stupid_modal_window");
  }
  catch(e) {
    defaultAboutState = false;
  }
  if( defaultAboutState )
    window.openDialog("chrome://global/content/about.xul", "About", "modal,chrome,resizable=yes,height=450,width=550");
  else
    window.openDialog( getBrowserURL(), "_blank", "chrome,all,dialog=no", 'about:' );
}


function goUpdateGlobalEditMenuItems()
{
  goUpdateCommand('cmd_undo');
  goUpdateCommand('cmd_redo');
  goUpdateCommand('cmd_cut');
  goUpdateCommand('cmd_copy');
  goUpdateCommand('cmd_paste');
  goUpdateCommand('cmd_selectAll');
  goUpdateCommand('cmd_delete');
  if (gShowBiDi)
    goUpdateCommand('cmd_switchTextDirection');
}


function goUpdateSelectEditMenuItems()
{
  goUpdateCommand('cmd_cut');
  goUpdateCommand('cmd_copy');
  goUpdateCommand('cmd_delete');
  goUpdateCommand('cmd_selectAll');
}


function goUpdateUndoEditMenuItems()
{
  goUpdateCommand('cmd_undo');
  goUpdateCommand('cmd_redo');
}


function goUpdatePasteMenuItems()
{
  goUpdateCommand('cmd_paste');
}


function goUpdateFindTypeMenuItems()
{
  goUpdateCommand('cmd_findTypeText');
  goUpdateCommand('cmd_findTypeLinks');
}


function extractFileNameFromUrl(urlstr) {
  if (!urlstr) return null;

  
  
  
  var lastSlash = urlstr.slice(urlstr.lastIndexOf( "/" )+1);
  if (lastSlash)
  { 
    var nameIndex = lastSlash.lastIndexOf( "filename=" );
    if (nameIndex != -1)
      return (lastSlash.slice(nameIndex+9));
    else
      return lastSlash;
  }
  return null; 
}


function gatherTextUnder ( root ) 
{
  var text = "";
  var node = root.firstChild;
  var depth = 1;
  while ( node && depth > 0 ) {
    
    if ( node.nodeType == Node.TEXT_NODE ) {
      
      text += " " + node.data;
    } else if ( node instanceof HTMLImageElement ) {
      
      var altText = node.getAttribute( "alt" );
      if ( altText && altText != "" ) {
        text += " " + altText;
      }
    }
    
    
    if ( node.hasChildNodes() ) {
      
      node = node.firstChild;
      depth++;
    } else {
      
      if ( node.nextSibling ) {
        node = node.nextSibling;
      } else {
        
        while ( node && depth > 0 ) {
          node = node.parentNode;
          depth--;
          if ( node.nextSibling ) {
            node = node.nextSibling;
            break;
          }
        }
      }
    }
  }
  
  text = text.replace( /^\s+/, "" );
  
  text = text.replace( /\s+$/, "" );
  
  text = text.replace( /\s+/g, " " );
  return text;
}

var offlineObserver = {
  observe: function(subject, topic, state) {
    
    if (topic != "network:offline-status-changed") return;
    setOfflineUI(state == "offline");
  }
}

var proxyTypeObserver = {
  observe: function(subject, topic, state) {
    
    var ioService = Components.classes[kIOServiceProgID]
                              .getService(Components.interfaces.nsIIOService);
    if (state == "network.proxy.type" && !ioService.offline)
      setProxyTypeUI();
  }
}

function utilityOnLoad(aEvent)
{
  var broadcaster = document.getElementById("Communicator:WorkMode");
  if (!broadcaster) return;

  var observerService = Components.classes[kObserverServiceProgID]
		          .getService(Components.interfaces.nsIObserverService);
  observerService.addObserver(offlineObserver, "network:offline-status-changed", false);
  
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);
  prefBranch = prefBranch.QueryInterface(Components.interfaces.nsIPrefBranch2);

  prefBranch.addObserver("network.proxy.type", proxyTypeObserver, false);

  addEventListener("unload", utilityOnUnload, false);

  
  var ioService = Components.classes[kIOServiceProgID]
		      .getService(Components.interfaces.nsIIOService);
  setOfflineUI(ioService.offline);
}

function utilityOnUnload(aEvent) 
{
  var observerService = Components.classes[kObserverServiceProgID]
			  .getService(Components.interfaces.nsIObserverService);
  observerService.removeObserver(offlineObserver, "network:offline-status-changed");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"];
  prefService = prefService.getService(Components.interfaces.nsIPrefService);
  var prefBranch = prefService.getBranch(null);
  prefBranch = prefBranch.QueryInterface(Components.interfaces.nsIPrefBranch2);
  
  prefBranch.removeObserver("network.proxy.type", proxyTypeObserver);
}

addEventListener("load", utilityOnLoad, false);

function GenerateValidFilename(filename, extension)
{
  if (filename) 
  {
    
    
    filename = validateFileName(filename).replace(/^\s+|\s+$/g, "");
    if (filename.length > 0)
      return filename + extension;
  }
  return null;
}

function validateFileName(aFileName)
{
  var re = /[\/]+/g;
  if (navigator.appVersion.indexOf("Windows") != -1) {
    re = /[\\\/\|]+/g;
    aFileName = aFileName.replace(/[\"]+/g, "'");
    aFileName = aFileName.replace(/[\*\:\?]+/g, " ");
    aFileName = aFileName.replace(/[\<]+/g, "(");
    aFileName = aFileName.replace(/[\>]+/g, ")");
  }
  else if (navigator.appVersion.indexOf("Macintosh") != -1)
    re = /[\:\/]+/g;
  
  return aFileName.replace(re, "_");
}


var gStartX;
var gStartY;
var gCurrX;
var gCurrY;
var gScrollingView;
var gAutoScrollTimer;
var gIgnoreMouseEvents;
var gAutoScrollPopup = null;
var gAccumulatedScrollErrorX;
var gAccumulatedScrollErrorY;

function startScrolling(event)
{
  var pref = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefBranch);

  if (gScrollingView || event.button != 1)
    return;

  if (!pref.getBoolPref("general.autoScroll"))
    return;

  if (event.originalTarget instanceof XULElement &&
      ((event.originalTarget.localName == "thumb")
        || (event.originalTarget.localName == "slider")
        || (event.originalTarget.localName == "scrollbarbutton")))
    return;

  if (!gAutoScrollPopup) {
    const XUL_NS
          = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    gAutoScrollPopup = document.createElementNS(XUL_NS, "popup");
    gAutoScrollPopup.id = "autoscroller";
    gAutoScrollPopup.addEventListener("popuphidden", stopScrolling, true);
    document.documentElement.appendChild(gAutoScrollPopup);
  }

  gScrollingView = event.originalTarget.ownerDocument.defaultView;
  var scrollingDir;
  if (gScrollingView.scrollMaxX > 0) {
    gAutoScrollPopup.setAttribute("scrolldir", gScrollingView.scrollMaxY > 0 ? "NSEW" : "EW");
  }
  else if (gScrollingView.scrollMaxY > 0) {
    gAutoScrollPopup.setAttribute("scrolldir", "NS");
  }
  else {
    gScrollingView = null; 
    return;
  }

  document.popupNode = null;
  gAutoScrollPopup.showPopup(document.documentElement, event.screenX, event.screenY,
                             "popup", null, null);
  gIgnoreMouseEvents = true;
  gStartX = event.screenX;
  gStartY = event.screenY;
  gCurrX = event.screenX;
  gCurrY = event.screenY;
  addEventListener('mousemove', handleMouseMove, true);
  addEventListener('mouseup', handleMouseUpDown, true);
  addEventListener('mousedown', handleMouseUpDown, true);
  addEventListener('contextmenu', handleMouseUpDown, true);
  gAutoScrollTimer = setInterval(autoScrollLoop, 10);
  gAccumulatedScrollErrorX = 0;
  gAccumulatedScrollErrorY = 0;
}

function handleMouseMove(event)
{
  gCurrX = event.screenX;
  gCurrY = event.screenY;
}

function roundToZero(num)
{
  if (num > 0)
    return Math.floor(num);
  return Math.ceil(num);
}

function accelerate(curr, start)
{
    var speed = 12;
    var val = (curr - start) / speed;

    if (val > 1)
        return val * Math.sqrt(val) - 1;
    if (val < -1)
        return val * Math.sqrt(-val) + 1;
    return 0;
}

function autoScrollLoop()
{
  var x = accelerate(gCurrX, gStartX);
  var y = accelerate(gCurrY, gStartY);

  if (x || y)
    gIgnoreMouseEvents = false;

  var desiredScrollX = gAccumulatedScrollErrorX + x;
  var actualScrollX = roundToZero(desiredScrollX);
  gAccumulatedScrollErrorX = (desiredScrollX - actualScrollX);
  var desiredScrollY = gAccumulatedScrollErrorY + y;
  var actualScrollY = roundToZero(desiredScrollY);
  gAccumulatedScrollErrorY = (desiredScrollY - actualScrollY);
  gScrollingView.scrollBy(actualScrollX, actualScrollY);
}

function handleMouseUpDown(event)
{
  if (!gIgnoreMouseEvents)
    gAutoScrollPopup.hidePopup();
  gIgnoreMouseEvents = false;
}

function stopScrolling()
{
  if (gScrollingView) {
    gScrollingView = null;
    removeEventListener('mousemove', handleMouseMove, true);
    removeEventListener('mousedown', handleMouseUpDown, true);
    removeEventListener('mouseup', handleMouseUpDown, true);
    removeEventListener('contextmenu', handleMouseUpDown, true);
    clearInterval(gAutoScrollTimer);
  }
}
