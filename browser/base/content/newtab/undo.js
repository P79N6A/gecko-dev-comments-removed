#ifdef 0



#endif





let gUndoDialog = {
  


  HIDE_TIMEOUT_MS: 15000,

  


  _undoData: null,

  


  init: function UndoDialog_init() {
    this._undoContainer = document.getElementById("newtab-undo-container");
    this._undoContainer.addEventListener("click", this, false);
    this._undoButton = document.getElementById("newtab-undo-button");
    this._undoCloseButton = document.getElementById("newtab-undo-close-button");
    this._undoRestoreButton = document.getElementById("newtab-undo-restore-button");
  },

  



  show: function UndoDialog_show(aSite) {
    if (this._undoData)
      clearTimeout(this._undoData.timeout);

    this._undoData = {
      index: aSite.cell.index,
      wasPinned: aSite.isPinned(),
      blockedLink: aSite.link,
      timeout: setTimeout(this.hide.bind(this), this.HIDE_TIMEOUT_MS)
    };

    this._undoContainer.removeAttribute("undo-disabled");
    this._undoButton.removeAttribute("tabindex");
    this._undoCloseButton.removeAttribute("tabindex");
    this._undoRestoreButton.removeAttribute("tabindex");
  },

  


  hide: function UndoDialog_hide() {
    if (!this._undoData)
      return;

    clearTimeout(this._undoData.timeout);
    this._undoData = null;
    this._undoContainer.setAttribute("undo-disabled", "true");
    this._undoButton.setAttribute("tabindex", "-1");
    this._undoCloseButton.setAttribute("tabindex", "-1");
    this._undoRestoreButton.setAttribute("tabindex", "-1");
  },

  



  handleEvent: function UndoDialog_handleEvent(aEvent) {
    switch (aEvent.target.id) {
      case "newtab-undo-button":
        this._undo();
        break;
      case "newtab-undo-restore-button":
        this._undoAll();
        break;
      case "newtab-undo-close-button":
        this.hide();
        break;
    }
  },

  


  _undo: function UndoDialog_undo() {
    if (!this._undoData)
      return;

    let {index, wasPinned, blockedLink} = this._undoData;
    gBlockedLinks.unblock(blockedLink);

    if (wasPinned) {
      gPinnedLinks.pin(blockedLink, index);
    }

    gUpdater.updateGrid();
    this.hide();
  },

  


  _undoAll: function UndoDialog_undoAll() {
    NewTabUtils.undoAll(function() {
      gUpdater.updateGrid();
      this.hide();
    }.bind(this));
  }
};

gUndoDialog.init();
