




let LoopUI;

(function() {
  const kNSXUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  const kBrowserSharingNotificationId = "loop-sharing-notification";
  const kPrefBrowserSharingInfoBar = "browserSharing.showInfoBar";

  LoopUI = {
    



    get toolbarButton() {
      delete this.toolbarButton;
      return this.toolbarButton = CustomizableUI.getWidget("loop-button").forWindow(window);
    },

    


    get panel() {
      delete this.panel;
      return this.panel = document.getElementById("loop-notification-panel");
    },

    



    get browser() {
      let browser = document.querySelector("#loop-notification-panel > #loop-panel-iframe");
      if (browser) {
        delete this.browser;
        this.browser = browser;
      }
      return browser;
    },

    




    get selectedTab() {
      if (!this.browser) {
        return null;
      }

      let selectedTab = this.browser.contentDocument.querySelector(".tab-view > .selected");
      return selectedTab && selectedTab.getAttribute("data-tab-name");
    },

    


    promiseDocumentVisible(aDocument) {
      if (!aDocument.hidden) {
        return Promise.resolve();
      }

      return new Promise((resolve) => {
        aDocument.addEventListener("visibilitychange", function onVisibilityChanged() {
          aDocument.removeEventListener("visibilitychange", onVisibilityChanged);
          resolve();
        });
      });
    },

    








    togglePanel: function(event, tabId = null) {
      if (this.panel.state == "open") {
        return new Promise(resolve => {
          this.panel.hidePopup();
          resolve();
        });
      }

      return this.openCallPanel(event, tabId);
    },

    








    openCallPanel: function(event, tabId = null) {
      return new Promise((resolve) => {
        let callback = iframe => {
          
          function showTab() {
            if (!tabId) {
              resolve(LoopUI.promiseDocumentVisible(iframe.contentDocument));
              return;
            }

            let win = iframe.contentWindow;
            let ev = new win.CustomEvent("UIAction", Cu.cloneInto({
              detail: {
                action: "selectTab",
                tab: tabId
              }
            }, win));
            win.dispatchEvent(ev);
            resolve(LoopUI.promiseDocumentVisible(iframe.contentDocument));
          }

          
          
          if (("contentWindow" in iframe) && iframe.contentWindow.document.readyState == "complete") {
            showTab();
            return;
          }

          let documentDOMLoaded = () => {
            iframe.removeEventListener("DOMContentLoaded", documentDOMLoaded, true);
  	    this.injectLoopAPI(iframe.contentWindow);
  	    iframe.contentWindow.addEventListener("loopPanelInitialized", function loopPanelInitialized() {
              iframe.contentWindow.removeEventListener("loopPanelInitialized",
                                                       loopPanelInitialized);
              showTab();
	    });
	  };
	  iframe.addEventListener("DOMContentLoaded", documentDOMLoaded, true); 
        };

        
        Services.obs.notifyObservers(null, "loop-status-changed", null);

        this.shouldResumeTour().then((resume) => {
          if (resume) {
            
            
            
            this.MozLoopService.resumeTour("waiting");
            resolve();
            return;
          }

          this.PanelFrame.showPopup(window, event ? event.target : this.toolbarButton.node,
                               "loop", null, "about:looppanel", null, callback);
        });
      });
    },

    







    shouldResumeTour: Task.async(function* () {
      
      
      if (!Services.prefs.getBoolPref("loop.gettingStarted.resumeOnFirstJoin")) {
        return false;
      }

      if (!this.LoopRooms.participantsCount) {
        
        return false;
      }

      let roomsWithNonOwners = yield this.roomsWithNonOwners();
      if (!roomsWithNonOwners.length) {
        
        return false;
      }

      return true;
    }),

    


    roomsWithNonOwners: function() {
      return new Promise(resolve => {
        this.LoopRooms.getAll((error, rooms) => {
          let roomsWithNonOwners = [];
          for (let room of rooms) {
            if (!("participants" in room)) {
              continue;
            }
            let numNonOwners = room.participants.filter(participant => !participant.owner).length;
            if (!numNonOwners) {
              continue;
            }
            roomsWithNonOwners.push(room);
          }
          resolve(roomsWithNonOwners);
        });
      });
    },

    



    init: function() {
      
      Services.obs.addObserver(this, "loop-status-changed", false);

      this.MozLoopService.initialize();
      this.updateToolbarState();
    },

    uninit: function() {
      Services.obs.removeObserver(this, "loop-status-changed");
    },

    
    observe: function(subject, topic, data) {
      if (topic != "loop-status-changed") {
        return;
      }
      this.updateToolbarState(data);
    },

    









    updateToolbarState: function(aReason = null) {
      if (!this.toolbarButton.node) {
        return;
      }
      let state = "";
      if (this.MozLoopService.errors.size) {
        state = "error";
      } else if (this.MozLoopService.screenShareActive) {
        state = "action";
      } else if (aReason == "login" && this.MozLoopService.userProfile) {
        state = "active";
      } else if (this.MozLoopService.doNotDisturb) {
        state = "disabled";
      } else if (this.MozLoopService.roomsParticipantsCount > 0) {
        state = "active";
      }
      this.toolbarButton.node.setAttribute("state", state);
    },

    















    showNotification: function(options) {
      if (this.MozLoopService.doNotDisturb) {
        return;
      }

      if (!options.title) {
        throw new Error("Missing title, can not display notification");
      }

      let notificationOptions = {
        body: options.message || ""
      };
      if (options.icon) {
        notificationOptions.icon = options.icon;
      }
      if (options.sound) {
        
        notificationOptions.mozbehavior = {
          soundFile: ""
        };
        this.playSound(options.sound);
      }

      let notification = new window.Notification(options.title, notificationOptions);
      notification.addEventListener("click", e => {
        if (window.closed) {
          return;
        }

        try {
          window.focus();
        } catch (ex) {}

        
        
        window.setTimeout(() => {
          if (typeof options.onclick == "function") {
            options.onclick();
          } else {
            
            this.openCallPanel(null, options.selectTab || null);
          }
        }, 0);
      });
    },

    




    playSound: function(name) {
      if (this.ActiveSound || this.MozLoopService.doNotDisturb) {
        return;
      }

      this.activeSound = new window.Audio();
      this.activeSound.src = `chrome://browser/content/loop/shared/sounds/${name}.ogg`;
      this.activeSound.load();
      this.activeSound.play();

      this.activeSound.addEventListener("ended", () => this.activeSound = undefined, false);
    },

    










    addBrowserSharingListener: function(listener) {
      if (!this._tabChangeListeners) {
        this._tabChangeListeners = new Set();
        gBrowser.tabContainer.addEventListener("TabSelect", this);
      }

      this._tabChangeListeners.add(listener);
      this._maybeShowBrowserSharingInfoBar();

      
      listener(null, gBrowser.selectedBrowser.outerWindowID);
    },

    




    removeBrowserSharingListener: function(listener) {
      if (!this._tabChangeListeners) {
        return;
      }

      if (this._tabChangeListeners.has(listener)) {
        this._tabChangeListeners.delete(listener);
      }

      if (!this._tabChangeListeners.size) {
        this._hideBrowserSharingInfoBar();
        gBrowser.tabContainer.removeEventListener("TabSelect", this);
        delete this._tabChangeListeners;
      }
    },

    






    _getString: function(key) {
      let str = this.MozLoopService.getStrings(key);
      if (str) {
        str = JSON.parse(str).textContent;
      }
      return str;
    },

    




    _maybeShowBrowserSharingInfoBar: function() {
      this._hideBrowserSharingInfoBar();

      
      if (!this.MozLoopService.getLoopPref(kPrefBrowserSharingInfoBar)) {
        return;
      }

      
      
      let menuPopup = document.createElementNS(kNSXUL, "menupopup");
      let menuItem = menuPopup.appendChild(document.createElementNS(kNSXUL, "menuitem"));
      menuItem.setAttribute("label", this._getString("infobar_menuitem_dontshowagain_label"));
      menuItem.setAttribute("accesskey", this._getString("infobar_menuitem_dontshowagain_accesskey"));
      menuItem.addEventListener("command", () => {
        
        this._hideBrowserSharingInfoBar(true);
      });

      let box = gBrowser.getNotificationBox();
      let bar = box.appendNotification(
        this._getString("infobar_screenshare_browser_message"),
        kBrowserSharingNotificationId,
        
        null,
        box.PRIORITY_WARNING_LOW,
        [{
          label: this._getString("infobar_button_gotit_label"),
          accessKey: this._getString("infobar_button_gotit_accesskey"),
          type: "menu-button",
          popup: menuPopup,
          anchor: "dropmarker",
          callback: () => {
            this._hideBrowserSharingInfoBar();
          }
        }]
      );

      
      bar.persistence = -1;
    },

    






    _hideBrowserSharingInfoBar: function(permanently = false, browser) {
      browser = browser || gBrowser.selectedBrowser;
      let box = gBrowser.getNotificationBox(browser);
      let notification = box.getNotificationWithValue(kBrowserSharingNotificationId);
      let removed = false;
      if (notification) {
        box.removeNotification(notification);
        removed = true;
      }

      if (permanently) {
        this.MozLoopService.setLoopPref(kPrefBrowserSharingInfoBar, false);
      }

      return removed;
    },

    


    handleEvent: function(event) {
      
      if (event.type != "TabSelect") {
        return;
      }

      let wasVisible = false;
      
      if (event.detail.previousTab) {
        wasVisible = this._hideBrowserSharingInfoBar(false, event.detail.previousTab.linkedBrowser);
      }

      
      for (let listener of this._tabChangeListeners) {
        try {
          listener(null, gBrowser.selectedBrowser.outerWindowID);
        } catch (ex) {
          Cu.reportError("Tab switch caused an error: " + ex.message);
        }
      };

      if (wasVisible) {
        
        
        this._maybeShowBrowserSharingInfoBar();
      }
    },
  };
})();

XPCOMUtils.defineLazyModuleGetter(LoopUI, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(LoopUI, "LoopRooms", "resource:///modules/loop/LoopRooms.jsm");
XPCOMUtils.defineLazyModuleGetter(LoopUI, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(LoopUI, "PanelFrame", "resource:///modules/PanelFrame.jsm");
