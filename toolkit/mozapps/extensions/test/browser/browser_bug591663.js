








let VIEW_ID = "addons://list/mock-addon";

let LIST_ID = "addon-list";
let EMPTY_ID = "addon-list-empty";

let gManagerWindow;
let gProvider;
let gItem;

let gInstallProperties = {
  name: "Bug 591663 Mock Install",
  type: "mock-addon"
};
let gAddonProperties = {
  id: "test1@tests.mozilla.org",
  name: "Bug 591663 Mock Add-on",
  type: "mock-addon"
};
let gExtensionProperties = {
  name: "Bug 591663 Extension Install",
  type: "extension"
};

function test() {
  waitForExplicitFinish();

  gProvider = new MockProvider(true, [{
    id: "mock-addon",
    name: "Mock Add-ons",
    uiPriority: 4500,
    flags: AddonManager.TYPE_UI_VIEW_LIST
  }]);

  open_manager(VIEW_ID, function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, finish);
}







function check_list(aItem) {
  
  let emptyNotice = gManagerWindow.document.getElementById(EMPTY_ID);
  ok(emptyNotice != null, "Should have found the empty notice");
  is(!emptyNotice.hidden, (aItem == null), "Empty notice should be showing if list empty");

  
  let list = gManagerWindow.document.getElementById(LIST_ID);
  is(list.itemCount, aItem ? 1 : 0, "Should get expected number of items in list");
  if (aItem != null) {
    let itemName = list.firstChild.getAttribute("name");
    is(itemName, aItem.name, "List item should have correct name");
  }
}



add_test(function() {
  check_list(null);
  run_next_test();
});


add_test(function() {
  gItem = gProvider.createInstalls([gInstallProperties])[0];
  check_list(null);
  run_next_test();
});


add_test(function() {
  gItem.addTestListener({
    onDownloadStarted: function() {
      
      check_list(null);
    },
    onInstallStarted: function() {
      check_list(gItem);
    },
    onInstallEnded: function() {
      check_list(gItem);
      run_next_test();
    }
  });

  gItem.install();
});


add_test(function() {
  restart_manager(gManagerWindow, VIEW_ID, function(aManagerWindow) {
    gManagerWindow = aManagerWindow;
    check_list(gItem);
    run_next_test();
  });
});


add_test(function() {
  gItem.cancel();
  gItem = null;
  check_list(null);
  run_next_test();
});


add_test(function() {
  let extension = gProvider.createInstalls([gExtensionProperties])[0];
  check_list(null);

  extension.addTestListener({
    onDownloadStarted: function() {
      check_list(null);
    },
    onInstallStarted: function() {
      check_list(null);
    },
    onInstallEnded: function() {
      check_list(null);
      extension.cancel();
      run_next_test();
    }
  });

  extension.install();
});


add_test(function() {
  gItem = gProvider.createAddons([gAddonProperties])[0];
  check_list(gItem);
  run_next_test();
});


add_test(function() {
  restart_manager(gManagerWindow, VIEW_ID, function(aManagerWindow) {
    gManagerWindow = aManagerWindow;
    check_list(gItem);
    run_next_test();
  });
});

