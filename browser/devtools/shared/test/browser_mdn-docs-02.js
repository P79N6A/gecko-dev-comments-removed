




















"use strict";

const {CssDocsTooltip} = require("devtools/shared/widgets/Tooltip");
const {setBaseCssDocsUrl, MdnDocsWidget} = devtools.require("devtools/shared/widgets/MdnDocsWidget");


const MDN_DOCS_TOOLTIP_FRAME = "chrome://browser/content/devtools/mdn-docs-frame.xhtml";

const BASIC_EXPECTED_SUMMARY = "A summary of the property.";
const BASIC_EXPECTED_SYNTAX = "/* The part we want   */\nthis: is-the-part-we-want";
const ERROR_MESSAGE = "Could not load docs page.";











const SYNTAX_OLD_STYLE = "html-mdn-css-syntax-old-style.html";
const NO_SUMMARY = "html-mdn-css-no-summary.html";
const NO_SYNTAX = "html-mdn-css-no-syntax.html";
const NO_SUMMARY_OR_SYNTAX = "html-mdn-css-no-summary-or-syntax.html";

const TEST_DATA = [{
  desc: "Test a property for which we don't have a page",
  docsPageUrl: "i-dont-exist.html",
  expectedContents: {
    propertyName: "i-dont-exist.html",
    summary: ERROR_MESSAGE,
    syntax: ""
  }
}, {
  desc: "Test a property whose syntax section is specified using an old-style page",
  docsPageUrl: SYNTAX_OLD_STYLE,
  expectedContents: {
    propertyName: SYNTAX_OLD_STYLE,
    summary: BASIC_EXPECTED_SUMMARY,
    syntax: BASIC_EXPECTED_SYNTAX
  }
},  {
  desc: "Test a property whose page doesn't have a summary",
  docsPageUrl: NO_SUMMARY,
  expectedContents: {
    propertyName: NO_SUMMARY,
    summary: "",
    syntax: BASIC_EXPECTED_SYNTAX
  }
}, {
  desc: "Test a property whose page doesn't have a syntax",
  docsPageUrl: NO_SYNTAX,
  expectedContents: {
    propertyName: NO_SYNTAX,
    summary: BASIC_EXPECTED_SUMMARY,
    syntax: ""
  }
}, {
  desc: "Test a property whose page doesn't have a summary or a syntax",
  docsPageUrl: NO_SUMMARY_OR_SYNTAX,
  expectedContents: {
    propertyName: NO_SUMMARY_OR_SYNTAX,
    summary: ERROR_MESSAGE,
    syntax: ""
  }
}
];

add_task(function*() {
  setBaseCssDocsUrl(TEST_URI_ROOT);

  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", MDN_DOCS_TOOLTIP_FRAME);
  let widget = new MdnDocsWidget(win.document);

  for (let {desc, docsPageUrl, expectedContents} of TEST_DATA) {
    info(desc);
    yield widget.loadCssDocs(docsPageUrl);
    checkTooltipContents(widget.elements, expectedContents);
  }
  host.destroy();
  gBrowser.removeCurrentTab();
});

function* testNonExistentPage(widget) {
  info("Test a property for which we don't have a page");
  yield widget.loadCssDocs("i-dont-exist.html");
  checkTooltipContents(widget.elements, {
    propertyName: "i-dont-exist.html",
    summary: ERROR_MESSAGE,
    syntax: ""
  });
}




function checkTooltipContents(doc, expected) {

  is(doc.heading.textContent,
     expected.propertyName,
     "Property name is correct");

  is(doc.summary.textContent,
     expected.summary,
     "Summary is correct");

  is(doc.syntax.textContent,
     expected.syntax,
     "Syntax is correct");
}
