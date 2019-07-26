


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
      case 'MozAppbarShowing':
        this.update();
        break;

      case 'MozAppbarDismissing':
        if (this.activeTileset) {
          this.activeTileset.clearSelection();
        }
        this.clearContextualActions();
        this.activeTileset = null;
        break;

      case 'MozContextActionsChange':
        let actions = aEvent.actions;
        let noun = aEvent.noun;
        let qty = aEvent.qty;
        
        this.showContextualActions(actions, noun, qty);
        break;

      case "selectionchange":
        let nodeName = aEvent.target.nodeName;
        if ('richgrid' === nodeName) {
          this._onTileSelectionChanged(aEvent);
        }
        break;
    }
  },

  




  update: function update() {
    this._updatePinButton();
    this._updateStarButton();
  },

  onDownloadButton: function() {
    PanelUI.show("downloads-container");
    ContextUI.dismiss();
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
      var typesArray = ["find-in-page"];

      if (ConsolePanelView.enabled) typesArray.push("open-error-console");
      if (!MetroUtils.immersive) typesArray.push("open-jsshell");

      try {
        
        
        var uri = Services.io.newURI(Browser.selectedBrowser.currentURI.spec,
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
        MetroUtils.launchInDesktop(Browser.selectedBrowser.currentURI.spec, "");
      }
    } catch(ex) {
    }
  },

  dispatchContextualAction: function(aActionName){
    let activeTileset = this.activeTileset;
    if (activeTileset) {
      
      
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

  showContextualActions: function(aVerbs, aNoun, aQty) {
    if (aVerbs.length)
      Elements.contextappbar.show();
    else
      Elements.contextappbar.hide();

    
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
        
        this._updateContextualActionLabel(button, verb, aNoun, aQty);
        if (button.hidden) {
          toShow.push(button);
        }
      } else if (!button.hidden) {
        
        toHide.push(button);
      }
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

  clearContextualActions: function() {
    this.showContextualActions([]);
  },

  _updateContextualActionLabel: function(aBtnNode, aVerb, aNoun, aQty) {
    
    
    let modifiesNoun = aBtnNode.getAttribute("modifies-noun") == "true";
    if (modifiesNoun && (!aNoun || isNaN(aQty))) {
      throw new Error("Appbar._updateContextualActionLabel: " +
                      "missing noun/quantity for " + aVerb);
    }

    let labelName = "contextAppbar." + aVerb + (modifiesNoun ? "." + aNoun : "");
    let label = Strings.browser.GetStringFromName(labelName);
    aBtnNode.label = modifiesNoun ? PluralForm.get(aQty, label) : label;
  },

  _onTileSelectionChanged: function _onTileSelectionChanged(aEvent){
    let activeTileset = aEvent.target;

    
    if (this.activeTileset && this.activeTileset !== activeTileset) {
      this.activeTileset.clearSelection();
    }
    
    this.activeTileset = activeTileset;

    
    
    let contextActions = activeTileset.contextActions;
    let verbs = [v for (v of contextActions)];

    
    let event = document.createEvent("Events");
    event.actions = verbs;
    event.noun = activeTileset.contextNoun;
    event.qty = activeTileset.selectedItems.length;
    event.initEvent("MozContextActionsChange", true, false);
    Elements.contextappbar.dispatchEvent(event);

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
