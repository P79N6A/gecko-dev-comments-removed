



const TEST_URI = "data:text/html;charset=utf-8,<p>test for bug 663443. test1";

const POSITION_PREF = "devtools.webconsole.position";
const POSITION_WINDOW = "window";

function consoleOpened() {
  document.removeEventListener("popupshown", consoleOpened, false);

  let HUD = HUDService.getHudByWindow(content);
  ok(HUD.consolePanel, "Web Console opened in a panel");

  let waitForTitleChange = {
    name: "panel title change",
    validatorFn: function() {
      return HUD.consolePanel.label.indexOf("test2") > -1;
    },
    successFn: testEnd,
    failureFn: testEnd,
  };

  waitForSuccess({
    name: "initial panel title",
    validatorFn: function() {
      return HUD.consolePanel.label.indexOf("test1") > -1;
    },
    successFn: function() {
      content.location = "data:text/html;charset=utf-8,<p>test2 for bug 663443";
      waitForSuccess(waitForTitleChange);
    },
    failureFn: testEnd,
  });
}

function testEnd() {
  closeConsole(null, finishTest);
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    Services.prefs.setCharPref(POSITION_PREF, POSITION_WINDOW);

    registerCleanupFunction(function() {
      Services.prefs.clearUserPref(POSITION_PREF);
    });
  
    document.addEventListener("popupshown", consoleOpened, false);

    openConsole();
  }, true);
}
