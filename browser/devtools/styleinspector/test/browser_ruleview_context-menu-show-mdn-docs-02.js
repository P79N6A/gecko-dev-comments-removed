
















"use strict";

const {setBaseCssDocsUrl} = devtools.require("devtools/shared/widgets/MdnDocsWidget");

const PROPERTYNAME = "color";

const TEST_DOC = `
<html>
  <body>
    <div style="color: red">
      Test "Show MDN Docs" context menu option
    </div>
  </body>
</html>`;

add_task(function* () {

  yield addTab("data:text/html;charset=utf8," + encodeURIComponent(TEST_DOC));

  let {inspector, view} = yield openRuleView();
  yield selectNode("div", inspector);

  yield testShowAndHideMdnTooltip(view);
});

function* testShowMdnTooltip(view) {
  setBaseCssDocsUrl(TEST_URL_ROOT);

  info("Setting the popupNode for the MDN docs tooltip");

  let {nameSpan} = getRuleViewProperty(view, "element", PROPERTYNAME);

  view.doc.popupNode = nameSpan.firstChild;
  view._contextMenuUpdate();

  let cssDocs = view.tooltips.cssDocs;

  info("Showing the MDN docs tooltip");
  let onShown = cssDocs.tooltip.once("shown");
  view.menuitemShowMdnDocs.click();
  yield onShown;
  ok(true, "The MDN docs tooltip was shown");
}








function* testShowAndHideMdnTooltip(view) {
  yield testShowMdnTooltip(view);

  info("Quick check that the tooltip contents are set");
  let cssDocs = view.tooltips.cssDocs;

  let tooltipDocument = cssDocs.tooltip.content.contentDocument;
  let h1 = tooltipDocument.getElementById("property-name");
  is(h1.textContent, PROPERTYNAME, "The MDN docs tooltip h1 is correct");

  info("Simulate pressing the 'Escape' key");
  let onHidden = cssDocs.tooltip.once("hidden");
  EventUtils.sendKey("escape");
  yield onHidden;
  ok(true, "The MDN docs tooltip was hidden on pressing 'escape'");
}




let rootElement = view => (view.element) ? view.element : view.styleDocument;
