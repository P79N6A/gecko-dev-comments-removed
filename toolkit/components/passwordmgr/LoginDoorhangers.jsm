



"use strict";

this.EXPORTED_SYMBOLS = [
  "LoginDoorhangers",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LoginManagerParent.jsm");

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";



function setDisabled(element, disabled) {
  if (disabled) {
    element.setAttribute("disabled", "true");
  } else {
    element.removeAttribute("disabled");
  }
}

this.LoginDoorhangers = {};







this.LoginDoorhangers.FillDoorhanger = function (properties) {
  
  this.el = new Proxy({}, {
    get: (target, name) => {
      return this.chromeDocument.getElementById("login-fill-" + name);
    },
  });
  this.eventHandlers = [];
  for (let elementName of Object.keys(this.events)) {
    let handlers = this.events[elementName];
    for (let eventName of Object.keys(handlers)) {
      let handler = handlers[eventName];
      this.eventHandlers.push([elementName, eventName, handler.bind(this)]);
    }
  };
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
    let PopupNotifications = this.chromeDocument.defaultView.PopupNotifications;
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

  





  get chromeDocument() {
    return this.browser.ownerDocument;
  },

  


  hide() {
    let PopupNotifications = this.chromeDocument.defaultView.PopupNotifications;
    if (PopupNotifications.isPanelOpen) {
      PopupNotifications.panel.hidePopup();
    }
  },

  


  promiseHidden: Promise.resolve(),

  


  remove() {
    this.notification.remove();
  },

  


  bind() {
    
    if (this.autoDetailLogin) {
      let formLogins = Services.logins.findLogins({}, this.loginFormOrigin, "",
                                                  null);
      if (formLogins.length == 1) {
        this.detailLogin = formLogins[0];
      }
      this.autoDetailLogin = false;
    }

    this.el.filter.setAttribute("value", this.filterString);
    this.refreshList();
    this.refreshDetailView();

    this.eventHandlers.forEach(([elementName, eventName, handler]) => {
      this.el[elementName].addEventListener(eventName, handler, true);
    });

    
    this.notification.owner.panel.firstElementChild.appendChild(this.el.doorhanger);
    this.el.doorhanger.hidden = false;

    this.bound = true;
  },

  


  unbind() {
    this.bound = false;

    this.eventHandlers.forEach(([elementName, eventName, handler]) => {
      this.el[elementName].removeEventListener(eventName, handler, true);
    });

    this.clearList();

    
    this.el.doorhanger.hidden = true;
    this.chromeDocument.getElementById("mainPopupSet")
                       .appendChild(this.el.doorhanger);
  },

  



  loginFormOrigin: "",

  



  loginFormPresent: false,

  


  filterString: "",

  


  autoDetailLogin: false,

  


  set detailLogin(detailLogin) {
    this._detailLogin = detailLogin;
    if (this.bound) {
      this.refreshDetailView();
    }
  },
  get detailLogin() {
    return this._detailLogin;
  },
  _detailLogin: null,

  


  events: {
    mainview: {
      focus(event) {
        
        
        this.detailLogin = null;
      },
    },
    filter: {
      input(event) {
        this.filterString = this.el.filter.value;
        this.refreshList();
      },
    },
    list: {
      click(event) {
        if (event.button == 0 && this.el.list.selectedItem) {
          this.displaySelectedLoginDetails();
        }
      },
      keypress(event) {
        if (event.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN &&
            this.el.list.selectedItem) {
          this.displaySelectedLoginDetails();
        }
      },
    },
    clickcapturer: {
      click(event) {
        this.detailLogin = null;
      },
    },
    details: {
      transitionend(event) {
        
        
        
        if (event.target == this.el.details && this.detailLogin) {
          if (this.loginFormPresent) {
            this.el.use.focus();
          } else {
            this.el.username.focus();
          }
        }
      },
    },
    use: {
      command(event) {
        this.fillLogin();
      },
    },
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
      let item = this.chromeDocument.createElementNS(XUL_NS, "richlistitem");
      item.classList.add("login-fill-item");
      item.setAttribute("hostname", hostname);
      item.setAttribute("username", username);
      if (hostname != this.loginFormOrigin) {
        item.classList.add("different-hostname");
      }
      this.el.list.appendChild(item);
    }
  },

  


  clearList() {
    let list = this.el.list;
    while (list.firstChild) {
      list.firstChild.remove();
    }
  },

  


  refreshDetailView() {
    if (this.detailLogin) {
      this.el.username.setAttribute("value", this.detailLogin.username);
      this.el.password.setAttribute("value", this.detailLogin.password);
      this.el.doorhanger.setAttribute("inDetailView", "true");
      setDisabled(this.el.username, false);
      setDisabled(this.el.use, !this.loginFormPresent);
    } else {
      this.el.doorhanger.removeAttribute("inDetailView");
      
      
      setDisabled(this.el.username, true);
      setDisabled(this.el.use, true);
    }
  },

  displaySelectedLoginDetails() {
    let selectedItem = this.el.list.selectedItem;
    let hostLogins = Services.logins.findLogins({},
                               selectedItem.getAttribute("hostname"), "", null);
    let login = hostLogins.find(login => {
      return login.username == selectedItem.getAttribute("username");
    });
    if (!login) {
      Cu.reportError("The selected login has been removed in the meantime.");
      return;
    }
    this.detailLogin = login;
  },

  fillLogin() {
    LoginManagerParent.fillForm({
      browser: this.browser,
      loginFormOrigin: this.loginFormOrigin,
      login: this.detailLogin,
    }).catch(Cu.reportError);
    this.hide();
  },
};











this.LoginDoorhangers.FillDoorhanger.find = function ({ browser }) {
  let PopupNotifications = browser.ownerDocument.defaultView.PopupNotifications;
  let notification = PopupNotifications.getNotification("login-fill",
                                                        browser);
  return notification && notification.doorhanger;
};
