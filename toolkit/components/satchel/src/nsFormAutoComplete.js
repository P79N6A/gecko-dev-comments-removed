




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function FormAutoComplete() {
    this.init();
}

FormAutoComplete.prototype = {
    classDescription: "FormAutoComplete",
    contractID: "@mozilla.org/satchel/form-autocomplete;1",
    classID: Components.ID("{c11c21b2-71c9-4f87-a0f8-5e13f50495fd}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIFormAutoComplete, Ci.nsISupportsWeakReference]),

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"].
                                getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    __formHistory : null,
    get _formHistory() {
        if (!this.__formHistory)
            this.__formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                                 getService(Ci.nsIFormHistory2);
        return this.__formHistory;
    },

    __observerService : null, 
    get _observerService() {
        if (!this.__observerService)
            this.__observerService = Cc["@mozilla.org/observer-service;1"].
                                     getService(Ci.nsIObserverService);
        return this.__observerService;
    },

    _prefBranch  : null,
    _enabled : true,  
    _debug   : false, 

    init : function() {
        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService).getBranch("browser.formfill.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);
        this._prefBranch.addObserver("", this.observer, false);
        this.observer._self = this;

        this._debug   = this._prefBranch.getBoolPref("debug");
        this._enabled = this._prefBranch.getBoolPref("enable");

        this._dbStmts = [];

        this._observerService.addObserver(this.observer, "xpcom-shutdown", false);
    },

    observer : {
        _self : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                                Ci.nsISupportsWeakReference]),

        observe : function (subject, topic, data) {
            let self = this._self;
            if (topic == "nsPref:changed") {
                let prefName = data;
                self.log("got change to " + prefName + " preference");

                if (prefName == "debug") {
                    self._debug = self._prefBranch.getBoolPref("debug");
                } else if (prefName == "enable") {
                    self._enabled = self._prefBranch.getBoolPref("enable");
                } else {
                    self.log("Oops! Pref not handled, change ignored.");
                }
            } else if (topic == "xpcom-shutdown") {
                self._dbStmts = null;
            }
        }
    },


    





    log : function (message) {
        if (!this._debug)
            return;
        dump("FormAutoComplete: " + message + "\n");
        this._logService.logStringMessage("FormAutoComplete: " + message);
    },


    








    autoCompleteSearch : function (aInputName, aSearchString, aPreviousResult) {
        if (!this._enabled)
            return null;

        this.log("AutoCompleteSearch invoked. Search is: " + aSearchString);

        let result = null;

        if (aPreviousResult) {
            this.log("Using previous autocomplete result");
            result = aPreviousResult;

            
            
            
            
            for (let i = result.matchCount - 1; i >= 0; i--) {
                let match = result.getValueAt(i);

                
                
                if (aSearchString.length > match.length ||
                    aSearchString.toLowerCase() !=
                        match.substr(0, aSearchString.length).toLowerCase())
                {
                    this.log("Removing autocomplete entry '" + match + "'");
                    result.removeValueAt(i, false);
                }
            }
        } else {
            this.log("Creating new autocomplete search result.");
            let entries = this.getAutoCompleteValues(aInputName, aSearchString);
            result = new FormAutoCompleteResult(this._formHistory, entries, aInputName, aSearchString);
        }

        return result;
    },

    getAutoCompleteValues : function (fieldName, searchString) {
        let values = [];

        let query = "SELECT value FROM moz_formhistory " +
                    "WHERE fieldname=:fieldname AND value LIKE :valuePrefix ESCAPE '/' " +
                    "ORDER BY UPPER(value) ASC";
        let params = {
            fieldname: fieldName,
            valuePrefix: null 
        }

        let stmt;
        try {
            stmt = this._dbCreateStatement(query, params);

            
            
            stmt.params.valuePrefix = stmt.escapeStringForLIKE(searchString, "/") + "%";

            while (stmt.step())
                values.push(stmt.row.value);
        } catch (e) {
            this.log("getValues failed: " + e.name + " : " + e.message);
            throw "DB failed getting form autocomplete falues";
        } finally {
            stmt.reset();
        }

        return values;
    },


    _dbStmts      : null,

    _dbCreateStatement : function (query, params) {
        let stmt = this._dbStmts[query];
        
        if (!stmt) {
            this.log("Creating new statement for query: " + query);
            stmt = this._formHistory.DBConnection.createStatement(query);
            this._dbStmts[query] = stmt;
        }
        
        if (params)
            for (let i in params)
                stmt.params[i] = params[i];
        return stmt;
    }

}; 





function FormAutoCompleteResult (formHistory, entries, fieldName, searchString) {
    this.formHistory = formHistory;
    this.entries = entries;
    this.fieldName = fieldName;
    this.searchString = searchString;

    if (entries.length > 0) {
        this.searchResult = Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
        this.defaultIndex = 0;
    }
}

FormAutoCompleteResult.prototype = {
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIAutoCompleteResult,
                                            Ci.nsISupportsWeakReference]),

    
    formHistory : null,
    entries : null,
    fieldName : null,

    _checkIndexBounds : function (index) {
        if (index < 0 || index >= this.entries.length)
            Components.Exception("Index out of range.", Cr.NS_ERROR_ILLEGAL_VALUE);
    },

    
    searchString : null,
    searchResult : Ci.nsIAutoCompleteResult.RESULT_NOMATCH,
    defaultIndex : -1,
    errorDescription : "",
    get matchCount() {
        return this.entries.length;
    },

    getValueAt : function (index) {
        this._checkIndexBounds(index);
        return this.entries[index];
    },

    getCommentAt : function (index) {
        this._checkIndexBounds(index);
        return "";
    },

    getStyleAt : function (index) {
        this._checkIndexBounds(index);
        return "";
    },

    getImageAt : function (index) {
        this._checkIndexBounds(index);
        return "";
    },

    removeValueAt : function (index, removeFromDB) {
        this._checkIndexBounds(index);

        let [removedEntry] = this.entries.splice(index, 1);

        if (this.defaultIndex > this.entries.length)
            this.defaultIndex--;

        if (removeFromDB)
            this.formHistory.removeEntry(this.fieldName, removedEntry);
    }
};

let component = [FormAutoComplete];
function NSGetModule (compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
