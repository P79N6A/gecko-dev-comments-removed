






function test()
{
  waitForExplicitFinish();

  let doc;
  let node;
  let inspector;

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;
    waitForFocus(setupKeyBindingsTest, content);
  }, true);

  content.location = "data:text/html,<html><head><title>Test for the " +
                     "highlighter keybindings</title></head><body><h1>Hello" +
                     "</h1><p><strong>Greetings, earthlings!</strong> I come" +
                     " in peace.</body></html>";

  function setupKeyBindingsTest()
  {
    openInspector(findAndHighlightNode);
  }

  function findAndHighlightNode(aInspector, aToolbox)
  {
    inspector = aInspector;

    executeSoon(function() {
      inspector.selection.once("new-node", highlightBodyNode);
      
      
      node = doc.querySelector("body");
      let bc = inspector.breadcrumbs;
      bc.nodeHierarchy[bc.currentIndex].button.focus();
      EventUtils.synthesizeKey("VK_RIGHT", { });
    });
  }

  function highlightBodyNode()
  {
    is(inspector.selection.node, node, "selected body element");

    executeSoon(function() {
      inspector.selection.once("new-node", highlightHeaderNode);
      
      node = doc.querySelector("h1");
      EventUtils.synthesizeKey("VK_RIGHT", { });
    });
  }

  function highlightHeaderNode()
  {
    is(inspector.selection.node, node, "selected h1 element");

    executeSoon(function() {
      inspector.selection.once("new-node", highlightParagraphNode);
      
      node = doc.querySelector("p");
      EventUtils.synthesizeKey("VK_DOWN", { });
    });
  }

  function highlightParagraphNode()
  {
    is(inspector.selection.node, node, "selected p element");

    executeSoon(function() {
      inspector.selection.once("new-node", highlightHeaderNodeAgain);
      
      node = doc.querySelector("h1");
      EventUtils.synthesizeKey("VK_UP", { });
    });
  }

  function highlightHeaderNodeAgain()
  {
    is(inspector.selection.node, node, "selected h1 element");

    executeSoon(function() {
      inspector.selection.once("new-node", highlightParentNode);
      
      node = doc.querySelector("body");
      EventUtils.synthesizeKey("VK_LEFT", { });
    });
  }

  function highlightParentNode()
  {
    is(inspector.selection.node, node, "selected body element");
    finishUp();
  }

  function finishUp() {
    doc = node = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}
