




































var WebappsUI = {
  _dialog: null,
  _manifest: null,
  _perms: [],

  checkBox: function(aEvent) {
    let elem = aEvent.originalTarget;
    let perm = elem.getAttribute("perm");
    if (this._manifest.capabilities && this._manifest.capabilities.indexOf(perm) != -1) {
      if (elem.checked) {
        elem.classList.remove("webapps-noperm");
        elem.classList.add("webapps-perm");
      } else {
        elem.classList.remove("webapps-perm");
        elem.classList.add("webapps-noperm");
      }
    }
  },

  show: function show(aManifest) {
    if (!aManifest) {
      
      let browser = Browser.selectedBrowser;
      let icon = browser.appIcon.href;
      if (!icon)
        icon = browser.mIconURL;
      if (!icon)
        icon = gFaviconService.getFaviconImageForPage(browser.currentURI).spec;

      
      aManifest = {
        uri: browser.currentURI.spec,
        name: browser.contentTitle,
        icon: icon,
        capabilities: [],
      };
    }

    this._manifest = aManifest;
    this._dialog = importDialog(window, "chrome://browser/content/webapps.xul", null);

    if (aManifest.name)
      document.getElementById("webapps-title").value = aManifest.name;
    if (aManifest.icon)
      document.getElementById("webapps-icon").src = aManifest.icon;

    let uri = Services.io.newURI(aManifest.uri, null, null);

    let perms = [["offline", "offline-app"], ["geoloc", "geo"], ["notifications", "desktop-notification"]];
    let self = this;
    perms.forEach(function(tuple) {
      let elem = document.getElementById("webapps-" + tuple[0] + "-checkbox");
      let currentPerm = Services.perms.testExactPermission(uri, tuple[1]);
      self._perms[tuple[1]] = (currentPerm == Ci.nsIPermissionManager.ALLOW_ACTION);
      if ((aManifest.capabilities && (aManifest.capabilities.indexOf(tuple[1]) != -1)) || (currentPerm == Ci.nsIPermissionManager.ALLOW_ACTION))
        elem.checked = true;
      else
        elem.checked = (currentPerm == Ci.nsIPermissionManager.ALLOW_ACTION);
      elem.classList.remove("webapps-noperm");
      elem.classList.add("webapps-perm");
    });

    BrowserUI.pushPopup(this, this._dialog);

    
    this._dialog.waitForClose();
  },

  hide: function hide() {
    this._dialog.close();
    this._dialog = null;
    BrowserUI.popPopup(this);
  },

  _updatePermission: function updatePermission(aId, aPerm) {
    try {
      let uri = Services.io.newURI(this._manifest.uri, null, null);
      let currentState = document.getElementById(aId).checked;
      if (currentState != this._perms[aPerm])
        Services.perms.add(uri, aPerm, currentState ? Ci.nsIPermissionManager.ALLOW_ACTION : Ci.nsIPermissionManager.DENY_ACTION);
    } catch(e) {
      Cu.reportError(e);
    }
  },

  launch: function launch() {
    let title = document.getElementById("webapps-title").value;
    if (!title)
      return;

    this._updatePermission("webapps-offline-checkbox", "offline-app");
    this._updatePermission("webapps-geoloc-checkbox", "geo");
    this._updatePermission("webapps-notifications-checkbox", "desktop-notification");

    this.hide();
    this.install(this._manifest.uri, title, this._manifest.icon);
  },

  updateWebappsInstall: function updateWebappsInstall(aNode) {
    if (document.getElementById("main-window").hasAttribute("webapp"))
      return false;

    let browser = Browser.selectedBrowser;

    let webapp = Cc["@mozilla.org/webapps/support;1"].getService(Ci.nsIWebappsSupport);
    return !(webapp && webapp.isApplicationInstalled(browser.currentURI.spec));
  },

  install: function(aURI, aTitle, aIcon) {
    const kIconSize = 64;

    let canvas = document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    canvas.setAttribute("style", "display: none");

    let self = this;
    let image = new Image();
    image.onload = function() {
      canvas.width = canvas.height = kIconSize; 
      let ctx = canvas.getContext("2d");
      ctx.drawImage(image, 0, 0, kIconSize, kIconSize);
      let data = canvas.toDataURL("image/png", "");
      canvas = null;
      try {
        let webapp = Cc["@mozilla.org/webapps/support;1"].getService(Ci.nsIWebappsSupport);
        webapp.installApplication(aTitle, aURI, aIcon, data);
      } catch(e) {
        Cu.reportError(e);
      }
    }
    image.onerror = function() {
      
      self.install(aURI, aTitle, "chrome://browser/skin/images/favicon-default-30.png");
    }
    image.src = aIcon;
  }
};
