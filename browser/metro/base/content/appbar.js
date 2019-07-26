


"use strict";

var Appbar = {
  get starButton()    { return document.getElementById('star-button'); },
  get pinButton()     { return document.getElementById('pin-button'); },
  get menuButton()    { return document.getElementById('menu-button'); },

  
  activeTileset: null,

  init: function Appbar_init() {
    
    Elements.contextappbar.addEventListener('MozAppbarShowing', this, false);
    Elements.contextappbar.addEventListener('MozAppbarDismissing', this, false);

    
    window.addEventListener('MozContextActionsChange', this, false);

    
    Elements.browsers.addEventListener('URLChanged', this, true);
    Elements.tabList.addEventListener('TabSelect', this, true);

    
    window.addEventListener("selectionchange", this, false);
  },

  handleEvent: function Appbar_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'URLChanged':
      case 'TabSelect':
        this.update();
        this.flushActiveTileset(aEvent.lastTab);
        break;

      case 'MozAppbarShowing':
        this.update();
        break;

      case 'MozAppbarDismissing':
        if (this.activeTileset && ('isBound' in this.activeTileset)) {
          this.activeTileset.clearSelection();
        }
        this._clearContextualActions();
        this.activeTileset = null;
        break;

      case 'MozContextActionsChange':
        let actions = aEvent.actions;
        let setName = aEvent.target.contextSetName;
        
        this.showContextualActions(actions, setName);
        break;

      case "selectionchange":
        let nodeName = aEvent.target.nodeName;
        if ('richgrid' === nodeName) {
          this._onTileSelectionChanged(aEvent);
        }
        break;
    }
  },

  flushActiveTileset: function flushActiveTileset(aTab) {
    try {
      let tab = aTab || Browser.selectedTab;
      
      
      if (this.activeTileset && tab && tab.browser && tab.browser.currentURI.spec == kStartURI) {
        ContextUI.dismiss();
      }
    } catch (ex) {}
  },

  shutdown: function shutdown() {
    this.flushActiveTileset();
  },

  




  update: function update() {
    this._updatePinButton();
    this._updateStarButton();
  },

  onPinButton: function() {
    if (this.pinButton.checked) {
      Browser.pinSite();
    } else {
      Browser.unpinSite();
    }
  },

  onStarButton: function(aValue) {
    if (aValue === undefined) {
      aValue = this.starButton.checked;
    }

    if (aValue) {
      Browser.starSite(function () {
        Appbar._updateStarButton();
      });
    } else {
      Browser.unstarSite(function () {
        Appbar._updateStarButton();
      });
    }
  },

  onMenuButton: function(aEvent) {
      let typesArray = [];

      if (!BrowserUI.isStartTabVisible)
        typesArray.push("find-in-page");
      if (ConsolePanelView.enabled)
        typesArray.push("open-error-console");
      if (!Services.metro.immersive)
        typesArray.push("open-jsshell");

      try {
        
        
        let uri = Services.io.newURI(Browser.selectedBrowser.currentURI.spec,
                                     null, null);
        if (uri.schemeIs('http') || uri.schemeIs('https')) {
          typesArray.push("view-on-desktop");
        }
      } catch(ex) {
      }

      var x = this.menuButton.getBoundingClientRect().left;
      var y = Elements.toolbar.getBoundingClientRect().top;
      ContextMenuUI.showContextMenu({
        json: {
          types: typesArray,
          string: '',
          xPos: x,
          yPos: y,
          leftAligned: true,
          bottomAligned: true
      }

      });
  },

  onViewOnDesktop: function() {
    try {
      
      
      var uri = Services.io.newURI(Browser.selectedBrowser.currentURI.spec,
                                   null, null);
      if (uri.schemeIs('http') || uri.schemeIs('https')) {
        Services.metro.launchInDesktop(Browser.selectedBrowser.currentURI.spec, "");
      }
    } catch(ex) {
    }
  },

  onAutocompleteCloseButton: function () {
    Elements.autocomplete.closePopup();
  },

  dispatchContextualAction: function(aActionName){
    let activeTileset = this.activeTileset;
    if (activeTileset && ('isBound' in this.activeTileset)) {
      
      
      let event = document.createEvent("Events");
      event.action = aActionName;
      event.initEvent("context-action", true, true); 
      activeTileset.dispatchEvent(event);
      if (!event.defaultPrevented) {
        activeTileset.clearSelection();
        Elements.contextappbar.dismiss();
      }
    }
  },

  showContextualActions: function(aVerbs, aSetName) {
    
    let immediate = !Elements.contextappbar.isShowing;

    if (aVerbs.length) {
      Elements.contextappbar.show();
    }

    
    let idsToVisibleVerbs = new Map();
    for (let verb of aVerbs) {
      let id = verb + "-selected-button";
      if (!document.getElementById(id)) {
        throw new Error("Appbar.showContextualActions: no button for " + verb);
      }
      idsToVisibleVerbs.set(id, verb);
    }

    
    let toHide = [], toShow = [];
    let buttons = Elements.contextappbar.getElementsByTagName("toolbarbutton");
    for (let button of buttons) {
      let verb = idsToVisibleVerbs.get(button.id);
      if (verb != undefined) {
        
        this._updateContextualActionLabel(button, verb, aSetName);
        if (button.hidden) {
          toShow.push(button);
        }
      } else if (!button.hidden) {
        
        toHide.push(button);
      }
    }

    if (immediate) {
      toShow.forEach(function(element) {
        element.removeAttribute("fade");
        element.hidden = false;
      });
      toHide.forEach(function(element) {
        element.setAttribute("fade", true);
        element.hidden = true;
      });
      return;
    }

    return Task.spawn(function() {
      if (toHide.length) {
        yield Util.transitionElementVisibility(toHide, false);
      }
      if (toShow.length) {
        yield Util.transitionElementVisibility(toShow, true);
      }
    });
  },

  _clearContextualActions: function() {
    this.showContextualActions([]);
  },

  _updateContextualActionLabel: function(aButton, aVerb, aSetName) {
    
    
    let usesSetName = aButton.hasAttribute("label-uses-set-name");
    let name = "contextAppbar2." + aVerb + (usesSetName ? "." + aSetName : "");
    aButton.label = Strings.browser.GetStringFromName(name);
  },

  _onTileSelectionChanged: function _onTileSelectionChanged(aEvent){
    let activeTileset = aEvent.target;

    
    
    if (this.activeTileset &&
          ('isBound' in this.activeTileset) &&
          this.activeTileset !== activeTileset) {
      this.activeTileset.clearSelection();
    }
    
    this.activeTileset = activeTileset;

    
    
    let contextActions = activeTileset.contextActions;
    let verbs = [v for (v of contextActions)];

    
    let event = document.createEvent("Events");
    event.actions = verbs;
    event.initEvent("MozContextActionsChange", true, false);
    activeTileset.dispatchEvent(event);

    if (verbs.length) {
      Elements.contextappbar.show(); 
    } else {
      Elements.contextappbar.dismiss();
    }
  },

  _updatePinButton: function() {
    this.pinButton.checked = Browser.isSitePinned();
  },

  _updateStarButton: function() {
    Browser.isSiteStarredAsync(function (isStarred) {
      this.starButton.checked = isStarred;
    }.bind(this));
  },
};
