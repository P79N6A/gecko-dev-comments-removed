


 "use strict";

const { Ci } = require("chrome");
const { getMostRecentBrowserWindow } = require('sdk/window/utils');
const { Loader } = require('sdk/test/loader');
const { merge } = require("sdk/util/object");
const observers = require("sdk/system/events");
const { defer } = require("sdk/core/promise");
const timer = require("sdk/timers");


const ITEM_CLASS = "addon-context-menu-item";
const SEPARATOR_CLASS = "addon-context-menu-separator";
const OVERFLOW_THRESH_DEFAULT = 10;
const OVERFLOW_THRESH_PREF =
  "extensions.addon-sdk.context-menu.overflowThreshold";
const OVERFLOW_MENU_CLASS = "addon-content-menu-overflow-menu";
const OVERFLOW_POPUP_CLASS = "addon-content-menu-overflow-popup";

const TEST_DOC_URL = module.uri.replace(/context-menu\/test-helper\.js$/, "test-context-menu.html");






function TestHelper(assert, done) {
  this.assert = assert;
  this.end = done;
  this.loaders = [];
  this.browserWindow = getMostRecentBrowserWindow();
  this.overflowThreshValue = require("sdk/preferences/service").
                             get(OVERFLOW_THRESH_PREF, OVERFLOW_THRESH_DEFAULT);
  this.done = this.done.bind(this);
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
    this.assert[methodName].apply(this.assert, args);
  },

  
  checkItemElt: function (elt, item) {
    let itemType = this.getItemType(item);

    switch (itemType) {
    case "Item":
      this.assert.equal(elt.localName, "menuitem",
                            "Item DOM element should be a xul:menuitem");
      if (typeof(item.data) === "string") {
        this.assert.equal(elt.getAttribute("value"), item.data,
                              "Item should have correct data");
      }
      break
    case "Menu":
      this.assert.equal(elt.localName, "menu",
                            "Menu DOM element should be a xul:menu");
      let subPopup = elt.firstChild;
      this.assert.ok(subPopup, "xul:menu should have a child");
      this.assert.equal(subPopup.localName, "menupopup",
                            "xul:menu's first child should be a menupopup");
      break;
    case "Separator":
      this.assert.equal(elt.localName, "menuseparator",
                         "Separator DOM element should be a xul:menuseparator");
      break;
    }

    if (itemType === "Item" || itemType === "Menu") {
      this.assert.equal(elt.getAttribute("label"), item.label,
                            "Item should have correct title");

      
      if (item.accesskey) {
        this.assert.equal(elt.getAttribute("accesskey"),
                          item.accesskey,
                          "Item should have correct accesskey");
      }
      else {
        this.assert.equal(elt.getAttribute("accesskey"),
                          "",
                          "Item should not have accesskey");
      }

      
      if (typeof(item.image) === "string") {
        this.assert.equal(elt.getAttribute("image"), item.image,
                              "Item should have correct image");
        if (itemType === "Menu")
          this.assert.ok(elt.classList.contains("menu-iconic"),
                           "Menus with images should have the correct class")
        else
          this.assert.ok(elt.classList.contains("menuitem-iconic"),
                           "Items with images should have the correct class")
      }
      else {
        this.assert.ok(!elt.getAttribute("image"),
                         "Item should not have image");
        this.assert.ok(!elt.classList.contains("menu-iconic") && !elt.classList.contains("menuitem-iconic"),
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
      this.assert.ok(!separator || separator.hidden,
                       "separator should not be present");
    }
    else {
      this.assert.ok(separator && !separator.hidden,
                       "separator should be present");
    }

    let mainNodes = this.browserWindow.document.querySelectorAll("#contentAreaContextMenu > ." + ITEM_CLASS);
    let overflowNodes = this.browserWindow.document.querySelectorAll("." + OVERFLOW_POPUP_CLASS + " > ." + ITEM_CLASS);

    this.assert.ok(mainNodes.length == 0 || overflowNodes.length == 0,
                     "Should only see nodes at the top level or in overflow");

    let overflow = this.overflowSubmenu;
    if (this.shouldOverflow(total)) {
      this.assert.ok(overflow && !overflow.hidden,
                       "overflow menu should be present");
      this.assert.equal(mainNodes.length, 0,
                            "should be no items in the main context menu");
    }
    else {
      this.assert.ok(!overflow || overflow.hidden,
                       "overflow menu should not be present");
      
      if (total > 0) {
        this.assert.equal(overflowNodes.length, 0,
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
        this.assert.ok(false, "Not enough nodes");
        return;
      }

      let hidden = absentItems.indexOf(item) >= 0;

      this.checkItemElt(nodes[pos], item);
      this.assert.equal(nodes[pos].hidden, hidden,
                            "hidden should be set correctly");

      
      if (!hidden && this.getItemType(item) == "Menu") {
        this.assert.equal(nodes[pos].firstChild.localName, "menupopup",
                              "menu XUL should contain a menupopup");
        this.checkNodes(nodes[pos].firstChild.childNodes, item.items, absentItems, removedItems);
      }

      if (pos > 0)
        this.assert.equal(nodes[pos].previousSibling, nodes[pos - 1],
                              "nodes should all be in the same group");
      pos++;
    }

    this.assert.equal(nodes.length, pos,
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
          self.assert.fail(err);
          self.end();
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

      this.end();
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
    const selfModule = require('sdk/self');
    let loader = Loader(module, null, null, {
      modules: {
        "sdk/self": merge({}, selfModule, {
          data: merge({}, selfModule.data, require("../fixtures"))
        })
      }
    });

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

  
  newPrivateLoader: function() {
    let base = require("@loader/options");

    
    let options = merge({}, base, {
      metadata: merge({}, base.metadata || {}, {
        permissions: merge({}, base.metadata.permissions || {}, {
          'private-browsing': true
        })
      })
    });

    const self = this;
    let loader = Loader(module, null, options);
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

  
  loadFrameScript: function(browser = this.browserWindow.gBrowser.selectedBrowser) {
    function frame_script() {
      let { interfaces: Ci } = Components;
      addMessageListener('test:contextmenu', ({ data: { selectors } }) => {
        let targetNode = null;
        let contentWin = content;
        if (selectors) {
          while (selectors.length) {
            targetNode = contentWin.document.querySelector(selectors.shift());
            if (selectors.length)
              contentWin = targetNode.contentWindow;
          }
        }

        let rect = targetNode ?
                   targetNode.getBoundingClientRect() :
                   { left: 0, top: 0, width: 0, height: 0 };
        contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils)
                  .sendMouseEvent('contextmenu',
                  rect.left + (rect.width / 2),
                  rect.top + (rect.height / 2),
                  2, 1, 0);
      });

      addMessageListener('test:ping', () => {
        sendAsyncMessage('test:pong');
      });

      addMessageListener('test:select', ({ data: { selector, start, end } }) => {
        let element = content.document.querySelector(selector);
        element.focus();
        if (end === null)
          end = element.value.length;
        element.setSelectionRange(start, end);
      });
    }

    let messageManager = browser.messageManager;
    messageManager.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);
  },

  selectRange: function(selector, start, end) {
    let messageManager = this.browserWindow.gBrowser.selectedBrowser.messageManager;
    messageManager.sendAsyncMessage('test:select', { selector, start, end });
  },

  
  
  
  
  
  
  showMenu: function(selectors, onshownCallback) {
    let { promise, resolve } = defer();

    if (selectors && !Array.isArray(selectors))
      selectors = [selectors];

    let sendEvent = () => {
      let menu = this.browserWindow.document.getElementById("contentAreaContextMenu");
      this.delayedEventListener(menu, "popupshowing",
        function (e) {
          let popup = e.target;
          if (onshownCallback) {
            onshownCallback.call(this, popup);
          }
          resolve(popup);
        }, false);

      let messageManager = this.browserWindow.gBrowser.selectedBrowser.messageManager;
      messageManager.sendAsyncMessage('test:contextmenu', { selectors });
    }

    
    
    
    let flushMessages = () => {
      let listener = () => {
        messageManager.removeMessageListener('test:pong', listener);
        sendEvent();
      };

      let messageManager = this.browserWindow.gBrowser.selectedBrowser.messageManager;
      messageManager.addMessageListener('test:pong', listener);
      messageManager.sendAsyncMessage('test:ping');
    }

    
    
    
    if (!selectors && !this.oldSelectedTab && !this.oldBrowserWindow) {
      this.oldSelectedTab = this.tabBrowser.selectedTab;
      this.tab = this.tabBrowser.addTab("about:blank");
      let browser = this.tabBrowser.getBrowserForTab(this.tab);

      this.delayedEventListener(browser, "load", function () {
        this.tabBrowser.selectedTab = this.tab;
        this.loadFrameScript();
        flushMessages();
      }, true);
    }
    else {
      flushMessages();
    }

    return promise;
  },

  hideMenu: function(onhiddenCallback) {
    this.delayedEventListener(this.browserWindow, "popuphidden", onhiddenCallback);

    this.contextMenuPopup.hidePopup();
  },

  
  
  withNewWindow: function (onloadCallback, makePrivate = false) {
    let win = this.browserWindow.OpenBrowserWindow({ private: makePrivate });
    observers.once("browser-delayed-startup-finished", () => {
      
      win.gBrowser.selectedTab = win.gBrowser.addTab();
      this.loadFrameScript();
      this.delayedEventListener(win.gBrowser.selectedBrowser, "load", onloadCallback, true);
    });
    this.oldBrowserWindow = this.browserWindow;
    this.browserWindow = win;
  },

  
  
  withNewPrivateWindow: function (onloadCallback) {
    this.withNewWindow(onloadCallback, true);
  },

  
  
  withTestDoc: function (onloadCallback) {
    this.oldSelectedTab = this.tabBrowser.selectedTab;
    this.tab = this.tabBrowser.addTab(TEST_DOC_URL);
    let browser = this.tabBrowser.getBrowserForTab(this.tab);

    this.delayedEventListener(browser, "load", function () {
      this.tabBrowser.selectedTab = this.tab;
      this.loadFrameScript();
      onloadCallback.call(this, browser.contentWindow, browser.contentDocument);
    }, true, function(evt) {
      return evt.target.location == TEST_DOC_URL;
    });
  }
};
exports.TestHelper = TestHelper;
