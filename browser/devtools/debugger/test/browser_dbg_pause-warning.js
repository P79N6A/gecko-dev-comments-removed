





var gPane = null;
var gTab = null;
var gDebugger = null;
var gView = null;
var gLH = null;
var gL10N = null;
var gToolbox = null;
var gTarget = null;

function test() {
  debug_tab_pane(STACK_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gPane = aPane;
    gDebugger = gPane.panelWin;
    gView = gDebugger.DebuggerView;
    gLH = gDebugger.LayoutHelpers;
    gL10N = gDebugger.L10N;

    gTarget = TargetFactory.forTab(gBrowser.selectedTab);
    gToolbox = gDevTools.getToolbox(gTarget);

    testPause();
  });
}

function testPause() {
  let button = gDebugger.document.getElementById("resume");

  gDebugger.DebuggerController.activeThread.addOneTimeListener("paused", function() {
    Services.tm.currentThread.dispatch({ run: function() {
      is(gDebugger.DebuggerController.activeThread.paused, true,
        "Debugger is paused.");

      ok(gTarget.isThreadPaused, "target.isThreadPaused has been updated");

      gToolbox.once("inspector-selected", testNotificationIsUp1);
      gToolbox.selectTool("inspector");
    }}, 0);
  });

  EventUtils.sendMouseEvent({ type: "mousedown" },
    gDebugger.document.getElementById("resume"),
    gDebugger);
}

function testNotificationIsUp1() {
  let notificationBox = gToolbox.getNotificationBox();
  let notification = notificationBox.getNotificationWithValue("inspector-script-paused");
  ok(notification, "Notification is present");
  gToolbox.once("jsdebugger-selected", testNotificationIsHidden);
  gToolbox.selectTool("jsdebugger");
}

function testNotificationIsHidden() {
  let notificationBox = gToolbox.getNotificationBox();
  let notification = notificationBox.getNotificationWithValue("inspector-script-paused");
  ok(!notification, "Notification is hidden");
  gToolbox.once("inspector-selected", testNotificationIsUp2);
  gToolbox.selectTool("inspector");
}

function testNotificationIsUp2() {
  let notificationBox = gToolbox.getNotificationBox();
  let notification = notificationBox.getNotificationWithValue("inspector-script-paused");
  ok(notification, "Notification is present");
  testResume();
}

function testResume() {
  gDebugger.DebuggerController.activeThread.addOneTimeListener("resumed", function() {
    Services.tm.currentThread.dispatch({ run: function() {

      ok(!gTarget.isThreadPaused, "target.isThreadPaused has been updated");
      let notificationBox = gToolbox.getNotificationBox();
      let notification = notificationBox.getNotificationWithValue("inspector-script-paused");
      ok(!notification, "No notification once debugger resumed");

      closeDebuggerAndFinish();
    }}, 0);
  });

  EventUtils.sendMouseEvent({ type: "mousedown" },
    gDebugger.document.getElementById("resume"),
    gDebugger);
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebugger = null;
  gView = null;
  gLH = null;
  gL10N = null;
  gToolbox = null;
  gTarget = null;
});
