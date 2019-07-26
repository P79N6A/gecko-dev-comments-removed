



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["CustomizableWidgets"];

Cu.import("resource:///modules/CustomizableUI.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

const kNSXUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const kPrefCustomizationDebug = "browser.uiCustomization.debug";

let gModuleName = "[CustomizableWidgets]";
#include logging.js

function setAttributes(aNode, aAttrs) {
  for (let [name, value] of Iterator(aAttrs)) {
    if (!value) {
      if (aNode.hasAttribute(name))
        aNode.removeAttribute(name);
    } else {
      if (name == "label" || name == "tooltiptext")
        value = CustomizableUI.getLocalizedProperty(aAttrs, name);
      aNode.setAttribute(name, value);
    }
  }
}

const CustomizableWidgets = [{
    id: "history-panelmenu",
    type: "view",
    viewId: "PanelUI-history",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL, CustomizableUI.AREA_NAVBAR],
    onViewShowing: function(aEvent) {
      
      const kMaxResults = 15;
      let doc = aEvent.detail.ownerDocument;

      let options = PlacesUtils.history.getNewQueryOptions();
      options.excludeQueries = true;
      options.includeHidden = false;
      options.resultType = options.RESULTS_AS_URI;
      options.queryType = options.QUERY_TYPE_HISTORY;
      options.sortingMode = options.SORT_BY_DATE_DESCENDING;
      options.maxResults = kMaxResults;
      let query = PlacesUtils.history.getNewQuery();

      let items = doc.getElementById("PanelUI-historyItems");
      
      while (items.firstChild) {
        items.removeChild(items.firstChild);
      }

      PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                         .asyncExecuteLegacyQueries([query], 1, options, {
        handleResult: function (aResultSet) {
          let fragment = doc.createDocumentFragment();
          for (let row, i = 0; (row = aResultSet.getNextRow()); i++) {
            try {
              let uri = row.getResultByIndex(1);
              let title = row.getResultByIndex(2);
              let icon = row.getResultByIndex(6);

              let item = doc.createElementNS(kNSXUL, "toolbarbutton");
              item.setAttribute("label", title || uri);
              item.addEventListener("click", function(aEvent) {
                if (aEvent.button == 0) {
                  doc.defaultView.openUILink(uri, aEvent);
                  doc.defaultView.PanelUI.hide();
                }
              });
              if (icon)
                item.setAttribute("image", "moz-anno:favicon:" + icon);
              fragment.appendChild(item);
            } catch (e) {
              ERROR("Error while showing history subview: " + e);
            }
          }
          items.appendChild(fragment);
        },
        handleError: function (aError) {
          LOG("History view tried to show but had an error: " + aError);
        },
        handleCompletion: function (aReason) {
          LOG("History view is being shown!");
        },
      });
    },
    onViewHiding: function(aEvent) {
      LOG("History view is being hidden!");
    }
  }, {
    id: "privatebrowsing-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(e) {
      if (e.target && e.target.ownerDocument && e.target.ownerDocument.defaultView) {
        let win = e.target.ownerDocument.defaultView;
        if (typeof win.OpenBrowserWindow == "function") {
          win.OpenBrowserWindow({private: true});
        }
      }
    }
  }, {
    id: "save-page-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target &&
                aEvent.target.ownerDocument &&
                aEvent.target.ownerDocument.defaultView;
      if (win && typeof win.saveDocument == "function") {
        win.saveDocument(win.content.document);
      }
    }
  }, {
    id: "find-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target &&
                aEvent.target.ownerDocument &&
                aEvent.target.ownerDocument.defaultView;
      if (win && win.gFindBar) {
        win.gFindBar.onFindCommand();
      }
    }
  }, {
    id: "open-file-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target
                && aEvent.target.ownerDocument
                && aEvent.target.ownerDocument.defaultView;
      if (win && typeof win.BrowserOpenFileWindow == "function") {
        win.BrowserOpenFileWindow();
      }
    }
  }, {
    id: "developer-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target &&
                aEvent.target.ownerDocument &&
                aEvent.target.ownerDocument.defaultView;
      if (win && win.gDevToolsBrowser) {
        win.gDevToolsBrowser.toggleToolboxCommand(win.gBrowser);
      }
    }
  }, {
    id: "add-ons-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target &&
                aEvent.target.ownerDocument &&
                aEvent.target.ownerDocument.defaultView;
      if (win && typeof win.BrowserOpenAddonsMgr == "function") {
        win.BrowserOpenAddonsMgr();
      }
    }
  }, {
    id: "preferences-button",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL],
    onCommand: function(aEvent) {
      let win = aEvent.target &&
                aEvent.target.ownerDocument &&
                aEvent.target.ownerDocument.defaultView;
      if (win && typeof win.openPreferences == "function") {
        win.openPreferences();
      }
    }
  }, {
    id: "zoom-controls",
    type: "custom",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL, CustomizableUI.AREA_NAVBAR],
    onBuild: function(aDocument) {
      let inPanel = (this.currentArea == CustomizableUI.AREA_PANEL);
      let noautoclose = inPanel ? "true" : null;
      let flex = inPanel ? "1" : null;
      let cls = inPanel ? "panel-combined-button" : "toolbarbutton-1";
      let buttons = [{
        id: "zoom-out-button",
        noautoclose: noautoclose,
        command: "cmd_fullZoomReduce",
        flex: flex,
        class: cls,
        label: true,
        tooltiptext: true
      }, {
        id: "zoom-reset-button",
        noautoclose: noautoclose,
        command: "cmd_fullZoomReset",
        flex: flex,
        class: cls,
        tooltiptext: true
      }, {
        id: "zoom-in-button",
        noautoclose: noautoclose,
        command: "cmd_fullZoomEnlarge",
        flex: flex,
        class: cls,
        label: true,
        tooltiptext: true
      }];

      let node = aDocument.createElementNS(kNSXUL, "toolbaritem");
      node.setAttribute("id", "zoom-controls");
      node.setAttribute("title", CustomizableUI.getLocalizedProperty(this, "tooltiptext"));
      
      node.setAttribute("removable", "true");
      if (inPanel)
        node.setAttribute("flex", "1");
      node.classList.add("chromeclass-toolbar-additional");

      buttons.forEach(function(aButton) {
        let btnNode = aDocument.createElementNS(kNSXUL, "toolbarbutton");
        setAttributes(btnNode, aButton);
        node.appendChild(btnNode);
      });

      
      let zoomResetButton = node.childNodes[1];
      let window = aDocument.defaultView;
      function updateZoomResetButton() {
        zoomResetButton.setAttribute("label", CustomizableUI.getLocalizedProperty(
          buttons[1], "label", [Math.floor(window.ZoomManager.zoom * 100)]
        ));
      };

      
      Services.obs.addObserver(updateZoomResetButton, "browser-fullZoom:zoomChange", false);
      Services.obs.addObserver(updateZoomResetButton, "browser-fullZoom:zoomReset", false);
      Services.obs.addObserver(updateZoomResetButton, "browser-fullZoom:locationChange", false);

      updateZoomResetButton();
      if (!inPanel)
        zoomResetButton.setAttribute("hidden", "true");

      function updateWidgetStyle(aInPanel) {
        let attrs = {
          noautoclose: aInPanel ? "true" : null,
          flex: aInPanel ? "1" : null,
          class: aInPanel ? "panel-combined-button" : "toolbarbutton-1"
        };
        for (let i = 0, l = node.childNodes.length; i < l; ++i) {
          setAttributes(node.childNodes[i], attrs);
        }
        zoomResetButton.setAttribute("hidden", aInPanel ? "false" : "true");
        if (aInPanel)
          node.setAttribute("flex", "1");
        else if (node.hasAttribute("flex"))
          node.removeAttribute("flex");
      }

      let listener = {
        onWidgetAdded: function(aWidgetId, aArea, aPosition) {
          if (aWidgetId != this.id)
            return;

          updateWidgetStyle(aArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetRemoved: function(aWidgetId, aPrevArea) {
          if (aWidgetId != this.id)
            return;

          
          
          updateWidgetStyle(false);
          zoomResetButton.setAttribute("hidden", "true");
        }.bind(this),

        onWidgetReset: function(aWidgetId) {
          if (aWidgetId != this.id)
            return;
          updateWidgetStyle(this.currentArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetMoved: function(aWidgetId, aArea) {
          if (aWidgetId != this.id)
            return;
          updateWidgetStyle(aArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetInstanceRemoved: function(aWidgetId, aDoc) {
          if (aWidgetId != this.id || aDoc != aDocument)
            return;

          CustomizableUI.removeListener(listener);
          Services.obs.removeObserver(updateZoomResetButton, "browser-fullZoom:zoomChange");
          Services.obs.removeObserver(updateZoomResetButton, "browser-fullZoom:zoomReset");
        }.bind(this)
      };
      CustomizableUI.addListener(listener);

      return node;
    }
  }, {
    id: "edit-controls",
    type: "custom",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL, CustomizableUI.AREA_NAVBAR],
    onBuild: function(aDocument) {
      let inPanel = (this.currentArea == CustomizableUI.AREA_PANEL);
      let flex = inPanel ? "1" : null;
      let cls = inPanel ? "panel-combined-button" : "toolbarbutton-1";
      let buttons = [{
        id: "cut-button",
        command: "cmd_cut",
        flex: flex,
        class: cls,
        label: true,
        tooltiptext: true
      }, {
        id: "copy-button",
        command: "cmd_copy",
        flex: flex,
        class: cls,
        label: true,
        tooltiptext: true
      }, {
        id: "paste-button",
        command: "cmd_paste",
        flex: flex,
        class: cls,
        label: true,
        tooltiptext: true
      }];

      let node = aDocument.createElementNS(kNSXUL, "toolbaritem");
      node.setAttribute("id", "edit-controls");
      node.setAttribute("title", CustomizableUI.getLocalizedProperty(this, "tooltiptext"));
      
      node.setAttribute("removable", "true");
      if (inPanel)
        node.setAttribute("flex", "1");
      node.classList.add("chromeclass-toolbar-additional");

      buttons.forEach(function(aButton) {
        let btnNode = aDocument.createElementNS(kNSXUL, "toolbarbutton");
        setAttributes(btnNode, aButton);
        node.appendChild(btnNode);
      });

      function updateWidgetStyle(aInPanel) {
        let attrs = {
          flex: aInPanel ? "1" : null,
          class: aInPanel ? "panel-combined-button" : "toolbarbutton-1"
        };
        for (let i = 0, l = node.childNodes.length; i < l; ++i) {
          setAttributes(node.childNodes[i], attrs);
        }
        if (aInPanel)
          node.setAttribute("flex", "1");
        else if (node.hasAttribute("flex"))
          node.removeAttribute("flex");
      }

      let listener = {
        onWidgetAdded: function(aWidgetId, aArea, aPosition) {
          if (aWidgetId != this.id)
            return;
          updateWidgetStyle(aArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetRemoved: function(aWidgetId, aPrevArea) {
          if (aWidgetId != this.id)
            return;
          
          
          updateWidgetStyle(false);
        }.bind(this),

        onWidgetReset: function(aWidgetId) {
          if (aWidgetId != this.id)
            return;
          updateWidgetStyle(this.currentArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetMoved: function(aWidgetId, aArea) {
          if (aWidgetId != this.id)
            return;
          updateWidgetStyle(aArea == CustomizableUI.AREA_PANEL);
        }.bind(this),

        onWidgetInstanceRemoved: function(aWidgetId, aDoc) {
          if (aWidgetId != this.id || aDoc != aDocument)
            return;
          CustomizableUI.removeListener(listener);
        }.bind(this)
      };
      CustomizableUI.addListener(listener);

      return node;
    }
  },
  {
    id: "feed-button",
    type: "view",
    viewId: "PanelUI-feeds",
    removable: true,
    defaultArea: CustomizableUI.AREA_PANEL,
    allowedAreas: [CustomizableUI.AREA_PANEL, CustomizableUI.AREA_NAVBAR],
    onClick: function(aEvent) {
      let win = aEvent.target.ownerDocument.defaultView;
      let feeds = win.gBrowser.selectedBrowser.feeds;

      
      
      let isClick = (aEvent.button == 0 || aEvent.button == 1);
      if (feeds && feeds.length == 1 && isClick) {
        aEvent.preventDefault();
        aEvent.stopPropagation();
        win.FeedHandler.subscribeToFeed(feeds[0].href, aEvent);
      }
    },
    onViewShowing: function(aEvent) {
      let doc = aEvent.detail.ownerDocument;
      let container = doc.getElementById("PanelUI-feeds");
      let gotView = doc.defaultView.FeedHandler.buildFeedList(container, true);

      
      if (!gotView) {
        aEvent.preventDefault();
        aEvent.stopPropagation();
        return;
      }
    },
    onCreated: function(node) {
      let win = node.ownerDocument.defaultView;
      let selectedBrowser = win.gBrowser.selectedBrowser;
      let feeds = selectedBrowser && selectedBrowser.feeds;
      if (!feeds || !feeds.length) {
        node.setAttribute("disabled", "true");
      }
    }
  }];
