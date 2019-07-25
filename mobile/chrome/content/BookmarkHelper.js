var BookmarkHelper = {
  _editor: null,

  logging: false,

  log: function(msg) {
    if (this.logging) {
      Services.console.logStringMessage(msg);
    }
  },

  get box() {
    delete this.box;
    this.box = document.getElementById("bookmark-container");
    return this.box;
  },

  edit: function BH_edit(aURI) {
    if (!aURI)
      aURI = getBrowser().currentURI;

    let itemId = PlacesUtils.getMostRecentBookmarkForURI(aURI);
    if (itemId == -1)
      return;

    
    
    BookmarkPopup.hide();

    let title = PlacesUtils.bookmarks.getItemTitle(itemId);
    let tags = PlacesUtils.tagging.getTagsForURI(aURI, {});

    const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    this._editor = document.createElementNS(XULNS, "placeitem");
    this._editor.setAttribute("id", "bookmark-item");
    this._editor.setAttribute("flex", "1");
    this._editor.setAttribute("type", "bookmark");
    this._editor.setAttribute("ui", "manage");
    this._editor.setAttribute("title", title);
    this._editor.setAttribute("uri", aURI.spec);
    this._editor.setAttribute("itemid", itemId);
    this._editor.setAttribute("tags", tags.join(", "));
    this._editor.setAttribute("onclose", "BookmarkHelper.close()");
    document.getElementById("bookmark-form").appendChild(this._editor);

    this.box.hidden = false;
    BrowserUI.pushDialog(this);

    function waitForWidget(self) {
      try {
        self._editor.startEditing();
      } catch(e) {
        setTimeout(waitForWidget, 0, self);
      }
    }
    setTimeout(waitForWidget, 0, this);
  },

  save: function BH_save() {
    this._editor.stopEditing(true);
  },

  close: function BH_close() {
    this.log("Bookmark helper: closing");
    BrowserUI.updateStar();

    
    
    this._editor.parentNode.removeChild(this._editor);
    this._editor = null;

    BrowserUI.popDialog();
    this.box.hidden = true;
  },

  createShortcut: function BH_createShortcut(aTitle, aURL, aIconURL) {
    
    
    const kIconSize = 72;
    const kOverlaySize = 32;
    const kOffset = 20;

    
    aTitle = aTitle || aURL;

    let canvas = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    canvas.setAttribute("style", "display: none");

    function _createShortcut() {
      let icon = canvas.toDataURL("image/png", "");
      canvas = null;
      try {
        let shell = Cc["@mozilla.org/browser/shell-service;1"].createInstance(Ci.nsIShellService);
        shell.createShortcut(aTitle, aURL, icon, "bookmark");
      } catch(e) {
        Cu.reportError(e);
      }
    }

    
    let image = new Image();
    image.onload = function() {
      canvas.width = canvas.height = kIconSize;
      let ctx = canvas.getContext("2d");
      ctx.drawImage(image, 0, 0, kIconSize, kIconSize);

      
      if (aIconURL) {
        let favicon = new Image();
        favicon.onload = function() {
          
          ctx.drawImage(favicon, kOffset, kOffset, kOverlaySize, kOverlaySize);
          _createShortcut();
        }

        favicon.onerror = function() {
          Cu.reportError("CreateShortcut: favicon image load error");
        }

        favicon.src = aIconURL;
      } else {
        _createShortcut();
      }
    }

    image.onerror = function() {
      Cu.reportError("CreateShortcut: background image load error");
    }

    
    image.src = aIconURL ? "chrome://browser/skin/images/homescreen-blank-hdpi.png"
                         : "chrome://browser/skin/images/homescreen-default-hdpi.png";
  },

  removeBookmarksForURI: function BH_removeBookmarksForURI(aURI) {
    
    
    
    let itemIds = PlacesUtils.getBookmarksForURI(aURI);
    itemIds.forEach(PlacesUtils.bookmarks.removeItem);

    BrowserUI.updateStar();
  }
};
