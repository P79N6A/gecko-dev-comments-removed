


"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_inspector_menu.html";

add_task(function* () {
  let { inspector, toolbox, testActor } = yield openInspectorForURL(TEST_URL);

  yield testShowDOMProperties();
  yield testDeleteNode();
  yield testDeleteRootNode();

  function* testShowDOMProperties() {
    info("Testing 'Show DOM Properties' menu item.");
    let showDOMPropertiesNode = inspector.panelDoc.getElementById("node-menu-showdomproperties");
    ok(showDOMPropertiesNode, "the popup menu has a show dom properties item");

    let consoleOpened = toolbox.once("webconsole-ready");

    info("Triggering 'Show DOM Properties' and waiting for inspector open");
    dispatchCommandEvent(showDOMPropertiesNode);
    yield consoleOpened;

    let webconsoleUI = toolbox.getPanel("webconsole").hud.ui;
    let messagesAdded = webconsoleUI.once("new-messages");
    yield messagesAdded;

    info("Checking if 'inspect($0)' was evaluated");
    ok(webconsoleUI.jsterm.history[0] === 'inspect($0)');

    yield toolbox.toggleSplitConsole();
  }

  function* testDeleteNode() {
    info("Testing 'Delete Node' menu item for normal elements.");

    yield selectNode("#delete", inspector);
    let deleteNode = inspector.panelDoc.getElementById("node-menu-delete");
    ok(deleteNode, "the popup menu has a delete menu item");

    let updated = inspector.once("inspector-updated");

    info("Triggering 'Delete Node' and waiting for inspector to update");
    dispatchCommandEvent(deleteNode);
    yield updated;

    ok(!(yield testActor.hasNode("#delete")), "Node deleted");
  }

  function* testDeleteRootNode() {
    info("Testing 'Delete Node' menu item does not delete root node.");
    yield selectNode("html", inspector);

    let deleteNode = inspector.panelDoc.getElementById("node-menu-delete");
    dispatchCommandEvent(deleteNode);

    let deferred = promise.defer();
    executeSoon(deferred.resolve);
    yield deferred.promise;

    ok((yield testActor.eval("!!content.document.documentElement")),
       "Document element still alive.");
  }

  function dispatchCommandEvent(node) {
    info("Dispatching command event on " + node);
    let commandEvent = document.createEvent("XULCommandEvent");
    commandEvent.initCommandEvent("command", true, true, window, 0, false, false,
                                  false, false, null);
    node.dispatchEvent(commandEvent);
  }
});
