









const BUG_1130901_URL = EXAMPLE_URL + "doc_bug_1130901.html";

add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(BUG_1130901_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, gAudioNodes } = panelWin;

  reload(target);

  yield waitForGraphRendered(panelWin, 3, 0);

  ok(true, "Successfully created a node from AudioContext via `call`.");
  ok(true, "Successfully created a node from AudioContext via `apply`.");

  yield teardown(target);
});
