"use strict";




let runAppObj;
window.addEventListener('load', function() {
  
  let args;
  try {
    let service = Cc["@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds"].getService(Ci.nsISupports);
    args = service.wrappedJSObject.cmdLine;
  } catch(e) {}

  if (!args) {
    return;
  }

  let appname;

  
  try {
    
    
    
    
    appname = args.handleFlagWithParam('runapp', false);
  } catch(e) {
    
    appname = '';
  }

  
  if (appname === null) {
    return;
  }

  runAppObj = new AppRunner(appname);
  Services.obs.addObserver(runAppObj, 'remote-browser-shown', false);
  Services.obs.addObserver(runAppObj, 'inprocess-browser-shown', false);
});

window.addEventListener('unload', function() {
  if (runAppObj) {
    Services.obs.removeObserver(runAppObj, 'remote-browser-shown');
    Services.obs.removeObserver(runAppObj, 'inprocess-browser-shown');
  }
});

function AppRunner(aName) {
  this._appName = aName;
  this._apps = [];
}
AppRunner.prototype = {
  observe: function(aSubject, aTopic, aData) {
    let frameLoader = aSubject;
    
    frameLoader.QueryInterface(Ci.nsIFrameLoader);
    
    if (!frameLoader.ownerIsBrowserOrAppFrame) {
      return;
    }

    let frame = frameLoader.ownerElement;
    if (!frame.appManifestURL) { 
      return;
    }

    if (aTopic == 'remote-browser-shown' ||
        aTopic == 'inprocess-browser-shown') {
      this.doRunApp(frame);
    }
  },

  doRunApp: function(currentFrame) {
    
    if (this._apps.length) {
      this.getAllSuccess(this._apps, currentFrame)
    } else {
      var req = navigator.mozApps.mgmt.getAll();
      req.onsuccess = function() {
        this._apps = req.result;
        this.getAllSuccess(this._apps, currentFrame)
      }.bind(this);
      req.onerror = this.getAllError.bind(this);
    }
  },

  getAllSuccess: function(apps, currentFrame) {
    function findAppWithName(name) {
      let normalizedSearchName = name.replace(/[- ]+/g, '').toLowerCase();

      for (let i = 0; i < apps.length; i++) {
        let app = apps[i];
        let normalizedAppName =
          app.manifest.name.replace(/[- ]+/g, '').toLowerCase();
        if (normalizedSearchName === normalizedAppName) {
          return app;
        }
      }
      return null;
    }

    function usageAndDie(justApps) {
      if (!justApps)
        dump(
          'The --runapp argument specifies an app to automatically run at\n'+
          'startup.  We match against app names per their manifest and \n' +
          'ignoring capitalization, dashes, and whitespace.\n' +
          '\nThe system will load as usual except the lock screen will be ' +
          'automatically be disabled.\n\n' +
          'Known apps:\n');

      for (let i = 0; i < apps.length; i++) {
        dump('  ' + apps[i].manifest.name + '\n');
      }

      
      Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
    }

    if (this._appName === '') {
      usageAndDie();
      return;
    }

    let appsService = Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);
    let currentApp = appsService.getAppByManifestURL(currentFrame.appManifestURL);

    if (!currentApp || currentApp.role !== 'homescreen') {
      return;
    }

    let app = findAppWithName(this._appName);
    if (!app) {
      dump('Could not find app: "' + this._appName + '". Maybe you meant one of:\n');
      usageAndDie(true);
      return;
    }

    currentFrame.addEventListener('mozbrowserloadend', launchApp);

    function launchApp() {
      currentFrame.removeEventListener('mozbrowserloadend', launchApp);

      let setReq =
        navigator.mozSettings.createLock().set({'lockscreen.enabled': false});
      setReq.onsuccess = function() {
        
        window.setTimeout(function() {
          dump('--runapp launching app: ' + app.manifest.name + '\n');
          app.launch();
        }, 100);
      };
      setReq.onerror = function() {
        dump('--runapp failed to disable lock-screen.  Giving up.\n');
      };

      dump('--runapp found app: ' + app.manifest.name +
           ', disabling lock screen...\n');

      
      Services.obs.removeObserver(runAppObj, 'remote-browser-shown');
      Services.obs.removeObserver(runAppObj, 'inprocess-browser-shown');
    }
  },

  getAllError: function() {
    dump('Problem getting the list of all apps!');
  }
};
