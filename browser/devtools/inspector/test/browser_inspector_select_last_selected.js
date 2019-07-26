





function test() {
  let inspector, toolbox;
  let page1 = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_select_last_selected.html";
  let page2 = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_select_last_selected2.html";

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
  content.location = page1;

  function startTests() {
    testSameNodeSelectedOnPageReload();
  }

  function endTests() {
    toolbox.destroy();
    toolbox = inspector = page1 = page2 = null;
    gBrowser.removeCurrentTab();
    finish();
  }

  function testReSelectingAnElement(id, callback) {
    let div = content.document.getElementById(id);
    inspector.selection.setNode(div);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, div);
      inspector.once("markuploaded", () => {
        is(inspector.selection.node.id, id, "Node re-selected after reload");
        callback();
      });
      content.location.reload();
    });
  }

  
  function testSameNodeSelectedOnPageReload()
  {
    
    
    testReSelectingAnElement("id1", () => {
      testReSelectingAnElement("id2", () => {
        testReSelectingAnElement("id3", () => {
          testReSelectingAnElement("id4", testBodySelectedOnNavigate);
        });
      });
    });
  }

  
  
  function testBodySelectedOnNavigate() {
    
    
    inspector.once("markuploaded", () => {
      is(
        inspector.selection.node.tagName.toLowerCase(),
        "body",
        "Node not found, selecting body"
      );
      testSameNodeSelectedOnNavigateAwayAndBack();
    });
    content.location = page2;
  }

  
  
  function testSameNodeSelectedOnNavigateAwayAndBack() {
    
    let id = "id5";
    let div = content.document.getElementById(id);
    inspector.selection.setNode(div);
    inspector.once("inspector-updated", () => {
      is(inspector.selection.node.id, id);
      
      inspector.once("markuploaded", () => {
        
        inspector.once("markuploaded", () => {
          is(inspector.selection.node.id, id, "Node re-selected after navigation");
          endTests();
        });
        content.location = page2;
      });
      content.location = page1;
    });
  }
}
