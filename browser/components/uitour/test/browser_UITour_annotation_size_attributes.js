






"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let highlight = document.getElementById("UITourHighlightContainer");
let tooltip = document.getElementById("UITourTooltip");

function test() {
  UITourTest();
}

let tests = [
  function test_highlight_size_attributes(done) {
    gContentAPI.showHighlight("appMenu");
    waitForElementToBeVisible(highlight, function moveTheHighlight() {
      gContentAPI.showHighlight("urlbar");
      waitForElementToBeVisible(highlight, function checkPanelAttributes() {
        SimpleTest.executeSoon(() => {
          is(highlight.height, "", "Highlight panel should have no explicit height set");
          is(highlight.width, "", "Highlight panel should have no explicit width set");
          done();
        });
      }, "Highlight should be moved to the urlbar");
    }, "Highlight should be shown after showHighlight() for the appMenu");
  },

  function test_info_size_attributes(done) {
    gContentAPI.showInfo("appMenu", "test title", "test text");
    waitForElementToBeVisible(tooltip, function moveTheTooltip() {
      gContentAPI.showInfo("urlbar", "new title", "new text");
      waitForElementToBeVisible(tooltip, function checkPanelAttributes() {
        SimpleTest.executeSoon(() => {
          is(tooltip.height, "", "Info panel should have no explicit height set");
          is(tooltip.width, "", "Info panel should have no explicit width set");
          done();
        });
      }, "Tooltip should be moved to the urlbar");
    }, "Tooltip should be shown after showInfo() for the appMenu");
  },

];
