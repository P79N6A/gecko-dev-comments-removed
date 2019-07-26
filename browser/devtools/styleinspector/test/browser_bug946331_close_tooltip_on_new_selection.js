



let contentDoc;
let inspector;
let ruleView;
let computedView;

const PAGE_CONTENT = '<div class="one">el 1</div><div class="two">el 2</div>';

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee, true);
    contentDoc = content.document;
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,rule/computed views tooltip hiding test";
}

function createDocument() {
  contentDoc.body.innerHTML = PAGE_CONTENT;

  openView("ruleview", (aInspector, aRuleView) => {
    inspector = aInspector;
    ruleView = aRuleView;
    openView("computedview", (_, aComputedView) => {
      computedView = aComputedView;
      startTests();
    });
  });
}

function startTests() {
  inspector.selection.setNode(contentDoc.querySelector(".one"));
  inspector.once("inspector-updated", testRuleView);
}

function endTests() {
  contentDoc = inspector = ruleView = computedView = null;
  gBrowser.removeCurrentTab();
  finish();
}

function testRuleView() {
  info("Testing rule view tooltip closes on new selection");

  
  let tooltip = ruleView.previewTooltip;
  tooltip.show();
  tooltip.once("shown", () => {
    
    tooltip.once("hidden", () => {
      ok(true, "Rule view tooltip closed after a new node got selected");
      inspector.once("inspector-updated", testComputedView);
    });
    inspector.selection.setNode(contentDoc.querySelector(".two"));
  });
}

function testComputedView() {
  info("Testing computed view tooltip closes on new selection");

  inspector.sidebar.select("computedview");

  
  let tooltip = computedView.tooltip;
  tooltip.show();
  tooltip.once("shown", () => {
    
    tooltip.once("hidden", () => {
      ok(true, "Computed view tooltip closed after a new node got selected");
      inspector.once("inspector-updated", endTests);
    });
    inspector.selection.setNode(contentDoc.querySelector(".one"));
  });
}
