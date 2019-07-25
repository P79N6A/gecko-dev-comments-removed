












































var gManagerWindow;
var gProvider;
var gInstall;

const EXTENSION_NAME = "Wunderbar";

function test() {
  waitForExplicitFinish();

  gProvider = new MockProvider();

  open_manager("addons://list/extension", function (aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function () {
    finish();
  });
}




add_test(function () {
  let addon = new MockAddon(undefined, EXTENSION_NAME, "extension", true);
  gInstall = new MockInstall(undefined, undefined, addon);
  gInstall.addTestListener({
    onNewInstall: function () {
      run_next_test();
    }
  });
  gProvider.addInstall(gInstall);
});



add_test(function () {
  gInstall.addTestListener({
    onInstallEnded: function () {
      let list = gManagerWindow.document.getElementById("addon-list");

      
      
      for (let i = 0; i < list.itemCount; i++) {
        let item = list.getItemAtIndex(i);
        if (item.getAttribute("name") === EXTENSION_NAME) {
          ok(true, "Item with correct name found");
          run_next_test();
          return;
        }
      }
      ok(false, "Item with correct name was not found");
      run_next_test();
    }
  });
  gInstall.install();
});
