"use strict";







const TESTCASE_URI = TEST_BASE + "missing.html";

add_task(function* () {
  let { ui, toolbox, panel } = yield openStyleEditorForURL(TESTCASE_URI);

  
  
  
  
  ok(ui.editors.length, "The UI contains style sheets.");

  let rootEl = panel.panelWindow.document.getElementById("style-editor-chrome");
  ok(!rootEl.classList.contains("loading"), "The loading indicator is hidden");

  let notifBox = toolbox.getNotificationBox();
  let notif = notifBox.currentNotification;
  ok(notif, "The notification box contains a message");
  ok(notif.label.indexOf("Style sheet could not be loaded") !== -1,
    "The error message is the correct one");
  ok(notif.label.indexOf("missing-stylesheet.css") !== -1,
    "The error message contains the missing stylesheet's URL");
});
