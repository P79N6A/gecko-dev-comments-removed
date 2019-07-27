






'use strict';

var GaiaLockScreen = {

  unlock: function() {
    let lockscreen = window.wrappedJSObject.lockScreen;
    let setlock = window.wrappedJSObject.SettingsListener.getSettingsLock();
    let system = window.wrappedJSObject.System;
    let obj = {'screen.timeout': 0};
    setlock.set(obj);

    window.wrappedJSObject.ScreenManager.turnScreenOn();

    waitFor(
      function() {
        window.wrappedJSObject.dispatchEvent(
          new window.wrappedJSObject.CustomEvent(
            'lockscreen-request-unlock', {
              detail: {
                forcibly: true
              }
            }));
        waitFor(
          function() {
            finish(system.locked);
          },
          function() {
            return !system.locked;
          }
        );
      },
      function() {
        return !!lockscreen;
      }
    );
  },

  lock: function() {
    let lwm = window.wrappedJSObject.lockScreenWindowManager;
    let lockscreen = window.wrappedJSObject.lockScreen;
    let system = window.wrappedJSObject.System;
    let setlock = window.wrappedJSObject.SettingsListener.getSettingsLock();
    let obj = {'screen.timeout': 0};
    let waitLock = function() {
      waitFor(
        function() {
        window.wrappedJSObject.dispatchEvent(
          new window.wrappedJSObject.CustomEvent(
            'lockscreen-request-lock', {
              detail: {
                forcibly: true
              }
            }));
          waitFor(
            function() {
              finish(!system.locked);
            },
            function() {
              return system.locked;
            }
          );
        },
        function() {
          return !!lockscreen;
        }
      );
    };

    setlock.set(obj);
    window.wrappedJSObject.ScreenManager.turnScreenOn();

    
    
    
    lwm.openApp();
    waitFor(function() {
      waitLock();
    }, function() {
      return lwm.states.instance.isActive();
    });
  }
};

