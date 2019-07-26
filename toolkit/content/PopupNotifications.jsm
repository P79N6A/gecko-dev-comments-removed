



this.EXPORTED_SYMBOLS = ["PopupNotifications"];

var Cc = Components.classes, Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/Services.jsm");

const NOTIFICATION_EVENT_DISMISSED = "dismissed";
const NOTIFICATION_EVENT_REMOVED = "removed";
const NOTIFICATION_EVENT_SHOWN = "shown";

const ICON_SELECTOR = ".notification-anchor-icon";
const ICON_ATTRIBUTE_SHOWING = "showing";

const PREF_SECURITY_DELAY = "security.notification_enable_delay";

let popupNotificationsMap = new WeakMap();
let gNotificationParents = new WeakMap;






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

  id: null,
  message: null,
  anchorID: null,
  mainAction: null,
  secondaryActions: null,
  browser: null,
  owner: null,
  options: null,
  timeShown: null,

  


  remove: function Notification_remove() {
    this.owner.remove(this);
  },

  get anchorElement() {
    let iconBox = this.owner.iconBox;
    if (!iconBox)
      return null;

    let anchorElement = null;
    if (this.anchorID)
      anchorElement = iconBox.querySelector("#"+this.anchorID);

    
    if (!anchorElement)
      anchorElement = iconBox.querySelector("#default-notification-icon") ||
                      iconBox;

    return anchorElement;
  },

  reshow: function() {
    this.owner._reshowNotificationForAnchor(this.anchorElement);
  }
};


















this.PopupNotifications = function PopupNotifications(tabbrowser, panel, iconBox) {
  if (!(tabbrowser instanceof Ci.nsIDOMXULElement))
    throw "Invalid tabbrowser";
  if (iconBox && !(iconBox instanceof Ci.nsIDOMXULElement))
    throw "Invalid iconBox";
  if (!(panel instanceof Ci.nsIDOMXULElement))
    throw "Invalid panel";

  this.window = tabbrowser.ownerDocument.defaultView;
  this.panel = panel;
  this.tabbrowser = tabbrowser;
  this.iconBox = iconBox;
  this.buttonDelay = Services.prefs.getIntPref(PREF_SECURITY_DELAY);

  this.panel.addEventListener("popuphidden", this, true);

  this.window.addEventListener("activate", this, true);
  this.tabbrowser.tabContainer.addEventListener("TabSelect", this, true);
}

PopupNotifications.prototype = {

  window: null,
  panel: null,
  tabbrowser: null,

  _iconBox: null,
  set iconBox(iconBox) {
    
    if (this._iconBox) {
      this._iconBox.removeEventListener("click", this, false);
      this._iconBox.removeEventListener("keypress", this, false);
    }
    this._iconBox = iconBox;
    if (iconBox) {
      iconBox.addEventListener("click", this, false);
      iconBox.addEventListener("keypress", this, false);
    }
  },
  get iconBox() {
    return this._iconBox;
  },

  










  getNotification: function PopupNotifications_getNotification(id, browser) {
    let n = null;
    let notifications = this._getNotificationsForBrowser(browser || this.tabbrowser.selectedBrowser);
    notifications.some(function(x) x.id == id && (n = x));
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

  



  locationChange: function PopupNotifications_locationChange(aBrowser) {
    if (!aBrowser)
      throw "PopupNotifications_locationChange: invalid browser";

    let notifications = this._getNotificationsForBrowser(aBrowser);

    notifications = notifications.filter(function (notification) {
      
      
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

      this._fireCallback(notification, NOTIFICATION_EVENT_REMOVED);
      return false;
    }, this);

    this._setNotificationsForBrowser(aBrowser, notifications);

    if (aBrowser == this.tabbrowser.selectedBrowser)
      this._update();
  },

  




  remove: function PopupNotifications_remove(notification) {
    let isCurrent = notification.browser == this.tabbrowser.selectedBrowser;
    this._remove(notification);

    
    if (isCurrent)
      this._update();
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "popuphidden":
        this._onPopupHidden(aEvent);
        break;
      case "activate":
      case "TabSelect":
        let self = this;
        
        
        this.window.setTimeout(function () {
          self._update();
        }, 0);
        break;
      case "click":
      case "keypress":
        this._onIconBoxCommand(aEvent);
        break;
    }
  },





  _ignoreDismissal: null,
  _currentAnchorElement: null,

  


  get _currentNotifications() {
    return this._getNotificationsForBrowser(this.tabbrowser.selectedBrowser);
  },

  _remove: function PopupNotifications_removeHelper(notification) {
    
    
    let notifications = this._getNotificationsForBrowser(notification.browser);
    if (!notifications)
      return;

    var index = notifications.indexOf(notification);
    if (index == -1)
      return;

    if (notification.browser == this.tabbrowser.selectedBrowser)
      notification.anchorElement.removeAttribute(ICON_ATTRIBUTE_SHOWING);

    
    notifications.splice(index, 1);
    this._fireCallback(notification, NOTIFICATION_EVENT_REMOVED);
  },

  


  _dismiss: function PopupNotifications_dismiss() {
    let browser = this.panel.firstChild &&
                  this.panel.firstChild.notification.browser;
    this.panel.hidePopup();
    if (browser)
      browser.focus();
  },

  


  _hidePanel: function PopupNotifications_hide() {
    this._ignoreDismissal = true;
    this.panel.hidePopup();
    this._ignoreDismissal = false;
  },

  


  _clearPanel: function () {
    let popupnotification;
    while ((popupnotification = this.panel.lastChild)) {
      this.panel.removeChild(popupnotification);

      
      
      let originalParent = gNotificationParents.get(popupnotification);
      if (originalParent) {
        popupnotification.notification = null;

        
        
        
        let contentNode = popupnotification.lastChild;
        while (contentNode) {
          let previousSibling = contentNode.previousSibling;
          if (contentNode.nodeName != "popupnotificationcontent")
            popupnotification.removeChild(contentNode);
          contentNode = previousSibling;
        }

        
        
        popupnotification.hidden = true;

        originalParent.appendChild(popupnotification);
      }
    }
  },

  _refreshPanel: function PopupNotifications_refreshPanel(notificationsToShow) {
    this._clearPanel();

    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

    notificationsToShow.forEach(function (n) {
      let doc = this.window.document;

      
      
      let popupnotificationID = n.id + "-notification";

      
      
      let popupnotification = doc.getElementById(popupnotificationID);
      if (popupnotification)
        gNotificationParents.set(popupnotification, popupnotification.parentNode);
      else
        popupnotification = doc.createElementNS(XUL_NS, "popupnotification");

      popupnotification.setAttribute("label", n.message);
      popupnotification.setAttribute("id", popupnotificationID);
      popupnotification.setAttribute("popupid", n.id);
      popupnotification.setAttribute("closebuttoncommand", "PopupNotifications._dismiss();");
      if (n.mainAction) {
        popupnotification.setAttribute("buttonlabel", n.mainAction.label);
        popupnotification.setAttribute("buttonaccesskey", n.mainAction.accessKey);
        popupnotification.setAttribute("buttoncommand", "PopupNotifications._onButtonCommand(event);");
        popupnotification.setAttribute("menucommand", "PopupNotifications._onMenuCommand(event);");
        popupnotification.setAttribute("closeitemcommand", "PopupNotifications._dismiss();event.stopPropagation();");
      } else {
        popupnotification.removeAttribute("buttonlabel");
        popupnotification.removeAttribute("buttonaccesskey");
        popupnotification.removeAttribute("buttoncommand");
        popupnotification.removeAttribute("menucommand");
        popupnotification.removeAttribute("closeitemcommand");
      }

      if (n.options.popupIconURL)
        popupnotification.setAttribute("icon", n.options.popupIconURL);
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

      
      
      popupnotification.hidden = false;
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

    
    
    this.panel.setAttribute("popupid", this.panel.firstChild.getAttribute("popupid"));
    this.panel.openPopup(anchorElement, "bottomcenter topleft");
    notificationsToShow.forEach(function (n) {
      n.timeShown = Date.now();
      this._fireCallback(n, NOTIFICATION_EVENT_SHOWN);
    }, this);
  },

  



  _update: function PopupNotifications_update(anchor) {
    if (this.iconBox) {
      
      this._hideIcons();
    }

    let anchorElement, notificationsToShow = [];
    let currentNotifications = this._currentNotifications;
    let haveNotifications = currentNotifications.length > 0;
    if (haveNotifications) {
      
      
      
      anchorElement = anchor || currentNotifications[0].anchorElement;

      if (this.iconBox) {
        this._showIcons(currentNotifications);
        this.iconBox.hidden = false;
      }

      
      notificationsToShow = currentNotifications.filter(function (n) {
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

  _showIcons: function PopupNotifications_showIcons(aCurrentNotifications) {
    for (let notification of aCurrentNotifications) {
      let anchorElm = notification.anchorElement;
      if (anchorElm) {
        anchorElm.setAttribute(ICON_ATTRIBUTE_SHOWING, "true");
      }
    }
  },

  _hideIcons: function PopupNotifications_hideIcons() {
    let icons = this.iconBox.querySelectorAll(ICON_SELECTOR);
    for (let icon of icons) {
      icon.removeAttribute(ICON_ATTRIBUTE_SHOWING);
    }
  },

  


  _getNotificationsForBrowser: function PopupNotifications_getNotifications(browser) {
    let notifications = popupNotificationsMap.get(browser);
    if (!notifications) {
      
      notifications = [];
      popupNotificationsMap.set(browser, notifications);
    }
    return notifications;
  },
  _setNotificationsForBrowser: function PopupNotifications_setNotifications(browser, notifications) {
    popupNotificationsMap.set(browser, notifications);
    return notifications;
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

    this._reshowNotificationForAnchor(anchor);
  },

  _reshowNotificationForAnchor: function PopupNotifications_reshowNotificationForAnchor(anchor) {
    
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
        this._fireCallback(notificationObj, NOTIFICATION_EVENT_DISMISSED);
      }
    }, this);

    this._clearPanel();

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
    let timeSinceShown = Date.now() - notification.timeShown;

    
    if (!notification.timeMainActionFirstTriggered) {
      notification.timeMainActionFirstTriggered = timeSinceShown;
      Services.telemetry.getHistogramById("POPUP_NOTIFICATION_MAINACTION_TRIGGERED_MS").
                         add(timeSinceShown);
    }

    if (timeSinceShown < this.buttonDelay) {
      Services.console.logStringMessage("PopupNotifications_onButtonCommand: " +
                                        "Button click happened before the security delay: " +
                                        timeSinceShown + "ms");
      return;
    }
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
  },
};
