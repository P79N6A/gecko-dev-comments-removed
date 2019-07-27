







const TAB_URL = EXAMPLE_URL + "doc_domnode-variables.html";

function test() {
  Task.spawn(function() {
    let [tab,, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let bubble = win.DebuggerView.VariableBubble;
    let tooltip = bubble._tooltip.panel;
    let toolbox = gDevTools.getToolbox(panel.target);

    function getDomNodeInTooltip(propertyName) {
      let domNodeProperties = tooltip.querySelectorAll(".token-domnode");
      for (let prop of domNodeProperties) {
        let propName = prop.parentNode.querySelector(".name");
        if (propName.getAttribute("value") === propertyName) {
          ok(true, "DOMNode " + propertyName + " was found in the tooltip");
          return prop;
        }
      }
      ok(false, "DOMNode " + propertyName + " wasn't found in the tooltip");
    }

    callInTab(tab, "start");
    yield waitForSourceAndCaretAndScopes(panel, ".html", 19);

    
    yield openVarPopup(panel, { line: 17, ch: 38 }, true);
    let property = getDomNodeInTooltip("firstElementChild");

    
    let highlighted = once(toolbox, "node-highlight");
    EventUtils.sendMouseEvent({ type: "mouseover" }, property,
      property.ownerDocument.defaultView);
    yield highlighted;
    ok(true, "The node-highlight event was fired on hover of the DOMNode");

    
    let button = property.parentNode.querySelector(".variables-view-open-inspector");
    ok(button, "The select-in-inspector button is present");
    let inspectorSelected = once(toolbox, "inspector-selected");
    EventUtils.sendMouseEvent({ type: "mousedown" }, button,
      button.ownerDocument.defaultView);
    yield inspectorSelected;
    ok(true, "The inspector got selected when clicked on the select-in-inspector");

    
    
    
    yield once(toolbox.getPanel("inspector"), "inspector-updated");

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
