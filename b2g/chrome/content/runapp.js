


window.addEventListener('load', function() {
  
  let args = window.arguments[0].QueryInterface(Ci.nsICommandLine);
  let appname;

  
  try {
    
    
    
    
    appname = args.handleFlagWithParam('runapp', false);
  }
  catch(e) {
    
    appname = '';
  }

  
  if (appname === null)
    return;

  
  let appsReq = navigator.mozApps.mgmt.getAll();
  appsReq.onsuccess = function() {
    let apps = appsReq.result;
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

    if (appname === '') {
      usageAndDie();
      return;
    }

    let app = findAppWithName(appname);
    if (!app) {
      dump('Could not find app: "' + appname + '". Maybe you meant one of:\n');
      usageAndDie(true);
      return;
    }

    let setReq =
      navigator.mozSettings.getLock().set({'lockscreen.enabled': false});
    setReq.onsuccess = function() {
      
      window.setTimeout(function() {
        dump('--runapp launching app: ' + app.manifest.name + '\n');
        app.launch();
      }, 0);
    };
    setReq.onerror = function() {
      dump('--runapp failed to disable lock-screen.  Giving up.\n');
    };

    dump('--runapp found app: ' + app.manifest.name +
         ', disabling lock screen...\n');
 };
 appsReq.onerror = function() {
   dump('Problem getting the list of all apps!');
 };
});
