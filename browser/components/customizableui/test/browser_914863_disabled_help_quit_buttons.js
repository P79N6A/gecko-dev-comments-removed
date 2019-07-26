




add_task(function() {
  yield startCustomizing();
  let helpButton = document.getElementById("PanelUI-help");
  let quitButton = document.getElementById("PanelUI-quit");
  ok(helpButton.getAttribute("disabled") == "true", "Help button should be disabled while in customization mode.");
  ok(quitButton.getAttribute("disabled") == "true", "Quit button should be disabled while in customization mode.");
  yield endCustomizing();

  ok(!helpButton.hasAttribute("disabled"), "Help button should not be disabled.");
  ok(!quitButton.hasAttribute("disabled"), "Quit button should not be disabled.");
});
