



"use strict";




Components.utils.import("resource://services-sync/main.js");



function test() {
  is(Weave.Status.checkSetup(), Weave.CLIENT_NOT_CONFIGURED, "Sync should be disabled on start");
  

  let vbox = document.getElementById("start-remotetabs");
  ok(vbox.hidden, "remote tabs in the start page should be hidden when sync is not enabled");
  
  let menulink = document.getElementById("menuitem-remotetabs");
  ok(menulink.hidden, "link to container should be hidden when sync is not enabled");

  
  
  Weave.Status._authManager.username = "jane doe"; 
  Weave.Status._authManager.basicPassword = "goatcheesesalad";
  Weave.Status._authManager.syncKey = "a-bcdef-abcde-acbde-acbde-acbde";
  
  isnot(Weave.Status.checkSetup(), Weave.CLIENT_NOT_CONFIGURED, "Sync is enabled");
  Weave.Svc.Obs.notify("weave:service:setup-complete");

  
  ok(vbox, "remote tabs grid is present on start page");
  
  is(vbox.hidden, false, "remote tabs should be visible in start page when sync is enabled");
  
  is(menulink.hidden, false, "link to container should be visible when sync is enabled");

  
  Weave.Status._authManager.deleteSyncCredentials();
  Weave.Svc.Obs.notify("weave:service:start-over");
  is(Weave.Status.checkSetup(), Weave.CLIENT_NOT_CONFIGURED, "Sync has been disabled");
  ok(vbox.hidden, "remote tabs in the start page should be hidden when sync is not enabled");
  ok(menulink.hidden, "link to container should be hidden when sync is not enabled");

}
