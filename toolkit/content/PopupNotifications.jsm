





































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
    if (!this.owner.iconBox)
      return null;

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
  if (iconBox && !(iconBox instanceof Ci.nsIDOMXULElement))
    throw "Invalid iconBox";
  if (!(panel instanceof Ci.nsIDOMXULElement))
    throw "Invalid panel";

  this.window = tabbrowser.ownerDocument.defaultView;
  this.panel = panel;
  this.tabbrowser = tabbrowser;

  this._onIconBoxCommand = this._onIconBoxCommand.bind(this);
  this.iconBox = iconBox;

  this.panel.addEventListener("popuphidden", this._onPopupHidden.bind(this), true);

  let self = this;
  function updateFromListeners() {
    
    
    self.window.setTimeout(function () {
      self._update();
    }, 0);
  }
  this.window.addEventListener("activate", updateFromListeners, true);
  this.tabbrowser.tabContainer.addEventListener("TabSelect", updateFromListeners, true);
}

PopupNotifications.prototype = {
  set iconBox(iconBox) {
    
    if (this._iconBox) {
      this._iconBox.removeEventListener("click", this._onIconBoxCommand, false);
      this._iconBox.removeEventListener("keypress", this._onIconBoxCommand, false);
    }
    this._iconBox = iconBox;
    if (iconBox) {
      iconBox.addEventListener("click", this._onIconBoxCommand, false);
      iconBox.addEventListener("keypress", this._onIconBoxCommand, false);
    }
  },
  get iconBox() {
    return this._iconBox;
  },

  










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
      
      
      if (notification.options.persistWhileVisible &&
          this.isPanelOpen) {
        if ("persistence" in notification.options &&
          notification.options.persistence)
          notification.options.persistence--;
        return true;
      }
      
      
      
      if ("persistence" in notification.options &&
          notification.options.persistence) {
        notification.options.persistence--;
        return true;
      }

      
      if ("timeout" in notification.options &&
          Date.now() <= notification.options.timeout) {
        return true;
      }

      this._fireCallback(notification, "removed");
      return false;
    }, this);

    this._update();
  },

  




  remove: function PopupNotifications_remove(notification) {
    let isCurrent = this._currentNotifications.indexOf(notification) != -1;
    this._remove(notification);

    
    if (isCurrent)
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
    this._fireCallback(notification, "removed");
  },
  
  


  _dismiss: function PopupNotifications_dismiss() {
    this.panel.hidePopup();
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
      
      
      popupnotification.setAttribute("id", n.id + "-notification");
      popupnotification.setAttribute("popupid", n.id);
      popupnotification.setAttribute("closebuttoncommand", "PopupNotifications._dismiss();");
      if (n.mainAction) {
        popupnotification.setAttribute("buttonlabel", n.mainAction.label);
        popupnotification.setAttribute("buttonaccesskey", n.mainAction.accessKey);
        popupnotification.setAttribute("buttoncommand", "PopupNotifications._onButtonCommand(event);");
        popupnotification.setAttribute("menucommand", "PopupNotifications._onMenuCommand(event);");
        popupnotification.setAttribute("closeitemcommand", "PopupNotifications._dismiss();event.stopPropagation();");
      }
      popupnotification.notification = n;

      if (n.secondaryActions) {
        n.secondaryActions.forEach(function (a) {
          let item = doc.createElementNS(XUL_NS, "menuitem");
          item.setAttribute("label", a.label);
          item.setAttribute("accesskey", a.accessKey);
          item.notification = n;
          item.action = a;

          popupnotification.appendChild(item);
        }, this);
  
        if (n.secondaryActions.length) {
          let closeItemSeparator = doc.createElementNS(XUL_NS, "menuseparator");
          popupnotification.appendChild(closeItemSeparator);
        }
      }

      this.panel.appendChild(popupnotification);
    }, this);
  },

  _showPanel: function PopupNotifications_showPanel(notificationsToShow, anchorElement) {
    this.panel.hidden = false;

    this._refreshPanel(notificationsToShow);

    if (this.isPanelOpen && this._currentAnchorElement == anchorElement)
      return;

    
    
    
    this._hidePanel();

    
    
    
    let selectedTab = this.tabbrowser.selectedTab;
    if (anchorElement) {
      let bo = anchorElement.boxObject;
      if (bo.height == 0 && bo.width == 0)
        anchorElement = selectedTab; 
    } else {
      anchorElement = selectedTab; 
    }

    this._currentAnchorElement = anchorElement;

    this.panel.openPopup(anchorElement, "bottomcenter topleft");
    notificationsToShow.forEach(function (n) {
      this._fireCallback(n, "shown");
    }, this);
  },

  



  _update: function PopupNotifications_update(anchor) {
    let anchorElement, notificationsToShow = [];
    let haveNotifications = this._currentNotifications.length > 0;
    if (haveNotifications) {
      
      
      
      anchorElement = anchor || this._currentNotifications[0].anchorElement;

      if (this.iconBox) {
        this.iconBox.hidden = false;
        this.iconBox.setAttribute("anchorid", anchorElement.id);
      }

      
      notificationsToShow = this._currentNotifications.filter(function (n) {
        return !n.dismissed && n.anchorElement == anchorElement &&
               !n.options.neverShow;
      });
    }

    if (notificationsToShow.length > 0) {
      this._showPanel(notificationsToShow, anchorElement);
    } else {
      
      this._notify("updateNotShowing");

      
      
      this._dismiss();

      
      
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

    
    let anchor = event.target;
    while (anchor && anchor.parentNode != this.iconBox)
      anchor = anchor.parentNode;

    
    this._currentNotifications.forEach(function (n) {
      if (n.anchorElement == anchor)
        n.dismissed = false;
    });

    
    this._update(anchor);
  },

  _fireCallback: function PopupNotifications_fireCallback(n, event) {
    if (n.options.eventCallback)
      n.options.eventCallback.call(n, event);
  },

  _onPopupHidden: function PopupNotifications_onPopupHidden(event) {
    if (event.target != this.panel || this._ignoreDismissal)
      return;

    let browser = this.panel.firstChild &&
                  this.panel.firstChild.notification.browser;
    if (!browser)
      return;

    let notifications = this._getNotificationsForBrowser(browser);
    
    Array.forEach(this.panel.childNodes, function (nEl) {
      let notificationObj = nEl.notification;
      
      if (notifications.indexOf(notificationObj) == -1)
        return;

      
      
      if (notificationObj.options.removeOnDismissal)
        this._remove(notificationObj);
      else {
        notificationObj.dismissed = true;
        this._fireCallback(notificationObj, "dismissed");
      }
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
