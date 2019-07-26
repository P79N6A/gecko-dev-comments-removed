




 'use strict';

let { Cc, Ci } = require("chrome");

const { Loader } = require('sdk/test/loader');
const timer = require("sdk/timers");


const ITEM_CLASS = "addon-context-menu-item";
const SEPARATOR_CLASS = "addon-context-menu-separator";
const OVERFLOW_THRESH_DEFAULT = 10;
const OVERFLOW_THRESH_PREF =
  "extensions.addon-sdk.context-menu.overflowThreshold";
const OVERFLOW_MENU_CLASS = "addon-content-menu-overflow-menu";
const OVERFLOW_POPUP_CLASS = "addon-content-menu-overflow-popup";

const TEST_DOC_URL = module.uri.replace(/\.js$/, ".html");



exports.testSeparatorPosition = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  
  let oldSeparator = test.contextMenuPopup.ownerDocument.createElement("menuseparator");
  oldSeparator.id = "jetpack-context-menu-separator";
  test.contextMenuPopup.appendChild(oldSeparator);

  
  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    test.assertEqual(test.contextMenuSeparator.nextSibling.nextSibling, oldSeparator,
                     "New separator should appear before the old one");
    test.contextMenuPopup.removeChild(oldSeparator);
    test.done();
  });
};



exports.testConstructDestroy = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  
  let item = new loader.cm.Item({ label: "item" });
  test.assertEqual(item.parentMenu, loader.cm.contentContextMenu,
                   "item's parent menu should be correct");

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item], [], []);
    popup.hidePopup();

    
    item.destroy();
    item.destroy();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item], [], [item]);
      test.done();
    });
  });
};



exports.testDestroyTwice = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });
  item.destroy();
  item.destroy();

  test.pass("Destroying an item twice should not cause an error.");
  test.done();
};




exports.testSelectorContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item",
    context: loader.cm.SelectorContext("img")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};





exports.testSelectorAncestorContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item",
    context: loader.cm.SelectorContext("a[href]")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("span-link"), function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};





exports.testSelectorContextNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item",
    context: loader.cm.SelectorContext("img")
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    test.done();
  });
};




exports.testPageContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({
      label: "item 0"
    }),
    new loader.cm.Item({
      label: "item 1",
      context: undefined
    }),
    new loader.cm.Item({
      label: "item 2",
      context: loader.cm.PageContext()
    }),
    new loader.cm.Item({
      label: "item 3",
      context: [loader.cm.PageContext()]
    })
  ];

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};




exports.testPageContextNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({
      label: "item 0"
    }),
    new loader.cm.Item({
      label: "item 1",
      context: undefined
    }),
    new loader.cm.Item({
      label: "item 2",
      context: loader.cm.PageContext()
    }),
    new loader.cm.Item({
      label: "item 3",
      context: [loader.cm.PageContext()]
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};



exports.testSelectionContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    window.getSelection().selectAllChildren(doc.body);
    test.showMenu(null, function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};




exports.testSelectionContextMatchInTextField = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    let textfield = doc.getElementById("textfield");
    textfield.setSelectionRange(0, textfield.value.length);
    test.showMenu(textfield, function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};




exports.testSelectionContextNoMatchInTextField = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    let textfield = doc.getElementById("textfield");
    textfield.setSelectionRange(0, 0);
    test.showMenu(textfield, function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};




exports.testSelectionContextNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    test.done();
  });
};




exports.testSelectionContextInNewTab = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    let link = doc.getElementById("targetlink");
    link.click();

    test.delayedEventListener(this.tabBrowser, "load", function () {
      let browser = test.tabBrowser.selectedBrowser;
      let window = browser.contentWindow;
      let doc = browser.contentDocument;
      window.getSelection().selectAllChildren(doc.body);

      test.showMenu(null, function (popup) {
        test.checkMenu([item], [], []);
        popup.hidePopup();

        test.tabBrowser.removeTab(test.tabBrowser.selectedTab);
        test.tabBrowser.selectedTab = test.tab;

        test.showMenu(null, function (popup) {
          test.checkMenu([item], [item], []);
          test.done();
        });
      });
    }, true);
  });
};



exports.testSelectionContextButtonMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    window.getSelection().selectAllChildren(doc.body);
    let button = doc.getElementById("button");
    test.showMenu(button, function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};



exports.testSelectionContextButtonNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    let button = doc.getElementById("button");
    test.showMenu(button, function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};



exports.testURLContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    loader.cm.Item({
      label: "item 0",
      context: loader.cm.URLContext(TEST_DOC_URL)
    }),
    loader.cm.Item({
      label: "item 1",
      context: loader.cm.URLContext([TEST_DOC_URL, "*.bogus.com"])
    }),
    loader.cm.Item({
      label: "item 2",
      context: loader.cm.URLContext([new RegExp(".*\\.html")])
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testURLContextNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    loader.cm.Item({
      label: "item 0",
      context: loader.cm.URLContext("*.bogus.com")
    }),
    loader.cm.Item({
      label: "item 1",
      context: loader.cm.URLContext(["*.bogus.com", "*.gnarly.com"])
    }),
    loader.cm.Item({
      label: "item 2",
      context: loader.cm.URLContext([new RegExp(".*\\.js")])
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};





exports.testURLContextRemove = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let shouldBeEvaled = false;
  let context = loader.cm.URLContext("*.bogus.com");
  let item = loader.cm.Item({
    label: "item",
    context: context,
    contentScript: 'self.postMessage("ok"); self.on("context", function () true);',
    onMessage: function (msg) {
      test.assert(shouldBeEvaled,
                  "content script should be evaluated when expected");
      test.assertEqual(msg, "ok", "Should have received the right message");
      shouldBeEvaled = false;
    }
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu([item], [item], []);

      item.context.remove(context);

      shouldBeEvaled = true;

      test.hideMenu(function () {
        test.showMenu(null, function (popup) {
          test.checkMenu([item], [], []);

          test.assert(!shouldBeEvaled,
                      "content script should have been evaluated");

          test.hideMenu(function () {
            
            test.showMenu(null, function (popup) {
              test.checkMenu([item], [], []);
              test.done();
            });
          });
        });
      });
    });
  });
};



exports.testPageReload = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "Item",
    contentScript: "var doc = document; self.on('context', function(node) doc.body.getAttribute('showItem') == 'true');"
  });

  test.withTestDoc(function (window, doc) {
    
    doc.body.setAttribute("showItem", "true");

    test.showMenu(null, function (popup) {
      
      test.checkMenu([item], [], []);
      test.hideMenu(function() {
        let browser = this.tabBrowser.getBrowserForTab(this.tab)
        test.delayedEventListener(browser, "load", function() {
          test.delayedEventListener(browser, "load", function() {
            window = browser.contentWindow;
            doc = window.document;

            
            doc.body.setAttribute("showItem", "false");

            test.showMenu(null, function (popup) {
              
              
              
              test.checkMenu([item], [item], []);

              test.done();
            });
          }, true);
          browser.loadURI(TEST_DOC_URL, null, null);
        }, true);
        
        
        browser.loadURI("about:blank", null, null);
      });
    });
  });
};









































exports.testContentContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () true);'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};




exports.testContentContextNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () false);'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    test.done();
  });
};




exports.testContentContextUndefined = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () {});'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    test.done();
  });
};




exports.testContentContextEmptyString = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    test.assertEqual(item.label, "item", "Label should still be correct");
    test.done();
  });
};




exports.testMultipleContentContextMatch1 = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () true); ' +
                   'self.on("context", function () false);',
    onMessage: function() {
      test.fail("Should not have called the second context listener");
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};




exports.testMultipleContentContextMatch2 = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () false); ' +
                   'self.on("context", function () true);'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};




exports.testMultipleContentContextString1 = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "new label"); ' +
                   'self.on("context", function () false);'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.assertEqual(item.label, "new label", "Label should have changed");
    test.done();
  });
};




exports.testMultipleContentContextString2 = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () false); ' +
                   'self.on("context", function () "new label");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.assertEqual(item.label, "new label", "Label should have changed");
    test.done();
  });
};



exports.testMultipleContentContextString3 = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "new label 1"); ' +
                   'self.on("context", function () "new label 2");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.assertEqual(item.label, "new label 1", "Label should have changed");
    test.done();
  });
};




exports.testContentContextMatchActiveElement = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({
      label: "item 1",
      contentScript: 'self.on("context", function () true);'
    }),
    new loader.cm.Item({
      label: "item 2",
      context: undefined,
      contentScript: 'self.on("context", function () true);'
    }),
    
    new loader.cm.Item({
      label: "item 3",
      context: loader.cm.PageContext(),
      contentScript: 'self.on("context", function () true);'
    }),
    new loader.cm.Item({
      label: "item 4",
      context: [loader.cm.PageContext()],
      contentScript: 'self.on("context", function () true);'
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu(items, [items[2], items[3]], []);
      test.done();
    });
  });
};




exports.testContentContextNoMatchActiveElement = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({
      label: "item 1",
      contentScript: 'self.on("context", function () false);'
    }),
    new loader.cm.Item({
      label: "item 2",
      context: undefined,
      contentScript: 'self.on("context", function () false);'
    }),
    
    new loader.cm.Item({
      label: "item 3",
      context: loader.cm.PageContext(),
      contentScript: 'self.on("context", function () false);'
    }),
    new loader.cm.Item({
      label: "item 4",
      context: [loader.cm.PageContext()],
      contentScript: 'self.on("context", function () false);'
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




exports.testContentContextNoMatchActiveElement = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({
      label: "item 1",
      contentScript: 'self.on("context", function () {});'
    }),
    new loader.cm.Item({
      label: "item 2",
      context: undefined,
      contentScript: 'self.on("context", function () {});'
    }),
    
    new loader.cm.Item({
      label: "item 3",
      context: loader.cm.PageContext(),
      contentScript: 'self.on("context", function () {});'
    }),
    new loader.cm.Item({
      label: "item 4",
      context: [loader.cm.PageContext()],
      contentScript: 'self.on("context", function () {});'
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




exports.testContentContextMatchString = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "first label",
    contentScript: 'self.on("context", function () "second label");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.assertEqual(item.label, "second label",
                     "item's label should be updated");
    test.done();
  });
};



exports.testContentScriptFile = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  
  test.assertRaises(function() {
      new loader.cm.Item({
        label: "item",
        contentScriptFile: "http://mozilla.com/context-menu.js"
      });
    },
    "The 'contentScriptFile' option must be a local file URL " +
    "or an array of local file URLs.",
    "Item throws when contentScriptFile is a remote URL");

  
  let item = new loader.cm.Item({
    label: "item",
    contentScriptFile: require("sdk/self").data.url("test-context-menu.js")
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};



exports.testContentContextArgs = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();
  let callbacks = 0;

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function (node) {' +
                   '  self.postMessage(node.tagName);' +
                   '  return false;' +
                   '});',
    onMessage: function (tagName) {
      test.assertEqual(tagName, "HTML", "node should be an HTML element");
      if (++callbacks == 2) test.done();
    }
  });

  test.showMenu(null, function () {
    if (++callbacks == 2) test.done();
  });
};



exports.testMultipleContexts = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    context: [loader.cm.SelectorContext("a[href]"), loader.cm.PageContext()],
    contentScript: 'self.on("context", function () self.postMessage());',
    onMessage: function () {
      test.fail("Context listener should not be called");
    }
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("span-link"), function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};


exports.testRemoveContext = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let ctxt = loader.cm.SelectorContext("img");
  let item = new loader.cm.Item({
    label: "item",
    context: ctxt
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {

      
      test.checkMenu([item], [], []);
      popup.hidePopup();

      
      item.context.remove(ctxt);
      test.showMenu(doc.getElementById("image"), function (popup) {
        test.checkMenu([item], [item], []);
        test.done();
      });
    });
  });
};



exports.testOverflow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [];
  for (let i = 0; i < OVERFLOW_THRESH_DEFAULT + 1; i++) {
    let item = new loader.cm.Item({ label: "item " + i });
    items.push(item);
  }

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};



exports.testUnload = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item], [], []);
    popup.hidePopup();

    
    loader.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item], [], [item]);
      test.done();
    });
  });
};




exports.testMultipleModulesAdd = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  
  let item0 = new loader0.cm.Item({ label: "item 0" });
  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [], []);
    popup.hidePopup();

    
    loader0.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item1], [], [item0]);
      popup.hidePopup();

      
      loader1.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item1], [], [item0, item1]);
        test.done();
      });
    });
  });
};



exports.testMultipleModulesAddOverflow = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  
  let items0 = [];
  for (let i = 0; i < OVERFLOW_THRESH_DEFAULT; i++) {
    let item = new loader0.cm.Item({ label: "item 0 " + i });
    items0.push(item);
  }

  
  let item1 = new loader1.cm.Item({ label: "item 1" });

  let allItems = items0.concat(item1);

  test.showMenu(null, function (popup) {

    
    test.checkMenu(allItems, [], []);
    popup.hidePopup();

    
    loader0.unload();
    test.showMenu(null, function (popup) {

      
      
      test.checkMenu(allItems, [], items0);
      popup.hidePopup();

      
      loader1.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu(allItems, [], allItems);
        test.done();
      });
    });
  });
};






exports.testMultipleModulesDiffContexts1 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item0 = new loader0.cm.Item({
    label: "item 0",
    context: loader0.cm.SelectorContext("img")
  });

  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [item0], []);
    popup.hidePopup();

    
    loader0.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item1], [], [item0]);
      popup.hidePopup();

      
      loader1.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item1], [], [item0, item1]);
        test.done();
      });
    });
  });
};






exports.testMultipleModulesDiffContexts2 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item1 = new loader1.cm.Item({ label: "item 1" });

  let item0 = new loader0.cm.Item({
    label: "item 0",
    context: loader0.cm.SelectorContext("img")
  });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [item0], []);
    popup.hidePopup();

    
    loader0.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item1], [], [item0]);
      popup.hidePopup();

      
      loader1.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item1], [], [item0, item1]);
        test.done();
      });
    });
  });
};






exports.testMultipleModulesDiffContexts3 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item0 = new loader0.cm.Item({
    label: "item 0",
    context: loader0.cm.SelectorContext("img")
  });

  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [item0], []);
    popup.hidePopup();

    
    loader1.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item1], [item0], [item1]);
      popup.hidePopup();

      
      loader0.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item1], [], [item0, item1]);
        test.done();
      });
    });
  });
};






exports.testMultipleModulesDiffContexts4 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item1 = new loader1.cm.Item({ label: "item 1" });

  let item0 = new loader0.cm.Item({
    label: "item 0",
    context: loader0.cm.SelectorContext("img")
  });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [item0], []);
    popup.hidePopup();

    
    loader1.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item1], [item0], [item1]);
      popup.hidePopup();

      
      loader0.unload();
      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item1], [], [item0, item1]);
        test.done();
      });
    });
  });
};




exports.testMultipleModulesAddRemove = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item = new loader0.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item], [], []);
    popup.hidePopup();

    
    item.destroy();
    test.showMenu(null, function (popup) {

      
      test.checkMenu([item], [], [item]);
      popup.hidePopup();

      
      loader1.unload();
      test.showMenu(null, function (popup) {

        
        
        test.checkMenu([item], [], [item]);
        test.done();
      });
    });
  });
};




exports.testMultipleModulesOrder = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  
  let item0 = new loader0.cm.Item({ label: "item 0" });
  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [], []);
    popup.hidePopup();

    let item2 = new loader0.cm.Item({ label: "item 2" });

    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item2, item1], [], []);
      popup.hidePopup();

      let item3 = new loader1.cm.Item({ label: "item 3" });

      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item2, item1, item3], [], []);
        test.done();
      });
    });
  });
};





exports.testMultipleModulesOrderOverflow = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  
  let item0 = new loader0.cm.Item({ label: "item 0" });
  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {

    
    test.checkMenu([item0, item1], [], []);
    popup.hidePopup();

    let item2 = new loader0.cm.Item({ label: "item 2" });

    test.showMenu(null, function (popup) {

      
      test.checkMenu([item0, item2, item1], [], []);
      popup.hidePopup();

      let item3 = new loader1.cm.Item({ label: "item 3" });

      test.showMenu(null, function (popup) {

        
        test.checkMenu([item0, item2, item1, item3], [], []);
        test.done();
      });
    });
  });
};




exports.testMultipleModulesOverflowHidden = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  
  let item0 = new loader0.cm.Item({ label: "item 0" });
  let item1 = new loader1.cm.Item({
    label: "item 1",
    context: loader1.cm.SelectorContext("a")
  });

  test.showMenu(null, function (popup) {
    
    test.checkMenu([item0, item1], [item1], []);
    test.done();
  });
};




exports.testMultipleModulesOverflowHidden2 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  
  let item0 = new loader0.cm.Item({
    label: "item 0",
    context: loader0.cm.SelectorContext("a")
  });
  let item1 = new loader1.cm.Item({ label: "item 1" });

  test.showMenu(null, function (popup) {
    
    test.checkMenu([item0, item1], [item0], []);
    test.done();
  });
};




exports.testOverflowIgnoresHidden = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let prefs = loader.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 2);

  let allItems = [
    new loader.cm.Item({
      label: "item 0"
    }),
    new loader.cm.Item({
      label: "item 1"
    }),
    new loader.cm.Item({
      label: "item 2",
      context: loader.cm.SelectorContext("a")
    })
  ];

  test.showMenu(null, function (popup) {
    
    test.checkMenu(allItems, [allItems[2]], []);
    test.done();
  });
};




exports.testOverflowIgnoresHiddenMultipleModules1 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 2);

  let allItems = [
    new loader0.cm.Item({
      label: "item 0"
    }),
    new loader0.cm.Item({
      label: "item 1"
    }),
    new loader1.cm.Item({
      label: "item 2",
      context: loader1.cm.SelectorContext("a")
    }),
    new loader1.cm.Item({
      label: "item 3",
      context: loader1.cm.SelectorContext("a")
    })
  ];

  test.showMenu(null, function (popup) {
    
    test.checkMenu(allItems, [allItems[2], allItems[3]], []);
    test.done();
  });
};




exports.testOverflowIgnoresHiddenMultipleModules2 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 2);

  let allItems = [
    new loader0.cm.Item({
      label: "item 0"
    }),
    new loader0.cm.Item({
      label: "item 1",
      context: loader0.cm.SelectorContext("a")
    }),
    new loader1.cm.Item({
      label: "item 2"
    }),
    new loader1.cm.Item({
      label: "item 3",
      context: loader1.cm.SelectorContext("a")
    })
  ];

  test.showMenu(null, function (popup) {
    
    test.checkMenu(allItems, [allItems[1], allItems[3]], []);
    test.done();
  });
};




exports.testOverflowIgnoresHiddenMultipleModules3 = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let prefs = loader0.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 2);

  let allItems = [
    new loader0.cm.Item({
      label: "item 0",
      context: loader0.cm.SelectorContext("a")
    }),
    new loader0.cm.Item({
      label: "item 1",
      context: loader0.cm.SelectorContext("a")
    }),
    new loader1.cm.Item({
      label: "item 2"
    }),
    new loader1.cm.Item({
      label: "item 3"
    })
  ];

  test.showMenu(null, function (popup) {
    
    test.checkMenu(allItems, [allItems[0], allItems[1]], []);
    test.done();
  });
};




exports.testOverflowTransition = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let prefs = loader.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 2);

  let pItems = [
    new loader.cm.Item({
      label: "item 0",
      context: loader.cm.SelectorContext("p")
    }),
    new loader.cm.Item({
      label: "item 1",
      context: loader.cm.SelectorContext("p")
    })
  ];

  let aItems = [
    new loader.cm.Item({
      label: "item 2",
      context: loader.cm.SelectorContext("a")
    }),
    new loader.cm.Item({
      label: "item 3",
      context: loader.cm.SelectorContext("a")
    })
  ];

  let allItems = pItems.concat(aItems);

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("link"), function (popup) {
      
      test.checkMenu(allItems, [], []);
      popup.hidePopup();

      test.showMenu(doc.getElementById("text"), function (popup) {
        
        test.checkMenu(allItems, aItems, []);
        popup.hidePopup();

        test.showMenu(null, function (popup) {
          
          test.checkMenu(allItems, allItems, []);
          popup.hidePopup();

          test.showMenu(doc.getElementById("text"), function (popup) {
            
            test.checkMenu(allItems, aItems, []);
            popup.hidePopup();

            test.showMenu(doc.getElementById("link"), function (popup) {
              
              test.checkMenu(allItems, [], []);
              popup.hidePopup();

              test.showMenu(null, function (popup) {
                
                test.checkMenu(allItems, allItems, []);
                popup.hidePopup();

                test.showMenu(doc.getElementById("link"), function (popup) {
                  
                  test.checkMenu(allItems, [], []);
                  test.done();
                });
              });
            });
          });
        });
      });
    });
  });
};



exports.testItemCommand = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item data",
    contentScript: 'self.on("click", function (node, data) {' +
                   '  self.postMessage({' +
                   '    tagName: node.tagName,' +
                   '    data: data' +
                   '  });' +
                   '});',
    onMessage: function (data) {
      test.assertEqual(this, item, "`this` inside onMessage should be item");
      test.assertEqual(data.tagName, "HTML", "node should be an HTML element");
      test.assertEqual(data.data, item.data, "data should be item data");
      test.done();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    let elt = test.getItemElt(popup, item);

    
    let evt = elt.ownerDocument.createEvent('Event');
    evt.initEvent('command', true, true);
    elt.dispatchEvent(evt);
  });
};






exports.testMenuCommand = function (test) {
  
  
  
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "submenu item",
    data: "submenu item data",
    context: loader.cm.SelectorContext("a"),
  });

  let submenu = new loader.cm.Menu({
    label: "submenu",
    context: loader.cm.SelectorContext("a"),
    items: [item]
  });

  let topMenu = new loader.cm.Menu({
    label: "top menu",
    contentScript: 'self.on("click", function (node, data) {' +
                   '  self.postMessage({' +
                   '    tagName: node.tagName,' +
                   '    data: data' +
                   '  });' +
                   '});',
    onMessage: function (data) {
      test.assertEqual(this, topMenu, "`this` inside top menu should be menu");
      test.assertEqual(data.tagName, "A", "Clicked node should be anchor");
      test.assertEqual(data.data, item.data,
                       "Clicked item data should be correct");
      test.done();
    },
    items: [submenu],
    context: loader.cm.SelectorContext("a")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("span-link"), function (popup) {
      test.checkMenu([topMenu], [], []);
      let topMenuElt = test.getItemElt(popup, topMenu);
      let topMenuPopup = topMenuElt.firstChild;
      let submenuElt = test.getItemElt(topMenuPopup, submenu);
      let submenuPopup = submenuElt.firstChild;
      let itemElt = test.getItemElt(submenuPopup, item);

      
      let evt = itemElt.ownerDocument.createEvent('Event');
      evt.initEvent('command', true, true);
      itemElt.dispatchEvent(evt);
    });
  });
};



exports.testItemCommandMultipleModules = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item0 = loader0.cm.Item({
    label: "loader 0 item",
    contentScript: 'self.on("click", self.postMessage);',
    onMessage: function () {
      test.fail("loader 0 item should not emit click event");
    }
  });
  let item1 = loader1.cm.Item({
    label: "loader 1 item",
    contentScript: 'self.on("click", self.postMessage);',
    onMessage: function () {
      test.pass("loader 1 item clicked as expected");
      test.done();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item0, item1], [], []);
    let item1Elt = test.getItemElt(popup, item1);

    
    let evt = item1Elt.ownerDocument.createEvent('Event');
    evt.initEvent('command', true, true);
    item1Elt.dispatchEvent(evt);
  });
};





exports.testItemClick = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item data",
    contentScript: 'self.on("click", function (node, data) {' +
                   '  self.postMessage({' +
                   '    tagName: node.tagName,' +
                   '    data: data' +
                   '  });' +
                   '});',
    onMessage: function (data) {
      test.assertEqual(this, item, "`this` inside onMessage should be item");
      test.assertEqual(data.tagName, "HTML", "node should be an HTML element");
      test.assertEqual(data.data, item.data, "data should be item data");
      test.done();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    let elt = test.getItemElt(popup, item);
    elt.click();
  });
};






exports.testMenuClick = function (test) {
  
  
  
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "submenu item",
    data: "submenu item data",
    context: loader.cm.SelectorContext("a"),
  });

  let submenu = new loader.cm.Menu({
    label: "submenu",
    context: loader.cm.SelectorContext("a"),
    items: [item]
  });

  let topMenu = new loader.cm.Menu({
    label: "top menu",
    contentScript: 'self.on("click", function (node, data) {' +
                   '  self.postMessage({' +
                   '    tagName: node.tagName,' +
                   '    data: data' +
                   '  });' +
                   '});',
    onMessage: function (data) {
      test.assertEqual(this, topMenu, "`this` inside top menu should be menu");
      test.assertEqual(data.tagName, "A", "Clicked node should be anchor");
      test.assertEqual(data.data, item.data,
                       "Clicked item data should be correct");
      test.done();
    },
    items: [submenu],
    context: loader.cm.SelectorContext("a")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("span-link"), function (popup) {
      test.checkMenu([topMenu], [], []);
      let topMenuElt = test.getItemElt(popup, topMenu);
      let topMenuPopup = topMenuElt.firstChild;
      let submenuElt = test.getItemElt(topMenuPopup, submenu);
      let submenuPopup = submenuElt.firstChild;
      let itemElt = test.getItemElt(submenuPopup, item);
      itemElt.click();
    });
  });
};


exports.testItemClickMultipleModules = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let item0 = loader0.cm.Item({
    label: "loader 0 item",
    contentScript: 'self.on("click", self.postMessage);',
    onMessage: function () {
      test.fail("loader 0 item should not emit click event");
    }
  });
  let item1 = loader1.cm.Item({
    label: "loader 1 item",
    contentScript: 'self.on("click", self.postMessage);',
    onMessage: function () {
      test.pass("loader 1 item clicked as expected");
      test.done();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item0, item1], [], []);
    let item1Elt = test.getItemElt(popup, item1);
    item1Elt.click();
  });
};



exports.testSeparator = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = new loader.cm.Menu({
    label: "submenu",
    items: [new loader.cm.Separator()]
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testParentMenu = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = new loader.cm.Menu({
    label: "submenu",
    items: [loader.cm.Item({ label: "item 1" })],
    parentMenu: loader.cm.contentContextMenu
  });

  let item = loader.cm.Item({
    label: "item 2",
    parentMenu: menu,
  });

  test.assertEqual(menu.items[1], item, "Item should be in the sub menu");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};


exports.testNewWindow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.withNewWindow(function () {
    test.showMenu(null, function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};




exports.testNewWindowMultipleModules = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();
  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    popup.hidePopup();
    loader.unload();
    test.withNewWindow(function () {
      test.showMenu(null, function (popup) {
        test.checkMenu([item], [], [item]);
        test.done();
      });
    });
  });
};



exports.testSorting = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  
  
  let items = [];
  for (let i = 0; i < OVERFLOW_THRESH_DEFAULT; i += 2) {
    items.push(new loader.cm.Item({ label: "item " + (i + 1) }));
    items.push(new loader.cm.Item({ label: "item " + i }));
  }

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};



exports.testSortingOverflow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  
  
  let items = [];
  for (let i = 0; i < OVERFLOW_THRESH_DEFAULT * 2; i += 2) {
    items.push(new loader.cm.Item({ label: "item " + (i + 1) }));
    items.push(new loader.cm.Item({ label: "item " + i }));
  }

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};



exports.testSortingMultipleModules = function (test) {
  test = new TestHelper(test);
  let loader0 = test.newLoader();
  let loader1 = test.newLoader();

  let items0 = [];
  let items1 = [];
  for (let i = 0; i < OVERFLOW_THRESH_DEFAULT; i++) {
    if (i % 2) {
      let item = new loader0.cm.Item({ label: "item " + i });
      items0.push(item);
    }
    else {
      let item = new loader1.cm.Item({ label: "item " + i });
      items1.push(item);
    }
  }
  let allItems = items0.concat(items1);

  test.showMenu(null, function (popup) {

    
    test.checkMenu(allItems, [], []);
    popup.hidePopup();
    loader0.unload();
    loader1.unload();
    test.showMenu(null, function (popup) {

      
      test.checkMenu(allItems, [], allItems);
      test.done();
    });
  });
};




exports.testContentCommunication = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'var potato;' +
                   'self.on("context", function () {' +
                   '  potato = "potato";' +
                   '  return true;' +
                   '});' +
                   'self.on("click", function () {' +
                   '  self.postMessage(potato);' +
                   '});',
  });

  item.on("message", function (data) {
    test.assertEqual(data, "potato", "That's a lot of potatoes!");
    test.done();
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    let elt = test.getItemElt(popup, item);
    elt.click();
  });
};





exports.testLoadWithOpenTab = function (test) {
  test = new TestHelper(test);
  test.withTestDoc(function (window, doc) {
    let loader = test.newLoader();
    let item = new loader.cm.Item({
      label: "item",
      contentScript:
        'self.on("click", function () self.postMessage("click"));',
      onMessage: function (msg) {
        if (msg === "click")
          test.done();
      }
    });
    test.showMenu(null, function (popup) {
      test.checkMenu([item], [], []);
      test.getItemElt(popup, item).click();
    });
  });
};



exports.testDrawImageOnClickNode = function (test) {
  test = new TestHelper(test);
  test.withTestDoc(function (window, doc) {
    let loader = test.newLoader();
    let item = new loader.cm.Item({
      label: "item",
      context: loader.cm.SelectorContext("img"),
      contentScript: "new " + function() {
        self.on("click", function (img, data) {
          let ctx = document.createElement("canvas").getContext("2d");
          ctx.drawImage(img, 1, 1, 1, 1);
          self.postMessage("done");
        });
      },
      onMessage: function (msg) {
        if (msg === "done")
          test.done();
      }
    });
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu([item], [], []);
      test.getItemElt(popup, item).click();
    });
  });
};




exports.testSetLabelBeforeShow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ]
  items[0].label = "z";
  test.assertEqual(items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};




exports.testSetLabelAfterShow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ];

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    popup.hidePopup();

    items[0].label = "z";
    test.assertEqual(items[0].label, "z");
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};




exports.testSetLabelBeforeShowOverflow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let prefs = loader.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ]
  items[0].label = "z";
  test.assertEqual(items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};




exports.testSetLabelAfterShowOverflow = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let prefs = loader.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ];

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    popup.hidePopup();

    items[0].label = "z";
    test.assertEqual(items[0].label, "z");
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testSetLabelMenuItem = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [loader.cm.Item({ label: "a" })]
  });
  menu.items[0].label = "z";

  test.assertEqual(menu.items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuAddItem = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [
      loader.cm.Item({ label: "item 0" })
    ]
  });
  menu.addItem(loader.cm.Item({ label: "item 1" }));
  menu.addItem(loader.cm.Item({ label: "item 2" }));

  test.assertEqual(menu.items.length, 3,
                   "menu should have correct number of items");
  for (let i = 0; i < 3; i++) {
    test.assertEqual(menu.items[i].label, "item " + i,
                     "item label should be correct");
    test.assertEqual(menu.items[i].parentMenu, menu,
                     "item's parent menu should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuAddItemTwice = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: []
  });
  let subitem = loader.cm.Item({ label: "item 1" })
  menu.addItem(subitem);
  menu.addItem(loader.cm.Item({ label: "item 0" }));
  menu.addItem(subitem);

  test.assertEqual(menu.items.length, 2,
                   "menu should have correct number of items");
  for (let i = 0; i < 2; i++) {
    test.assertEqual(menu.items[i].label, "item " + i,
                     "item label should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuRemoveItem = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let subitem = loader.cm.Item({ label: "item 1" });
  let menu = loader.cm.Menu({
    label: "menu",
    items: [
      loader.cm.Item({ label: "item 0" }),
      subitem,
      loader.cm.Item({ label: "item 2" })
    ]
  });

  
  menu.removeItem(subitem);
  menu.removeItem(subitem);

  test.assertEqual(subitem.parentMenu, null,
                   "item's parent menu should be correct");

  test.assertEqual(menu.items.length, 2,
                   "menu should have correct number of items");
  test.assertEqual(menu.items[0].label, "item 0",
                   "item label should be correct");
  test.assertEqual(menu.items[1].label, "item 2",
                   "item label should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuItemSwap = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let subitem = loader.cm.Item({ label: "item" });
  let menu0 = loader.cm.Menu({
    label: "menu 0",
    items: [subitem]
  });
  let menu1 = loader.cm.Menu({
    label: "menu 1",
    items: []
  });
  menu1.addItem(subitem);

  test.assertEqual(menu0.items.length, 0,
                   "menu should have correct number of items");

  test.assertEqual(menu1.items.length, 1,
                   "menu should have correct number of items");
  test.assertEqual(menu1.items[0].label, "item",
                   "item label should be correct");

  test.assertEqual(subitem.parentMenu, menu1,
                   "item's parent menu should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu0, menu1], [menu0], []);
    test.done();
  });
};



exports.testMenuItemDestroy = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let subitem = loader.cm.Item({ label: "item" });
  let menu = loader.cm.Menu({
    label: "menu",
    items: [subitem]
  });
  subitem.destroy();

  test.assertEqual(menu.items.length, 0,
                   "menu should have correct number of items");
  test.assertEqual(subitem.parentMenu, null,
                   "item's parent menu should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [menu], []);
    test.done();
  });
};



exports.testMenuItemsSetter = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [
      loader.cm.Item({ label: "old item 0" }),
      loader.cm.Item({ label: "old item 1" })
    ]
  });
  menu.items = [
    loader.cm.Item({ label: "new item 0" }),
    loader.cm.Item({ label: "new item 1" }),
    loader.cm.Item({ label: "new item 2" })
  ];

  test.assertEqual(menu.items.length, 3,
                   "menu should have correct number of items");
  for (let i = 0; i < 3; i++) {
    test.assertEqual(menu.items[i].label, "new item " + i,
                     "item label should be correct");
    test.assertEqual(menu.items[i].parentMenu, menu,
                     "item's parent menu should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testItemDataSetter = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item = loader.cm.Item({ label: "old item 0", data: "old" });
  item.data = "new";

  test.assertEqual(item.data, "new", "item should have correct data");

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};




exports.testAlreadyOpenIframe = function (test) {
  test = new TestHelper(test);
  test.withTestDoc(function (window, doc) {
    let loader = test.newLoader();
    let item = new loader.cm.Item({
      label: "item"
    });
    test.showMenu(doc.getElementById("iframe"), function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};



exports.testItemNoLabel = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  try {
    new loader.cm.Item({});
    test.assert(false, "Should have seen exception");
  }
  catch (e) {
    test.assert(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: null });
    test.assert(false, "Should have seen exception");
  }
  catch (e) {
    test.assert(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: undefined });
    test.assert(false, "Should have seen exception");
  }
  catch (e) {
    test.assert(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: "" });
    test.assert(false, "Should have seen exception");
  }
  catch (e) {
    test.assert(true, "Should have seen exception");
  }

  test.done();
}



exports.testItemNoData = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  function checkData(data) {
    test.assertEqual(data, undefined, "Data should be undefined");
  }

  let item1 = new loader.cm.Item({
    label: "item 1",
    contentScript: 'self.on("click", function(node, data) self.postMessage(data))',
    onMessage: checkData
  });
  let item2 = new loader.cm.Item({
    label: "item 2",
    data: null,
    contentScript: 'self.on("click", function(node, data) self.postMessage(data))',
    onMessage: checkData
  });
  let item3 = new loader.cm.Item({
    label: "item 3",
    data: undefined,
    contentScript: 'self.on("click", function(node, data) self.postMessage(data))',
    onMessage: checkData
  });

  test.assertEqual(item1.data, undefined, "Should be no defined data");
  test.assertEqual(item2.data, null, "Should be no defined data");
  test.assertEqual(item3.data, undefined, "Should be no defined data");

  test.showMenu(null, function (popup) {
    test.checkMenu([item1, item2, item3], [], []);

    let itemElt = test.getItemElt(popup, item1);
    itemElt.click();

    test.hideMenu(function() {
      test.showMenu(null, function (popup) {
        let itemElt = test.getItemElt(popup, item2);
        itemElt.click();

        test.hideMenu(function() {
          test.showMenu(null, function (popup) {
            let itemElt = test.getItemElt(popup, item3);
            itemElt.click();

            test.done();
          });
        });
      });
    });
  });
}



exports.testItemNoImage = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let item1 = new loader.cm.Item({ label: "item 1" });
  let item2 = new loader.cm.Item({ label: "item 2", image: null });
  let item3 = new loader.cm.Item({ label: "item 3", image: undefined });

  test.assertEqual(item1.image, undefined, "Should be no defined image");
  test.assertEqual(item2.image, null, "Should be no defined image");
  test.assertEqual(item3.image, undefined, "Should be no defined image");

  test.showMenu(null, function (popup) {
    test.checkMenu([item1, item2, item3], [], []);

    test.done();
  });
}



exports.testItemImage = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let imageURL = require("sdk/self").data.url("moz_favicon.ico");
  let item = new loader.cm.Item({ label: "item", image: imageURL });
  let menu = new loader.cm.Menu({ label: "menu", image: imageURL, items: [
    loader.cm.Item({ label: "subitem" })
  ]});
  test.assertEqual(item.image, imageURL, "Should have set the image correctly");
  test.assertEqual(menu.image, imageURL, "Should have set the image correctly");

  test.showMenu(null, function (popup) {
    test.checkMenu([item, menu], [], []);

    let imageURL2 = require("sdk/self").data.url("dummy.ico");
    item.image = imageURL2;
    menu.image = imageURL2;
    test.assertEqual(item.image, imageURL2, "Should have set the image correctly");
    test.assertEqual(menu.image, imageURL2, "Should have set the image correctly");
    test.checkMenu([item, menu], [], []);

    item.image = null;
    menu.image = null;
    test.assertEqual(item.image, null, "Should have set the image correctly");
    test.assertEqual(menu.image, null, "Should have set the image correctly");
    test.checkMenu([item, menu], [], []);

    test.done();
  });
};



exports.testMenuDestroy = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [
      loader.cm.Item({ label: "item 0" }),
      loader.cm.Menu({
        label: "item 1",
        items: [
          loader.cm.Item({ label: "subitem 0" }),
          loader.cm.Item({ label: "subitem 1" }),
          loader.cm.Item({ label: "subitem 2" })
        ]
      }),
      loader.cm.Item({ label: "item 2" })
    ]
  });
  menu.destroy();

  






  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], [menu]);
    test.done();
  });
};



exports.testSubItemContextNoMatchHideMenu = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    loader.cm.Menu({
      label: "menu 1",
      items: [
        loader.cm.Item({
          label: "subitem 1",
          context: loader.cm.SelectorContext(".foo")
        })
      ]
    }),
    loader.cm.Menu({
      label: "menu 2",
      items: [
        loader.cm.Item({
          label: "subitem 2",
          contentScript: 'self.on("context", function () false);'
        })
      ]
    }),
    loader.cm.Menu({
      label: "menu 3",
      items: [
        loader.cm.Item({
          label: "subitem 3",
          context: loader.cm.SelectorContext(".foo")
        }),
        loader.cm.Item({
          label: "subitem 4",
          contentScript: 'self.on("context", function () false);'
        })
      ]
    })
  ];

  test.showMenu(null, function (popup) {
    test.checkMenu(items, items, []);
    test.done();
  });
};




exports.testSubItemContextMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let hiddenItems = [
    loader.cm.Item({
      label: "subitem 3",
      context: loader.cm.SelectorContext(".foo")
    }),
    loader.cm.Item({
      label: "subitem 6",
      contentScript: 'self.on("context", function () false);'
    })
  ];

  let items = [
    loader.cm.Menu({
      label: "menu 1",
      items: [
        loader.cm.Item({
          label: "subitem 1",
          context: loader.cm.URLContext(TEST_DOC_URL)
        })
      ]
    }),
    loader.cm.Menu({
      label: "menu 2",
      items: [
        loader.cm.Item({
          label: "subitem 2",
          contentScript: 'self.on("context", function () true);'
        })
      ]
    }),
    loader.cm.Menu({
      label: "menu 3",
      items: [
        hiddenItems[0],
        loader.cm.Item({
          label: "subitem 4",
          contentScript: 'self.on("context", function () true);'
        })
      ]
    }),
    loader.cm.Menu({
      label: "menu 4",
      items: [
        loader.cm.Item({
          label: "subitem 5",
          context: loader.cm.URLContext(TEST_DOC_URL)
        }),
        hiddenItems[1]
      ]
    }),
    loader.cm.Menu({
      label: "menu 5",
      items: [
        loader.cm.Item({
          label: "subitem 7",
          context: loader.cm.URLContext(TEST_DOC_URL)
        }),
        loader.cm.Item({
          label: "subitem 8",
          contentScript: 'self.on("context", function () true);'
        })
      ]
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, hiddenItems, []);
      test.done();
    });
  });
};



exports.testSubItemDefaultVisible = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let items = [
    loader.cm.Menu({
      label: "menu 1",
      context: loader.cm.SelectorContext("img"),
      items: [
        loader.cm.Item({
          label: "subitem 1"
        }),
        loader.cm.Item({
          label: "subitem 2",
          context: loader.cm.SelectorContext("img")
        }),
        loader.cm.Item({
          label: "subitem 3",
          context: loader.cm.SelectorContext("a")
        })
      ]
    })
  ];

  
  let hiddenItems = [items[0].items[2]];

  test.withTestDoc(function (window, doc) {
    test.showMenu(doc.getElementById("image"), function (popup) {
      test.checkMenu(items, hiddenItems, []);
      test.done();
    });
  });
};



exports.testSubItemClick = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let state = 0;

  let items = [
    loader.cm.Menu({
      label: "menu 1",
      items: [
        loader.cm.Item({
          label: "subitem 1",
          data: "foobar",
          contentScript: 'self.on("click", function (node, data) {' +
                         '  self.postMessage({' +
                         '    tagName: node.tagName,' +
                         '    data: data' +
                         '  });' +
                         '});',
          onMessage: function(msg) {
            test.assertEqual(msg.tagName, "HTML", "should have seen the right node");
            test.assertEqual(msg.data, "foobar", "should have seen the right data");
            test.assertEqual(state, 0, "should have seen the event at the right time");
            state++;
          }
        })
      ],
      contentScript: 'self.on("click", function (node, data) {' +
                     '  self.postMessage({' +
                     '    tagName: node.tagName,' +
                     '    data: data' +
                     '  });' +
                     '});',
      onMessage: function(msg) {
        test.assertEqual(msg.tagName, "HTML", "should have seen the right node");
        test.assertEqual(msg.data, "foobar", "should have seen the right data");
        test.assertEqual(state, 1, "should have seen the event at the right time");

        test.done();
      }
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);

      let topMenuElt = test.getItemElt(popup, items[0]);
      let topMenuPopup = topMenuElt.firstChild;
      let itemElt = test.getItemElt(topMenuPopup, items[0].items[0]);
      itemElt.click();
    });
  });
};



exports.testSubItemCommand = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let state = 0;

  let items = [
    loader.cm.Menu({
      label: "menu 1",
      items: [
        loader.cm.Item({
          label: "subitem 1",
          data: "foobar",
          contentScript: 'self.on("click", function (node, data) {' +
                         '  self.postMessage({' +
                         '    tagName: node.tagName,' +
                         '    data: data' +
                         '  });' +
                         '});',
          onMessage: function(msg) {
            test.assertEqual(msg.tagName, "HTML", "should have seen the right node");
            test.assertEqual(msg.data, "foobar", "should have seen the right data");
            test.assertEqual(state, 0, "should have seen the event at the right time");
            state++;
          }
        })
      ],
      contentScript: 'self.on("click", function (node, data) {' +
                     '  self.postMessage({' +
                     '    tagName: node.tagName,' +
                     '    data: data' +
                     '  });' +
                     '});',
      onMessage: function(msg) {
        test.assertEqual(msg.tagName, "HTML", "should have seen the right node");
        test.assertEqual(msg.data, "foobar", "should have seen the right data");
        test.assertEqual(state, 1, "should have seen the event at the right time");
        state++

        test.done();
      }
    })
  ];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);

      let topMenuElt = test.getItemElt(popup, items[0]);
      let topMenuPopup = topMenuElt.firstChild;
      let itemElt = test.getItemElt(topMenuPopup, items[0].items[0]);

      
      let evt = itemElt.ownerDocument.createEvent('Event');
      evt.initEvent('command', true, true);
      itemElt.dispatchEvent(evt);
    });
  });
};



exports.testSelectionInInnerFrameNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let state = 0;

  let items = [
    loader.cm.Item({
      label: "test item",
      context: loader.cm.SelectionContext()
    })
  ];

  test.withTestDoc(function (window, doc) {
    let frame = doc.getElementById("iframe");
    frame.contentWindow.getSelection().selectAllChildren(frame.contentDocument.body);

    test.showMenu(null, function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};



exports.testSelectionInInnerFrameMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let state = 0;

  let items = [
    loader.cm.Item({
      label: "test item",
      context: loader.cm.SelectionContext()
    })
  ];

  test.withTestDoc(function (window, doc) {
    let frame = doc.getElementById("iframe");
    frame.contentWindow.getSelection().selectAllChildren(frame.contentDocument.body);

    test.showMenu(frame.contentDocument.getElementById("text"), function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testSelectionInOuterFrameNoMatch = function (test) {
  test = new TestHelper(test);
  let loader = test.newLoader();

  let state = 0;

  let items = [
    loader.cm.Item({
      label: "test item",
      context: loader.cm.SelectionContext()
    })
  ];

  test.withTestDoc(function (window, doc) {
    let frame = doc.getElementById("iframe");
    window.getSelection().selectAllChildren(doc.body);

    test.showMenu(frame.contentDocument.getElementById("text"), function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




if (!require("sdk/system/xul-app").is("Firefox")) {
  module.exports = {
    testAppNotSupported: function (test) {
      test.pass("context-menu does not support this application.");
    }
  };
}







function TestHelper(test) {
  
  
  test.waitUntilDone(30*1000);
  this.test = test;
  this.loaders = [];
  this.browserWindow = Cc["@mozilla.org/appshell/window-mediator;1"].
                       getService(Ci.nsIWindowMediator).
                       getMostRecentWindow("navigator:browser");
  this.overflowThreshValue = require("sdk/preferences/service").
                             get(OVERFLOW_THRESH_PREF, OVERFLOW_THRESH_DEFAULT);
}

TestHelper.prototype = {
  get contextMenuPopup() {
    return this.browserWindow.document.getElementById("contentAreaContextMenu");
  },

  get contextMenuSeparator() {
    return this.browserWindow.document.querySelector("." + SEPARATOR_CLASS);
  },

  get overflowPopup() {
    return this.browserWindow.document.querySelector("." + OVERFLOW_POPUP_CLASS);
  },

  get overflowSubmenu() {
    return this.browserWindow.document.querySelector("." + OVERFLOW_MENU_CLASS);
  },

  get tabBrowser() {
    return this.browserWindow.gBrowser;
  },

  
  __noSuchMethod__: function (methodName, args) {
    this.test[methodName].apply(this.test, args);
  },

  
  checkItemElt: function (elt, item) {
    let itemType = this.getItemType(item);

    switch (itemType) {
    case "Item":
      this.test.assertEqual(elt.localName, "menuitem",
                            "Item DOM element should be a xul:menuitem");
      if (typeof(item.data) === "string") {
        this.test.assertEqual(elt.getAttribute("value"), item.data,
                              "Item should have correct data");
      }
      break
    case "Menu":
      this.test.assertEqual(elt.localName, "menu",
                            "Menu DOM element should be a xul:menu");
      let subPopup = elt.firstChild;
      this.test.assert(subPopup, "xul:menu should have a child");
      this.test.assertEqual(subPopup.localName, "menupopup",
                            "xul:menu's first child should be a menupopup");
      break;
    case "Separator":
      this.test.assertEqual(elt.localName, "menuseparator",
                         "Separator DOM element should be a xul:menuseparator");
      break;
    }

    if (itemType === "Item" || itemType === "Menu") {
      this.test.assertEqual(elt.getAttribute("label"), item.label,
                            "Item should have correct title");
      if (typeof(item.image) === "string") {
        this.test.assertEqual(elt.getAttribute("image"), item.image,
                              "Item should have correct image");
        if (itemType === "Menu")
          this.test.assert(elt.classList.contains("menu-iconic"),
                           "Menus with images should have the correct class")
        else
          this.test.assert(elt.classList.contains("menuitem-iconic"),
                           "Items with images should have the correct class")
      }
      else {
        this.test.assert(!elt.getAttribute("image"),
                         "Item should not have image");
        this.test.assert(!elt.classList.contains("menu-iconic") && !elt.classList.contains("menuitem-iconic"),
                         "The iconic classes should not be present")
      }
    }
  },

  
  
  
  
  checkMenu: function (presentItems, absentItems, removedItems) {
    
    let total = 0;
    for (let item of presentItems) {
      if (absentItems.indexOf(item) < 0 && removedItems.indexOf(item) < 0)
        total++;
    }

    let separator = this.contextMenuSeparator;
    if (total == 0) {
      this.test.assert(!separator || separator.hidden,
                       "separator should not be present");
    }
    else {
      this.test.assert(separator && !separator.hidden,
                       "separator should be present");
    }

    let mainNodes = this.browserWindow.document.querySelectorAll("#contentAreaContextMenu > ." + ITEM_CLASS);
    let overflowNodes = this.browserWindow.document.querySelectorAll("." + OVERFLOW_POPUP_CLASS + " > ." + ITEM_CLASS);

    this.test.assert(mainNodes.length == 0 || overflowNodes.length == 0,
                     "Should only see nodes at the top level or in overflow");

    let overflow = this.overflowSubmenu;
    if (this.shouldOverflow(total)) {
      this.test.assert(overflow && !overflow.hidden,
                       "overflow menu should be present");
      this.test.assertEqual(mainNodes.length, 0,
                            "should be no items in the main context menu");
    }
    else {
      this.test.assert(!overflow || overflow.hidden,
                       "overflow menu should not be present");
      
      if (total > 0) {
        this.test.assertEqual(overflowNodes.length, 0,
                              "should be no items in the overflow context menu");
      }
    }

    
    let nodes = mainNodes.length ? mainNodes : overflowNodes;
    this.checkNodes(nodes, presentItems, absentItems, removedItems)
    let pos = 0;
  },

  
  
  
  
  checkNodes: function (nodes, presentItems, absentItems, removedItems) {
    let pos = 0;
    for (let item of presentItems) {
      
      if (removedItems.indexOf(item) >= 0)
        continue;

      if (nodes.length <= pos) {
        this.test.assert(false, "Not enough nodes");
        return;
      }

      let hidden = absentItems.indexOf(item) >= 0;

      this.checkItemElt(nodes[pos], item);
      this.test.assertEqual(nodes[pos].hidden, hidden,
                            "hidden should be set correctly");

      
      if (!hidden && this.getItemType(item) == "Menu") {
        this.test.assertEqual(nodes[pos].firstChild.localName, "menupopup",
                              "menu XUL should contain a menupopup");
        this.checkNodes(nodes[pos].firstChild.childNodes, item.items, absentItems, removedItems);
      }

      if (pos > 0)
        this.test.assertEqual(nodes[pos].previousSibling, nodes[pos - 1],
                              "nodes should all be in the same group");
      pos++;
    }

    this.test.assertEqual(nodes.length, pos,
                          "should have checked all the XUL nodes");
  },

  
  
  
  
  
  
  
  delayedEventListener: function (node, event, callback, useCapture, isValid) {
    const self = this;
    node.addEventListener(event, function handler(evt) {
      if (isValid && !isValid(evt))
        return;
      node.removeEventListener(event, handler, useCapture);
      timer.setTimeout(function () {
        try {
          callback.call(self, evt);
        }
        catch (err) {
          self.test.exception(err);
          self.test.done();
        }
      }, 20);
    }, useCapture);
  },

  
  done: function () {
    const self = this;
    function commonDone() {
      this.closeTab();

      while (this.loaders.length) {
        this.loaders[0].unload();
      }

      require("sdk/preferences/service").set(OVERFLOW_THRESH_PREF, self.overflowThreshValue);

      this.test.done();
    }

    function closeBrowserWindow() {
      if (this.oldBrowserWindow) {
        this.delayedEventListener(this.browserWindow, "unload", commonDone,
                                  false);
        this.browserWindow.close();
        this.browserWindow = this.oldBrowserWindow;
        delete this.oldBrowserWindow;
      }
      else {
        commonDone.call(this);
      }
    };

    if (this.contextMenuPopup.state == "closed") {
      closeBrowserWindow.call(this);
    }
    else {
      this.delayedEventListener(this.contextMenuPopup, "popuphidden",
                                function () closeBrowserWindow.call(this),
                                false);
      this.contextMenuPopup.hidePopup();
    }
  },

  closeTab: function() {
    if (this.tab) {
      this.tabBrowser.removeTab(this.tab);
      this.tabBrowser.selectedTab = this.oldSelectedTab;
      this.tab = null;
    }
  },

  
  
  
  getItemElt: function (popup, item) {
    let nodes = popup.childNodes;
    for (let i = nodes.length - 1; i >= 0; i--) {
      if (this.getItemType(item) === "Separator") {
        if (nodes[i].localName === "menuseparator")
          return nodes[i];
      }
      else if (nodes[i].getAttribute("label") === item.label)
        return nodes[i];
    }
    return null;
  },

  
  getItemType: function (item) {
    
    
    
    
    return /^\[object (Item|Menu|Separator)/.exec(item.toString())[1];
  },

  
  
  
  
  newLoader: function () {
    const self = this;
    let loader = Loader(module);
    let wrapper = {
      loader: loader,
      cm: loader.require("sdk/context-menu"),
      globalScope: loader.sandbox("sdk/context-menu"),
      unload: function () {
        loader.unload();
        let idx = self.loaders.indexOf(wrapper);
        if (idx < 0)
          throw new Error("Test error: tried to unload nonexistent loader");
        self.loaders.splice(idx, 1);
      }
    };
    this.loaders.push(wrapper);
    return wrapper;
  },

  
  shouldOverflow: function (count) {
    return count >
           (this.loaders.length ?
            this.loaders[0].loader.require("sdk/preferences/service").
              get(OVERFLOW_THRESH_PREF, OVERFLOW_THRESH_DEFAULT) :
            OVERFLOW_THRESH_DEFAULT);
  },

  
  
  
  showMenu: function(targetNode, onshownCallback) {
    function sendEvent() {
      this.delayedEventListener(this.browserWindow, "popupshowing",
        function (e) {
          let popup = e.target;
          onshownCallback.call(this, popup);
        }, false);

      let rect = targetNode ?
                 targetNode.getBoundingClientRect() :
                 { left: 0, top: 0, width: 0, height: 0 };
      let contentWin = targetNode ? targetNode.ownerDocument.defaultView
                                  : this.browserWindow.content;
      contentWin.
        QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils).
        sendMouseEvent("contextmenu",
                       rect.left + (rect.width / 2),
                       rect.top + (rect.height / 2),
                       2, 1, 0);
    }

    
    
    
    if (!targetNode && !this.oldSelectedTab && !this.oldBrowserWindow) {
      this.oldSelectedTab = this.tabBrowser.selectedTab;
      this.tab = this.tabBrowser.addTab("about:blank");
      let browser = this.tabBrowser.getBrowserForTab(this.tab);

      this.delayedEventListener(browser, "load", function () {
        this.tabBrowser.selectedTab = this.tab;
        sendEvent.call(this);
      }, true);
    }
    else
      sendEvent.call(this);
  },

  hideMenu: function(onhiddenCallback) {
    this.delayedEventListener(this.browserWindow, "popuphidden", onhiddenCallback);

    this.contextMenuPopup.hidePopup();
  },

  
  
  withNewWindow: function (onloadCallback) {
    let win = this.browserWindow.OpenBrowserWindow();
    this.delayedEventListener(win, "load", onloadCallback, true);
    this.oldBrowserWindow = this.browserWindow;
    this.browserWindow = win;
  },

  
  
  withTestDoc: function (onloadCallback) {
    this.oldSelectedTab = this.tabBrowser.selectedTab;
    this.tab = this.tabBrowser.addTab(TEST_DOC_URL);
    let browser = this.tabBrowser.getBrowserForTab(this.tab);

    this.delayedEventListener(browser, "load", function () {
      this.tabBrowser.selectedTab = this.tab;
      onloadCallback.call(this, browser.contentWindow, browser.contentDocument);
    }, true, function(evt) {
      return evt.target.location == TEST_DOC_URL;
    });
  }
};
