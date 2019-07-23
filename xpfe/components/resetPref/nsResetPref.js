


























































function nsResetPref() {
}

nsResetPref.prototype = {
    
    debug: false,

    
    get commandLineArgument() { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get prefNameForStartup()  { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },
    get chromeUrlForTask()    {
        try {
            
            

            
            var cmdLine  = Components.classes[ "@mozilla.org/app-startup/commandLineService;1" ]
                             .getService( Components.interfaces.nsICmdLineService );
            var prefList = cmdLine.getCmdLineValue( "-resetPref" ).split( "," );

            
            var prefs    = Components.classes[ "@mozilla.org/preferences-service;1" ]
                             .getService( Components.interfaces.nsIPrefService );

            
            for ( i in prefList ) {
                var pref = prefs.getBranch( prefList[ i ] );
                try {
                    pref.clearUserPref( "" );
                } catch( e ) {
                }
            }
        } catch( e ) {
            this.dump( "exception: " + e );
        }

        
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
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

    
    dump: function( text ) {
        if ( this.debug ) {
            dump( "nsResetPref: " + text + "\n" );
        }
    },

    
    
    module: {
        
        registerSelf: function (compMgr, fileSpec, location, type) {
            var compReg = compMgr.QueryInterface( Components.interfaces.nsIComponentRegistrar );
            compReg.registerFactoryLocation( this.cid,
                                             "Pref Reset Component",
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

        
        cid: Components.ID("{15ABFAF7-AD4F-4450-899B-0373EE9FAD95}"),

        
        contractId: "@mozilla.org/commandlinehandler/general-startup;1?type=resetPref",

        
        factory: {
            
            createInstance: function (outer, iid) {
                if (outer != null)
                    throw Components.results.NS_ERROR_NO_AGGREGATION;

                return (new nsResetPref()).QueryInterface(iid);
            }
        },

        
        canUnload: function(compMgr) {
            return true;
        }
    }
}


function NSGetModule(compMgr, fileSpec) {
    return nsResetPref.prototype.module;
}
