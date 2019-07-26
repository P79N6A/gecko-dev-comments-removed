









const TEST_PAGE = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_bug_848731_reset_selection_on_delete.html";

function test() {
  let inspector, toolbox;

  waitForExplicitFinish();

  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(function() {
      openInspector((aInspector, aToolbox) => {
        inspector = aInspector;
        toolbox = aToolbox;
        startTests();
      });
    }, content);
  }, true);
  content.location = TEST_PAGE;

  function startTests() {
    testManuallyDeleteSelectedNode();
  }

  function getContainerForRawNode(rawNode) {
    let front = inspector.markup.walker.frontForRawNode(rawNode);
    let container = inspector.markup.getContainer(front);
    return container;
  }

  
  
  function testManuallyDeleteSelectedNode() {
    info("Deleting a node via the devtools contextual menu");

    
    let div = content.document.getElementById("deleteManually");
    inspector.selection.setNode(div);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, div, "Test node is selected");

      
      let container = getContainerForRawNode(div);

      
      EventUtils.synthesizeMouse(container.tagLine, 2, 2,
        {type: "contextmenu", button: 2}, inspector.panelWin);

      
      let contextMenu = inspector.panelDoc.getElementById("inspectorPopupSet");
      contextMenu.addEventListener("popupshown", function contextShown() {
        contextMenu.removeEventListener("popupshown", contextShown, false);

        
        inspector.panelDoc.getElementById("node-menu-delete").click();

        
        inspector.once("inspector-updated", () => {
          let parent = content.document.getElementById("deleteChildren");
          assertNodeSelectedAndPanelsUpdated(parent, "ul#deleteChildren");
          testAutomaticallyDeleteSelectedNode();
        });
      }, false);
    });
  }

  
  function testAutomaticallyDeleteSelectedNode() {
    info("Deleting a node via javascript");

    
    let div = content.document.getElementById("deleteAutomatically");
    inspector.selection.setNode(div);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, div, "Test node is selected");

      
      let parent = content.document.getElementById("deleteChildren");
      parent.removeChild(div);

      
      inspector.once("inspector-updated", () => {
        assertNodeSelectedAndPanelsUpdated(parent, "ul#deleteChildren");
        testDeleteSelectedNodeContainerFrame();
      });
    });
  }

  
  
  function testDeleteSelectedNodeContainerFrame() {
    info("Deleting an iframe via javascript");

    
    let iframe = content.document.getElementById("deleteIframe");
    let div = iframe.contentDocument.getElementById("deleteInIframe");
    inspector.selection.setNode(div);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, div, "Test node is selected");

      
      let parent = content.document.body;
      parent.removeChild(iframe);

      
      inspector.once("inspector-updated", () => {
        assertNodeSelectedAndPanelsUpdated(parent, "body");
        endTests();
      });
    });
  }

  function endTests() {
    executeSoon(() => {
      toolbox.destroy();
      toolbox = inspector = null;
      gBrowser.removeCurrentTab();
      finish();
    });
  }

  function assertNodeSelectedAndPanelsUpdated(node, crumbLabel) {
    
    is(inspector.selection.nodeFront, getNodeFront(node),
      "The right node is selected");

    
    let breadcrumbs = inspector.panelDoc.getElementById("inspector-breadcrumbs");
    is(breadcrumbs.querySelector("button[checked=true]").textContent, crumbLabel,
      "The right breadcrumb is selected");

    
    ok(!inspector.highlighter.isHidden(), "The highlighter is shown");
  }
}
