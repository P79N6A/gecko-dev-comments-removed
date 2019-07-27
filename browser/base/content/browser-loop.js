




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoopRooms", "resource:///modules/loop/LoopRooms.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PanelFrame", "resource:///modules/PanelFrame.jsm");


(function() {
  LoopUI = {
    get toolbarButton() {
      delete this.toolbarButton;
      return this.toolbarButton = CustomizableUI.getWidget("loop-button").forWindow(window);
    },

    get panel() {
      delete this.panel;
      return this.panel = document.getElementById("loop-notification-panel");
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
        this.panel.hidePopup();
      } else {
        this.openCallPanel(event, tabId);
      }
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

          iframe.addEventListener("DOMContentLoaded", function documentDOMLoaded() {
            iframe.removeEventListener("DOMContentLoaded", documentDOMLoaded, true);
            injectLoopAPI(iframe.contentWindow);
            iframe.contentWindow.addEventListener("loopPanelInitialized", function loopPanelInitialized() {
              iframe.contentWindow.removeEventListener("loopPanelInitialized",
                                                       loopPanelInitialized);
              showTab();
            });
          }, true);
        };

        
        Services.obs.notifyObservers(null, "loop-status-changed", null);

        this.shouldResumeTour().then((resume) => {
          if (resume) {
            
            
            
            MozLoopService.resumeTour("waiting");
            resolve();
            return;
          }

          PanelFrame.showPopup(window, event ? event.target : this.toolbarButton.node,
                               "loop", null, "about:looppanel", null, callback);
        });
      });
    },

    







    shouldResumeTour: Task.async(function* () {
      
      
      if (!Services.prefs.getBoolPref("loop.gettingStarted.resumeOnFirstJoin")) {
        return false;
      }

      if (!LoopRooms.participantsCount) {
        
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
        LoopRooms.getAll((error, rooms) => {
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

      MozLoopService.initialize();
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
      if (MozLoopService.errors.size) {
        state = "error";
      } else if (MozLoopService.screenShareActive) {
        state = "action";
      } else if (aReason == "login" && MozLoopService.userProfile) {
        state = "active";
      } else if (MozLoopService.doNotDisturb) {
        state = "disabled";
      } else if (MozLoopService.roomsParticipantsCount > 0) {
        state = "active";
      }
      this.toolbarButton.node.setAttribute("state", state);
    },

    















    showNotification: function(options) {
      if (MozLoopService.doNotDisturb) {
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
      if (this.ActiveSound || MozLoopService.doNotDisturb) {
        return;
      }

      this.activeSound = new window.Audio();
      this.activeSound.src = `chrome://browser/content/loop/shared/sounds/${name}.ogg`;
      this.activeSound.load();
      this.activeSound.play();

      this.activeSound.addEventListener("ended", () => this.activeSound = undefined, false);
    },
  };
})();
