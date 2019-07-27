






function spawnTest() {
  Services.prefs.setBoolPref("devtools.command-button-frames.enabled", true);

  let [target, debuggee, panel, toolbox] = yield initWebAudioEditor(IFRAME_CONTEXT_URL);
  let { gFront, $ } = panel.panelWin;

  is($("#reload-notice").hidden, false,
    "The 'reload this page' notice should initially be visible.");
  is($("#waiting-notice").hidden, true,
    "The 'waiting for an audio context' notice should initially be hidden.");
  is($("#content").hidden, true,
    "The tool's content should initially be hidden.");

  let btn = toolbox.doc.getElementById("command-button-frames");
  ok(!btn.firstChild.getAttribute("hidden"), "The frame list button is visible");
  let frameBtns = btn.firstChild.querySelectorAll("[data-window-id]");
  is(frameBtns.length, 2, "We have both frames in the list");

  
  frameBtns[1].click();

  let navigating = once(target, "will-navigate");

  yield navigating;

  is($("#reload-notice").hidden, false,
    "The 'reload this page' notice should still be visible when switching to a frame.");
  is($("#waiting-notice").hidden, true,
    "The 'waiting for an audio context' notice should be kept hidden when switching to a frame.");
  is($("#content").hidden, true,
    "The tool's content should still be hidden.");

  navigating = once(target, "will-navigate");
  let started = once(gFront, "start-context");

  reload(target);

  yield Promise.all([navigating, started]);

  is($("#reload-notice").hidden, true,
    "The 'reload this page' notice should be hidden after reloading the frame.");
  is($("#waiting-notice").hidden, true,
    "The 'waiting for an audio context' notice should be hidden after reloading the frame.");
  is($("#content").hidden, false,
    "The tool's content should appear after reload.");

  yield teardown(panel);
  finish();
}
