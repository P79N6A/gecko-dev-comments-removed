



"use strict";



const PAGE_CONTENT = [
  '<style type="text/css">',
  '  #testElement {',
  '    font: italic bold .8em/1.2 Arial;',
  '  }',
  '</style>',
  '<div id="testElement">test element</div>'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,font family shorthand tooltip test");

  info("Creating the test document");
  content.document.body.innerHTML = PAGE_CONTENT;

  info("Opening the rule view");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test node");
  yield selectNode("#testElement", inspector);

  yield testRuleView(view, inspector.selection.nodeFront);
});

function* testRuleView(ruleView, nodeFront) {
  info("Testing font-family tooltips in the rule view");

  let tooltip = ruleView.tooltips.previewTooltip;
  let panel = tooltip.panel;

  
  ok(tooltip, "Tooltip instance exists");
  ok(panel, "XUL panel exists");

  
  let propertyList = ruleView.element.querySelectorAll(".ruleview-propertylist");
  let fontExpander = propertyList[1].querySelectorAll(".ruleview-expander")[0];
  fontExpander.click();

  let rule = getRuleViewRule(ruleView, "#testElement");
  let valueSpan = rule.querySelector(".ruleview-computed .ruleview-propertyvalue");

  
  yield assertHoverTooltipOn(tooltip, valueSpan);

  let images = panel.getElementsByTagName("image");
  is(images.length, 1, "Tooltip contains an image");
  ok(images[0].getAttribute("src").startsWith("data:"), "Tooltip contains a data-uri image as expected");

  let dataURL = yield getFontFamilyDataURL(valueSpan.textContent, nodeFront);
  is(images[0].getAttribute("src"), dataURL, "Tooltip contains the correct data-uri image");
}
