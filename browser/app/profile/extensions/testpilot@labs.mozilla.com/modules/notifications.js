









EXPORTED_SYMBOLS = ["CustomNotificationManager", "PopupNotificationManager"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;



function CustomNotificationManager(anchorId, tailIsUp) {
  this._anchorId = anchorId;
  this._tailIsUp = tailIsUp;
}
CustomNotificationManager.prototype = {
  showNotification: function TP_OldNotfn_showNotification(window, features, choices) {
    let doc = window.document;
    let popup = doc.getElementById("pilot-notification-popup");
    let textLabel = doc.getElementById("pilot-notification-text");
    let titleLabel = doc.getElementById("pilot-notification-title");
    let icon = doc.getElementById("pilot-notification-icon");
    let button = doc.getElementById("pilot-notification-submit");
    let closeBtn = doc.getElementById("pilot-notification-close");
    let link = doc.getElementById("pilot-notification-link");
    let checkbox = doc.getElementById("pilot-notification-always-submit-checkbox");
    let self = this;
    let buttonChoice = null;
    let linkChoice = null;
    let checkBoxChoice = null;

    if (this._tailIsUp) {
      popup.setAttribute("class", "tail-up");
    } else {
      popup.setAttribute("class", "tail-down");
    }

    popup.setAttribute("noautohide", !(features.fragile));
    if (features.title) {
      titleLabel.setAttribute("value", features.title);
    }
    while (textLabel.lastChild) {
      textLabel.removeChild(textLabel.lastChild);
    }
    if (features.text) {
      textLabel.appendChild(doc.createTextNode(features.text));
    }
    if (features.iconClass) {
      
      icon.setAttribute("class", features.iconClass);
    }

    

    for (let i = 0; i < choices.length; i++) {
      switch(choices[i].customUiType) {
      case "button":
        buttonChoice = choices[i];
        break;
      case "link":
        linkChoice = choices[i];
        break;
      case "checkbox":
        checkBoxChoice = choices[i];
        break;
      }
    }
    
    if (checkBoxChoice) {
      checkbox.removeAttribute("hidden");
      checkbox.setAttribute("label", checkBoxChoice.label);
    } else {
      checkbox.setAttribute("hidden", true);
    }

    
    if (buttonChoice) {
      button.setAttribute("label", buttonChoice.label);
      button.onclick = function(event) {
        if (event.button == 0) {
          if (checkbox.checked && checkBoxChoice) {
            checkBoxChoice.callback();
          }
          buttonChoice.callback();
          self.hideNotification(window);
          if (features.closeCallback) {
            features.closeCallback();
          }
        }
      };
      button.removeAttribute("hidden");
    } else {
      button.setAttribute("hidden", true);
    }

    
    if (linkChoice) {
      link.setAttribute("value", linkChoice.label);
      link.setAttribute("class", "notification-link");
      link.onclick = function(event) {
        if (event.button == 0) {
          linkChoice.callback();
          self.hideNotification(window);
          if (features.closeCallback) {
            features.closeCallback();
          }
        }
      };
      link.removeAttribute("hidden");
    } else {
      link.setAttribute("hidden", true);
    }

    closeBtn.onclick = function() {
      self.hideNotification(window);
      if (features.closeCallback) {
        features.closeCallback();
      }
    };

    
    popup.hidden = false;
    popup.setAttribute("open", "true");
    let anchorElement = window.document.getElementById(this._anchorId);
    popup.openPopup(anchorElement, "after_end");
  },

  hideNotification: function TP_OldNotfn_hideNotification(window) {
    let popup = window.document.getElementById("pilot-notification-popup");
    popup.removeAttribute("open");
    popup.hidePopup();
  }
};


function PopupNotificationManager() {
  

  this._popupModule = {};
  Components.utils.import("resource://gre/modules/PopupNotifications.jsm", this._popupModule);
  this._pn = null;
}
PopupNotificationManager.prototype = {
  showNotification: function TP_NewNotfn_showNotification(window, features, choices) {
    let self = this;
    let tabbrowser = window.getBrowser();
    let panel = window.document.getElementById("testpilot-notification-popup");
    let iconBox = window.document.getElementById("tp-notification-popup-box");
    let defaultChoice = null;
    let additionalChoices = [];

    
    this.hideNotification();

    
    this._pn = new this._popupModule.PopupNotifications(tabbrowser, panel, iconBox);

    

    for (let i = 0; i < choices.length; i++) {
      let choice = choices[i];
      let choiceWithHide = {
        label: choice.label,
        accessKey: choice.accessKey,
        callback: function() {
          self.hideNotification();
          choice.callback();
        }};
      
      if (i == 0) {
        defaultChoice = choiceWithHide;
      } else {
        additionalChoices.push(choiceWithHide);
      }
    }

    this._notifRef = this._pn.show(tabbrowser.selectedBrowser,
                             "testpilot",
                             features.text,
                             "tp-notification-popup-icon", 
                             defaultChoice,
                             additionalChoices,
                             {persistWhileVisible: true,
                              removeOnDismissal: features.fragile,
                              title: features.title,
                              iconClass: features.iconClass,
                              closeButtonFunc: function() {
                                self.hideNotification();
                              },
                              eventCallback: function(stateChange){
                                



                                if (stateChange == "removed" && features.closeCallback) {
                                  features.closeCallback();
                                }
                                self._notifRef = null;
                              }});
    
  },

  hideNotification: function TP_NewNotfn_hideNotification() {
    if (this._notifRef && this._pn) {
      this._pn.remove(this._notifRef);
      this._notifRef = null;
    }
  }
};
