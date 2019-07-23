














































function nsCloseAllWindows() {
}

nsCloseAllWindows.prototype = {

    
    QueryInterface: function (iid) {
        if (iid.equals(Components.interfaces.nsICloseAllWindows) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    

    
    closeAll: function(aAskToSave)  {

        var windowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].
                                getService(Components.interfaces.nsIWindowMediator);
        var enumerator = windowMediator.getEnumerator(null);

        while (enumerator.hasMoreElements()) {
           var domWindow = enumerator.getNext();
           if (aAskToSave && ("tryToClose" in domWindow)) {
               if (!domWindow.tryToClose())
                   return false;
           }
           domWindow.close();
        };

        return true;
    }
}



var module = {
    firstTime: true,

    
    registerSelf: function (compMgr, fileSpec, location, type) {
        if (this.firstTime) {
            this.firstTime = false;
            throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
        }
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation( this.cid,
                                         "Close All Windows",
                                         this.contractId,
                                         fileSpec,
                                         location,
                                         type );
    },

    
    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.cid)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        if (!iid.equals(Components.interfaces.nsIFactory)) {
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        }

        return this.factory;
    },

    
    cid: Components.ID("{2f977d48-5485-11d4-87e2-0010a4e75ef2}"),

    
    contractId: "@mozilla.org/appshell/closeallwindows;1",

    
    factory: {
        
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new nsCloseAllWindows()).QueryInterface(iid);
        }
    },

    
    canUnload: function(compMgr) {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return module;
}
