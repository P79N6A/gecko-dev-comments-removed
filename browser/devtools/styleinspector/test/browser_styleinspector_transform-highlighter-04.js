



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

const TYPE = "CssTransformHighlighter";

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8," + PAGE_CONTENT);

  let {view: rView, inspector} = yield openRuleView();
  yield selectNode(".test", inspector);

  let hs = rView.highlighters;

  info("Faking a mousemove on the overriden property");
  let {valueSpan} = getRuleViewProperty(rView, "div", "transform");
  hs._onMouseMove({target: valueSpan});
  ok(!hs.highlighters[TYPE], "No highlighter was created for the overriden property");
  ok(!hs.promises[TYPE], "And no highlighter is being initialized either");

  info("Disabling the applied property");
  let classRuleEditor = getRuleViewRuleEditor(rView, 1);
  let propEditor = classRuleEditor.rule.textProps[0].editor;
  propEditor.enable.click();
  yield classRuleEditor.rule._applyingModifications;

  info("Faking a mousemove on the disabled property");
  ({valueSpan} = getRuleViewProperty(rView, ".test", "transform"));
  hs._onMouseMove({target: valueSpan});
  ok(!hs.highlighters[TYPE], "No highlighter was created for the disabled property");
  ok(!hs.promises[TYPE], "And no highlighter is being initialized either");

  info("Faking a mousemove on the now unoverriden property");
  ({valueSpan} = getRuleViewProperty(rView, "div", "transform"));
  hs._onMouseMove({target: valueSpan});
  ok(hs.promises[TYPE], "The highlighter is being initialized now");
  let h = yield hs.promises[TYPE];
  is(h, hs.highlighters[TYPE], "The initialized highlighter is the right one");
});
