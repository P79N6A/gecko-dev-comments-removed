







const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

function test() {
  Task.spawn(function() {
    let [tab, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let events = win.EVENTS;
    let editor = win.DebuggerView.editor;
    let bubble = win.DebuggerView.VariableBubble;
    let tooltip = bubble._tooltip.panel;

    function openPopup(coords) {
      let popupshown = once(tooltip, "popupshown");
      let fetched = waitForDebuggerEvents(panel, events.FETCHED_BUBBLE_PROPERTIES);
      let { left, top } = editor.getCoordsFromPosition(coords);
      bubble._findIdentifier(left, top);
      return promise.all([popupshown, fetched]);
    }

    function verifyContents() {
      is(tooltip.querySelectorAll(".variables-view-container").length, 1,
        "There should be one variables view container added to the tooltip.");

      is(tooltip.querySelectorAll(".variables-view-scope[non-header]").length, 1,
        "There should be one scope with no header displayed.");
      is(tooltip.querySelectorAll(".variables-view-variable[non-header]").length, 1,
        "There should be one variable with no header displayed.");

      is(tooltip.querySelectorAll(".variables-view-property").length, 2,
        "There should be 2 properties displayed.");

      is(tooltip.querySelectorAll(".variables-view-property .name")[0].getAttribute("value"), "a",
        "The first property's name is correct.");
      is(tooltip.querySelectorAll(".variables-view-property .value")[0].getAttribute("value"), "1",
        "The first property's value is correct.");

      is(tooltip.querySelectorAll(".variables-view-property .name")[1].getAttribute("value"), "__proto__",
        "The second property's name is correct.");
      is(tooltip.querySelectorAll(".variables-view-property .value")[1].getAttribute("value"), "Object",
        "The second property's value is correct.");
    }

    
    executeSoon(() => debuggee.start());
    yield waitForSourceAndCaretAndScopes(panel, ".html", 24);

    
    yield openPopup({ line: 16, ch: 12 });
    verifyContents();

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
