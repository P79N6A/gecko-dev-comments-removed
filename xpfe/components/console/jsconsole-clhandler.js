













































const JSCONSOLEHANDLER_CONTRACTID =
    "@mozilla.org/commandlinehandler/general-startup;1?type=jsconsole";

const JSCONSOLEHANDLER_CID = 
    Components.ID('{1698ef18-c128-41a1-b4d0-7f9acd2ae86c}');






function jsConsoleHandler() {}


jsConsoleHandler.prototype = {
    commandLineArgument: '-jsconsole',
    prefNameForStartup: 'general.startup.jsconsole',
    chromeUrlForTask: 'chrome://global/content/console.xul',
    helpText: 'Start with Javascript Console',
    handlesArgs: false,
    defaultArgs: null,
    openWindowWithArgs: false
};






var jsConsoleHandlerModule = {
    registerSelf: function(compMgr, fileSpec, location, type) {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);

        compMgr.registerFactoryLocation(JSCONSOLEHANDLER_CID, 
                                        'JS Console Commandline Handler component',
                                        JSCONSOLEHANDLER_CONTRACTID, 
                                        fileSpec,
                                        location, 
                                        type);
        var catman = Components.classes["@mozilla.org/categorymanager;1"]
            .getService(Components.interfaces.nsICategoryManager);
        catman.addCategoryEntry("command-line-argument-handlers", "jsconsole command line handler",
            JSCONSOLEHANDLER_CONTRACTID,
            true, true);
    },

    unregisterSelf: function(compMgr, fileSpec, location) {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.unregisterFactoryLocation(JSCONSOLEHANDLER_CID, fileSpec);
        var catman = Components.classes["@mozilla.org/categorymanager;1"]
            .getService(Components.interfaces.nsICategoryManager);
        catman.deleteCategoryEntry("command-line-argument-handlers",
            JSCONSOLEHANDLER_CONTRACTID, true);
    },

    getClassObject: function(compMgr, cid, iid) {
        if (!cid.equals(JSCONSOLEHANDLER_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return jsConsoleHandlerFactory;
    },

    canUnload: function(compMgr) { return true; }
};


var jsConsoleHandlerFactory = {
    createInstance: function(outer, iid) {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;
    
        if (!iid.equals(Components.interfaces.nsICmdLineHandler) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return new jsConsoleHandler();
    }
}






function NSGetModule(comMgr, fileSpec) { return jsConsoleHandlerModule; }


