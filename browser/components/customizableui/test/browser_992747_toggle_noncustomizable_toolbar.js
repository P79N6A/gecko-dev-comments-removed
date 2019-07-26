



"use strict";

const TOOLBARID = "test-noncustomizable-toolbar-for-toggling";
function test() {
  let tb = document.createElementNS(kNSXUL, "toolbar");
  tb.id = TOOLBARID;
  gNavToolbox.appendChild(tb);
  try {
    CustomizableUI.setToolbarVisibility(TOOLBARID, false);
  } catch (ex) {
    ok(false, "Should not throw exceptions trying to set toolbar visibility.");
  }
  is(tb.getAttribute("collapsed"), "true", "Toolbar should be collapsed");
  try {
    CustomizableUI.setToolbarVisibility(TOOLBARID, true);
  } catch (ex) {
    ok(false, "Should not throw exceptions trying to set toolbar visibility.");
  }
  is(tb.getAttribute("collapsed"), "false", "Toolbar should be uncollapsed");
  tb.remove();
};

