






const LIGHT_BG = "#fcfcfc";
const DARK_BG = "#14171a";

let test = Task.async(function*() {
  let [target, debuggee, panel] = yield initFrontend(SIMPLE_URL);
  let { $, EVENTS, ProfileView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel, { waitForDisplay: true });

  let graph = ProfileView._getCategoriesGraph();
  let refreshed = once(graph, "refresh");
  setTheme("dark");

  yield refreshed;
  is(graph.backgroundColor, DARK_BG,
    "correct theme (dark) after toggle.");

  refreshed = once(graph, "refresh");
  setTheme("light");

  yield refreshed;
  is(graph.backgroundColor, LIGHT_BG,
    "correct theme (light) after toggle.");

  yield teardown(panel);
  finish();
});






function setTheme (newTheme) {
  let oldTheme = Services.prefs.getCharPref("devtools.theme");
  info("Setting `devtools.theme` to \"" + newTheme + "\"");
  Services.prefs.setCharPref("devtools.theme", newTheme);
  gDevTools.emit("pref-changed", {
    pref: "devtools.theme",
    newValue: newTheme,
    oldValue: oldTheme
  });
}
