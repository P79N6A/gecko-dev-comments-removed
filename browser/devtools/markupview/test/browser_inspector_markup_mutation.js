











function test() {
  waitForExplicitFinish();

  
  let contentTab;
  let doc;

  
  let markup;

  
  let parseTab;
  let parseDoc;

  
  function stripWhitespace(node)
  {
    node.normalize();
    let iter = node.ownerDocument.createNodeIterator(node, NodeFilter.SHOW_TEXT + NodeFilter.SHOW_COMMENT,
      null, false);

    while ((node = iter.nextNode())) {
      node.nodeValue = node.nodeValue.replace(/\s+/g, '');
      if (node.nodeType == Node.TEXT_NODE &&
        !/[^\s]/.exec(node.nodeValue)) {
        node.parentNode.removeChild(node);
      }
    }
  }

  
  function checkMarkup()
  {
    markup.expandAll();

    let contentNode = doc.querySelector("body");
    let panelNode = markup._containers.get(contentNode).elt;
    let parseNode = parseDoc.querySelector("body");

    
    let sel = panelNode.ownerDocument.defaultView.getSelection();
    sel.selectAllChildren(panelNode);

    
    parseNode.outerHTML = sel;
    parseNode = parseDoc.querySelector("body");

    
    
    stripWhitespace(parseNode);

    ok(contentNode.isEqualNode(parseNode), "Markup panel should match document.");
  }

  
  let mutations = [
    
    function() {
      let node1 = doc.querySelector("#node1");
      node1.setAttribute("newattr", "newattrval");
    },
    function() {
      let node1 = doc.querySelector("#node1");
      node1.removeAttribute("newattr");
    },
    function() {
      let node1 = doc.querySelector("#node1");
      node1.textContent = "newtext";
    },
    function() {
      let node2 = doc.querySelector("#node2");
      node2.innerHTML = "<div><span>foo</span></div>";
    },

    function() {
      let node4 = doc.querySelector("#node4");
      while (node4.firstChild) {
        node4.removeChild(node4.firstChild);
      }
    },
    function() {
      
      let node17 = doc.querySelector("#node17");
      let node1 = doc.querySelector("#node2");
      node1.appendChild(node17);
    },

    function() {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      let node18 = doc.querySelector("#node18");
      let node20 = doc.querySelector("#node20");

      let node1 = doc.querySelector("#node1");

      node1.appendChild(node20);
      node20.appendChild(node18);
    },
  ];

  
  parseTab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    parseDoc = content.document;

    
    contentTab = gBrowser.selectedTab = gBrowser.addTab();
    gBrowser.selectedBrowser.addEventListener("load", function onload2() {
      gBrowser.selectedBrowser.removeEventListener("load", onload2, true);
      doc = content.document;
      
      stripWhitespace(doc.documentElement);
      waitForFocus(setupTest, content);
    }, true);
    content.location = "http://mochi.test:8888/browser/browser/devtools/markupview/test/browser_inspector_markup_mutation.html";
  }, true);

  content.location = "data:text/html,<html></html>";

  function setupTest() {
    Services.obs.addObserver(runTests, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    InspectorUI.toggleInspectorUI();
  }

  function runTests() {
    Services.obs.removeObserver(runTests, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
    InspectorUI.currentInspector.once("markuploaded", startTests);
    InspectorUI.select(doc.body, true, true, true);
    InspectorUI.stopInspecting();
    InspectorUI.toggleHTMLPanel();
  }

  function startTests() {
    markup = InspectorUI.currentInspector.markup;
    checkMarkup();
    nextStep(0);
  }

  function nextStep(cursor) {
    if (cursor >= mutations.length) {
      Services.obs.addObserver(finishUp, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
      InspectorUI.closeInspectorUI();
      return;
    }
    mutations[cursor]();
    InspectorUI.currentInspector.once("markupmutation", function() {
      executeSoon(function() {
        checkMarkup();
        nextStep(cursor + 1);
      });
    });
  }

  function finishUp() {
    Services.obs.removeObserver(finishUp, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);
    doc = null;
    gBrowser.removeTab(contentTab);
    gBrowser.removeTab(parseTab);
    finish();
  }
}
