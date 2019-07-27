




let LoopUI;

XPCOMUtils.defineLazyModuleGetter(this, "injectLoopAPI", "resource:///modules/loop/MozLoopAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService", "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PanelFrame", "resource:///modules/PanelFrame.jsm");


(function() {
  LoopUI = {
    get toolbarButton() {
      delete this.toolbarButton;
      return this.toolbarButton = CustomizableUI.getWidget("loop-button").forWindow(window);
    },

    







    openCallPanel: function(event, tabId = null) {
      let callback = iframe => {
        
        function showTab() {
          if (!tabId) {
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

      PanelFrame.showPopup(window, event ? event.target : this.toolbarButton.node,
                           "loop", null, "about:looppanel", null, callback);
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
          soundFile: `chrome://browser/content/loop/shared/sounds/${options.sound}.ogg`
        };
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
