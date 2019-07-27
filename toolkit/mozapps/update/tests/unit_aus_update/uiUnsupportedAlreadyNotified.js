



function run_test() {
  setupTestCommon();

  debugDump("testing nsIUpdatePrompt notifications should not be displayed " +
            "when showUpdateAvailable is called for an unsupported system " +
            "update when the unsupported notification has already been " +
            "shown (bug 843497)");

  setUpdateURLOverride();
  
  overrideXHR(callHandleEvent);
  standardInit();

  let registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                            "Fake Window Watcher",
                            "@mozilla.org/embedcomp/window-watcher;1",
                            WindowWatcherFactory);
  registrar.registerFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af56}"),
                            "Fake Window Mediator",
                            "@mozilla.org/appshell/window-mediator;1",
                            WindowMediatorFactory);

  Services.prefs.setBoolPref(PREF_APP_UPDATE_SILENT, false);
  Services.prefs.setBoolPref(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED, true);
  
  
  Services.prefs.setIntPref(PREF_APP_UPDATE_BACKGROUNDERRORS, 1);

  gResponseBody = getRemoteUpdatesXMLString("  <update type=\"major\" " +
                                            "name=\"Unsupported Update\" " +
                                            "unsupported=\"true\" " +
                                            "detailsURL=\"" + URL_HOST +
                                            "\"></update>\n");
  gAUS.notify(null);
  do_execute_soon(check_test);
}

function check_test() {
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS)) {
    do_execute_soon(check_test);
    return;
  }
  do_check_true(true);

  doTestFinish();
}

function end_test() {
  let registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.unregisterFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af55}"),
                              WindowWatcherFactory);
  registrar.unregisterFactory(Components.ID("{1dfeb90a-2193-45d5-9cb8-864928b2af56}"),
                              WindowMediatorFactory);
}



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  } catch (e) {
  }
  let e = { target: gXHR };
  gXHR.onload(e);
}

function check_showUpdateAvailable() {
  do_throw("showUpdateAvailable should not have called openWindow!");
}

const WindowWatcher = {
  openWindow: function(aParent, aUrl, aName, aFeatures, aArgs) {
    check_showUpdateAvailable();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowWatcher])
};

const WindowWatcherFactory = {
  createInstance: function createInstance(aOuter, aIID) {
    if (aOuter != null) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return WindowWatcher.QueryInterface(aIID);
  }
};

const WindowMediator = {
  getMostRecentWindow: function(aWindowType) {
    return null;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowMediator])
};

const WindowMediatorFactory = {
  createInstance: function createInstance(aOuter, aIID) {
    if (aOuter != null) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return WindowMediator.QueryInterface(aIID);
  }
};
