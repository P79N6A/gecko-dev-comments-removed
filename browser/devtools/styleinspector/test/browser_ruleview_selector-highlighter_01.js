



"use strict";




const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body, p, td {',
  '    background: red;',
  '  }',
  '</style>',
  'Test the selector highlighter'
].join("\n");

let TYPE = "SelectorHighlighter";

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + PAGE_CONTENT);

  let {view: rView} = yield openRuleView();
  let hs = rView.highlighters;

  ok(!hs.highlighters[TYPE], "No highlighter exists in the rule-view (1)");
  ok(!hs.promises[TYPE], "No highlighter is being created in the rule-view (1)");

  info("Faking a mousemove NOT on a selector");
  let {valueSpan} = getRuleViewProperty(rView, "body, p, td", "background");
  hs._onMouseMove({target: valueSpan});
  ok(!hs.highlighters[TYPE], "No highlighter exists in the rule-view (2)");
  ok(!hs.promises[TYPE], "No highlighter is being created in the rule-view (2)");

  info("Faking a mousemove on the body selector");
  let selectorContainer = getRuleViewSelector(rView, "body, p, td");
  
  let bodySelector = selectorContainer.firstElementChild;
  hs._onMouseMove({target: bodySelector});
  ok(hs.promises[TYPE], "The highlighter is being initialized");
  let h = yield hs.promises[TYPE];
  is(h, hs.highlighters[TYPE], "The initialized highlighter is the right one");
});
