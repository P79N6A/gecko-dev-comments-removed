


function test() {
  waitForExplicitFinish();

  
  let contentTab;
  let doc;
  let listElement;

  
  let markup;
  let inspector;

  
  contentTab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;
    waitForFocus(setupTest, content);
  }, true);
  content.location = "http://mochi.test:8888/browser/browser/devtools/markupview/test/browser_inspector_markup_mutation_flashing.html";

  function setupTest() {
    var target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
      inspector = toolbox.getCurrentPanel();
      startTests();
    });
  }

  function startTests() {
    markup = inspector.markup;

    
    listElement = doc.querySelector(".list");

    
    inspector.selection.setNode(listElement.lastElementChild);
    inspector.once("inspector-updated", () => {
      
      
      
      
      let testData = [{
        
        mutate: () => {
          let newLi = doc.createElement("LI");
          newLi.textContent = "new list item";
          listElement.appendChild(newLi);
        },
        assert: () => {
          assertNodeFlashing(listElement.lastElementChild);
        }
      }, {
        
        mutate: () => {
          listElement.removeChild(listElement.lastElementChild);
        },
        assert: () => {
          assertNodeFlashing(listElement);
        }
      }, {
        
        mutate: () => {
          listElement.appendChild(listElement.firstElementChild);
        },
        assert: () => {
          assertNodeFlashing(listElement.lastElementChild);
        }
      }, {
        
        mutate: () => {
          listElement.setAttribute("name-" + Date.now(), "value-" + Date.now());
        },
        assert: () => {
          assertNodeFlashing(listElement);
        }
      }, {
        
        mutate: () => {
          listElement.setAttribute("class", "list value-" + Date.now());
        },
        assert: () => {
          assertNodeFlashing(listElement);
        }
      }, {
        
        mutate: () => {
          listElement.removeAttribute("class");
        },
        assert: () => {
          assertNodeFlashing(listElement);
        }
      }];
      testMutation(testData, 0);
    });
  }

  function testMutation(testData, cursor) {
    if (cursor < testData.length) {
      let {mutate, assert} = testData[cursor];
      mutate();
      inspector.once("markupmutation", () => {
        assert();
        testMutation(testData, cursor + 1);
      });
    } else {
      endTests();
    }
  }

  function endTests() {
    gBrowser.removeTab(contentTab);
    doc = inspector = contentTab = markup = listElement = null;
    finish();
  }

  function assertNodeFlashing(rawNode) {
    let container = getContainerForRawNode(markup, rawNode);

    if(!container) {
      ok(false, "Node not found");
    } else {
      ok(container.highlighter.classList.contains("theme-bg-contrast"),
        "Node is flashing");
    }
  }
}
