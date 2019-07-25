


 
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");





function AlertsService() { }

AlertsService.prototype = {
  classID: Components.ID("{5dce03b2-8faa-4b6e-9242-6ddb0411750c}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAlertsService]),

  showAlertNotification: function(aImageUrl, aTitle, aText, aTextClickable, aCookie, aAlertListener, aName) {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    browser.AlertsHelper.showAlertNotification(aImageUrl, aTitle, aText, aTextClickable, aCookie, aAlertListener, aName);
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([AlertsService]);
