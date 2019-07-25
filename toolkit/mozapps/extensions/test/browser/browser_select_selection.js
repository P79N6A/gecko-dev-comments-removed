





var gProvider;
var gWin;

const PROFILE = AddonManager.SCOPE_PROFILE;
const USER = AddonManager.SCOPE_USER;
const APP = AddonManager.SCOPE_APPLICATION;
const SYSTEM = AddonManager.SCOPE_SYSTEM;
const DIST = -1;




var ADDONS = [
  
  [false,          true,          false,         false,    false,     true,       PROFILE, true,        42,       "enabled",           ""],               
  [false,          true,          false,         false,    true,      true,       PROFILE, true,        43,       "enabled",           ""],               
  [false,          true,          false,         false,    true,      false,      PROFILE, true,        52,       "unneededupdate",    ""],               
  [false,          false,         false,         true,     false,     true,       PROFILE, true,        53,       "",                  "disabled"],       
  [false,          false,         false,         true,     true,      true,       PROFILE, true,        54,       "",                  "disabled"],       
  [false,          false,         false,         true,     true,      false,      PROFILE, true,        55,       "unneededupdate",    "disabled"],       
  [false,          true,          true,          false,    false,     true,       PROFILE, true,        56,       "incompatible",      ""],               
  [false,          true,          true,          false,    true,      true,       PROFILE, true,        57,       "autoupdate",        ""],               
  [false,          true,          true,          false,    true,      false,      PROFILE, true,        58,       "neededupdate",      ""],               
  [false,          false,         true,          true,     false,     true,       PROFILE, true,        59,       "incompatible",      "disabled"],       
  [false,          true,          true,          true,     true,      true,       PROFILE, true,        44,       "autoupdate",        "disabled"],       
  [false,          true,          true,          true,     true,      false,      PROFILE, true,        45,       "neededupdate",      "disabled"],       
  [true,           false,         false,         false,    false,     true,       PROFILE, false,       46,       "enabled",           ""],               
  [true,           false,         false,         false,    true,      true,       PROFILE, false,       47,       "enabled",           ""],               
  [true,           false,         false,         false,    true,      false,      PROFILE, false,       48,       "unneededupdate",    ""],               

  

  [true,           true,          true,          false,    false,     true,       PROFILE, false,       49,       "incompatible",      ""],               
  [true,           true,          true,          false,    true,      true,       PROFILE, false,       50,       "autoupdate",        ""],               
  [true,           true,          true,          false,    true,      false,      PROFILE, false,       51,       "neededupdate",      ""],               

  

  
  [false,          false,         false,         true,     true,      false,      USER,    false,       0,        "",                  "disabled"],       
  [true,           true,          false,         false,    true,      false,      USER,    false,       1,        "enabled",           ""],               
  [false,          true,          true,          true,     true,      false,      USER,    false,       2,        "incompatible",      "disabled"],       
  [true,           true,          true,          false,    true,      false,      USER,    false,       3,        "incompatible",      ""],               
  [false,          false,         false,         true,     true,      false,      SYSTEM,  false,       4,        "",                  "disabled"],       
  [true,           true,          false,         false,    true,      false,      SYSTEM,  false,       5,        "enabled",           ""],               
  [false,          true,          true,          true,     true,      false,      SYSTEM,  false,       6,        "incompatible",      "disabled"],       
  [true,           true,          true,          false,    true,      false,      SYSTEM,  false,       7,        "incompatible",      ""],               
  [false,          false,         false,         true,     true,      false,      APP,     false,       8,        "",                  "disabled"],       
  [true,           true,          false,         false,    true,      false,      APP,     false,       9,        "enabled",           ""],               
  [false,          true,          true,          true,     true,      false,      APP,     false,       10,       "incompatible",      "disabled"],       
  [true,           true,          true,          false,    true,      false,      APP,     false,       11,       "incompatible",      ""],               
];

function waitForView(aView, aCallback) {
  var view = gWin.document.getElementById(aView);
  view.addEventListener("ViewChanged", function() {
    view.removeEventListener("ViewChanged", arguments.callee, false);
    aCallback();
  }, false);
}

function getString(aName) {
  if (!aName)
    return "";

  var strings = Services.strings.createBundle("chrome://mozapps/locale/extensions/selectAddons.properties");
  return strings.GetStringFromName("action." + aName);
}

function getSourceString(aSource) {
  if (!aSource)
    return "";

  var strings = Services.strings.createBundle("chrome://mozapps/locale/extensions/selectAddons.properties");
  switch (aSource) {
    case PROFILE:
      return strings.GetStringFromName("source.profile");
    case DIST:
      return strings.GetStringFromName("source.bundled");
    default:
      return strings.GetStringFromName("source.other");
  }
}

function test() {
  waitForExplicitFinish();

  gProvider = new MockProvider();

  
  Services.prefs.setBoolPref("extensions.installedDistroAddon.test3@tests.mozilla.org", true);
  Services.prefs.setBoolPref("extensions.installedDistroAddon.test12@tests.mozilla.org", true);
  Services.prefs.setBoolPref("extensions.installedDistroAddon.test15@tests.mozilla.org", true);

  ADDONS.forEach(function(aAddon, aPos) {
    var addon = new MockAddon("test" + aPos + "@tests.mozilla.org",
                              "Test Add-on " + aPos, "extension");
    addon.version = "1.0";
    addon.userDisabled = aAddon[0];
    addon.appDisabled = aAddon[1];
    addon.isActive = aAddon[3];
    addon.applyBackgroundUpdates = aAddon[5] ? AddonManager.AUTOUPDATE_ENABLE
                                             : AddonManager.AUTOUPDATE_DISABLE;
    addon.scope = aAddon[6];

    
    if (addon.scope != AddonManager.SCOPE_PROFILE)
      addon._permissions -= AddonManager.PERM_CAN_UPGRADE;

    addon.findUpdates = function(aListener, aReason, aAppVersion, aPlatformVersion) {
      addon.appDisabled = aAddon[2];
      addon.isActive = addon.shouldBeActive;

      if (aAddon[4]) {
        var newAddon = new MockAddon(this.id, this.name, "extension");
        newAddon.version = "2.0";
        var install = new MockInstall(this.name, this.type, newAddon);
        install.existingAddon = this;
        aListener.onUpdateAvailable(this, install);
      }

      aListener.onUpdateFinished(this, AddonManager.UPDATE_STATUS_NO_ERROR);
    };

    gProvider.addAddon(addon);
  });

  gWin = Services.ww.openWindow(null,
                                "chrome://mozapps/content/extensions/selectAddons.xul",
                                "",
                                "chrome,centerscreen,dialog,titlebar",
                                null);
  waitForFocus(function() {
    waitForView("select", run_next_test);
  }, gWin);
}

function end_test() {
  gWin.close();
  finish();
}


add_test(function checking_test() {
  
  var progress = gWin.document.getElementById("checking-progress");
  is(progress.mode, "determined", "Should be a determined progress bar");
  is(progress.value, progress.max, "Should be at full progress");

  run_next_test();
});


add_test(function selection_test() {
  function check_state() {
    var str = addon[keep.checked ? 9 : 10];
    var expected = getString(str);
    var showCheckbox = str == "neededupdate" || str == "unneededupdate";
    is(action.textContent, expected, "Action message should have the right text");
    is(!is_hidden(update), showCheckbox, "Checkbox should have the right visibility");
    is(is_hidden(action), showCheckbox, "Message should have the right visibility");
    if (showCheckbox)
      ok(update.checked, "Optional update checkbox should be checked");

    if (keep.checked) {
      is(row.hasAttribute("active"), !addon[2] || hasUpdate,
       "Add-on will be active if it isn't appDisabled or an update is available");

      if (showCheckbox) {
        info("Flipping update checkbox");
        EventUtils.synthesizeMouseAtCenter(update, { }, gWin);
        is(row.hasAttribute("active"), str == "unneededupdate",
           "If the optional update isn't needed then the add-on will still be active");

        info("Flipping update checkbox");
        EventUtils.synthesizeMouseAtCenter(update, { }, gWin);
        is(row.hasAttribute("active"), !addon[2] || hasUpdate,
         "Add-on will be active if it isn't appDisabled or an update is available");
      }
    }
    else {
      ok(!row.hasAttribute("active"), "Add-on won't be active when not keeping");

      if (showCheckbox) {
        info("Flipping update checkbox");
        EventUtils.synthesizeMouseAtCenter(update, { }, gWin);
        ok(!row.hasAttribute("active"),
           "Unchecking the update checkbox shouldn't make the add-on active");

        info("Flipping update checkbox");
        EventUtils.synthesizeMouseAtCenter(update, { }, gWin);
        ok(!row.hasAttribute("active"),
           "Re-checking the update checkbox shouldn't make the add-on active");
      }
    }
  }

  is(gWin.document.getElementById("view-deck").selectedPanel.id, "select",
     "Should be on the right view");

  var pos = 0;
  var scrollbox = gWin.document.getElementById("select-scrollbox");
  var scrollBoxObject = scrollbox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
  for (var row = gWin.document.getElementById("select-rows").firstChild; row; row = row.nextSibling) {
    
    
    if (row.localName == "separator") {
      pos += 30;
      continue;
    }

    is(row._addon.type, "extension", "Should only be listing extensions");

    
    if (row.id.substr(-18) != "@tests.mozilla.org")
      continue;

    var id = parseInt(row.id.substring(4, row.id.length - 18));
    var addon = ADDONS[id];

    info("Testing add-on " + id);
    scrollBoxObject.ensureElementIsVisible(row);
    var keep = gWin.document.getAnonymousElementByAttribute(row, "anonid", "keep");
    var action = gWin.document.getAnonymousElementByAttribute(row, "class", "addon-action-message");
    var update = gWin.document.getAnonymousElementByAttribute(row, "anonid", "update");
    var source = gWin.document.getAnonymousElementByAttribute(row, "class", "addon-source");

    if (id == 3 || id == 12 || id == 15) {
      
      is(source.textContent, getSourceString(DIST), "Source message should have the right text for Distributed Addons");
    } else {
      is(source.textContent, getSourceString(addon[6]), "Source message should have the right text");
    }

    
    
    var hasUpdate = addon[4] && addon[6] == PROFILE;

    is(pos, addon[8], "Should have been in the right position");
    is(keep.checked, addon[7], "Keep checkbox should be in the right state");

    check_state();

    info("Flipping keep");
    EventUtils.synthesizeMouseAtCenter(keep, { }, gWin);
    is(keep.checked, !addon[7], "Keep checkbox should be in the right state");

    check_state();

    pos++;
  }

  is(pos, 60, "Should have seen the right number of add-ons");

  run_next_test();
});
