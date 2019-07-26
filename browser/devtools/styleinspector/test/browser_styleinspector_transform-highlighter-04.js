



"use strict";








const PAGE_CONTENT = [
  '<style type="text/css">',
  '  div {',
  '    background: purple;',
  '    width:300px;height:300px;',
  '    transform: rotate(16deg);',
  '  }',
  '  .test {',
  '    transform: skew(25deg);',
  '  }',
  '</style>',
  '<div class="test"></div>'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html," + PAGE_CONTENT);

  let {view: rView, inspector} = yield openRuleView();
  yield selectNode(".test", inspector);

  info("Faking a mousemove on the overriden property");
  let {valueSpan} = getRuleViewProperty(rView, "div", "transform");
  rView._onMouseMove({target: valueSpan});
  ok(!rView.transformHighlighter, "No highlighter was created for the overriden property");
  ok(!rView.transformHighlighterPromise, "And no highlighter is being initialized either");

  info("Disabling the applied property");
  let classRuleEditor = rView.element.children[1]._ruleEditor;
  let propEditor = classRuleEditor.rule.textProps[0].editor;
  propEditor.enable.click();
  yield classRuleEditor.rule._applyingModifications;

  info("Faking a mousemove on the disabled property");
  let {valueSpan} = getRuleViewProperty(rView, ".test", "transform");
  rView._onMouseMove({target: valueSpan});
  ok(!rView.transformHighlighter, "No highlighter was created for the disabled property");
  ok(!rView.transformHighlighterPromise, "And no highlighter is being initialized either");

  info("Faking a mousemove on the now unoverriden property");
  let {valueSpan} = getRuleViewProperty(rView, "div", "transform");
  rView._onMouseMove({target: valueSpan});
  ok(rView.transformHighlighterPromise, "The highlighter is being initialized now");
  let h = yield rView.transformHighlighterPromise;
  is(h, rView.transformHighlighter, "The initialized highlighter is the right one");
});
