



"use strict";



const PAGE_CONTENT = [
  '<style type="text/css">',
  '  #testElement {',
  '    font-family: cursive;',
  '    color: #333;',
  '    padding-left: 70px;',
  '  }',
  '</style>',
  '<div id="testElement">test element</div>'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,font family longhand tooltip test");

  info("Creating the test document");
  content.document.body.innerHTML = PAGE_CONTENT;

  info("Opening the rule view");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test node");
  yield selectNode("#testElement", inspector);

  yield testRuleView(view);

  info("Opening the computed view");
  let {toolbox, inspector, view} = yield openComputedView();

  yield testComputedView(view);
});

function* testRuleView(ruleView) {
  info("Testing font-family tooltips in the rule view");

  let panel = ruleView.previewTooltip.panel;

  
  ok(ruleView.previewTooltip, "Tooltip instance exists");
  ok(panel, "XUL panel exists");

  
  let {valueSpan} = getRuleViewProperty(ruleView, "#testElement", "font-family");

  
  yield assertHoverTooltipOn(ruleView.previewTooltip, valueSpan);

  let description = panel.getElementsByTagName("description")[0];
  is(description.style.fontFamily, "cursive", "Tooltips contains correct font-family style");
}

function* testComputedView(computedView) {
  info("Testing font-family tooltips in the computed view");

  let panel = computedView.tooltip.panel;
  let {valueSpan} = getComputedViewProperty(computedView, "font-family");

  yield assertHoverTooltipOn(computedView.tooltip, valueSpan);

  let description = panel.getElementsByTagName("description")[0];
  is(description.style.fontFamily, "cursive", "Tooltips contains correct font-family style");
}
