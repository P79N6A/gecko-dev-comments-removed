



















"use strict";

const {CssDocsTooltip} = require("devtools/shared/widgets/Tooltip");
const {setBaseCssDocsUrl, MdnDocsWidget} = devtools.require("devtools/shared/widgets/MdnDocsWidget");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});


const MDN_DOCS_TOOLTIP_FRAME = "chrome://browser/content/devtools/mdn-docs-frame.xhtml";









const BASIC_TESTING_PROPERTY = "html-mdn-css-basic-testing.html";

const BASIC_EXPECTED_SUMMARY = "A summary of the property.";
const BASIC_EXPECTED_SYNTAX = "/* The part we want   */\nthis: is-the-part-we-want";

const URI_PARAMS = "?utm_source=mozilla&utm_medium=firefox-inspector&utm_campaign=default";

add_task(function*() {
  setBaseCssDocsUrl(TEST_URI_ROOT);

  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", MDN_DOCS_TOOLTIP_FRAME);
  let widget = new MdnDocsWidget(win.document);

  yield testTheBasics(widget);

  host.destroy();
  gBrowser.removeCurrentTab();
});









function* testTheBasics(widget) {
  info("Test all the basic functionality in the widget");

  info("Get the widget state before docs have loaded");
  let promise = widget.loadCssDocs(BASIC_TESTING_PROPERTY);

  info("Check initial contents before docs have loaded");
  checkTooltipContents(widget.elements, {
    propertyName: BASIC_TESTING_PROPERTY,
    summary: "",
    syntax: ""
  });

  
  ok(widget.elements.info.classList.contains("devtools-throbber"),
     "Throbber is set");

  info("Now let the widget finish loading");
  yield promise;

  info("Check contents after docs have loaded");
  checkTooltipContents(widget.elements, {
    propertyName: BASIC_TESTING_PROPERTY,
    summary: BASIC_EXPECTED_SUMMARY,
    syntax: BASIC_EXPECTED_SYNTAX
  });

  
  ok(!widget.elements.info.classList.contains("devtools-throbber"),
     "Throbber is not set");

  info("Check that MDN link text is correct and onclick behavior is correct");

  let mdnLink = widget.elements.linkToMdn;
  let expectedHref = TEST_URI_ROOT + BASIC_TESTING_PROPERTY + URI_PARAMS;
  is(mdnLink.href, expectedHref, "MDN link href is correct");

  let uri = yield checkLinkClick(mdnLink);
  is(uri, expectedHref, "New tab opened with the expected URI");
}

 


















function checkLinkClick(link) {

  function loadListener(e) {
    let tab = e.target;
    var browser = getBrowser().getBrowserForTab(tab);
    var uri = browser.currentURI.spec;
    
    
    
    if (uri != "about:blank") {
      info("New browser tab has loaded");
      tab.removeEventListener("load", loadListener);
      gBrowser.removeTab(tab);
      info("Resolve promise with new tab URI");
      deferred.resolve(uri);
    }
  }

  function newTabListener(e) {
    gBrowser.tabContainer.removeEventListener("TabOpen", newTabListener);
    var tab = e.target;
    tab.addEventListener("load", loadListener, false);
  }

  let deferred = promise.defer();
  info("Check that clicking the link opens a new tab with the correct URI");
  gBrowser.tabContainer.addEventListener("TabOpen", newTabListener, false);
  info("Click the link to MDN");
  link.click();
  return deferred.promise;
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
