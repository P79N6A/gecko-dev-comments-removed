


























































function nsSetDefaultBrowser() {
}

nsSetDefaultBrowser.prototype = {

    
    get commandLineArgument() { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get prefNameForStartup()  { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },

    get chromeUrlForTask()    {
      
      var winHooks = Components.classes[ "@mozilla.org/winhooks;1" ]
                        .getService( Components.interfaces.nsIWindowsHooks );

      
      
      var settings = winHooks.settings;

      
      settings.isHandlingHTTP  = true;
      settings.isHandlingHTTPS = true;
      settings.isHandlingFTP   = true;
      settings.isHandlingHTML  = true;
      settings.isHandlingXHTML = true;
      settings.isHandlingXML   = true;

      
      winHooks.settings = settings;

      
      var cmdLineService = Components.classes[ "@mozilla.org/app-startup/commandLineService;1" ]
                              .getService( Components.interfaces.nsICmdLineService );

      
      
      
      
      
      
      
      var option = cmdLineService.getCmdLineValue( "-setDefaultBrowser" );
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
                                             "Default Browser Component",
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

        
        cid: Components.ID("{C66E05DC-509C-4972-A1F2-EE5AC34B9800}"),

        
        contractId: "@mozilla.org/commandlinehandler/general-startup;1?type=setDefaultBrowser",

        
        factory: {
            
            createInstance: function (outer, iid) {
                if (outer != null)
                    throw Components.results.NS_ERROR_NO_AGGREGATION;

                return (new nsSetDefaultBrowser()).QueryInterface(iid);
            }
        },

        
        canUnload: function(compMgr) {
            return true;
        }
    }
}


function NSGetModule(compMgr, fileSpec) {
    return nsSetDefaultBrowser.prototype.module;
}
