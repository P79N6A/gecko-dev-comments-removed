






"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let highlight = document.getElementById("UITourHighlight");
let tooltip = document.getElementById("UITourTooltip");

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  registerCleanupFunction(() => {
    
    gBrowser.getFindBar(gBrowser.selectedTab).close();
  });
  UITourTest();
}

let tests = [
  function test_highlight_move_outside_panel(done) {
    gContentAPI.showInfo("urlbar", "test title", "test text");
    gContentAPI.showHighlight("customize");
    waitForElementToBeVisible(highlight, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      gContentAPI.showHighlight("appMenu");
      waitForPopupAtAnchor(highlight.parentElement, document.getElementById("PanelUI-button"), () => {
        isnot(PanelUI.panel.state, "open",
              "Panel should have closed after the highlight moved elsewhere.");
        ok(tooltip.state == "showing" || tooltip.state == "open", "The info panel should have remained open");
        done();
      }, "Highlight should move to the appMenu button and still be visible");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_highlight_panel_hideMenu(done) {
    gContentAPI.showHighlight("customize");
    gContentAPI.showInfo("search", "test title", "test text");
    waitForElementToBeVisible(highlight, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      gContentAPI.hideMenu("appMenu");
      waitForElementToBeHidden(highlight, function checkPanelIsClosed() {
        isnot(PanelUI.panel.state, "open",
              "Panel still should have closed");
        ok(tooltip.state == "showing" || tooltip.state == "open", "The info panel should have remained open");
        done();
      }, "Highlight should have disappeared when panel closed");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_highlight_panel_click_find(done) {
    gContentAPI.showHighlight("help");
    gContentAPI.showInfo("searchProvider", "test title", "test text");
    waitForElementToBeVisible(highlight, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      let findButton = document.getElementById("find-button");
      EventUtils.synthesizeMouseAtCenter(findButton, {});
      waitForElementToBeHidden(highlight, function checkPanelIsClosed() {
        isnot(PanelUI.panel.state, "open",
              "Panel should have closed when the find bar opened");
        ok(tooltip.state == "showing" || tooltip.state == "open", "The info panel should have remained open");
        done();
      }, "Highlight should have disappeared when panel closed");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_highlight_info_panel_click_find(done) {
    gContentAPI.showHighlight("help");
    gContentAPI.showInfo("customize", "customize me!", "awesome!");
    waitForElementToBeVisible(highlight, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      let findButton = document.getElementById("find-button");
      EventUtils.synthesizeMouseAtCenter(findButton, {});
      waitForElementToBeHidden(highlight, function checkPanelIsClosed() {
        isnot(PanelUI.panel.state, "open",
              "Panel should have closed when the find bar opened");
        waitForElementToBeHidden(tooltip, function checkTooltipIsClosed() {
          isnot(tooltip.state, "open", "The info panel should have closed too");
          done();
        }, "Tooltip should hide with the menu");
      }, "Highlight should have disappeared when panel closed");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_highlight_panel_open_subview(done) {
    gContentAPI.showHighlight("customize");
    gContentAPI.showInfo("backForward", "test title", "test text");
    waitForElementToBeVisible(highlight, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      let helpButton = document.getElementById("PanelUI-help");
      EventUtils.synthesizeMouseAtCenter(helpButton, {});
      waitForElementToBeHidden(highlight, function highlightHidden() {
        is(PanelUI.panel.state, "open",
           "Panel should have stayed open when the subview opened");
        ok(tooltip.state == "showing" || tooltip.state == "open", "The info panel should have remained open");
        PanelUI.hide();
        done();
      }, "Highlight should have disappeared when the subview opened");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_info_panel_open_subview(done) {
    gContentAPI.showHighlight("urlbar");
    gContentAPI.showInfo("customize", "customize me!", "Open a subview");
    waitForElementToBeVisible(tooltip, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      let helpButton = document.getElementById("PanelUI-help");
      EventUtils.synthesizeMouseAtCenter(helpButton, {});
      waitForElementToBeHidden(tooltip, function tooltipHidden() {
        is(PanelUI.panel.state, "open",
           "Panel should have stayed open when the subview opened");
        is(highlight.parentElement.state, "open", "The highlight should have remained open");
        PanelUI.hide();
        done();
      }, "Tooltip should have disappeared when the subview opened");
    }, "Highlight should be shown after showHighlight() for fixed panel items");
  },

  function test_info_move_outside_panel(done) {
    gContentAPI.showInfo("addons", "test title", "test text");
    gContentAPI.showHighlight("urlbar");
    let addonsButton = document.getElementById("add-ons-button");
    waitForPopupAtAnchor(tooltip, addonsButton, function checkPanelIsOpen() {
      isnot(PanelUI.panel.state, "closed", "Panel should have opened");

      
      gContentAPI.showInfo("appMenu", "Cool menu button", "It's three lines");
      waitForPopupAtAnchor(tooltip, document.getElementById("PanelUI-button"), () => {
        isnot(PanelUI.panel.state, "open",
              "Menu should have closed after the highlight moved elsewhere.");
        is(highlight.parentElement.state, "open", "The highlight should have remained visible");
        done();
      }, "Tooltip should move to the appMenu button and still be visible");
    }, "Tooltip should be shown after showInfo() for a panel item");
  },

];
