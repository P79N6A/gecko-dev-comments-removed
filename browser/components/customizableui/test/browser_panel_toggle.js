







let gTests = [
  {
    desc: "Show and hide the menu panel programmatically without an event (like UITour.jsm would)",
    setup: null,
    run: function() {
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
    },
  },
  {
    desc: "Toggle the menu panel open and closed",
    setup: null,
    run: function() {
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
    },
  },
];

function test() {
  waitForExplicitFinish();
  runTests(gTests);
}
