













































var MODULE_NAME = 'UtilsAPI';

const gTimeout = 5000;





var appInfo = {
  _service: null,

  




  get appInfo() {
    if (!this._appInfo) {
      this._service = Cc["@mozilla.org/xre/app-info;1"]
                        .getService(Ci.nsIXULAppInfo)
                        .QueryInterface(Ci.nsIXULRuntime);
    }
    return this._service;
  },

  




  get buildID() this.appInfo.appBuildID,

  




  get ID() this.appInfo.ID,

  




  get name() this.appInfo.name,

  




  get os() this.appInfo.OS,

  




  get vendor() this.appInfo.vendor,

  




  get version() this.appInfo.version,

  




  get platformBuildID() this.appInfo.platformBuildID,

  




  get platformVersion() this.appInfo.platformVersion,

  




  get locale() {
    var registry = Cc["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Ci.nsIXULChromeRegistry);
    return registry.getSelectedLocale("global");
  },

  




  get userAgent() {
    var window = mozmill.wm.getMostRecentWindow("navigator:browser");
    if (window)
      return window.navigator.userAgent;
    return "";
  }
};













function assertElementVisible(controller, elem, expectedVisibility) {
  var element = elem.getNode();
  var visible;

  switch (element.nodeName) {
    case 'panel':
      visible = (element.state == 'open');
      break;
    default:
      var style = controller.window.getComputedStyle(element, '');
      var state = style.getPropertyValue('visibility');
      visible = (state == 'visible');
  }

  controller.assertJS('subject.visible == subject.expectedVisibility', {
    visible: visible,
    expectedVisibility: expectedVisibility
  });
}










function assertLoadedUrlEqual(controller, targetUrl)
{
  var locationBar = new elementslib.ID(controller.window.document, "urlbar");
  var currentURL = locationBar.getNode().value;

  
  controller.open(targetUrl);
  controller.waitForPageLoad();

  
  controller.waitForEval("subject.targetURL.value == subject.currentURL", gTimeout, 100,
                         {targetURL: locationBar.getNode(),  currentURL: currentURL});
}







function closeContentAreaContextMenu(controller)
{
  var contextMenu = new elementslib.ID(controller.window.document, "contentAreaContextMenu");
  controller.keypress(contextMenu, "VK_ESCAPE", {});
}















function checkSearchField(controller, searchField, searchTerm, submitButton, timeout)
{
  controller.waitThenClick(searchField, timeout);
  controller.type(searchField, searchTerm);

  if (submitButton != undefined) {
    controller.waitThenClick(submitButton, timeout);
  }
}














function createURI(spec, originCharset, baseURI)
{
  let iosvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

  return iosvc.newURI(spec, originCharset, baseURI);
}




function emptyClipboard() {
  var clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
                  getService(Ci.nsIClipboardHelper);
  clipboard.copyString("");
}









function formatUrlPref(prefName)
{
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"]
                     .getService(Ci.nsIURLFormatter);

  return formatter.formatURLPref(prefName);
}







function getDefaultHomepage() {
  var prefs = collector.getModule('PrefsAPI').preferences;

  var prefValue = prefs.getPref("browser.startup.homepage", "",
                                true, Ci.nsIPrefLocalizedString);
  return prefValue.data;
}












function getEntity(urls, entityId)
{
  
  urls.push("resource:///res/dtd/htmlmathml-f.ent");

  
  var extEntities = "";
  for (i = 0; i < urls.length; i++) {
    extEntities += '<!ENTITY % dtd' + i + ' SYSTEM "' +
                   urls[i] + '">%dtd' + i + ';';
  }

  var parser = Cc["@mozilla.org/xmlextras/domparser;1"]
                  .createInstance(Ci.nsIDOMParser);
  var header = '<?xml version="1.0"?><!DOCTYPE elem [' + extEntities + ']>';
  var elem = '<elem id="elementID">&' + entityId + ';</elem>';
  var doc = parser.parseFromString(header + elem, 'text/xml');
  var elemNode = doc.querySelector('elem[id="elementID"]');

  if (elemNode == null)
    throw new Error(arguments.callee.name + ": Unknown entity - " + entityId);

  return elemNode.textContent;
}












function getProperty(url, prefName)
{
  var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
            .getService(Ci.nsIStringBundleService);
  var bundle = sbs.createBundle(url);

  try {
    return bundle.GetStringFromName(prefName);
  } catch (ex) {
    throw new Error(arguments.callee.name + ": Unknown property - " + prefName);
  }
}














function handleWindow(type, text, callback, dontClose) {
  var func_ptr = null;
  var window = null;

  if (dontClose === undefined)
    dontClose = false;

  
  switch (type) {
    case "type":
      func_ptr = mozmill.utils.getWindowByType;
      break;
    case "title":
      func_ptr = mozmill.utils.getWindowByTitle;
      break;
    default:
      throw new Error(arguments.callee.name + ": Unknown opener type - " + type);
  }

  try {
    
    mozmill.controller.waitForEval("subject.getWindow(subject.text) != null", gTimeout, 100,
                                   {getWindow: func_ptr, text: text});
    window = func_ptr(text);

    
    
    var ctrl = new mozmill.controller.MozMillController(window);
    ctrl.sleep(200);

    if (callback)
      callback(ctrl);
  } finally {
    
    if (dontClose != true & window != null) {
      window.close();
      mozmill.controller.waitForEval("subject.getWindow(subject.text) != subject.window",
                                     gTimeout, 100,
                                     {getWindow: func_ptr, text: text, window: window});
      return null;
    }

    return ctrl;
  }
}
