



"use strict";







const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body {',
  '    background: red;',
  '  }',
  '  p {',
  '    color: white;',
  '  }',
  '</style>',
  '<p>Testing the selector highlighter</p>'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + PAGE_CONTENT);

  let {inspector, view} = yield openRuleView();

  
  let HighlighterFront = {
    isShown: false,
    nodeFront: null,
    options: null,
    show: function(nodeFront, options) {
      this.nodeFront = nodeFront;
      this.options = options;
      this.isShown = true;
    },
    hide: function() {
      this.nodeFront = null;
      this.options = null;
      this.isShown = false;
    }
  };

  
  view.selectorHighlighter = HighlighterFront;

  let icon = getRuleViewSelectorHighlighterIcon(view, "body");

  info("Checking that the HighlighterFront's show/hide methods are called");

  info("Clicking once on the body selector highlighter icon");
  yield clickSelectorIcon(icon, view);
  ok(HighlighterFront.isShown, "The highlighter is shown");

  info("Clicking once again on the body selector highlighter icon");
  yield clickSelectorIcon(icon, view);
  ok(!HighlighterFront.isShown, "The highlighter is hidden");

  info("Checking that the right NodeFront reference and options are passed");
  yield selectNode("p", inspector);
  icon = getRuleViewSelectorHighlighterIcon(view, "p");

  yield clickSelectorIcon(icon, view);
  is(HighlighterFront.nodeFront.tagName, "P",
    "The right NodeFront is passed to the highlighter (1)");
  is(HighlighterFront.options.selector, "p",
    "The right selector option is passed to the highlighter (1)");

  yield selectNode("body", inspector);
  icon = getRuleViewSelectorHighlighterIcon(view, "body");
  yield clickSelectorIcon(icon, view);
  is(HighlighterFront.nodeFront.tagName, "BODY",
    "The right NodeFront is passed to the highlighter (2)");
  is(HighlighterFront.options.selector, "body",
    "The right selector option is passed to the highlighter (2)");
});

function* clickSelectorIcon(icon, view) {
  let onToggled = view.once("ruleview-selectorhighlighter-toggled");
  EventUtils.synthesizeMouseAtCenter(icon, {}, view.styleWindow);
  yield onToggled;
}
