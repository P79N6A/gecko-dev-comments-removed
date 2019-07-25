




































var EXPORTED_SYMBOLS = ["PopupNotifications"];

var Cc = Components.classes, Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/Services.jsm");






function Notification(id, message, anchorID, mainAction, secondaryActions,
                      browser, owner, options) {
  this.id = id;
  this.message = message;
  this.anchorID = anchorID;
  this.mainAction = mainAction;
  this.secondaryActions = secondaryActions || [];
  this.browser = browser;
  this.owner = owner;
  this.options = options || {};
}

Notification.prototype = {
  


  remove: function Notification_remove() {
    this.owner.remove(this);
  },

  get anchorElement() {
    let anchorElement = null;
    if (this.anchorID)
      anchorElement = this.owner.iconBox.querySelector("#"+this.anchorID);

    if (!anchorElement)
      anchorElement = this.owner.iconBox;

    return anchorElement;
  }
};


















function PopupNotifications(tabbrowser, panel, iconBox) {
  if (!(tabbrowser instanceof Ci.nsIDOMXULElement))
    throw "Invalid tabbrowser";
  if (!(iconBox instanceof Ci.nsIDOMXULElement))
    throw "Invalid iconBox";
  if (!(panel instanceof Ci.nsIDOMXULElement))
    throw "Invalid panel";

  this.window = tabbrowser.ownerDocument.defaultView;
  this.panel = panel;
  this.iconBox = iconBox;
  this.tabbrowser = tabbrowser;

  let self = this;
  this.iconBox.addEventListener("click", function (event) {
    self._onIconBoxCommand(event);
  }, false);
  this.iconBox.addEventListener("keypress", function (event) {
    self._onIconBoxCommand(event);
  }, false);
  this.panel.addEventListener("popuphidden", function (event) {
    self._onPopupHidden(event);
  }, true);

  function updateFromListeners() {
    
    
    self.window.setTimeout(function () {
      self._update();
    }, 0);
  }
  this.window.addEventListener("activate", updateFromListeners, true);
  this.tabbrowser.tabContainer.addEventListener("TabSelect", updateFromListeners, true);
}

PopupNotifications.prototype = {
  










  getNotification: function PopupNotifications_getNotification(id, browser) {
    let n = null;
    let notifications = this._getNotificationsForBrowser(browser || this.tabbrowser.selectedBrowser);
    notifications.some(function(x) x.id == id && (n = x))
    return n;
  },

  









































  show: function PopupNotifications_show(browser, id, message, anchorID,
                                         mainAction, secondaryActions, options) {
    function isInvalidAction(a) {
      return !a || !(typeof(a.callback) == "function") || !a.label || !a.accessKey;
    }

    if (!browser)
      throw "PopupNotifications_show: invalid browser";
    if (!id)
      throw "PopupNotifications_show: invalid ID";
    if (!message)
      throw "PopupNotifications_show: invalid message";
    if (mainAction && isInvalidAction(mainAction))
      throw "PopupNotifications_show: invalid mainAction";
    if (secondaryActions && secondaryActions.some(isInvalidAction))
      throw "PopupNotifications_show: invalid secondaryActions";

    let notification = new Notification(id, message, anchorID, mainAction,
                                        secondaryActions, browser, this, options);

    if (options && options.dismissed)
      notification.dismissed = true;

    let existingNotification = this.getNotification(id, browser);
    if (existingNotification)
      this._remove(existingNotification);

    let notifications = this._getNotificationsForBrowser(browser);
    notifications.push(notification);

    let fm = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);
    if (browser == this.tabbrowser.selectedBrowser && fm.activeWindow == this.window) {
      
      this._update(notification.anchorElement);
    } else {
      
      

      
      this._notify("backgroundShow");
    }

    return notification;
  },

  


  get isPanelOpen() {
    let panelState = this.panel.state;

    return panelState == "showing" || panelState == "open";
  },

  



  locationChange: function PopupNotifications_locationChange() {
    this._currentNotifications = this._currentNotifications.filter(function(notification) {
      
      
      if ("persistence" in notification.options &&
          notification.options.persistence) {
        notification.options.persistence--;
        return true;
      }

      
      if ("timeout" in notification.options &&
          Date.now() <= notification.options.timeout) {
        return true;
      }

      return false;
    });

    this._update();
  },

  




  remove: function PopupNotifications_remove(notification) {
    let isCurrent = this._currentNotifications.indexOf(notification) != -1;
    this._remove(notification);

    
    if (this.isPanelOpen && isCurrent)
      this._update();
  },





  


  get _currentNotifications() {
    return this._getNotificationsForBrowser(this.tabbrowser.selectedBrowser);
  },
  set _currentNotifications(a) {
    return this.tabbrowser.selectedBrowser.popupNotifications = a;
  },

  _remove: function PopupNotifications_removeHelper(notification) {
    
    
    let notifications = this._getNotificationsForBrowser(notification.browser);
    if (!notifications)
      return;

    var index = notifications.indexOf(notification);
    if (index == -1)
      return;

    
    notifications.splice(index, 1);
  },

  


  _hidePanel: function PopupNotifications_hide() {
    this._ignoreDismissal = true;
    this.panel.hidePopup();
    this._ignoreDismissal = false;
  },

  


  _refreshPanel: function PopupNotifications_refreshPanel(notificationsToShow) {
    while (this.panel.lastChild)
      this.panel.removeChild(this.panel.lastChild);

    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

    notificationsToShow.forEach(function (n) {
      let doc = this.window.document;
      let popupnotification = doc.createElementNS(XUL_NS, "popupnotification");
      popupnotification.setAttribute("label", n.message);
      popupnotification.setAttribute("id", n.id);
      if (n.mainAction) {
        popupnotification.setAttribute("buttonlabel", n.mainAction.label);
        popupnotification.setAttribute("buttonaccesskey", n.mainAction.accessKey);
        popupnotification.setAttribute("buttoncommand", "PopupNotifications._onButtonCommand(event);");
        if (n.secondaryActions.length) {
          popupnotification.setAttribute("buttontype", "menu-button");
          popupnotification.setAttribute("menucommand", "PopupNotifications._onMenuCommand(event);");
        }
      }
      popupnotification.notification = n;

      this.panel.appendChild(popupnotification);

      if (n.secondaryActions) {
        n.secondaryActions.forEach(function (a) {
          let item = doc.createElementNS(XUL_NS, "menuitem");
          item.setAttribute("label", a.label);
          item.setAttribute("accesskey", a.accessKey);
          item.notification = n;
          item.action = a;

          popupnotification.appendChild(item);
        }, this);
      }
    }, this);
  },

  _showPanel: function PopupNotifications_showPanel(notificationsToShow, anchorElement) {
    this.panel.hidden = false;

    this._refreshPanel(notificationsToShow);

    if (this.isPanelOpen && this._currentAnchorElement == anchorElement)
      return;

    
    var position = (this.window.getComputedStyle(this.panel, "").direction == "rtl") ? "after_end" : "after_start";

    this._currentAnchorElement = anchorElement;

    this.panel.openPopup(anchorElement, position);
  },

  



  _update: function PopupNotifications_update(anchor) {
    let anchorElement, notificationsToShow = [];
    let haveNotifications = this._currentNotifications.length > 0;
    if (haveNotifications) {
      
      
      
      anchorElement = anchor || this._currentNotifications[0].anchorElement;

      this.iconBox.hidden = false;
      this.iconBox.setAttribute("anchorid", anchorElement.id);

      
      notificationsToShow = this._currentNotifications.filter(function (n) {
        return !n.dismissed && n.anchorElement == anchorElement;
      });
    }

    if (notificationsToShow.length > 0) {
      this._showPanel(notificationsToShow, anchorElement);
    } else {
      
      this._notify("updateNotShowing");

      this._hidePanel();

      
      
      if (this.iconBox && !haveNotifications)
        this.iconBox.hidden = true;
    }
  },

  _getNotificationsForBrowser: function PopupNotifications_getNotifications(browser) {
    if (browser.popupNotifications)
      return browser.popupNotifications;

    return browser.popupNotifications = [];
  },

  _onIconBoxCommand: function PopupNotifications_onIconBoxCommand(event) {
    
    let type = event.type;
    if (type == "click" && event.button != 0)
      return;

    if (type == "keypress" &&
        !(event.charCode == Ci.nsIDOMKeyEvent.DOM_VK_SPACE ||
          event.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN))
      return;

    if (this._currentNotifications.length == 0)
      return;

    let anchor = event.originalTarget;

    
    this._currentNotifications.forEach(function (n) {
      if (n.anchorElement == anchor)
        n.dismissed = false;
    });

    
    this._update(anchor);
  },

  _onPopupHidden: function PopupNotifications_onPopupHidden(event) {
    if (event.target != this.panel || this._ignoreDismissal)
      return;

    
    Array.forEach(this.panel.childNodes, function (nEl) {
      let notificationObj = nEl.notification;
      notificationObj.dismissed = true;
    }, this);

    this._update();
  },

  _onButtonCommand: function PopupNotifications_onButtonCommand(event) {
    
    
    
    
    let target = event.originalTarget;
    let notificationEl;
    let parent = target;
    while (parent && (parent = target.ownerDocument.getBindingParent(parent)))
      notificationEl = parent;

    if (!notificationEl)
      throw "PopupNotifications_onButtonCommand: couldn't find notification element";

    if (!notificationEl.notification)
      throw "PopupNotifications_onButtonCommand: couldn't find notification";

    let notification = notificationEl.notification;
    notification.mainAction.callback.call();

    this._remove(notification);
    this._update();
  },

  _onMenuCommand: function PopupNotifications_onMenuCommand(event) {
    let target = event.originalTarget;
    if (!target.action || !target.notification)
      throw "menucommand target has no associated action/notification";

    event.stopPropagation();
    target.action.callback.call();

    this._remove(target.notification);
    this._update();
  },

  _notify: function PopupNotifications_notify(topic) {
    Services.obs.notifyObservers(null, "PopupNotifications-" + topic, "");
  }
}
