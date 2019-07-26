







const TAB_URL = EXAMPLE_URL + "browser_dbg_blackboxing.html";

var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;

function test()
{
  let scriptShown = false;
  let framesAdded = false;
  let resumed = false;
  let testStarted = false;

  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    resumed = true;
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.panelWin;

    once(gDebugger, "Debugger:SourceShown", function () {
      blackBoxSources();
    });
  });
}

function blackBoxSources() {
  let timesFired = 0;
  gDebugger.addEventListener("Debugger:BlackBoxChange", function _onBlackboxChange() {
    if (++timesFired !== 3) {
      return;
    }
    gDebugger.removeEventListener("Debugger:BlackBoxChange", _onBlackboxChange, false);

    const { activeThread } = gDebugger.DebuggerController;
    activeThread.addOneTimeListener("framesadded", testStackFrames);

    gDebuggee.one();
  }, false);

  getBlackBoxCheckbox(EXAMPLE_URL + "blackboxing_one.js").click();
  getBlackBoxCheckbox(EXAMPLE_URL + "blackboxing_two.js").click();
  getBlackBoxCheckbox(EXAMPLE_URL + "blackboxing_three.js").click();
}

function testStackFrames() {
  const frames = gDebugger.DebuggerView.StackFrames.widget._list;
  is(frames.querySelectorAll(".dbg-stackframe").length, 4,
     "Should get 4 frames (one -> two -> three -> doDebuggerStatement)");
  is(frames.querySelectorAll(".dbg-stackframe-black-boxed").length, 3,
     "And one, two, and three should each have their own black boxed frame.");

  closeDebuggerAndFinish();
}

function getBlackBoxCheckbox(url) {
  return gDebugger.document.querySelector(
    ".side-menu-widget-item[tooltiptext=\""
      + url + "\"] .side-menu-widget-item-checkbox");
}

function once(target, event, callback) {
  target.addEventListener(event, function _listener(...args) {
    target.removeEventListener(event, _listener, false);
    callback.apply(null, args);
  }, false);
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
});
