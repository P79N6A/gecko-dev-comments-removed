


 'use strict';

require("sdk/context-menu");

const { defer } = require("sdk/core/promise");
const packaging = require('@loader/options');


const OVERFLOW_THRESH_DEFAULT = 10;
const OVERFLOW_THRESH_PREF =
  "extensions.addon-sdk.context-menu.overflowThreshold";

const TEST_DOC_URL = module.uri.replace(/\.js$/, ".html");
const data = require("./fixtures");

const { TestHelper } = require("./context-menu/test-helper.js")



exports.testSeparatorPosition = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  
  let oldSeparator = test.contextMenuPopup.ownerDocument.createElement("menuseparator");
  oldSeparator.id = "jetpack-context-menu-separator";
  test.contextMenuPopup.appendChild(oldSeparator);

  
  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    assert.equal(test.contextMenuSeparator.nextSibling.nextSibling, oldSeparator,
                     "New separator should appear before the old one");
    test.contextMenuPopup.removeChild(oldSeparator);
    test.done();
  });
};



exports.testConstructDestroy = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  
  let item = new loader.cm.Item({ label: "item" });
  assert.equal(item.parentMenu, loader.cm.contentContextMenu,
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



exports.testDestroyTwice = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });
  item.destroy();
  item.destroy();

  test.pass("Destroying an item twice should not cause an error.");
  test.done();
};




exports.testSelectorContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item",
    context: loader.cm.SelectorContext("img")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};





exports.testSelectorAncestorContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    data: "item",
    context: loader.cm.SelectorContext("a[href]")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#span-link", function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};





exports.testSelectorContextNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testPageContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testPageContextNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};



exports.testSelectionContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testSelectionContextMatchInTextField = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    test.selectRange("#textfield", 0, null);
    test.showMenu("#textfield", function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};




exports.testSelectionContextNoMatchInTextField = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    test.selectRange("#textfield", 0, 0);
    test.showMenu("#textfield", function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};




exports.testSelectionContextNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testSelectionContextInNewTab = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    let link = doc.getElementById("targetlink");
    link.click();

    let tablistener = event => {
      this.tabBrowser.tabContainer.removeEventListener("TabOpen", tablistener, false);
      let tab = event.target;
      let browser = tab.linkedBrowser;
      this.loadFrameScript(browser);
      this.delayedEventListener(browser, "load", () => {
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
    };
    this.tabBrowser.tabContainer.addEventListener("TabOpen", tablistener, false);
  });
};



exports.testSelectionContextButtonMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    window.getSelection().selectAllChildren(doc.body);
    test.showMenu("#button", function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};



exports.testSelectionContextButtonNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({
    label: "item",
    context: loader.cm.SelectionContext()
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#button", function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};



exports.testURLContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testURLContextNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testPageReload = function (assert, done) {
  let test = new TestHelper(assert, done);
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








































exports.testContentContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testContentContextNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testContentContextUndefined = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testContentContextEmptyString = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [item], []);
    assert.equal(item.label, "item", "Label should still be correct");
    test.done();
  });
};




exports.testMultipleContentContextMatch1 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleContentContextMatch2 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleContentContextString1 = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "new label"); ' +
                   'self.on("context", function () false);'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    assert.equal(item.label, "new label", "Label should have changed");
    test.done();
  });
};




exports.testMultipleContentContextString2 = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () false); ' +
                   'self.on("context", function () "new label");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    assert.equal(item.label, "new label", "Label should have changed");
    test.done();
  });
};



exports.testMultipleContentContextString3 = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function () "new label 1"); ' +
                   'self.on("context", function () "new label 2");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    assert.equal(item.label, "new label 1", "Label should have changed");
    test.done();
  });
};




exports.testContentContextMatchActiveElement = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, [items[2], items[3]], []);
      test.done();
    });
  });
};




exports.testContentContextNoMatchActiveElement = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




exports.testContentContextNoMatchActiveElement = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




exports.testContentContextMatchString = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "first label",
    contentScript: 'self.on("context", function () "second label");'
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    assert.equal(item.label, "second label",
                     "item's label should be updated");
    test.done();
  });
};



exports.testContentScriptFile = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();
  let { defer, all } = require("sdk/core/promise");
  let itemScript = [defer(), defer()];
  let menuShown = defer();
  let menuPromises = itemScript.concat(menuShown).map(({promise}) => promise);

  
  assert.throws(function() {
      new loader.cm.Item({
        label: "item",
        contentScriptFile: "http://mozilla.com/context-menu.js"
      });
    },
    /The `contentScriptFile` option must be a local URL or an array of URLs/,
    "Item throws when contentScriptFile is a remote URL");

  
  let item = new loader.cm.Item({
    label: "item",
    contentScriptFile: data.url("test-contentScriptFile.js"),
    onMessage: (message) => {
      assert.equal(message, "msg from contentScriptFile",
        "contentScriptFile loaded with absolute url");
      itemScript[0].resolve();
    }
  });

  let item2 = new loader.cm.Item({
    label: "item2",
    contentScriptFile: "./test-contentScriptFile.js",
    onMessage: (message) => {
      assert.equal(message, "msg from contentScriptFile",
        "contentScriptFile loaded with relative url");
      itemScript[1].resolve();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item, item2], [], []);
    menuShown.resolve();
  });

  all(menuPromises).then(() => test.done());
};



exports.testContentContextArgs = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();
  let callbacks = 0;

  let item = new loader.cm.Item({
    label: "item",
    contentScript: 'self.on("context", function (node) {' +
                   '  self.postMessage(node.tagName);' +
                   '  return false;' +
                   '});',
    onMessage: function (tagName) {
      assert.equal(tagName, "HTML", "node should be an HTML element");
      if (++callbacks == 2) test.done();
    }
  });

  test.showMenu(null, function () {
    if (++callbacks == 2) test.done();
  });
};


exports.testMultipleContexts = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({
    label: "item",
    context: [loader.cm.SelectorContext("a[href]"), loader.cm.PageContext()],
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#span-link", function (popup) {
      test.checkMenu([item], [item], []);
      test.done();
    });
  });
};


exports.testRemoveContext = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let ctxt = loader.cm.SelectorContext("img");
  let item = new loader.cm.Item({
    label: "item",
    context: ctxt
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {

      
      test.checkMenu([item], [], []);
      popup.hidePopup();

      
      item.context.remove(ctxt);
      test.showMenu("#image", function (popup) {
        test.checkMenu([item], [item], []);
        test.done();
      });
    });
  });
};


exports.testSetContextRemove = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let ctxt = loader.cm.SelectorContext("img");
  let item = new loader.cm.Item({
    label: "item",
    context: ctxt
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {

      
      test.checkMenu([item], [], []);
      popup.hidePopup();

      
      item.context = [];
      test.showMenu("#image", function (popup) {
        test.checkMenu([item], [item], []);
        test.done();
      });
    });
  });
};


exports.testAddContext = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let ctxt = loader.cm.SelectorContext("img");
  let item = new loader.cm.Item({
    label: "item"
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {

      
      test.checkMenu([item], [item], []);
      popup.hidePopup();

      
      item.context.add(ctxt);
      test.showMenu("#image", function (popup) {
        test.checkMenu([item], [], []);
        test.done();
      });
    });
  });
};


exports.testSetContextAdd = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let ctxt = loader.cm.SelectorContext("img");
  let item = new loader.cm.Item({
    label: "item"
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {

      
      test.checkMenu([item], [item], []);
      popup.hidePopup();

      
      item.context = [ctxt];
      test.showMenu("#image", function (popup) {
        test.checkMenu([item], [], []);
        test.done();
      });
    });
  });
};


exports.testOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testUnload = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleModulesAdd = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testMultipleModulesAddOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
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






exports.testMultipleModulesDiffContexts1 = function (assert, done) {
  let test = new TestHelper(assert, done);
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






exports.testMultipleModulesDiffContexts2 = function (assert, done) {
  let test = new TestHelper(assert, done);
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






exports.testMultipleModulesDiffContexts3 = function (assert, done) {
  let test = new TestHelper(assert, done);
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






exports.testMultipleModulesDiffContexts4 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleModulesAddRemove = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleModulesOrder = function (assert, done) {
  let test = new TestHelper(assert, done);
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





exports.testMultipleModulesOrderOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleModulesOverflowHidden = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testMultipleModulesOverflowHidden2 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testOverflowIgnoresHidden = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testOverflowIgnoresHiddenMultipleModules1 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testOverflowIgnoresHiddenMultipleModules2 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testOverflowIgnoresHiddenMultipleModules3 = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testOverflowTransition = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#link", function (popup) {
      
      test.checkMenu(allItems, [], []);
      popup.hidePopup();

      test.showMenu("#text", function (popup) {
        
        test.checkMenu(allItems, aItems, []);
        popup.hidePopup();

        test.showMenu(null, function (popup) {
          
          test.checkMenu(allItems, allItems, []);
          popup.hidePopup();

          test.showMenu("#text", function (popup) {
            
            test.checkMenu(allItems, aItems, []);
            popup.hidePopup();

            test.showMenu("#link", function (popup) {
              
              test.checkMenu(allItems, [], []);
              popup.hidePopup();

              test.showMenu(null, function (popup) {
                
                test.checkMenu(allItems, allItems, []);
                popup.hidePopup();

                test.showMenu("#link", function (popup) {
                  
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



exports.testItemCommand = function (assert, done) {
  let test = new TestHelper(assert, done);
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
      assert.equal(this, item, "`this` inside onMessage should be item");
      assert.equal(data.tagName, "HTML", "node should be an HTML element");
      assert.equal(data.data, item.data, "data should be item data");
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






exports.testMenuCommand = function (assert, done) {
  
  
  
  let test = new TestHelper(assert, done);
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
      assert.equal(this, topMenu, "`this` inside top menu should be menu");
      assert.equal(data.tagName, "A", "Clicked node should be anchor");
      assert.equal(data.data, item.data,
                       "Clicked item data should be correct");
      test.done();
    },
    items: [submenu],
    context: loader.cm.SelectorContext("a")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#span-link", function (popup) {
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



exports.testItemCommandMultipleModules = function (assert, done) {
  let test = new TestHelper(assert, done);
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





exports.testItemClick = function (assert, done) {
  let test = new TestHelper(assert, done);
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
      assert.equal(this, item, "`this` inside onMessage should be item");
      assert.equal(data.tagName, "HTML", "node should be an HTML element");
      assert.equal(data.data, item.data, "data should be item data");
      test.done();
    }
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    let elt = test.getItemElt(popup, item);
    elt.click();
  });
};






exports.testMenuClick = function (assert, done) {
  
  
  
  let test = new TestHelper(assert, done);
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
      assert.equal(this, topMenu, "`this` inside top menu should be menu");
      assert.equal(data.tagName, "A", "Clicked node should be anchor");
      assert.equal(data.data, item.data,
                       "Clicked item data should be correct");
      test.done();
    },
    items: [submenu],
    context: loader.cm.SelectorContext("a")
  });

  test.withTestDoc(function (window, doc) {
    test.showMenu("#span-link", function (popup) {
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


exports.testItemClickMultipleModules = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSeparator = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testParentMenu = function (assert, done) {
  let test = new TestHelper(assert, done);
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

  assert.equal(menu.items[1], item, "Item should be in the sub menu");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testNewWindow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.withNewWindow(function () {
    test.showMenu(null, function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};




exports.testNewWindowMultipleModules = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testNewPrivateWindow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    popup.hidePopup();

    test.withNewPrivateWindow(function () {
      test.showMenu(null, function (popup) {
        test.checkMenu([], [], []);
        test.done();
      });
    });
  });
};




exports.testNewPrivateEnabledWindow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newPrivateLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    popup.hidePopup();

    test.withNewPrivateWindow(function () {
      test.showMenu(null, function (popup) {
        test.checkMenu([item], [], []);
        test.done();
      });
    });
  });
};




exports.testNewPrivateEnabledWindowUnloaded = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newPrivateLoader();

  let item = new loader.cm.Item({ label: "item" });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    popup.hidePopup();

    loader.unload();

    test.withNewPrivateWindow(function () {
      test.showMenu(null, function (popup) {
        test.checkMenu([], [], []);
        test.done();
      });
    });
  });
};



exports.testSorting = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSortingOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSortingMultipleModules = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testContentCommunication = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    assert.equal(data, "potato", "That's a lot of potatoes!");
    test.done();
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    let elt = test.getItemElt(popup, item);
    elt.click();
  });
};





exports.testLoadWithOpenTab = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testDrawImageOnClickNode = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu([item], [], []);
      test.getItemElt(popup, item).click();
    });
  });
};




exports.testSetLabelBeforeShow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ]
  items[0].label = "z";
  assert.equal(items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};




exports.testSetLabelAfterShow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ];

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    popup.hidePopup();

    items[0].label = "z";
    assert.equal(items[0].label, "z");
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};




exports.testSetLabelBeforeShowOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let prefs = loader.loader.require("sdk/preferences/service");
  prefs.set(OVERFLOW_THRESH_PREF, 0);

  let items = [
    new loader.cm.Item({ label: "a" }),
    new loader.cm.Item({ label: "b" })
  ]
  items[0].label = "z";
  assert.equal(items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu(items, [], []);
    test.done();
  });
};




exports.testSetLabelAfterShowOverflow = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    assert.equal(items[0].label, "z");
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testSetLabelMenuItem = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [loader.cm.Item({ label: "a" })]
  });
  menu.items[0].label = "z";

  assert.equal(menu.items[0].label, "z");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuAddItem = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: [
      loader.cm.Item({ label: "item 0" })
    ]
  });
  menu.addItem(loader.cm.Item({ label: "item 1" }));
  menu.addItem(loader.cm.Item({ label: "item 2" }));

  assert.equal(menu.items.length, 3,
                   "menu should have correct number of items");
  for (let i = 0; i < 3; i++) {
    assert.equal(menu.items[i].label, "item " + i,
                     "item label should be correct");
    assert.equal(menu.items[i].parentMenu, menu,
                     "item's parent menu should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuAddItemTwice = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let menu = loader.cm.Menu({
    label: "menu",
    items: []
  });
  let subitem = loader.cm.Item({ label: "item 1" })
  menu.addItem(subitem);
  menu.addItem(loader.cm.Item({ label: "item 0" }));
  menu.addItem(subitem);

  assert.equal(menu.items.length, 2,
                   "menu should have correct number of items");
  for (let i = 0; i < 2; i++) {
    assert.equal(menu.items[i].label, "item " + i,
                     "item label should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuRemoveItem = function (assert, done) {
  let test = new TestHelper(assert, done);
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

  assert.equal(subitem.parentMenu, null,
                   "item's parent menu should be correct");

  assert.equal(menu.items.length, 2,
                   "menu should have correct number of items");
  assert.equal(menu.items[0].label, "item 0",
                   "item label should be correct");
  assert.equal(menu.items[1].label, "item 2",
                   "item label should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testMenuItemSwap = function (assert, done) {
  let test = new TestHelper(assert, done);
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

  assert.equal(menu0.items.length, 0,
                   "menu should have correct number of items");

  assert.equal(menu1.items.length, 1,
                   "menu should have correct number of items");
  assert.equal(menu1.items[0].label, "item",
                   "item label should be correct");

  assert.equal(subitem.parentMenu, menu1,
                   "item's parent menu should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu0, menu1], [menu0], []);
    test.done();
  });
};



exports.testMenuItemDestroy = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let subitem = loader.cm.Item({ label: "item" });
  let menu = loader.cm.Menu({
    label: "menu",
    items: [subitem]
  });
  subitem.destroy();

  assert.equal(menu.items.length, 0,
                   "menu should have correct number of items");
  assert.equal(subitem.parentMenu, null,
                   "item's parent menu should be correct");

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [menu], []);
    test.done();
  });
};



exports.testMenuItemsSetter = function (assert, done) {
  let test = new TestHelper(assert, done);
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

  assert.equal(menu.items.length, 3,
                   "menu should have correct number of items");
  for (let i = 0; i < 3; i++) {
    assert.equal(menu.items[i].label, "new item " + i,
                     "item label should be correct");
    assert.equal(menu.items[i].parentMenu, menu,
                     "item's parent menu should be correct");
  }

  test.showMenu(null, function (popup) {
    test.checkMenu([menu], [], []);
    test.done();
  });
};



exports.testItemDataSetter = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = loader.cm.Item({ label: "old item 0", data: "old" });
  item.data = "new";

  assert.equal(item.data, "new", "item should have correct data");

  test.showMenu(null, function (popup) {
    test.checkMenu([item], [], []);
    test.done();
  });
};




exports.testAlreadyOpenIframe = function (assert, done) {
  let test = new TestHelper(assert, done);
  test.withTestDoc(function (window, doc) {
    let loader = test.newLoader();
    let item = new loader.cm.Item({
      label: "item"
    });
    test.showMenu("#iframe", function (popup) {
      test.checkMenu([item], [], []);
      test.done();
    });
  });
};



exports.testItemNoLabel = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  try {
    new loader.cm.Item({});
    assert.ok(false, "Should have seen exception");
  }
  catch (e) {
    assert.ok(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: null });
    assert.ok(false, "Should have seen exception");
  }
  catch (e) {
    assert.ok(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: undefined });
    assert.ok(false, "Should have seen exception");
  }
  catch (e) {
    assert.ok(true, "Should have seen exception");
  }

  try {
    new loader.cm.Item({ label: "" });
    assert.ok(false, "Should have seen exception");
  }
  catch (e) {
    assert.ok(true, "Should have seen exception");
  }

  test.done();
}



exports.testItemNoData = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  function checkData(data) {
    assert.equal(data, undefined, "Data should be undefined");
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

  assert.equal(item1.data, undefined, "Should be no defined data");
  assert.equal(item2.data, null, "Should be no defined data");
  assert.equal(item3.data, undefined, "Should be no defined data");

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


exports.testItemNoAccessKey = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item1 = new loader.cm.Item({ label: "item 1" });
  let item2 = new loader.cm.Item({ label: "item 2", accesskey: null });
  let item3 = new loader.cm.Item({ label: "item 3", accesskey: undefined });

  assert.equal(item1.accesskey, undefined, "Should be no defined image");
  assert.equal(item2.accesskey, null, "Should be no defined image");
  assert.equal(item3.accesskey, undefined, "Should be no defined image");

  test.showMenu().
  then((popup) => test.checkMenu([item1, item2, item3], [], [])).
  then(test.done).
  catch(assert.fail);
}



exports.testItemAccessKey = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item = new loader.cm.Item({ label: "item", accesskey: "i" });
  assert.equal(item.accesskey, "i", "Should have set the image to i");

  let menu = new loader.cm.Menu({ label: "menu", accesskey: "m", items: [
    loader.cm.Item({ label: "subitem" })
  ]});
  assert.equal(menu.accesskey, "m", "Should have set the accesskey to m");

  test.showMenu().then((popup) => {
    test.checkMenu([item, menu], [], []);

    let accesskey = "e";
    menu.accesskey = item.accesskey = accesskey;
    assert.equal(item.accesskey, accesskey, "Should have set the accesskey to " + accesskey);
    assert.equal(menu.accesskey, accesskey, "Should have set the accesskey to " + accesskey);
    test.checkMenu([item, menu], [], []);

    item.accesskey = null;
    menu.accesskey = null;
    assert.equal(item.accesskey, null, "Should have set the accesskey to " + accesskey);
    assert.equal(menu.accesskey, null, "Should have set the accesskey to " + accesskey);
    test.checkMenu([item, menu], [], []);
  }).
  then(test.done).
  catch(assert.fail);
};



exports.testItemNoImage = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let item1 = new loader.cm.Item({ label: "item 1" });
  let item2 = new loader.cm.Item({ label: "item 2", image: null });
  let item3 = new loader.cm.Item({ label: "item 3", image: undefined });

  assert.equal(item1.image, undefined, "Should be no defined image");
  assert.equal(item2.image, null, "Should be no defined image");
  assert.equal(item3.image, undefined, "Should be no defined image");

  test.showMenu(null, function (popup) {
    test.checkMenu([item1, item2, item3], [], []);

    test.done();
  });
}



exports.testItemImage = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let imageURL = data.url("moz_favicon.ico");
  let item = new loader.cm.Item({ label: "item", image: imageURL });
  let menu = new loader.cm.Menu({ label: "menu", image: imageURL, items: [
    loader.cm.Item({ label: "subitem" })
  ]});
  assert.equal(item.image, imageURL, "Should have set the image correctly");
  assert.equal(menu.image, imageURL, "Should have set the image correctly");

  test.showMenu(null, function (popup) {
    test.checkMenu([item, menu], [], []);

    let imageURL2 = data.url("dummy.ico");
    item.image = imageURL2;
    menu.image = imageURL2;
    assert.equal(item.image, imageURL2, "Should have set the image correctly");
    assert.equal(menu.image, imageURL2, "Should have set the image correctly");
    test.checkMenu([item, menu], [], []);

    item.image = null;
    menu.image = null;
    assert.equal(item.image, null, "Should have set the image correctly");
    assert.equal(menu.image, null, "Should have set the image correctly");
    test.checkMenu([item, menu], [], []);

    test.done();
  });
};


exports.testItemImageValidURL = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  assert.throws(function(){
      new loader.cm.Item({
        label: "item 1",
        image: "foo"
      })
    }, /Image URL validation failed/
  );

  assert.throws(function(){
      new loader.cm.Item({
        label: "item 2",
        image: false
      })
    }, /Image URL validation failed/
  );

  assert.throws(function(){
      new loader.cm.Item({
        label: "item 3",
        image: 0
      })
    }, /Image URL validation failed/
  );

  let imageURL = data.url("moz_favicon.ico");
  let item4 = new loader.cm.Item({ label: "item 4", image: imageURL });
  let item5 = new loader.cm.Item({ label: "item 5", image: null });
  let item6 = new loader.cm.Item({ label: "item 6", image: undefined });

  assert.equal(item4.image, imageURL, "Should be proper image URL");
  assert.equal(item5.image, null, "Should be null image");
  assert.equal(item6.image, undefined, "Should be undefined image");

  test.done();
};



exports.testMenuDestroy = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSubItemContextNoMatchHideMenu = function (assert, done) {
  let test = new TestHelper(assert, done);
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




exports.testSubItemContextMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSubItemDefaultVisible = function (assert, done) {
  let test = new TestHelper(assert, done);
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
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, hiddenItems, []);
      test.done();
    });
  });
};



exports.testSubItemClick = function (assert, done) {
  let test = new TestHelper(assert, done);
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
            assert.equal(msg.tagName, "HTML", "should have seen the right node");
            assert.equal(msg.data, "foobar", "should have seen the right data");
            assert.equal(state, 0, "should have seen the event at the right time");
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
        assert.equal(msg.tagName, "HTML", "should have seen the right node");
        assert.equal(msg.data, "foobar", "should have seen the right data");
        assert.equal(state, 1, "should have seen the event at the right time");

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



exports.testSubItemCommand = function (assert, done) {
  let test = new TestHelper(assert, done);
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
            assert.equal(msg.tagName, "HTML", "should have seen the right node");
            assert.equal(msg.data, "foobar", "should have seen the right data");
            assert.equal(state, 0, "should have seen the event at the right time");
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
        assert.equal(msg.tagName, "HTML", "should have seen the right node");
        assert.equal(msg.data, "foobar", "should have seen the right data");
        assert.equal(state, 1, "should have seen the event at the right time");
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



exports.testSelectionInInnerFrameNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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



exports.testSelectionInInnerFrameMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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

    test.showMenu(["#iframe", "#text"], function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testSelectionInOuterFrameNoMatch = function (assert, done) {
  let test = new TestHelper(assert, done);
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

    test.showMenu(["#iframe", "#text"], function (popup) {
      test.checkMenu(items, items, []);
      test.done();
    });
  });
};




exports.testPredicateContextControl = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let itemTrue = loader.cm.Item({
    label: "visible",
    context: loader.cm.PredicateContext(function () { return true; })
  });

  let itemFalse = loader.cm.Item({
    label: "hidden",
    context: loader.cm.PredicateContext(function () { return false; })
  });

  test.showMenu(null, function (popup) {
    test.checkMenu([itemTrue, itemFalse], [itemFalse], []);
    test.done();
  });
};


exports.testPredicateContextDocumentType = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.equal(data.documentType, 'text/html');
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextDocumentURL = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.equal(data.documentURL, TEST_DOC_URL);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextTargetName = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.targetName, "input");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#button", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextTargetIDSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.targetID, "button");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#button", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetIDNotSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.targetID, null);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu(".predicate-test-a", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTextBoxIsEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, true);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#textbox", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextReadonlyTextBoxIsNotEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, false);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#readonly-textbox", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextDisabledTextBoxIsNotEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, false);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#disabled-textbox", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTextAreaIsEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, true);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#textfield", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextButtonIsNotEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, false);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#button", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextNonInputIsNotEditable = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, false);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextEditableElement = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.isEditable, true);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#editable", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextNoSelectionInPage = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.selectionText, null);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextSelectionInPage = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      
      assert.ok(data.selectionText && data.selectionText.search(/^\s*Some text.\s*$/) != -1,
		'Expected "Some text.", got "' + data.selectionText + '"');
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    window.getSelection().selectAllChildren(doc.getElementById("text"));
    test.showMenu(null, function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextSelectionInTextBox = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      
      assert.strictEqual(data.selectionText, "t v");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    let textbox = doc.getElementById("textbox");
    test.selectRange("#textbox", 3, 6);
    test.showMenu("#textbox", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetSrcSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();
  let image;

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.srcURL, image.src);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    image = doc.getElementById("image");
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetSrcNotSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.srcURL, null);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#link", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};



exports.testPredicateContextTargetLinkSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();
  let image;

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.linkURL, TEST_DOC_URL + "#test");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu(".predicate-test-a", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetLinkNotSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.linkURL, null);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetLinkSetNestedImage = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.linkURL, TEST_DOC_URL + "#nested-image");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#predicate-test-nested-image", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetLinkSetNestedStructure = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.linkURL, TEST_DOC_URL + "#nested-structure");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#predicate-test-nested-structure", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetValueSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();
  let image;

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.value, "test value");
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#textbox", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};


exports.testPredicateContextTargetValueNotSet = function (assert, done) {
  let test = new TestHelper(assert, done);
  let loader = test.newLoader();

  let items = [loader.cm.Item({
    label: "item",
    context: loader.cm.PredicateContext(function (data) {
      assert.strictEqual(data.value, null);
      return true;
    })
  })];

  test.withTestDoc(function (window, doc) {
    test.showMenu("#image", function (popup) {
      test.checkMenu(items, [], []);
      test.done();
    });
  });
};

if (packaging.isNative) {
  module.exports = {
    "test skip on jpm": (assert) => assert.pass("skipping this file with jpm")
  };
}

require('sdk/test').run(exports);
