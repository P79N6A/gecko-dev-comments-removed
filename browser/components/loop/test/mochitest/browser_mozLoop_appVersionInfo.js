







"use strict";

Components.utils.import("resource://gre/modules/Promise.jsm", this);

add_task(loadLoopPanel);

add_task(function* test_mozLoop_appVersionInfo() {
  Assert.ok(gMozLoopAPI, "mozLoop should exist");

  let appVersionInfo = gMozLoopAPI.appVersionInfo;

  Assert.ok(appVersionInfo, "should have appVersionInfo");

  Assert.equal(appVersionInfo.channel,
               Services.prefs.getCharPref("app.update.channel"),
               "appVersionInfo.channel should match the application channel");

  var appInfo = Cc["@mozilla.org/xre/app-info;1"]
                  .getService(Ci.nsIXULAppInfo)
                  .QueryInterface(Ci.nsIXULRuntime);

  Assert.equal(appVersionInfo.version,
               appInfo.version,
               "appVersionInfo.version should match the application version");

  Assert.equal(appVersionInfo.OS,
               appInfo.OS,
               "appVersionInfo.os should match the running os");
});
