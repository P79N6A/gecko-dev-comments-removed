# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Tab View
#
# The Initial Developer of the Original Code is Mozilla Foundation.
# Portions created by the Initial Developer are Copyright (C) 2010
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Raymond Lee <raymond@appcoast.com>
#   Ian Gilman <ian@iangilman.com>
#   Tim Taubert <tim.taubert@gmx.de>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

let TabView = {
  _deck: null,
  _iframe: null,
  _window: null,
  _browserKeyHandlerInitialized: false,
  _isFrameLoading: false,
  _initFrameCallbacks: [],
  PREF_BRANCH: "browser.panorama.",
  PREF_FIRST_RUN: "browser.panorama.experienced_first_run",
  PREF_STARTUP_PAGE: "browser.startup.page",
  PREF_RESTORE_ENABLED_ONCE: "browser.panorama.session_restore_enabled_once",
  VISIBILITY_IDENTIFIER: "tabview-visibility",
  GROUPS_IDENTIFIER: "tabview-groups",

  
  get windowTitle() {
    delete this.windowTitle;
    let brandBundle = document.getElementById("bundle_brand");
    let brandShortName = brandBundle.getString("brandShortName");
    let title = gNavigatorBundle.getFormattedString("tabView2.title", [brandShortName]);
    return this.windowTitle = title;
  },

  
  get firstUseExperienced() {
    let pref = this.PREF_FIRST_RUN;
    if (Services.prefs.prefHasUserValue(pref))
      return Services.prefs.getBoolPref(pref);

    return false;
  },

  
  set firstUseExperienced(val) {
    Services.prefs.setBoolPref(this.PREF_FIRST_RUN, val);
  },

  
  get sessionRestoreEnabledOnce() {
    let pref = this.PREF_RESTORE_ENABLED_ONCE;
    if (Services.prefs.prefHasUserValue(pref))
      return Services.prefs.getBoolPref(pref);

    return false;
  },

  
  set sessionRestoreEnabledOnce(val) {
    Services.prefs.setBoolPref(this.PREF_RESTORE_ENABLED_ONCE, val);
  },

  
  init: function TabView_init() {
    if (this.firstUseExperienced) {
      if ((gBrowser.tabs.length - gBrowser.visibleTabs.length) > 0)
        this._setBrowserKeyHandlers();

      
      let sessionstore =
        Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);

      let data = sessionstore.getWindowValue(window, this.VISIBILITY_IDENTIFIER);
      if (data && data == "true") {
        this.show();
      } else {
        try {
          data = sessionstore.getWindowValue(window, this.GROUPS_IDENTIFIER);
          if (data) {
            let parsedData = JSON.parse(data);
            this.updateGroupNumberBroadcaster(parsedData.totalNumber || 0);
          }
        } catch (e) { }

        let self = this;
        
        
        this._tabShowEventListener = function (event) {
          if (!self._window)
            self._initFrame(function() {
              self._window.UI.onTabSelect(gBrowser.selectedTab);
            });
        };
        gBrowser.tabContainer.addEventListener(
          "TabShow", this._tabShowEventListener, true);
      }
    }

    Services.prefs.addObserver(this.PREF_BRANCH, this, false);
  },

  
  
  observe: function TabView_observe(subject, topic, data) {
    if (data == this.PREF_FIRST_RUN && this.firstUseExperienced) {
      this._addToolbarButton();
      this.enableSessionRestore();
    }
  },

  
  
  uninit: function TabView_uninit() {
    Services.prefs.removeObserver(this.PREF_BRANCH, this);

    if (this._tabShowEventListener) {
      gBrowser.tabContainer.removeEventListener(
        "TabShow", this._tabShowEventListener, true);
    }
  },

  
  
  
  _initFrame: function TabView__initFrame(callback) {
    let hasCallback = typeof callback == "function";

    if (this._window) {
      if (hasCallback)
        callback();
      return;
    }

    if (hasCallback)
      this._initFrameCallbacks.push(callback);

    if (this._isFrameLoading)
      return;

    this._isFrameLoading = true;

    
    this._deck = document.getElementById("tab-view-deck");

    
    this._iframe = document.createElement("iframe");
    this._iframe.id = "tab-view";
    this._iframe.setAttribute("transparent", "true");
    this._iframe.flex = 1;

    let self = this;

    window.addEventListener("tabviewframeinitialized", function onInit() {
      window.removeEventListener("tabviewframeinitialized", onInit, false);

      self._isFrameLoading = false;
      self._window = self._iframe.contentWindow;
      self._setBrowserKeyHandlers();

      if (self._tabShowEventListener) {
        gBrowser.tabContainer.removeEventListener(
          "TabShow", self._tabShowEventListener, true);
        self._tabShowEventListener = null;
      }

      self._initFrameCallbacks.forEach(function (cb) cb());
      self._initFrameCallbacks = [];
    }, false);

    this._iframe.setAttribute("src", "chrome://browser/content/tabview.html");
    this._deck.appendChild(this._iframe);
  },

  
  getContentWindow: function TabView_getContentWindow() {
    return this._window;
  },

  
  isVisible: function TabView_isVisible() {
    return (this._deck ? this._deck.selectedPanel == this._iframe : false);
  },

  
  show: function() {
    if (this.isVisible())
      return;

    let self = this;
    this._initFrame(function() {
      self._window.UI.showTabView(true);
    });
  },

  
  hide: function() {
    if (!this.isVisible())
      return;

    this._window.UI.exit();
  },

  
  toggle: function() {
    if (this.isVisible())
      this.hide();
    else 
      this.show();
  },
  
  getActiveGroupName: function TabView_getActiveGroupName() {
    
    
    
    
    let activeTab = window.gBrowser.selectedTab;
    if (activeTab._tabViewTabItem && activeTab._tabViewTabItem.parent){
      let groupName = activeTab._tabViewTabItem.parent.getTitle();
      if (groupName)
        return groupName;
    }
    return null;
  },  

  
  updateContextMenu: function(tab, popup) {
    let separator = document.getElementById("context_tabViewNamedGroups");
    let isEmpty = true;

    while (popup.firstChild && popup.firstChild != separator)
      popup.removeChild(popup.firstChild);

    let self = this;
    this._initFrame(function() {
      let activeGroup = tab._tabViewTabItem.parent;
      let groupItems = self._window.GroupItems.groupItems;

      groupItems.forEach(function(groupItem) {
        
        
        
        if (groupItem.getTitle().length > 0 && !groupItem.hidden &&
            (!activeGroup || activeGroup.id != groupItem.id)) {
          let menuItem = self._createGroupMenuItem(groupItem);
          popup.insertBefore(menuItem, separator);
          isEmpty = false;
        }
      });
      separator.hidden = isEmpty;
    });
  },

  
  _createGroupMenuItem: function TabView__createGroupMenuItem(groupItem) {
    let menuItem = document.createElement("menuitem")
    menuItem.setAttribute("label", groupItem.getTitle());
    menuItem.setAttribute(
      "oncommand", 
      "TabView.moveTabTo(TabContextMenu.contextTab,'" + groupItem.id + "')");

    return menuItem;
  },

  
  moveTabTo: function TabView_moveTabTo(tab, groupItemId) {
    if (this._window) {
      this._window.GroupItems.moveTabToGroupItem(tab, groupItemId);
    } else {
      let self = this;
      this._initFrame(function() {
        self._window.GroupItems.moveTabToGroupItem(tab, groupItemId);
      });
    }
  },

  
  
  
  _setBrowserKeyHandlers: function TabView__setBrowserKeyHandlers() {
    if (this._browserKeyHandlerInitialized)
      return;

    this._browserKeyHandlerInitialized = true;

    let self = this;
    window.addEventListener("keypress", function(event) {
      if (self.isVisible() ||
          (gBrowser.tabs.length - gBrowser.visibleTabs.length) == 0)
        return;

      let charCode = event.charCode;
      
      if (event.ctrlKey && !event.metaKey && !event.altKey &&
          (charCode == 96 || charCode == 126)) {
        event.stopPropagation();
        event.preventDefault();

        self._initFrame(function() {
          let groupItems = self._window.GroupItems;
          let tabItem = groupItems.getNextGroupItemTab(event.shiftKey);
          if (!tabItem)
            return;

          
          let oldGroupItem = groupItems.getActiveGroupItem();
          window.gBrowser.selectedTab = tabItem.tab;
          oldGroupItem.closeIfEmpty();
        });
      }
    }, true);
  },

  
  
  prepareUndoCloseTab: function(blankTabToRemove) {
    if (this._window) {
      this._window.UI.restoredClosedTab = true;

      if (blankTabToRemove)
        blankTabToRemove._tabViewTabIsRemovedAfterRestore = true;
    }
  },

  
  
  afterUndoCloseTab: function () {
    if (this._window)
      this._window.UI.restoredClosedTab = false;
  },

  
  
  moveToGroupPopupShowing: function TabView_moveToGroupPopupShowing(event) {
    
    
    let numHiddenTabs = gBrowser.tabs.length - gBrowser.visibleTabs.length;
    if (this._window || numHiddenTabs > 0)
      this.updateContextMenu(TabContextMenu.contextTab, event.target);
  },

  
  
  
  _addToolbarButton: function TabView__addToolbarButton() {
    let buttonId = "tabview-button";

    if (document.getElementById(buttonId))
      return;

    let toolbar = document.getElementById("TabsToolbar");
    let currentSet = toolbar.currentSet.split(",");

    let alltabsPos = currentSet.indexOf("alltabs-button");
    if (-1 == alltabsPos)
      return;

    currentSet[alltabsPos] += "," + buttonId;
    currentSet = currentSet.join(",");
    toolbar.currentSet = currentSet;
    toolbar.setAttribute("currentset", currentSet);
    document.persist(toolbar.id, "currentset");
  },

  
  
  
  updateGroupNumberBroadcaster: function TabView_updateGroupNumberBroadcaster(number) {
    let groupsNumber = document.getElementById("tabviewGroupsNumber");
    groupsNumber.setAttribute("groups", number);
  },

  
  
  
  
  enableSessionRestore: function UI_enableSessionRestore() {
    if (!this._window || !this.firstUseExperienced)
      return;

    
    if (this.sessionRestoreEnabledOnce)
      return;

    this.sessionRestoreEnabledOnce = true;

    
    Services.prefs.setIntPref(this.PREF_STARTUP_PAGE, 3);
  }
};
