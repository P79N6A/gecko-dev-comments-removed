





var gManagerWindow;
var gDocument;
var gCategoryUtilities;
var gProvider;

function test() {
  requestLongerTimeout(2);
  waitForExplicitFinish();

  gProvider = new MockProvider();

  gProvider.createAddons([{
    id: "addon1@tests.mozilla.org",
    name: "Uninstall needs restart",
    type: "extension",
    operationsRequiringRestart: AddonManager.OP_NEEDS_RESTART_UNINSTALL
  }, {
    id: "addon2@tests.mozilla.org",
    name: "Uninstall doesn't need restart 1",
    type: "extension",
    operationsRequiringRestart: AddonManager.OP_NEEDS_RESTART_NONE
  }, {
    id: "addon3@tests.mozilla.org",
    name: "Uninstall doesn't need restart 2",
    type: "extension",
    operationsRequiringRestart: AddonManager.OP_NEEDS_RESTART_NONE
  }, {
    id: "addon4@tests.mozilla.org",
    name: "Uninstall doesn't need restart 3",
    type: "extension",
    operationsRequiringRestart: AddonManager.OP_NEEDS_RESTART_NONE
  }, {
    id: "addon5@tests.mozilla.org",
    name: "Uninstall doesn't need restart 4",
    type: "extension",
    operationsRequiringRestart: AddonManager.OP_NEEDS_RESTART_NONE
  }]);

  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    gDocument = gManagerWindow.document;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    finish();
  });
}

function get_item_in_list(aId, aList) {
  var item = aList.firstChild;
  while (item) {
    if ("mAddon" in item && item.mAddon.id == aId) {
      aList.ensureElementIsVisible(item);
      return item;
    }
    item = item.nextSibling;
  }
  return null;
}


add_test(function() {
  var ID = "addon1@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL, "Add-on should require a restart to uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(!button.hidden, "Restart button should not be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      run_next_test();
    });
  });
});


add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(aAddon.isActive, "Add-on should be active");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      run_next_test();
    });
  });
});



add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      aAddon.userDisabled = true;

      ok(!aAddon.isActive, "Add-on should be inactive");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(!aAddon.isActive, "Add-on should be inactive");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      aAddon.userDisabled = false;
      ok(aAddon.isActive, "Add-on should be active");

      run_next_test();
    });
  });
});


add_test(function() {
  var ID = "addon1@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL, "Add-on should require a restart to uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(!button.hidden, "Restart button should not be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      run_next_test();
    });
  });
});


add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(aAddon.isActive, "Add-on should be active");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      run_next_test();
    });
  });
});



add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      aAddon.userDisabled = true;

      ok(!aAddon.isActive, "Add-on should be inactive");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      ok(!aAddon.isActive, "Add-on should be inactive");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      aAddon.userDisabled = false;
      ok(aAddon.isActive, "Add-on should be active");

      run_next_test();
    });
  });
});



add_test(function() {
  var ID = "addon1@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL, "Add-on should require a restart to uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      EventUtils.synthesizeMouse(item, 2, 2, { clickCount: 2 }, gManagerWindow);
      wait_for_view_load(gManagerWindow, function() {
        is(gDocument.getElementById("view-port").selectedPanel.id, "detail-view", "Should be in the detail view");

        var button = gDocument.getElementById("detail-uninstall");
        isnot(button, null, "Should have a remove button");
        ok(!button.disabled, "Button should not be disabled");

        EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

        wait_for_view_load(gManagerWindow, function() {
          is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

          var item = get_item_in_list(ID, list);
          isnot(item, null, "Should have found the add-on in the list");
          is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

          ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

          
          item.clientTop;

          var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
          isnot(button, null, "Should have a restart button");
          ok(!button.hidden, "Restart button should not be hidden");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
          isnot(button, null, "Should have an undo button");

          EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

          
          item.clientTop;

          ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
          isnot(button, null, "Should have a remove button");
          ok(!button.disabled, "Button should not be disabled");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      EventUtils.synthesizeMouse(item, 2, 2, { clickCount: 2 }, gManagerWindow);
      wait_for_view_load(gManagerWindow, function() {
        is(gDocument.getElementById("view-port").selectedPanel.id, "detail-view", "Should be in the detail view");

        var button = gDocument.getElementById("detail-uninstall");
        isnot(button, null, "Should have a remove button");
        ok(!button.disabled, "Button should not be disabled");

        EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

        wait_for_view_load(gManagerWindow, function() {
          is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

          var item = get_item_in_list(ID, list);
          isnot(item, null, "Should have found the add-on in the list");
          is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

          ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
          ok(!aAddon.isActive, "Add-on should be inactive");

          
          item.clientTop;

          var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
          isnot(button, null, "Should have a restart button");
          ok(button.hidden, "Restart button should be hidden");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
          isnot(button, null, "Should have an undo button");

          EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

          
          item.clientTop;

          ok(aAddon.isActive, "Add-on should be active");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
          isnot(button, null, "Should have a remove button");
          ok(!button.disabled, "Button should not be disabled");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      aAddon.userDisabled = true;

      ok(!aAddon.isActive, "Add-on should be inactive");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      EventUtils.synthesizeMouse(item, 2, 2, { clickCount: 2 }, gManagerWindow);
      wait_for_view_load(gManagerWindow, function() {
        is(gDocument.getElementById("view-port").selectedPanel.id, "detail-view", "Should be in the detail view");

        var button = gDocument.getElementById("detail-uninstall");
        isnot(button, null, "Should have a remove button");
        ok(!button.disabled, "Button should not be disabled");

        EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

        wait_for_view_load(gManagerWindow, function() {
          is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

          var item = get_item_in_list(ID, list);
          isnot(item, null, "Should have found the add-on in the list");
          is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

          ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
          ok(!aAddon.isActive, "Add-on should be inactive");

          
          item.clientTop;

          var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
          isnot(button, null, "Should have a restart button");
          ok(button.hidden, "Restart button should be hidden");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
          isnot(button, null, "Should have an undo button");

          EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

          
          item.clientTop;

          ok(!aAddon.isActive, "Add-on should be inactive");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
          isnot(button, null, "Should have a remove button");
          ok(!button.disabled, "Button should not be disabled");

          aAddon.userDisabled = false;
          ok(aAddon.isActive, "Add-on should be active");

          run_next_test();
        });
      });
    });
  });
});


add_test(function() {
  var ID = "addon1@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL, "Add-on should require a restart to uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(!button.hidden, "Restart button should not be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      gCategoryUtilities.openType("plugin", function() {
        is(gCategoryUtilities.selectedCategory, "plugin", "View should have changed to plugin");
        gCategoryUtilities.openType("extension", function() {
          is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

          var item = get_item_in_list(ID, list);
          isnot(item, null, "Should have found the add-on in the list");
          is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

          ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

          var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
          isnot(button, null, "Should have a restart button");
          ok(!button.hidden, "Restart button should not be hidden");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
          isnot(button, null, "Should have an undo button");

          EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

          
          item.clientTop;
          ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
          isnot(button, null, "Should have a remove button");
          ok(!button.disabled, "Button should not be disabled");

          run_next_test();
        });
      });
    });
  });
});


add_test(function() {
  var ID = "addon1@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL, "Add-on should require a restart to uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(!button.hidden, "Restart button should not be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      gCategoryUtilities.openType("plugin", function() {
        is(gCategoryUtilities.selectedCategory, "plugin", "View should have changed to plugin");
        searchBox.value = "Uninstall";

        EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
        EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

        wait_for_view_load(gManagerWindow, function() {
          is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

          var item = get_item_in_list(ID, list);
          isnot(item, null, "Should have found the add-on in the list");
          is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

          ok(!!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should be pending uninstall");

          var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
          isnot(button, null, "Should have a restart button");
          ok(!button.hidden, "Restart button should not be hidden");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
          isnot(button, null, "Should have an undo button");

          EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

          
          item.clientTop;
          ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
          button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
          isnot(button, null, "Should have a remove button");
          ok(!button.disabled, "Button should not be disabled");

          run_next_test();
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon2@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      gCategoryUtilities.openType("plugin", function() {
        is(gCategoryUtilities.selectedCategory, "plugin", "View should have changed to extension");

        AddonManager.getAddonByID(ID, function(aAddon) {
          is(aAddon, null, "Add-on should no longer be installed");

          gCategoryUtilities.openType("extension", function() {
            is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

            var item = get_item_in_list(ID, list);
            is(item, null, "Should not have found the add-on in the list");

            run_next_test();
          });
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon3@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      gCategoryUtilities.openType("plugin", function() {
        is(gCategoryUtilities.selectedCategory, "plugin", "View should have changed to extension");

        AddonManager.getAddonByID(ID, function(aAddon) {
          is(aAddon, null, "Add-on should no longer be installed");

          searchBox.value = "Uninstall";

          EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
          EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

          wait_for_view_load(gManagerWindow, function() {
            is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

            var item = get_item_in_list(ID, list);
            is(item, null, "Should not have found the add-on in the list");

            run_next_test();
          });
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon4@tests.mozilla.org";
  var list = gDocument.getElementById("addon-list");

  
  gCategoryUtilities.openType("extension", function() {
    is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      close_manager(gManagerWindow, function() {
        AddonManager.getAddonByID(ID, function(aAddon) {
          is(aAddon, null, "Add-on should no longer be installed");

          open_manager(null, function(aWindow) {
            gManagerWindow = aWindow;
            gDocument = gManagerWindow.document;
            gCategoryUtilities = new CategoryUtilities(gManagerWindow);
            var list = gDocument.getElementById("addon-list");

            is(gCategoryUtilities.selectedCategory, "extension", "View should have changed to extension");

            var item = get_item_in_list(ID, list);
            is(item, null, "Should not have found the add-on in the list");

            run_next_test();
          });
        });
      });
    });
  });
});



add_test(function() {
  var ID = "addon5@tests.mozilla.org";
  var list = gDocument.getElementById("search-list");

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = "Uninstall";

  EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

    AddonManager.getAddonByID(ID, function(aAddon) {
      ok(aAddon.isActive, "Add-on should be active");
      ok(!(aAddon.operationsRequiringRestart & AddonManager.OP_NEEDS_RESTART_UNINSTALL), "Add-on should not require a restart to uninstall");
      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");

      var item = get_item_in_list(ID, list);
      isnot(item, null, "Should have found the add-on in the list");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
      isnot(button, null, "Should have a remove button");
      ok(!button.disabled, "Button should not be disabled");

      EventUtils.synthesizeMouse(button, 2, 2, { }, gManagerWindow);

      
      item.clientTop;

      is(item.getAttribute("status"), "uninstalled", "Add-on should be uninstalling");

      ok(!(aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL), "Add-on should not be pending uninstall");
      ok(!aAddon.isActive, "Add-on should be inactive");

      var button = gDocument.getAnonymousElementByAttribute(item, "anonid", "restart-btn");
      isnot(button, null, "Should have a restart button");
      ok(button.hidden, "Restart button should be hidden");
      button = gDocument.getAnonymousElementByAttribute(item, "anonid", "undo-btn");
      isnot(button, null, "Should have an undo button");

      close_manager(gManagerWindow, function() {
        AddonManager.getAddonByID(ID, function(aAddon) {
          is(aAddon, null, "Add-on should no longer be installed");

          open_manager(null, function(aWindow) {
            gManagerWindow = aWindow;
            gDocument = gManagerWindow.document;
            gCategoryUtilities = new CategoryUtilities(gManagerWindow);
            var list = gDocument.getElementById("search-list");
            var searchBox = gManagerWindow.document.getElementById("header-search");

            searchBox.value = "Uninstall";

            EventUtils.synthesizeMouse(searchBox, 2, 2, { }, gManagerWindow);
            EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

            wait_for_view_load(gManagerWindow, function() {
              is(gCategoryUtilities.selectedCategory, "search", "View should have changed to search");

              var item = get_item_in_list(ID, list);
              is(item, null, "Should not have found the add-on in the list");

              run_next_test();
            });
          });
        });
      });
    });
  });
});
