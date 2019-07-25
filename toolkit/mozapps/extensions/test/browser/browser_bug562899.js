






Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm");

const xpi = "browser/toolkit/mozapps/extensions/test/browser/browser_installssl.xpi";

var gManagerWindow;

function test() {
  waitForExplicitFinish();
  Services.prefs.setBoolPref(PREF_LOGGING_ENABLED, true);

  
  LightweightThemeManager.currentTheme = {
    id: "test",
    name: "Test lightweight theme",
    headerURL: "http://example.com/header.png"
  };

  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  Services.prefs.clearUserPref(PREF_LOGGING_ENABLED);
  gManagerWindow.close();
  LightweightThemeManager.forgetUsedTheme("test");

  finish();
}



add_test(function() {
  var themeCount = null;
  var pluginCount = null;
  var themeItem = gManagerWindow.document.getElementById("category-themes");
  var pluginItem = gManagerWindow.document.getElementById("category-plugins");
  var list = gManagerWindow.document.getElementById("addon-list");

  EventUtils.synthesizeMouse(themeItem, 2, 2, { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    themeCount = list.childNodes.length;
    ok(themeCount > 0, "Test is useless if there are no themes");

    EventUtils.synthesizeMouse(pluginItem, 2, 2, { }, gManagerWindow);

    wait_for_view_load(gManagerWindow, function() {
      pluginCount = list.childNodes.length;
      ok(pluginCount > 0, "Test is useless if there are no plugins");

      EventUtils.synthesizeMouse(themeItem, 2, 2, { }, gManagerWindow);
      EventUtils.synthesizeMouse(pluginItem, 2, 2, { }, gManagerWindow);

      wait_for_view_load(gManagerWindow, function() {
        is(list.childNodes.length, pluginCount, "Should only see the plugins");

        var item = list.firstChild;
        while (item) {
          is(item.getAttribute("type"), "plugin", "All items should be plugins");
          item = item.nextSibling;
        }

        
        

        EventUtils.synthesizeMouse(themeItem, 2, 2, { }, gManagerWindow);
        EventUtils.synthesizeMouse(pluginItem, 2, 2, { }, gManagerWindow);
        EventUtils.synthesizeMouse(themeItem, 2, 2, { }, gManagerWindow);

        wait_for_view_load(gManagerWindow, function() {
          is(list.childNodes.length, themeCount, "Should only see the theme");

          var item = list.firstChild;
          while (item) {
            is(item.getAttribute("type"), "theme", "All items should be theme");
            item = item.nextSibling;
          }

          run_next_test();
        });
      });
    });
  });
});
