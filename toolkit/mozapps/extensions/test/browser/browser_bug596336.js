






var gManagerWindow;
var gCategoryUtilities;

function test() {
  waitForExplicitFinish();

  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    finish();
  });
}

function get_list_item_count() {
  return get_test_items_in_list(gManagerWindow).length;
}

function get_node(parent, anonid) {
  return parent.ownerDocument.getAnonymousElementByAttribute(parent, "anonid", anonid);
}

function get_class_node(parent, cls) {
  return parent.ownerDocument.getAnonymousElementByAttribute(parent, "class", cls);
}

function install_addon(aXpi, aCallback) {
  AddonManager.getInstallForURL(TESTROOT + "addons/" + aXpi + ".xpi",
                                function(aInstall) {
    aInstall.addListener({
      onInstallEnded: function(aInstall) {
        executeSoon(aCallback);
      }
    });
    aInstall.install();
  }, "application/x-xpinstall");
}

function check_addon(aAddon, version) {
  is(get_list_item_count(), 1, "Should be one item in the list");
  is(aAddon.version, version, "Add-on should have the right version");

  let item = get_addon_element(gManagerWindow, "addon1@tests.mozilla.org");
  ok(!!item, "Should see the add-on in the list");

  
  item.clientTop;

  is(get_node(item, "version").value, version, "Version should be correct");

  if (aAddon.userDisabled)
    is_element_visible(get_class_node(item, "disabled-postfix"), "Disabled postfix should be hidden");
  else
    is_element_hidden(get_class_node(item, "disabled-postfix"), "Disabled postfix should be hidden");
}


add_test(function() {
  install_addon("browser_bug596336_1", function() {
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
      check_addon(aAddon, "1.0");
      ok(!aAddon.userDisabled, "Add-on should not be disabled");

      install_addon("browser_bug596336_2", function() {
        AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
          check_addon(aAddon, "2.0");
          ok(!aAddon.userDisabled, "Add-on should not be disabled");

          aAddon.uninstall();

          is(get_list_item_count(), 0, "Should be no items in the list");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  install_addon("browser_bug596336_1", function() {
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
      aAddon.userDisabled = true;
      check_addon(aAddon, "1.0");
      ok(aAddon.userDisabled, "Add-on should be disabled");

      install_addon("browser_bug596336_2", function() {
        AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
          check_addon(aAddon, "2.0");
          ok(aAddon.userDisabled, "Add-on should be disabled");

          aAddon.uninstall();

          is(get_list_item_count(), 0, "Should be no items in the list");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  install_addon("browser_bug596336_1", function() {
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
      check_addon(aAddon, "1.0");
      ok(!aAddon.userDisabled, "Add-on should not be disabled");

      let item = get_addon_element(gManagerWindow, "addon1@tests.mozilla.org");
      EventUtils.synthesizeMouseAtCenter(get_node(item, "remove-btn"), { }, gManagerWindow);

      
      item.clientTop;

      ok(aAddon.userDisabled, "Add-on should be disabled");
      ok(!aAddon.pendingUninstall, "Add-on should not be pending uninstall");
      is_element_visible(get_class_node(item, "pending"), "Pending message should be visible");

      install_addon("browser_bug596336_2", function() {
        AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
          check_addon(aAddon, "2.0");
          ok(!aAddon.userDisabled, "Add-on should not be disabled");

          aAddon.uninstall();

          is(get_list_item_count(), 0, "Should be no items in the list");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  install_addon("browser_bug596336_1", function() {
    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
      aAddon.userDisabled = true;
      check_addon(aAddon, "1.0");
      ok(aAddon.userDisabled, "Add-on should be disabled");

      let item = get_addon_element(gManagerWindow, "addon1@tests.mozilla.org");
      EventUtils.synthesizeMouseAtCenter(get_node(item, "remove-btn"), { }, gManagerWindow);

      
      item.clientTop;

      ok(aAddon.userDisabled, "Add-on should be disabled");
      ok(!aAddon.pendingUninstall, "Add-on should not be pending uninstall");
      is_element_visible(get_class_node(item, "pending"), "Pending message should be visible");

      install_addon("browser_bug596336_2", function() {
        AddonManager.getAddonByID("addon1@tests.mozilla.org", function(aAddon) {
          check_addon(aAddon, "2.0");
          ok(aAddon.userDisabled, "Add-on should be disabled");

          aAddon.uninstall();

          is(get_list_item_count(), 0, "Should be no items in the list");

          run_next_test();
        });
      });
    });
  });
});
