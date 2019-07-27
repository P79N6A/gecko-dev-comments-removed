"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let setDefaultBrowserCalled = false;

Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochikit/content/tests/SimpleTest/MockObjects.js", this);

function MockShellService() {};
MockShellService.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIShellService]),
  isDefaultBrowser: function(aStartupCheck, aForAllTypes) { return false; },
  setDefaultBrowser: function(aClaimAllTypes, aForAllUsers) {
    setDefaultBrowserCalled = true;
  },
  shouldCheckDefaultBrowser: false,
  canSetDesktopBackground: false,
  BACKGROUND_TILE      : 1,
  BACKGROUND_STRETCH   : 2,
  BACKGROUND_CENTER    : 3,
  BACKGROUND_FILL      : 4,
  BACKGROUND_FIT       : 5,
  setDesktopBackground: function(aElement, aPosition) {},
  APPLICATION_MAIL : 0,
  APPLICATION_NEWS : 1,
  openApplication: function(aApplication) {},
  desktopBackgroundColor: 0,
  openApplicationWithURI: function(aApplication, aURI) {},
  defaultFeedReader: 0,
};

let mockShellService = new MockObjectRegisterer("@mozilla.org/browser/shell-service;1",
                                                MockShellService);




function test() {
  UITourTest();
}

let tests = [

  











  taskify(function* test_isDefaultBrowser(done) {
    let shell = Components.classes["@mozilla.org/browser/shell-service;1"]
                          .getService(Components.interfaces.nsIShellService);
    let isDefault = shell.isDefaultBrowser(false);
    gContentAPI.isDefaultBrowser(function(data) {
      is(data.value, isDefault, "gContentAPI.isDefaultBrowser should match shellService.isDefaultBrowser");
      done();
    });
  })
];
