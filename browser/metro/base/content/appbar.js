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
    window.addEventListener('MozContextUIShow', this, false);
    window.addEventListener('MozPrecisePointer', this, false);
    window.addEventListener('MozImprecisePointer', this, false);

    this._updateDebugButtons();
    this._updateZoomButtons();

    
    window.addEventListener("selectionchange", this, false);
  },

  handleEvent: function Appbar_handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'MozContextUIShow':
        this._updatePinButton();
        this._updateStarButton();
        break;
      case 'MozPrecisePointer':
      case 'MozImprecisePointer':
        this._updateZoomButtons();
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
          forcePosition: true,
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
      event.initEvent("context-action", true, false);
      activeTileset.dispatchEvent(event);

      
      activeTileset.clearSelection();
    }
    this.appbar.dismiss();
  },

  showContextualActions: function(aVerbs){
    let doc = document;

    
    let buttonsMap = new Map();
    for (let verb of aVerbs) {
      let id = verb + "-selected-button";
      let buttonNode = doc.getElementById(id);
      if (buttonNode) {
        buttonsMap.set(id, verb);
      } else {
        Util.dumpLn("Appbar.showContextualActions: no button for " + verb);
      }
    }

    
    let buttons = doc.querySelectorAll("#contextualactions-tray > toolbarbutton");

    for (let btnNode of buttons) {
      if (buttonsMap.has(btnNode.id)){
        btnNode.hidden = false;
      } else {
        btnNode.hidden = true;
      }
    };

    if (buttonsMap.size) {
      
      
    } else {
      
      
    }
  },

  _onTileSelectionChanged: function _onTileSelectionChanged(aEvent){
    let activeTileset = aEvent.target;

    
    if (this.activeTileset && this.activeTileset !== activeTileset) {
      this.activeTileset.clearSelection();
    }
    
    this.activeTileset = activeTileset;

    
    
    let contextActions = activeTileset.contextActions;
    let verbs = [v for (v of contextActions)];

    
    this.showContextualActions(verbs);

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
