



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_links.html";
const STRINGS = Services.strings
  .createBundle("chrome://browser/locale/devtools/inspector.properties");
const TOOLBOX_STRINGS = Services.strings
  .createBundle("chrome://browser/locale/devtools/toolbox.properties");










const TEST_DATA = [{
  selector: "link",
  attributeName: "href",
  popupNodeSelector: ".link",
  isLinkFollowItemVisible: true,
  isLinkCopyItemVisible: true,
  linkFollowItemLabel: TOOLBOX_STRINGS.GetStringFromName("toolbox.viewCssSourceInStyleEditor.label"),
  linkCopyItemLabel: STRINGS.GetStringFromName("inspector.menu.copyUrlToClipboard.label")
}, {
  selector: "link[rel=icon]",
  attributeName: "href",
  popupNodeSelector: ".link",
  isLinkFollowItemVisible: true,
  isLinkCopyItemVisible: true,
  linkFollowItemLabel: STRINGS.GetStringFromName("inspector.menu.openUrlInNewTab.label"),
  linkCopyItemLabel: STRINGS.GetStringFromName("inspector.menu.copyUrlToClipboard.label")
}, {
  selector: "link",
  attributeName: "rel",
  popupNodeSelector: ".attr-value",
  isLinkFollowItemVisible: false,
  isLinkCopyItemVisible: false
}, {
  selector: "output",
  attributeName: "for",
  popupNodeSelector: ".link",
  isLinkFollowItemVisible: true,
  isLinkCopyItemVisible: false,
  linkFollowItemLabel: STRINGS.formatStringFromName(
    "inspector.menu.selectElement.label", ["name"], 1)
}, {
  selector: "script",
  attributeName: "src",
  popupNodeSelector: ".link",
  isLinkFollowItemVisible: true,
  isLinkCopyItemVisible: true,
  linkFollowItemLabel: TOOLBOX_STRINGS.GetStringFromName("toolbox.viewJsSourceInDebugger.label"),
  linkCopyItemLabel: STRINGS.GetStringFromName("inspector.menu.copyUrlToClipboard.label")
}, {
  selector: "p[for]",
  attributeName: "for",
  popupNodeSelector: ".attr-value",
  isLinkFollowItemVisible: false,
  isLinkCopyItemVisible: false
}];

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let linkFollow = inspector.panelDoc.getElementById("node-menu-link-follow");
  let linkCopy = inspector.panelDoc.getElementById("node-menu-link-copy");

  for (let test of TEST_DATA) {
    info("Selecting test node " + test.selector);
    yield selectNode(test.selector, inspector);

    info("Finding the popupNode to anchor the context-menu to");
    let {editor} = yield getContainerForSelector(test.selector, inspector);
    let popupNode = editor.attrElements.get(test.attributeName)
                    .querySelector(test.popupNodeSelector);
    ok(popupNode, "Found the popupNode in attribute " + test.attributeName);

    info("Simulating a context click on the popupNode");
    contextMenuClick(popupNode);

    
    
    
    
    yield inspector.target.actorHasMethod("inspector", "resolveRelativeURL");

    is(linkFollow.hasAttribute("hidden"), !test.isLinkFollowItemVisible,
      "The follow-link item display is correct");
    is(linkCopy.hasAttribute("hidden"), !test.isLinkCopyItemVisible,
      "The copy-link item display is correct");

    if (test.isLinkFollowItemVisible) {
      is(linkFollow.getAttribute("label"), test.linkFollowItemLabel,
        "the follow-link label is correct");
    }
    if (test.isLinkCopyItemVisible) {
      is(linkCopy.getAttribute("label"), test.linkCopyItemLabel,
        "the copy-link label is correct");
    }
  }
});
