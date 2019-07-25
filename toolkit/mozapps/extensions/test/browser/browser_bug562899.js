






Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm");

const xpi = "browser/toolkit/mozapps/extensions/test/browser/browser_installssl.xpi";

var gManagerWindow;
var gCategoryUtilities;

function test() {
  waitForExplicitFinish();

  
  LightweightThemeManager.currentTheme = {
    id: "test",
    name: "Test lightweight theme",
    headerURL: "http://example.com/header.png"
  };

  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    LightweightThemeManager.forgetUsedTheme("test");
    finish();
  });
}



add_test(function() {
  var themeCount = null;
  var pluginCount = null;
  var themeItem = gCategoryUtilities.get("theme");
  var pluginItem = gCategoryUtilities.get("plugin");
  var list = gManagerWindow.document.getElementById("addon-list");

  gCategoryUtilities.open(themeItem, function() {
    themeCount = list.childNodes.length;
    ok(themeCount > 0, "Test is useless if there are no themes");

    gCategoryUtilities.open(pluginItem, function() {
      pluginCount = list.childNodes.length;
      ok(pluginCount > 0, "Test is useless if there are no plugins");

      gCategoryUtilities.open(themeItem);

      gCategoryUtilities.open(pluginItem, function() {
        is(list.childNodes.length, pluginCount, "Should only see the plugins");

        var item = list.firstChild;
        while (item) {
          is(item.getAttribute("type"), "plugin", "All items should be plugins");
          item = item.nextSibling;
        }

        
        

        gCategoryUtilities.open(themeItem);
        gCategoryUtilities.open(pluginItem);
        gCategoryUtilities.open(themeItem, function() {
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
