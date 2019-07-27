



"use strict";




const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body, p, td {',
  '    background: red;',
  '  }',
  '</style>',
  'Test the selector highlighter'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + PAGE_CONTENT);

  let {view} = yield openRuleView();
  ok(!view.selectorHighlighter, "No selectorhighlighter exist in the rule-view");

  info("Clicking on a selector icon");
  let icon = getRuleViewSelectorHighlighterIcon(view, "body, p, td");

  let onToggled = view.once("ruleview-selectorhighlighter-toggled");
  EventUtils.synthesizeMouseAtCenter(icon, {}, view.styleWindow);
  let isVisible = yield onToggled;

  ok(view.selectorHighlighter, "The selectorhighlighter instance was created");
  ok(isVisible, "The toggle event says the highlighter is visible");
});
