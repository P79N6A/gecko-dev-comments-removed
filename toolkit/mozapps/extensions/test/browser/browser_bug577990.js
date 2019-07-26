






var gManagerWindow;
var gCategoryUtilities;
var gProvider;
var gInstall;
var gInstallProperties = [{
  name: "Locale Category Test",
  type: "locale"
}];

function test() {
  try {
    if (Components.classes["@mozilla.org/gfx/info;1"].getService(Components.interfaces.nsIGfxInfo).D2DEnabled) {
      requestLongerTimeout(2);
    }
  } catch(e) {}
  waitForExplicitFinish();

  gProvider = new MockProvider();

  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, finish);
}

function install_locale(aCallback) {
  gInstall = gProvider.createInstalls(gInstallProperties)[0];
  gInstall.addTestListener({
    onInstallEnded: function(aInstall) {
      gInstall.removeTestListener(this);
      aCallback();
    }
  });
  gInstall.install();
}

function check_hidden(aExpectedHidden) {
  var hidden = !gCategoryUtilities.isTypeVisible("locale");
  is(hidden, !!aExpectedHidden, "Should have correct hidden state");
}

function run_open_test(aTestSetup, aLoadHidden, aInitializedHidden, aSelected) {
  function loadCallback(aManagerWindow) {
    gManagerWindow = aManagerWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    check_hidden(aLoadHidden);
  }

  function run() {
    open_manager(null, function() {
      check_hidden(aInitializedHidden);
      var selected = (gCategoryUtilities.selectedCategory == "locale");
      is(selected, !!aSelected, "Should have correct selected state");

      run_next_test();
    }, loadCallback);
  }

  close_manager(gManagerWindow, function() {
    
    aTestSetup ? aTestSetup(run) : run();
  });
}



add_test(function() {
  run_open_test(null, true, true);
});



add_test(function() {
  check_hidden(true);
  install_locale(function() {
    check_hidden(false);
    run_next_test();
  });
});



add_test(function() {
  run_open_test(null, false, false);
});



add_test(function() {
  gInstall.cancel();
  run_open_test(null, false, true)
});



add_test(function() {
  run_open_test(null, true, true);
});




add_test(function() {
  run_open_test(install_locale, true, false);
});


add_test(function() {
  gCategoryUtilities.openType("locale", function() {
    run_open_test(null, false, false, true);
  });
});



add_test(function() {
  gInstall.cancel();
  run_open_test(null, false, true);
});

