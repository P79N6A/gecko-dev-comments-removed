



"use strict";

let Preferences = Cu.import("resource://gre/modules/Preferences.jsm", {}).Preferences;
Cu.import("resource://gre/modules/Promise.jsm");

add_task(function() {
  yield PanelUI.show({type: "command"});

  let historyButton = document.getElementById("history-panelmenu");
  let historySubview = document.getElementById("PanelUI-history");
  let subviewShownPromise = subviewShown(historySubview);
  EventUtils.synthesizeMouseAtCenter(historyButton, {});
  yield subviewShownPromise;

  let tabsFromOtherComputers = document.getElementById("sync-tabs-menuitem2");
  is(tabsFromOtherComputers.hidden, true, "The Tabs From Other Computers menuitem should be hidden when sync isn't enabled.");

  let hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield hiddenPanelPromise;

  
  Weave.Service.createAccount("john@doe.com", "mysecretpw",
                              "challenge", "response");
  Weave.Service.identity.account = "john@doe.com";
  Weave.Service.identity.basicPassword = "mysecretpw";
  Weave.Service.identity.syncKey = Weave.Utils.generatePassphrase();
  Weave.Svc.Prefs.set("firstSync", "newAccount");
  Weave.Service.persistLogin();

  yield PanelUI.show({type: "command"});

  subviewShownPromise = subviewShown(historySubview);
  EventUtils.synthesizeMouseAtCenter(historyButton, {});
  yield subviewShownPromise;

  is(tabsFromOtherComputers.hidden, false, "The Tabs From Other Computers menuitem should be shown when sync is enabled.");

  let syncPrefBranch = new Preferences("services.sync.");
  syncPrefBranch.resetBranch("");
  Services.logins.removeAllLogins();

  hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPanelPromise;
});
