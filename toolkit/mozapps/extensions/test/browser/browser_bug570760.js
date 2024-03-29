




thisTestLeaksUncaughtRejectionsAndShouldBeFixed("");



var gManagerWindow;
var focusCount = 0;

function test() {
  waitForExplicitFinish();

  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;

    var searchBox = gManagerWindow.document.getElementById("header-search");
    function focusHandler() {
      searchBox.blur();
      focusCount++;
    }
    searchBox.addEventListener("focus", focusHandler);
    f_key_test();
    slash_key_test();
    searchBox.removeEventListener("focus", focusHandler);
    end_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, finish);
}

function f_key_test() {
  EventUtils.synthesizeKey("f", { accelKey: true }, gManagerWindow);
  is(focusCount, 1, "Search box should have been focused due to the f key");
}

function slash_key_test() {
  EventUtils.synthesizeKey("/", { }, gManagerWindow);
  is(focusCount, 2, "Search box should have been focused due to the / key");
}
