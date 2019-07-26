



"use strict";




const TEST_URI = TEST_URL_ROOT + "browser_bug722196_identify_media_queries.html";

let test = asyncTest(function*() {
  yield addTab(TEST_URI);
  let {inspector, view} = yield openRuleView();

  yield selectNode("div", inspector);

  let elementStyle = view._elementStyle;

  let _strings = Services.strings
    .createBundle("chrome://global/locale/devtools/styleinspector.properties");

  let inline = _strings.GetStringFromName("rule.sourceInline");

  is(elementStyle.rules.length, 3, "Should have 3 rules.");
  is(elementStyle.rules[0].title, inline, "check rule 0 title");
  is(elementStyle.rules[1].title, inline +
    ":15 @media screen and (min-width: 1px)", "check rule 1 title");
  is(elementStyle.rules[2].title, inline + ":8", "check rule 2 title");
});

