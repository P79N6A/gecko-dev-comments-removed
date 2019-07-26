var Appbar = {
  get appbar()        { return document.getElementById('appbar'); },
  get consoleButton() { return document.getElementById('console-button'); },
  get jsShellButton() { return document.getElementById('jsshell-button'); },
  get zoomInButton()  { return document.getElementById('zoomin-button'); },
  get zoomOutButton() { return document.getElementById('zoomout-button'); },
  get starButton()    { return document.getElementById('star-button'); },
  get pinButton()     { return document.getElementById('pin-button'); },
  get moreButton()    { return document.getElementById('more-button'); },

  
  activeTileset: null,

  init: function Appbar_init() {
    window.addEventListener('MozAppbarShowing', this, false);
    window.addEventListener('MozPrecisePointer', this, false);
    window.addEventListener('MozImprecisePointer', this, false);
    window.addEventListener('MozContextActionsChange', this, false);
    Elements.browsers.addEventListener('URLChanged', this, true);
    Elements.tabList.addEventListener('TabSelect', this, true);

    this._updateDebugButtons();
    this._updateZoomButtons();

    
    window.addEventListener("selectionchange", this, false);
  },

  handleEvent: function Appbar_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'URLChanged':
      case 'TabSelect':
        this.appbar.dismiss();
        break;
      case 'MozAppbarShowing':
        this._updatePinButton();
        this._updateStarButton();
        break;
      case 'MozPrecisePointer':
      case 'MozImprecisePointer':
        this._updateZoomButtons();
        break;
      case 'MozContextActionsChange':
        let actions = aEvent.actions;
        
        this.showContextualActions(actions);
        break;
      case "selectionchange":
        let nodeName = aEvent.target.nodeName;
        if ('richgrid' === nodeName) {
          this._onTileSelectionChanged(aEvent);
        }
        break;
    }
  },

  onDownloadButton: function() {
    PanelUI.show("downloads-container");
    ContextUI.dismiss();
  },

  onZoomOutButton: function() {
    Browser.zoom(1);
  },

  onZoomInButton: function() {
    Browser.zoom(-1);
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

  onMoreButton: function(aEvent) {
      var typesArray = ["find-in-page"];
      try {
        
        
        var uri = Services.io.newURI(Browser.selectedBrowser.currentURI.spec,
                                     null, null);
        if (uri.schemeIs('http') || uri.schemeIs('https')) {
          typesArray.push("view-on-desktop");
        }
      } catch(ex) {
      }

      var x = this.moreButton.getBoundingClientRect().left;
      var y = this.appbar.getBoundingClientRect().top;
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

  onConsoleButton: function() {
    PanelUI.show("console-container");
  },

  onJSShellButton: function() {
    
    if (!MetroUtils.immersive)
      window.openDialog("chrome://browser/content/shell.xul", "_blank",
                        "all=no,scrollbars=yes,resizable=yes,dialog=no");
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
        this.appbar.dismiss();
      }
    }
  },
  showContextualActions: function(aVerbs){
    let doc = document;
    
    let buttonsMap = new Map();
    for (let verb of aVerbs) {
      let id = verb + "-selected-button";
      if (!doc.getElementById(id)) {
        throw new Error("Appbar.showContextualActions: no button for " + verb);
      }
      buttonsMap.set(id, verb);
    }

    
    let toHide = [],
        toShow = [];
    for (let btnNode of this.appbar.querySelectorAll("#contextualactions-tray > toolbarbutton")) {
      
      
      if (buttonsMap.has(btnNode.id)) {
        if (btnNode.hidden) toShow.push(btnNode);
      } else if (!btnNode.hidden) {
        toHide.push(btnNode);
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
    event.initEvent("MozContextActionsChange", true, false);
    this.appbar.dispatchEvent(event);

    if (verbs.length) {
      this.appbar.show(); 
    } else {
      this.appbar.dismiss();
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

  _updateDebugButtons: function() {
    this.consoleButton.disabled = !ConsolePanelView.enabled;
    this.jsShellButton.disabled = MetroUtils.immersive;
  },

  _updateZoomButtons: function() {
    let zoomDisabled = !InputSourceHelper.isPrecise;
    this.zoomOutButton.disabled = this.zoomInButton.disabled = zoomDisabled;
  }
  };
