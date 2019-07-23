

























































function nsKillAll() {
}

nsKillAll.prototype = {

    
    get commandLineArgument() { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get prefNameForStartup()  { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },

    get chromeUrlForTask()    {

      

      var wasMozillaAlreadyRunning = false;
      var appStartup = Components.classes["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Components.interfaces.nsIAppStartup);
      var nativeAppSupport = {isServerMode: false};
      try {
        nativeAppSupport = appStartup.nativeAppSupport;
      } catch ( ex ) {
      }

      var originalServerMode = false;
      if (nativeAppSupport.isServerMode) {
        originalServerMode = true;
        wasMozillaAlreadyRunning = true;
        nativeAppSupport.isServerMode = false;
      }

      

      var gObserverService = Components.classes["@mozilla.org/observer-service;1"]
                             .getService(Components.interfaces.nsIObserverService);
      if (gObserverService) {
        try {
          gObserverService.notifyObservers(null, "quit-application", null);
        } catch (ex) {
          
        }
      }

      

      var windowManager =
        Components.classes['@mozilla.org/appshell/window-mediator;1']
        .getService(Components.interfaces.nsIWindowMediator);
      var enumerator = windowManager.getEnumerator(null);
      while(enumerator.hasMoreElements()) {
        wasMozillaAlreadyRunning = true;
        var domWindow = enumerator.getNext();
        if (("tryToClose" in domWindow) && !domWindow.tryToClose()) {
          
          nativeAppSupport.isServerMode = originalServerMode
          
          throw Components.results.NS_ERROR_ABORT;
        }
        domWindow.close();
      }

      

      if (wasMozillaAlreadyRunning) {
        
        appStartup.quit(Components.interfaces.nsIAppStartup.eAttemptQuit);
      }

      
      
      
      throw Components.results.NS_ERROR_NOT_AVAILABLE;
    },

    get helpText()            { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get handlesArgs()         { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get defaultArgs()         { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get openWindowWithArgs()  { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },

    

    
    QueryInterface: function (iid) {
        if (iid.equals(Components.interfaces.nsICmdLineHandler) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    
    
    module: {
        
        registerSelf: function (compMgr, fileSpec, location, type) {
            var compReg = compMgr.QueryInterface( Components.interfaces.nsIComponentRegistrar );
            compReg.registerFactoryLocation( this.cid,
                                             "Kill All Component",
                                             this.contractId,
                                             fileSpec,
                                             location,
                                             type );
        },

        
        getClassObject: function (compMgr, cid, iid) {
            if (!cid.equals(this.cid))
                throw Components.results.NS_ERROR_NO_INTERFACE;

            if (!iid.equals(Components.interfaces.nsIFactory))
                throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

            return this.factory;
        },

        
        cid: Components.ID("{F1F25940-4C8F-11d6-A651-0010A401EB10}"),

        
        contractId: "@mozilla.org/commandlinehandler/general-startup;1?type=killAll",

        
        factory: {
            
            createInstance: function (outer, iid) {
                if (outer != null)
                    throw Components.results.NS_ERROR_NO_AGGREGATION;

                return (new nsKillAll()).QueryInterface(iid);
            }
        },

        
        canUnload: function(compMgr) {
            return true;
        }
    }
}


function NSGetModule(compMgr, fileSpec) {
    return nsKillAll.prototype.module;
}
