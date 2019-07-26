





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
    executeSoon(() => {
      toolbox.destroy();
      toolbox = inspector = page1 = page2 = null;
      gBrowser.removeCurrentTab();
      finish();
    });
  }

  function loadPageAnd(page, callback) {
    inspector.once("markuploaded", () => {
      executeSoon(callback);
    });

    if (page) {
      content.location = page;
    } else {
      content.location.reload();
    }
  }

  function reloadAndReselect(id, callback) {
    let div = content.document.getElementById(id);

    inspector.once("inspector-updated", () => {
      is(inspector.selection.node, div);

      loadPageAnd(false, () => {
        is(inspector.selection.node.id, id, "Node re-selected after reload");
        executeSoon(callback);
      });
    });

    inspector.selection.setNode(div);
  }

  
  function testSameNodeSelectedOnPageReload()
  {
    
    
    reloadAndReselect("id1", () => {
      reloadAndReselect("id2", () => {
        reloadAndReselect("id3", () => {
          reloadAndReselect("id4", testBodySelectedOnNavigate);
        });
      });
    });
  }

  
  
  function testBodySelectedOnNavigate() {
    
    
    loadPageAnd(page2, () => {
      executeSoon(() => {
        is(
          inspector.selection.node.tagName.toLowerCase(),
          "body",
          "Node not found, body selected"
        );
        executeSoon(testSameNodeSelectedOnNavigateAwayAndBack);
      });
    });
  }

  
  
  function testSameNodeSelectedOnNavigateAwayAndBack() {
    
    let id = "id5";
    let div = content.document.getElementById(id);

    inspector.once("inspector-updated", () => {
      is(inspector.selection.node.id, id);

      executeSoon(() => {
        
        loadPageAnd(page1, () => {

          executeSoon(() => {
            
            loadPageAnd(page2, () => {
              is(inspector.selection.node.id, id, "Node re-selected after navigation");
              executeSoon(endTests);
            });
          });
        });
      });
    });

    inspector.selection.setNode(div);
  }
}
