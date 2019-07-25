












































var MODULE_NAME = 'UtilsAPI';





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













function assertElementVisible(controller, element, visibility)
{
  var style = controller.window.getComputedStyle(element.getNode(), "");
  var state = style.getPropertyValue("visibility");

  if (visibility) {
    controller.assertJS("subject.visibilityState == 'visible'",
                        {visibilityState: state});
  } else {
    controller.assertJS("subject.visibilityState != 'visible'",
                        {visibilityState: state});
  }
}










function assertLoadedUrlEqual(controller, targetUrl)
{
  var locationBar = new elementslib.ID(controller.window.document, "urlbar");
  var currentUrl = locationBar.getNode().value;

  
  controller.open(targetUrl);
  controller.waitForPageLoad();

  
  controller.assertValue(locationBar, currentUrl);
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









function formatUrlPref(prefName)
{
  var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"]
                     .getService(Ci.nsIURLFormatter);

  return formatter.formatURLPref(prefName);
}












function getProperty(url, prefName)
{
  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].
                       getService(Components.interfaces.nsIStringBundleService);
  var bundle = sbs.createBundle(url);

  return bundle.GetStringFromName(prefName);
}
