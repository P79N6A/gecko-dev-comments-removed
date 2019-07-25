
















function test() {
  let tempScope = {}
  Cu.import("resource:///modules/devtools/CssRuleView.jsm", tempScope);
  let inplaceEditor = tempScope._getInplaceEditorForSpan;

  waitForExplicitFinish();

  
  let doc;

  
  let markup;

  


  function editField(aField, aValue)
  {
    aField.focus();
    EventUtils.sendKey("return");
    let input = inplaceEditor(aField).input;
    input.value = aValue;
    input.blur();
  }

  function assertAttributes(aElement, aAttributes)
  {
    let attrs = Object.getOwnPropertyNames(aAttributes);
    is(aElement.attributes.length, attrs.length, "Node has the correct number of attributes");
    for (let attr of attrs) {
      is(aElement.getAttribute(attr), aAttributes[attr], "Node has the correct " + attr + " attribute.");
    }
  }

  
  let edits = [
    
    {
      before: function() {
        assertAttributes(doc.querySelector("#node1"), {
          id: "node1",
          class: "node1"
        });
      },
      execute: function() {
        let editor = markup.getContainer(doc.querySelector("#node1")).editor;
        let attr = editor.attrs["class"].querySelector(".editable");
        editField(attr, 'class="changednode1"');
      },
      after: function() {
        assertAttributes(doc.querySelector("#node1"), {
          id: "node1",
          class: "changednode1"
        });
      }
    },

    
    {
      before: function() {
        assertAttributes(doc.querySelector("#node4"), {
          id: "node4",
          class: "node4"
        });
      },
      execute: function() {
        let editor = markup.getContainer(doc.querySelector("#node4")).editor;
        let attr = editor.attrs["class"].querySelector(".editable");
        editField(attr, '');
      },
      after: function() {
        assertAttributes(doc.querySelector("#node4"), {
          id: "node4",
        });
      }
    },

    
    {
      before: function() {
        assertAttributes(doc.querySelector("#node14"), {
          id: "node14",
        });
      },
      execute: function() {
        let editor = markup.getContainer(doc.querySelector("#node14")).editor;
        let attr = editor.newAttr;
        editField(attr, 'class="newclass" style="color:green"');
      },
      after: function() {
        assertAttributes(doc.querySelector("#node14"), {
          id: "node14",
          class: "newclass",
          style: "color:green"
        });
      }
    },

    
    {
      before: function() {
        assertAttributes(doc.querySelector("#node18"), {
          id: "node18",
        });
      },
      execute: function() {
        let editor = markup.getContainer(doc.querySelector("#node18")).editor;
        let attr = editor.attrs["id"].querySelector(".editable");
        editField(attr, attr.textContent + ' class="newclass" style="color:green"');
      },
      after: function() {
        assertAttributes(doc.querySelector("#node18"), {
          id: "node18",
          class: "newclass",
          style: "color:green"
        });
      }
    },

    
    {
      before: function() {
        ok(!!doc.querySelector("#node18"), "Node 18 should exist.");
      },
      execute: function() {
        markup.selectNode(doc.querySelector("#node18"));
        EventUtils.sendKey("delete");
      },
      after: function() {
        ok(!doc.querySelector("#node18"), "Node 18 should not exist.")
      }
    },

    
    {
      before: function() {
        let node = doc.querySelector('.node6').firstChild;
        is(node.nodeValue, "line6", "Text should be unchanged");
      },
      execute: function() {
        let node = doc.querySelector('.node6').firstChild;
        let editor = markup.getContainer(node).editor;
        let field = editor.elt.querySelector("pre");
        editField(field, "New text");
      },
      after: function() {
        let node = doc.querySelector('.node6').firstChild;
        is(node.nodeValue, "New text", "Text should be changed.");
      },
    }
  ];

  
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;
    waitForFocus(setupTest, content);
  }, true);
  content.location = "http://mochi.test:8888/browser/browser/devtools/markupview/test/browser_inspector_markup_edit.html";

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
    let startNode = doc.documentElement.cloneNode();
    markup = InspectorUI.currentInspector.markup;
    markup.expandAll();
    for (let step of edits) {
      step.before();
      step.execute();
      step.after();
      ok(markup.undo.canUndo(), "Should be able to undo.");
      markup.undo.undo();
      step.before();
      ok(markup.undo.canRedo(), "Should be able to redo.");
      markup.undo.redo();
      step.after();
    }
    while (markup.undo.canUndo()) {
      markup.undo.undo();
    }
    
    
    ok(doc.documentElement.isEqualNode(startNode), "Clearing the undo stack should leave us where we started.");
    Services.obs.addObserver(finishUp, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
    InspectorUI.closeInspectorUI();
  }

  function finishUp() {
    Services.obs.removeObserver(finishUp, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);
    doc = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}
