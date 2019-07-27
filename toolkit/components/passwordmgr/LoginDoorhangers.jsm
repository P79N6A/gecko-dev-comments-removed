



"use strict";

this.EXPORTED_SYMBOLS = [
  "LoginDoorhangers",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LoginManagerParent.jsm");

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

this.LoginDoorhangers = {};







this.LoginDoorhangers.FillDoorhanger = function (properties) {
  this.onFilterInput = this.onFilterInput.bind(this);
  this.onListDblClick = this.onListDblClick.bind(this);
  this.onListKeyPress = this.onListKeyPress.bind(this);

  for (let name of Object.getOwnPropertyNames(properties)) {
    this[name] = properties[name];
  }
};

this.LoginDoorhangers.FillDoorhanger.prototype = {
  


  bound: false,

  






  set browser(browser) {
    const MAX_DATE_VALUE = new Date(8640000000000000);

    this._browser = browser;

    let doorhanger = this;
    let PopupNotifications = this.chomeDocument.defaultView.PopupNotifications;
    let notification = PopupNotifications.show(
      browser,
      "login-fill",
      "",
      "login-fill-notification-icon",
      null,
      null,
      {
        dismissed: true,
        
        
        
        timeout: MAX_DATE_VALUE,
        eventCallback: function (topic, otherBrowser) {
          switch (topic) {
            case "shown":
              
              
              
              doorhanger.bound = true;
              doorhanger.promiseHidden =
                         new Promise(resolve => doorhanger.onUnbind = resolve);
              doorhanger.bind();
              break;

            case "dismissed":
            case "removed":
              if (doorhanger.bound) {
                doorhanger.unbind();
                doorhanger.onUnbind();
              }
              break;

            case "swapping":
              doorhanger._browser = otherBrowser;
              return true;
          }
          return false;
        },
      }
    );

    this.notification = notification;
    notification.doorhanger = this;
  },
  get browser() {
    return this._browser;
  },
  _browser: null,

  





  get chomeDocument() {
    return this.browser.ownerDocument;
  },

  


  hide() {
    let PopupNotifications = this.chomeDocument.defaultView.PopupNotifications;
    if (PopupNotifications.isPanelOpen) {
      PopupNotifications.panel.hidePopup();
    }
  },

  


  promiseHidden: Promise.resolve(),

  


  remove() {
    this.notification.remove();
  },

  


  bind() {
    this.element = this.chomeDocument.getElementById("login-fill-doorhanger");
    this.list = this.chomeDocument.getElementById("login-fill-list");
    this.filter = this.chomeDocument.getElementById("login-fill-filter");

    this.filter.setAttribute("value", this.filterString);

    this.refreshList();

    this.filter.addEventListener("input", this.onFilterInput);
    this.list.addEventListener("dblclick", this.onListDblClick);
    this.list.addEventListener("keypress", this.onListKeyPress);

    
    this.notification.owner.panel.firstElementChild.appendChild(this.element);
    this.element.hidden = false;
  },

  


  unbind() {
    this.filter.removeEventListener("input", this.onFilterInput);
    this.list.removeEventListener("dblclick", this.onListDblClick);
    this.list.removeEventListener("keypress", this.onListKeyPress);

    this.clearList();

    
    this.element.hidden = true;
    this.chomeDocument.getElementById("mainPopupSet").appendChild(this.element);
  },

  



  loginFormOrigin: "",

  



  loginFormPresent: false,

  


  filterString: "",

  


  onFilterInput() {
    this.filterString = this.filter.value;
    this.refreshList();
  },

  


  refreshList() {
    this.clearList();

    let formLogins = Services.logins.findLogins({}, "", "", null);
    let filterToUse = this.filterString.trim().toLowerCase();
    if (filterToUse) {
      formLogins = formLogins.filter(login => {
        return login.hostname.toLowerCase().indexOf(filterToUse) != -1 ||
               login.username.toLowerCase().indexOf(filterToUse) != -1 ;
      });
    }

    for (let { hostname, username } of formLogins) {
      let item = this.chomeDocument.createElementNS(XUL_NS, "richlistitem");
      item.classList.add("login-fill-item");
      item.setAttribute("hostname", hostname);
      item.setAttribute("username", username);
      if (hostname != this.loginFormOrigin) {
        item.classList.add("different-hostname");
      }
      if (!this.loginFormPresent) {
        item.setAttribute("disabled", "true");
      }
      this.list.appendChild(item);
    }
  },

  


  clearList() {
    while (this.list.firstChild) {
      this.list.removeChild(this.list.firstChild);
    }
  },

  


  onListDblClick(event) {
    if (event.button != 0 || !this.list.selectedItem) {
      return;
    }
    this.fillLogin();
  },
  onListKeyPress(event) {
    if (event.keyCode != Ci.nsIDOMKeyEvent.DOM_VK_RETURN ||
        !this.list.selectedItem) {
      return;
    }
    this.fillLogin();
  },
  fillLogin() {
    if (this.list.selectedItem.hasAttribute("disabled")) {
      return;
    }
    let formLogins = Services.logins.findLogins({}, "", "", null);
    let login = formLogins.find(login => {
      return login.hostname == this.list.selectedItem.getAttribute("hostname") &&
             login.username == this.list.selectedItem.getAttribute("username");
    });
    if (login) {
      LoginManagerParent.fillForm({
        browser: this.browser,
        loginFormOrigin: this.loginFormOrigin,
        login,
      }).catch(Cu.reportError);
    } else {
      Cu.reportError("The selected login has been removed in the meantime.");
    }
    this.hide();
  },
};











this.LoginDoorhangers.FillDoorhanger.find = function ({ browser }) {
  let PopupNotifications = browser.ownerDocument.defaultView.PopupNotifications;
  let notification = PopupNotifications.getNotification("login-fill",
                                                        browser);
  return notification && notification.doorhanger;
};
