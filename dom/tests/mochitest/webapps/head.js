


const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function runAll(steps) {
  SimpleTest.waitForExplicitFinish();

  










  SpecialPowers.setAllAppsLaunchable(true);

  
  steps = steps.concat();
  function next() {
    if (steps.length) {
      steps.shift()(next);
    }
    else {
      SimpleTest.finish();
    }
  }
  next();
}

function confirmNextPopup() {
  var Ci = SpecialPowers.Ci;

  var popupNotifications = SpecialPowers.wrap(window).top.
                           QueryInterface(Ci.nsIInterfaceRequestor).
                           getInterface(Ci.nsIWebNavigation).
                           QueryInterface(Ci.nsIDocShell).
                           chromeEventHandler.ownerDocument.defaultView.
                           PopupNotifications;

  var popupPanel = popupNotifications.panel;

  function onPopupShown() {
    popupPanel.removeEventListener("popupshown", onPopupShown, false);
    SpecialPowers.wrap(this).childNodes[0].button.doCommand();
    popupNotifications._dismiss();
  }
  popupPanel.addEventListener("popupshown", onPopupShown, false);
}




const CID = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID();
const ALERTS_SERVICE_CONTRACT_ID = "@mozilla.org/alerts-service;1";
const ALERTS_SERVICE_CID = Components.ID(Cc[ALERTS_SERVICE_CONTRACT_ID].number);

var AlertsService = {
  classID: Components.ID(CID),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory,
                                         Ci.nsIAlertsService]),

  createInstance: function(aOuter, aIID) {
    if (aOuter) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }

    return this.QueryInterface(aIID);
  },

  init: function() {
    Components.manager.nsIComponentRegistrar.registerFactory(this.classID,
      "", ALERTS_SERVICE_CONTRACT_ID, this);
  },

  restore: function() {
    Components.manager.nsIComponentRegistrar.registerFactory(ALERTS_SERVICE_CID,
      "", ALERTS_SERVICE_CONTRACT_ID, null);
  },

  showAlertNotification: function() {
  },
};

AlertsService.init();

SimpleTest.registerCleanupFunction(() => {
  AlertsService.restore();
});
