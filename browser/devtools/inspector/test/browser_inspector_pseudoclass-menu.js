


"use strict";




const TEST_URI = "data:text/html;charset=UTF-8," +
                 "pseudo-class lock node menu tests" +
                 "<div>test div</div>";
const PSEUDOS = ["hover", "active", "focus"];

add_task(function*() {
  let {inspector, testActor} = yield openInspectorForURL(TEST_URI);
  yield selectNode("div", inspector);

  info("Getting the inspector ctx menu and opening it");
  let menu = inspector.panelDoc.getElementById("inspector-node-popup");
  yield openMenu(menu);

  yield testMenuItems(testActor, menu, inspector);

  menu.hidePopup();
});

function openMenu(menu) {
  let promise = once(menu, "popupshowing", true);
  menu.openPopup();
  return promise;
}

function* testMenuItems(testActor, menu, inspector) {
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

    let hasLock = yield testActor.hasPseudoClassLock("div", ":" + pseudo);
    ok(hasLock, "pseudo-class lock has been applied");
  }
}
