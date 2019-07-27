


"use strict";




const DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
const PSEUDOS = ["hover", "active", "focus"];

add_task(function*() {
  yield addTab("data:text/html,pseudo-class lock node menu tests");

  info("Creating the test element");
  let div = content.document.createElement("div");
  div.textContent = "test div";
  content.document.body.appendChild(div);

  let {inspector} = yield openInspector();
  yield selectNode("div", inspector);

  info("Getting the inspector ctx menu and opening it");
  let menu = inspector.panelDoc.getElementById("inspector-node-popup");
  yield openMenu(menu);

  yield testMenuItems(div, menu, inspector);
});

function openMenu(menu) {
  let promise = once(menu, "popupshowing", true);
  menu.openPopup();
  return promise;
}

function* testMenuItems(div, menu, inspector) {
  for (let pseudo of PSEUDOS) {
    let menuitem = inspector.panelDoc.getElementById("node-menu-pseudo-" + pseudo);
    ok(menuitem, ":" + pseudo + " menuitem exists");

    
    let onPseudo = inspector.selection.once("pseudoclass");
    let onRefresh = inspector.once("rule-view-refreshed");

    
    let onMutations = once(inspector.walker, "mutations");

    menuitem.doCommand();

    yield onPseudo;
    yield onRefresh;
    yield onMutations;

    let {data: hasLock} = yield executeInContent("Test:HasPseudoClassLock",
                                                 {pseudo: ":" + pseudo},
                                                 {node: div});
    ok(hasLock, "pseudo-class lock has been applied");
  }
}
