





let tmp = {};
Cu.import("resource:///modules/translation/Translation.jsm", tmp);
Cu.import("resource://gre/modules/Promise.jsm", tmp);
let {Translation, Promise} = tmp;

const kLanguagesPref = "browser.translation.neverForLanguages";
const kShowUIPref = "browser.translation.ui.show";

function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref(kShowUIPref, true);
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  registerCleanupFunction(function () {
    gBrowser.removeTab(tab);
    Services.prefs.clearUserPref(kShowUIPref);
  });
  tab.linkedBrowser.addEventListener("load", function onload() {
    tab.linkedBrowser.removeEventListener("load", onload, true);
    Task.spawn(function* () {
      for (let test of gTests) {
        info(test.desc);
        yield test.run();
      }
    }).then(finish, ex => {
     ok(false, "Unexpected Exception: " + ex);
     finish();
    });
   }, true);

  content.location = "http://example.com/";
}

function getLanguageExceptions() {
  let langs = Services.prefs.getCharPref(kLanguagesPref);
  return langs ? langs.split(",") : [];
}

function getDomainExceptions() {
  let results = [];
  let enumerator = Services.perms.enumerator;
  while (enumerator.hasMoreElements()) {
    let perm = enumerator.getNext().QueryInterface(Ci.nsIPermission);

    if (perm.type == "translate" &&
        perm.capability == Services.perms.DENY_ACTION)
      results.push(perm.host);
  }

  return results;
}

function getInfoBar() {
  return gBrowser.getNotificationBox().getNotificationWithValue("translation");
}

function openPopup(aPopup) {
  let deferred = Promise.defer();

  aPopup.addEventListener("popupshown", function popupShown() {
    aPopup.removeEventListener("popupshown", popupShown);
    deferred.resolve();
  });

  aPopup.focus();
  
  EventUtils.synthesizeKey("VK_DOWN",
                           { altKey: !navigator.platform.contains("Mac") });

  return deferred.promise;
}

function waitForWindowLoad(aWin) {
  let deferred = Promise.defer();

  aWin.addEventListener("load", function onload() {
    aWin.removeEventListener("load", onload, true);
    deferred.resolve();
  }, true);

  return deferred.promise;
}


let gTests = [

{
  desc: "clean exception lists at startup",
  run: function checkNeverForLanguage() {
    is(getLanguageExceptions().length, 0,
       "we start with an empty list of languages to never translate");
    is(getDomainExceptions().length, 0,
       "we start with an empty list of sites to never translate");
  }
},

{
  desc: "never for language",
  run: function* checkNeverForLanguage() {
    
    Translation.documentStateReceived(gBrowser.selectedBrowser,
                                      {state: Translation.STATE_OFFER,
                                       originalShown: true,
                                       detectedLanguage: "fr"});
    let notif = getInfoBar();
    ok(notif, "the infobar is visible");
    let ui = gBrowser.selectedBrowser.translationUI;
    let uri = gBrowser.selectedBrowser.currentURI;
    ok(ui.shouldShowInfoBar(uri, "fr"),
       "check shouldShowInfoBar initially returns true");

    
    yield openPopup(notif._getAnonElt("options"));
    ok(notif._getAnonElt("options").getAttribute("open"),
       "the options menu is open");

    
    ok(!notif._getAnonElt("neverForLanguage").disabled,
       "The 'Never translate <language>' item isn't disabled");

    
    notif._getAnonElt("neverForLanguage").click();
    ok(!getInfoBar(), "infobar hidden");

    
    let langs = getLanguageExceptions();
    is(langs.length, 1, "one language in the exception list");
    is(langs[0], "fr", "correct language in the exception list");
    ok(!ui.shouldShowInfoBar(uri, "fr"),
       "the infobar wouldn't be shown anymore");

    
    PopupNotifications.getNotification("translate").anchorElement.click();
    notif = getInfoBar();
    
    yield openPopup(notif._getAnonElt("options"));
    ok(notif._getAnonElt("neverForLanguage").disabled,
       "The 'Never translate French' item is disabled");

    
    Services.prefs.setCharPref(kLanguagesPref, "");
    notif.close();
  }
},

{
  desc: "never for site",
  run: function* checkNeverForSite() {
    
    Translation.documentStateReceived(gBrowser.selectedBrowser,
                                      {state: Translation.STATE_OFFER,
                                       originalShown: true,
                                       detectedLanguage: "fr"});
    let notif = getInfoBar();
    ok(notif, "the infobar is visible");
    let ui = gBrowser.selectedBrowser.translationUI;
    let uri = gBrowser.selectedBrowser.currentURI;
    ok(ui.shouldShowInfoBar(uri, "fr"),
       "check shouldShowInfoBar initially returns true");

    
    yield openPopup(notif._getAnonElt("options"));
    ok(notif._getAnonElt("options").getAttribute("open"),
       "the options menu is open");

    
    ok(!notif._getAnonElt("neverForSite").disabled,
       "The 'Never translate site' item isn't disabled");

    
    notif._getAnonElt("neverForSite").click();
    ok(!getInfoBar(), "infobar hidden");

    
    let sites = getDomainExceptions();
    is(sites.length, 1, "one site in the exception list");
    is(sites[0], "example.com", "correct site in the exception list");
    ok(!ui.shouldShowInfoBar(uri, "fr"),
       "the infobar wouldn't be shown anymore");

    
    PopupNotifications.getNotification("translate").anchorElement.click();
    notif = getInfoBar();
    
    yield openPopup(notif._getAnonElt("options"));
    ok(notif._getAnonElt("neverForSite").disabled,
       "The 'Never translate French' item is disabled");

    
    Services.perms.remove("example.com", "translate");
    notif.close();
  }
},

{
  desc: "language exception list",
  run: function* checkLanguageExceptions() {
    
    
    Services.prefs.setCharPref(kLanguagesPref, "fr,de");

    
    let win = openDialog("chrome://browser/content/preferences/translation.xul",
                         "Browser:TranslationExceptions",
                         "", null);
    yield waitForWindowLoad(win);

    
    let getById = win.document.getElementById.bind(win.document);
    let tree = getById("languagesTree");
    let remove = getById("removeLanguage");
    let removeAll = getById("removeAllLanguages");
    is(tree.view.rowCount, 2, "The language exceptions list has 2 items");
    ok(remove.disabled, "The 'Remove Language' button is disabled");
    ok(!removeAll.disabled, "The 'Remove All Languages' button is enabled");

    
    tree.view.selection.select(0);
    ok(!remove.disabled, "The 'Remove Language' button is enabled");

    
    remove.click();
    is(tree.view.rowCount, 1, "The language exceptions now contains 1 item");
    is(getLanguageExceptions().length, 1, "One exception in the pref");

    
    Services.prefs.setCharPref(kLanguagesPref, "");
    is(tree.view.rowCount, 0, "The language exceptions list is empty");
    ok(remove.disabled, "The 'Remove Language' button is disabled");
    ok(removeAll.disabled, "The 'Remove All Languages' button is disabled");

    
    Services.prefs.setCharPref(kLanguagesPref, "fr");
    is(tree.view.rowCount, 1, "The language exceptions list has 1 item");
    ok(remove.disabled, "The 'Remove Language' button is disabled");
    ok(!removeAll.disabled, "The 'Remove All Languages' button is enabled");

    
    removeAll.click();
    is(tree.view.rowCount, 0, "The language exceptions list is empty");
    ok(remove.disabled, "The 'Remove Language' button is disabled");
    ok(removeAll.disabled, "The 'Remove All Languages' button is disabled");
    is(Services.prefs.getCharPref(kLanguagesPref), "", "The pref is empty");

    win.close();
  }
},

{
  desc: "domains exception list",
  run: function* checkDomainExceptions() {
    
    
    let perms = Services.perms;
    perms.add(makeURI("http://example.org"), "translate", perms.DENY_ACTION);
    perms.add(makeURI("http://example.com"), "translate", perms.DENY_ACTION);

    
    let win = openDialog("chrome://browser/content/preferences/translation.xul",
                         "Browser:TranslationExceptions",
                         "", null);
    yield waitForWindowLoad(win);

    
    let getById = win.document.getElementById.bind(win.document);
    let tree = getById("sitesTree");
    let remove = getById("removeSite");
    let removeAll = getById("removeAllSites");
    is(tree.view.rowCount, 2, "The sites exceptions list has 2 items");
    ok(remove.disabled, "The 'Remove Site' button is disabled");
    ok(!removeAll.disabled, "The 'Remove All Sites' button is enabled");

    
    tree.view.selection.select(0);
    ok(!remove.disabled, "The 'Remove Site' button is enabled");

    
    remove.click();
    is(tree.view.rowCount, 1, "The site exceptions now contains 1 item");
    is(getDomainExceptions().length, 1, "One exception in the permissions");

    
    perms.remove("example.org", "translate");
    perms.remove("example.com", "translate");
    is(tree.view.rowCount, 0, "The site exceptions list is empty");
    ok(remove.disabled, "The 'Remove Site' button is disabled");
    ok(removeAll.disabled, "The 'Remove All Site' button is disabled");

    
    perms.add(makeURI("http://example.com"), "translate", perms.DENY_ACTION);
    is(tree.view.rowCount, 1, "The site exceptions list has 1 item");
    ok(remove.disabled, "The 'Remove Site' button is disabled");
    ok(!removeAll.disabled, "The 'Remove All Sites' button is enabled");

    
    removeAll.click();
    is(tree.view.rowCount, 0, "The site exceptions list is empty");
    ok(remove.disabled, "The 'Remove Site' button is disabled");
    ok(removeAll.disabled, "The 'Remove All Sites' button is disabled");
    is(getDomainExceptions().length, 0, "No exceptions in the permissions");

    win.close();
  }
}

];
