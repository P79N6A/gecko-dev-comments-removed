










































var module = {
    categoryEntry: "pluginfinder xbl binding",
    categoryValue: "chrome://mozapps/content/plugins/missingPluginBinding.css",

    
    registerSelf: function (compMgr, fileSpec, location, type) {
      var catman = Components.classes['@mozilla.org/categorymanager;1']
                             .getService(Components.interfaces.nsICategoryManager);
      catman.addCategoryEntry("agent-style-sheets", this.categoryEntry,
                              this.categoryValue, true, true);

    },

    
    unregisterSelf: function (aCompMgr, aLocation, aLoaderStr) {
      catman.deleteCategoryEntry("agent-style-sheets", this.categoryEntry,
                                 true);
    },

    
    getClassObject: function (compMgr, cid, iid) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },

    
    canUnload: function(compMgr) {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return module;
}

