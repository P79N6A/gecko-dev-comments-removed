













"use strict";

const { PrefObserver } = devtools.require("devtools/styleeditor/utils");
const PREF_ENABLE_MDN_DOCS_TOOLTIP = "devtools.inspector.mdnDocsTooltip.enabled";
const PROPERTY_NAME_CLASS = "ruleview-propertyname";

const TEST_DOC = `
<html>
  <body>
    <div style="color: red">
      Test the pref to enable/disable the "Show MDN Docs" context menu option
    </div>
  </body>
</html>`;

add_task(function* () {
  info("Ensure the pref is true to begin with");
  let initial = Services.prefs.getBoolPref(PREF_ENABLE_MDN_DOCS_TOOLTIP);
  if (initial != true) {
    setBooleanPref(PREF_ENABLE_MDN_DOCS_TOOLTIP, true);
  }

  yield addTab("data:text/html;charset=utf8," + encodeURIComponent(TEST_DOC));

  let {inspector, view} = yield openRuleView();
  yield selectNode("div", inspector);
  yield testMdnContextMenuItemVisibility(view, true);

  yield setBooleanPref(PREF_ENABLE_MDN_DOCS_TOOLTIP, false);
  yield testMdnContextMenuItemVisibility(view, false);

  info("Close the Inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  ({inspector, view} = yield openRuleView());
  yield selectNode("div", inspector);
  yield testMdnContextMenuItemVisibility(view, false);

  yield setBooleanPref(PREF_ENABLE_MDN_DOCS_TOOLTIP, true);
  yield testMdnContextMenuItemVisibility(view, true);

  info("Ensure the pref is reset to its initial value");
  let eventual = Services.prefs.getBoolPref(PREF_ENABLE_MDN_DOCS_TOOLTIP);
  if (eventual != initial) {
    setBooleanPref(PREF_ENABLE_MDN_DOCS_TOOLTIP, initial);
  }
});














function* setBooleanPref(pref, state) {
  let oncePrefChanged = promise.defer();
  let prefObserver = new PrefObserver("devtools.");
  prefObserver.on(pref, oncePrefChanged.resolve);

  info("Set the pref " + pref + " to: " + state);
  Services.prefs.setBoolPref(pref, state);

  info("Wait for prefObserver to call back so the UI can update");
  yield oncePrefChanged.promise;
  prefObserver.off(pref, oncePrefChanged.resolve);
}








function* testMdnContextMenuItemVisibility(view, shouldBeVisible) {
  let message = shouldBeVisible? "shown": "hidden";
  info("Test that MDN context menu item is " + message);

  info("Set a CSS property name as popupNode");
  let root = rootElement(view);
  let node = root.querySelector("." + PROPERTY_NAME_CLASS).firstChild;
  view.styleDocument.popupNode = node;

  info("Update context menu state");
  view._contextmenu._updateMenuItems();
  let isVisible = !view._contextmenu.menuitemShowMdnDocs.hidden;
  is(isVisible, shouldBeVisible,
     "The MDN context menu item is " + message);
}




let rootElement = view => (view.element) ? view.element : view.styleDocument;
