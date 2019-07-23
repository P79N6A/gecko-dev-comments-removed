


























































function nsSetDefaultMail() {
}

nsSetDefaultMail.prototype = {

    
    get commandLineArgument() { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get prefNameForStartup()  { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },

    get chromeUrlForTask()    {

      var mapiRegistry;
      try {
          var mapiRegistryProgID = "@mozilla.org/mapiregistry;1"
          
          if (mapiRegistryProgID in Components.classes) {
            mapiRegistry = Components.classes[mapiRegistryProgID].getService(Components.interfaces.nsIMapiRegistry);
          }
          else {
            mapiRegistry = null;
          }
      }
      catch (ex) {
          mapiRegistry = null;
      }

      
      if(mapiRegistry)
          mapiRegistry.isDefaultMailClient = true;

      
      var cmdLineService = Components.classes[ "@mozilla.org/app-startup/commandLineService;1" ]
                              .getService( Components.interfaces.nsICmdLineService );

      
      
      
      
      
      
      
      var option = cmdLineService.getCmdLineValue( "-setDefaultMail" );
      if (!option) {
        
        
        throw Components.results.NS_ERROR_NOT_AVAILABLE;
      }

      
      
      
      
      return "chrome://global/content/dummyWindow.xul";
    },

    get helpText()            { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get handlesArgs()         { return false; },
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
                                             "Set Mailnews as Default mail handler",
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

        
        cid: Components.ID("{8b26281d-c3b2-4b57-9653-419fc705a02d}"),

        
        contractId: "@mozilla.org/commandlinehandler/general-startup;1?type=setDefaultMail",

        
        factory: {
            
            createInstance: function (outer, iid) {
                if (outer != null)
                    throw Components.results.NS_ERROR_NO_AGGREGATION;

                return (new nsSetDefaultMail()).QueryInterface(iid);
            }
        },

        
        canUnload: function(compMgr) {
            return true;
        }
    }
}


function NSGetModule(compMgr, fileSpec) {
    return nsSetDefaultMail.prototype.module;
}
