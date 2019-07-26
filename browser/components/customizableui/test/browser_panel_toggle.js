



"use strict";






add_task(function() {
  let shownPromise = promisePanelShown(window);
  PanelUI.show();
  yield shownPromise;

  is(PanelUI.panel.getAttribute("panelopen"), "true", "Check that panel has panelopen attribute");
  is(PanelUI.panel.state, "open", "Check that panel state is 'open'");

  let hiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield hiddenPromise;

  ok(!PanelUI.panel.hasAttribute("panelopen"), "Check that panel doesn't have the panelopen attribute");
  is(PanelUI.panel.state, "closed", "Check that panel state is 'closed'");
});


add_task(function() {
  let shownPromise = promisePanelShown(window);
  PanelUI.toggle({type: "command"});
  yield shownPromise;

  is(PanelUI.panel.getAttribute("panelopen"), "true", "Check that panel has panelopen attribute");
  is(PanelUI.panel.state, "open", "Check that panel state is 'open'");

  let hiddenPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPromise;

  ok(!PanelUI.panel.hasAttribute("panelopen"), "Check that panel doesn't have the panelopen attribute");
  is(PanelUI.panel.state, "closed", "Check that panel state is 'closed'");
});
