






const { setTheme } = devtools.require("devtools/shared/theme");

add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_CONTEXT_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, MARKER_STYLING } = panelWin;

  let currentTheme = Services.prefs.getCharPref("devtools.theme");

  ok(MARKER_STYLING.light, "Marker styling exists for light theme.");
  ok(MARKER_STYLING.dark, "Marker styling exists for dark theme.");

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    get3(gFront, "create-node"),
    waitForGraphRendered(panelWin, 3, 2)
  ]);

  is(getFill($("#arrowhead")), MARKER_STYLING[currentTheme],
    "marker initially matches theme.");

  
  setTheme("light");
  is(getFill($("#arrowhead")), MARKER_STYLING.light,
    "marker styling matches light theme on change.");

  
  setTheme("dark");
  is(getFill($("#arrowhead")), MARKER_STYLING.dark,
    "marker styling matches dark theme on change.");

  
  setTheme("dark");
  is(getFill($("#arrowhead")), MARKER_STYLING.dark,
    "marker styling remains dark.");

  
  setTheme("light");
  is(getFill($("#arrowhead")), MARKER_STYLING.light,
    "marker styling switches back to light once again.");

  yield teardown(target);
});





function getFill (el) {
  return el.getAttribute("style").match(/(#.*)$/)[1];
}
