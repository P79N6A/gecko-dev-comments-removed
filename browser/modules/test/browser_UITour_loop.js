


"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let loopButton;
let loopPanel = document.getElementById("loop-notification-panel");

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  UITourTest();
}

let tests = [
  taskify(function* test_menu_show_hide() {
    ise(loopButton.open, false, "Menu should initially be closed");
    gContentAPI.showMenu("loop");

    yield waitForConditionPromise(() => {
      return loopButton.open;
    }, "Menu should be visible after showMenu()");

    ok(loopPanel.hasAttribute("noautohide"), "@noautohide should be on the loop panel");
    ok(loopPanel.hasAttribute("panelopen"), "The panel should have @panelopen");
    is(loopPanel.state, "open", "The panel should be open");
    ok(loopButton.hasAttribute("open"), "Loop button should know that the menu is open");

    gContentAPI.hideMenu("loop");
    yield waitForConditionPromise(() => {
        return !loopButton.open;
    }, "Menu should be hidden after hideMenu()");

    checkLoopPanelIsHidden();
  }),
  
  taskify(function* setup_menu_cleanup() {
    gContentAPI.showMenu("loop");

    yield waitForConditionPromise(() => {
      return loopButton.open;
    }, "Menu should be visible after showMenu()");

    
  }),
  taskify(function* test_menu_cleanup() {
    
    checkLoopPanelIsHidden();
  }),
  function test_availableTargets(done) {
    gContentAPI.showMenu("loop");
    gContentAPI.getConfiguration("availableTargets", (data) => {
      for (let targetName of ["loop-newRoom", "loop-roomList", "loop-signInUpLink"]) {
        isnot(data.targets.indexOf(targetName), -1, targetName + " should exist");
      }
      done();
    });
  },
  function test_hideMenuHidesAnnotations(done) {
    let infoPanel = document.getElementById("UITourTooltip");
    let highlightPanel = document.getElementById("UITourHighlightContainer");

    gContentAPI.showMenu("loop", function menuCallback() {
      gContentAPI.showHighlight("loop-roomList");
      gContentAPI.showInfo("loop-newRoom", "Make a new room", "AKA. conversation");
      UITour.getTarget(window, "loop-newRoom").then((target) => {
        waitForPopupAtAnchor(infoPanel, target.node, Task.async(function* checkPanelIsOpen() {
          isnot(loopPanel.state, "closed", "Loop panel should still be open");
          ok(loopPanel.hasAttribute("noautohide"), "@noautohide should still be on the loop panel");
          is(highlightPanel.getAttribute("targetName"), "loop-roomList", "Check highlight @targetname");
          is(infoPanel.getAttribute("targetName"), "loop-newRoom", "Check info panel @targetname");

          info("Close the loop menu and make sure the annotations inside disappear");
          let hiddenPromises = [promisePanelElementHidden(window, infoPanel),
                                promisePanelElementHidden(window, highlightPanel)];
          gContentAPI.hideMenu("loop");
          yield Promise.all(hiddenPromises);
          isnot(infoPanel.state, "open", "Info panel should have automatically hid");
          isnot(highlightPanel.state, "open", "Highlight panel should have automatically hid");
          done();
        }), "Info panel should be anchored to the new room button");
      });
    });
  },
];

function checkLoopPanelIsHidden() {
  ok(!loopPanel.hasAttribute("noautohide"), "@noautohide on the loop panel should have been cleaned up");
  ok(!loopPanel.hasAttribute("panelopen"), "The panel shouldn't have @panelopen");
  isnot(loopPanel.state, "open", "The panel shouldn't be open");
  is(loopButton.hasAttribute("open"), false, "Loop button should know that the panel is closed");
}

if (Services.prefs.getBoolPref("loop.enabled")) {
  loopButton = window.LoopUI.toolbarButton.node;
  
  Services.prefs.setBoolPref("loop.gettingStarted.seen", true);
  Services.prefs.setCharPref("loop.server", "http://localhost");
  Services.prefs.setCharPref("services.push.serverURL", "ws://localhost/");

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("loop.gettingStarted.seen");
    Services.prefs.clearUserPref("loop.server");
    Services.prefs.clearUserPref("services.push.serverURL");

    
    
    
    
    let frameId = loopButton.getAttribute("notificationFrameId");
    let frame = document.getElementById(frameId);
    if (frame) {
      frame.remove();
    }
  });
} else {
  ok(true, "Loop is disabled so skip the UITour Loop tests");
  tests = [];
}
