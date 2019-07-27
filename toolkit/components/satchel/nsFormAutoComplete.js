




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
                                  "resource://gre/modules/Deprecated.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
                                  "resource://gre/modules/FormHistory.jsm");

function FormAutoComplete() {
    this.init();
}






FormAutoComplete.prototype = {
    classID          : Components.ID("{c11c21b2-71c9-4f87-a0f8-5e13f50495fd}"),
    QueryInterface   : XPCOMUtils.generateQI([Ci.nsIFormAutoComplete, Ci.nsISupportsWeakReference]),

    _prefBranch         : null,
    _debug              : true, 
    _enabled            : true, 
    _agedWeight         : 2,
    _bucketSize         : 1,
    _maxTimeGroupings   : 25,
    _timeGroupingSize   : 7 * 24 * 60 * 60 * 1000 * 1000,
    _expireDays         : null,
    _boundaryWeight     : 25,
    _prefixWeight       : 5,

    
    
    
    
    
    _pendingQuery       : null,

    init : function() {
        
        this._prefBranch = Services.prefs.getBranch("browser.formfill.");
        this._prefBranch.addObserver("", this.observer, true);
        this.observer._self = this;

        this._debug            = this._prefBranch.getBoolPref("debug");
        this._enabled          = this._prefBranch.getBoolPref("enable");
        this._agedWeight       = this._prefBranch.getIntPref("agedWeight");
        this._bucketSize       = this._prefBranch.getIntPref("bucketSize");
        this._maxTimeGroupings = this._prefBranch.getIntPref("maxTimeGroupings");
        this._timeGroupingSize = this._prefBranch.getIntPref("timeGroupingSize") * 1000 * 1000;
        this._expireDays       = this._prefBranch.getIntPref("expire_days");
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

                switch (prefName) {
                    case "agedWeight":
                        self._agedWeight = self._prefBranch.getIntPref(prefName);
                        break;
                    case "debug":
                        self._debug = self._prefBranch.getBoolPref(prefName);
                        break;
                    case "enable":
                        self._enabled = self._prefBranch.getBoolPref(prefName);
                        break;
                    case "maxTimeGroupings":
                        self._maxTimeGroupings = self._prefBranch.getIntPref(prefName);
                        break;
                    case "timeGroupingSize":
                        self._timeGroupingSize = self._prefBranch.getIntPref(prefName) * 1000 * 1000;
                        break;
                    case "bucketSize":
                        self._bucketSize = self._prefBranch.getIntPref(prefName);
                        break;
                    case "boundaryWeight":
                        self._boundaryWeight = self._prefBranch.getIntPref(prefName);
                        break;
                    case "prefixWeight":
                        self._prefixWeight = self._prefBranch.getIntPref(prefName);
                        break;
                    default:
                        self.log("Oops! Pref not handled, change ignored.");
                }
            }
        }
    },


    





    log : function (message) {
        if (!this._debug)
            return;
        dump("FormAutoComplete: " + message + "\n");
        Services.console.logStringMessage("FormAutoComplete: " + message);
    },

    









    autoCompleteSearchAsync : function (aInputName, aUntrimmedSearchString, aField, aPreviousResult, aListener) {
        function sortBytotalScore (a, b) {
            return b.totalScore - a.totalScore;
        }

        let result = null;
        if (!this._enabled) {
            result = new FormAutoCompleteResult(FormHistory, [], aInputName, aUntrimmedSearchString);
            if (aListener) {
              aListener.onSearchCompletion(result);
            }
            return;
        }

        
        if (aInputName == 'searchbar-history' && aField) {
            this.log('autoCompleteSearch for input name "' + aInputName + '" is denied');
            result = new FormAutoCompleteResult(FormHistory, [], aInputName, aUntrimmedSearchString);
            if (aListener) {
              aListener.onSearchCompletion(result);
            }
            return;
        }

        this.log("AutoCompleteSearch invoked. Search is: " + aUntrimmedSearchString);
        let searchString = aUntrimmedSearchString.trim().toLowerCase();

        
        
        
        if (aPreviousResult && aPreviousResult.searchString.trim().length > 1 &&
            searchString.indexOf(aPreviousResult.searchString.trim().toLowerCase()) >= 0) {
            this.log("Using previous autocomplete result");
            result = aPreviousResult;
            result.wrappedJSObject.searchString = aUntrimmedSearchString;

            let searchTokens = searchString.split(/\s+/);
            
            
            let entries = result.wrappedJSObject.entries;
            let filteredEntries = [];
            for (let i = 0; i < entries.length; i++) {
                let entry = entries[i];
                
                
                if(searchTokens.some(function (tok) entry.textLowerCase.indexOf(tok) < 0))
                    continue;
                this._calculateScore(entry, searchString, searchTokens);
                this.log("Reusing autocomplete entry '" + entry.text +
                         "' (" + entry.frecency +" / " + entry.totalScore + ")");
                filteredEntries.push(entry);
            }
            filteredEntries.sort(sortBytotalScore);
            result.wrappedJSObject.entries = filteredEntries;

            if (aListener) {
              aListener.onSearchCompletion(result);
            }
        } else {
            this.log("Creating new autocomplete search result.");

            
            result = new FormAutoCompleteResult(FormHistory, [], aInputName, aUntrimmedSearchString);

            let processEntry = function(aEntries) {
              if (aField && aField.maxLength > -1) {
                result.entries =
                  aEntries.filter(function (el) { return el.text.length <= aField.maxLength; });
              } else {
                result.entries = aEntries;
              }

              if (aListener) {
                aListener.onSearchCompletion(result);
              }
            }

            this.getAutoCompleteValues(aInputName, searchString, processEntry);
        }
    },

    stopAutoCompleteSearch : function () {
        if (this._pendingQuery) {
            this._pendingQuery.cancel();
            this._pendingQuery = null;
        }
    },

    








    getAutoCompleteValues : function (fieldName, searchString, callback) {
        let params = {
            agedWeight:         this._agedWeight,
            bucketSize:         this._bucketSize,
            expiryDate:         1000 * (Date.now() - this._expireDays * 24 * 60 * 60 * 1000),
            fieldname:          fieldName,
            maxTimeGroupings:   this._maxTimeGroupings,
            timeGroupingSize:   this._timeGroupingSize,
            prefixWeight:       this._prefixWeight,
            boundaryWeight:     this._boundaryWeight
        }

        this.stopAutoCompleteSearch();

        let results = [];
        let processResults = {
          handleResult: aResult => {
            results.push(aResult);
          },
          handleError: aError => {
            this.log("getAutocompleteValues failed: " + aError.message);
          },
          handleCompletion: aReason => {
            
            
            
            if (query == this._pendingQuery) {
              this._pendingQuery = null;
              if (!aReason) {
                callback(results);
              }
            }
          }
        };

        let query = FormHistory.getAutoCompleteResults(searchString, params, processResults);
        this._pendingQuery = query;
    },

    








    _calculateScore : function (entry, aSearchString, searchTokens) {
        let boundaryCalc = 0;
        
        for each (let token in searchTokens) {
            boundaryCalc += (entry.textLowerCase.indexOf(token) == 0);
            boundaryCalc += (entry.textLowerCase.indexOf(" " + token) >= 0);
        }
        boundaryCalc = boundaryCalc * this._boundaryWeight;
        
        
        boundaryCalc += this._prefixWeight *
                        (entry.textLowerCase.
                         indexOf(aSearchString) == 0);
        entry.totalScore = Math.round(entry.frecency * Math.max(1, boundaryCalc));
    }

}; 









function FormAutoCompleteChild() {
  this.init();
}

FormAutoCompleteChild.prototype = {
    classID          : Components.ID("{c11c21b2-71c9-4f87-a0f8-5e13f50495fd}"),
    QueryInterface   : XPCOMUtils.generateQI([Ci.nsIFormAutoComplete, Ci.nsISupportsWeakReference]),

    _debug: false,
    _enabled: true,
    _pendingSearch: null,

    






    init: function() {
      this._debug    = Services.prefs.getBoolPref("browser.formfill.debug");
      this._enabled  = Services.prefs.getBoolPref("browser.formfill.enable");
      this.log("init");
    },

    




    log : function (message) {
      if (!this._debug)
        return;
      dump("FormAutoCompleteChild: " + message + "\n");
    },

    autoCompleteSearchAsync : function (aInputName, aUntrimmedSearchString, aField, aPreviousResult, aListener) {
      this.log("autoCompleteSearchAsync");

      if (this._pendingSearch) {
        this.stopAutoCompleteSearch();
      }

      let window = aField.ownerDocument.defaultView;

      let rect = BrowserUtils.getElementBoundingScreenRect(aField);
      let direction = window.getComputedStyle(aField).direction;
      let topLevelDocshell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIDocShell)
                                   .sameTypeRootTreeItem
                                   .QueryInterface(Ci.nsIDocShell);

      let mm = topLevelDocshell.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIContentFrameMessageManager);

      mm.sendAsyncMessage("FormHistory:AutoCompleteSearchAsync", {
        inputName: aInputName,
        untrimmedSearchString: aUntrimmedSearchString,
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        direction: direction,
      });

      let search = this._pendingSearch = {};
      let searchFinished = message => {
        mm.removeMessageListener("FormAutoComplete:AutoCompleteSearchAsyncResult", searchFinished);

        
        
        if (search != this._pendingSearch) {
          return;
        }
        this._pendingSearch = null;

        let result = new FormAutoCompleteResult(
          null,
          [for (res of message.data.results) {text: res}],
          null,
          null
        );
        if (aListener) {
          aListener.onSearchCompletion(result);
        }
      }

      mm.addMessageListener("FormAutoComplete:AutoCompleteSearchAsyncResult", searchFinished);
      this.log("autoCompleteSearchAsync message was sent");
    },

    stopAutoCompleteSearch : function () {
      this.log("stopAutoCompleteSearch");
      this._pendingSearch = null;
    },
}; 


function FormAutoCompleteResult (formHistory, entries, fieldName, searchString) {
    this.formHistory = formHistory;
    this.entries = entries;
    this.fieldName = fieldName;
    this.searchString = searchString;
}

FormAutoCompleteResult.prototype = {
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIAutoCompleteResult,
                                            Ci.nsISupportsWeakReference]),

    
    formHistory : null,
    entries : null,
    fieldName : null,

    _checkIndexBounds : function (index) {
        if (index < 0 || index >= this.entries.length)
            throw Components.Exception("Index out of range.", Cr.NS_ERROR_ILLEGAL_VALUE);
    },

    
    
    get wrappedJSObject() {
        return this;
    },

    
    searchString : null,
    errorDescription : "",
    get defaultIndex() {
        if (this.entries.length == 0)
            return -1;
        else
            return 0;
    },
    get searchResult() {
        if (this.entries.length == 0)
            return Ci.nsIAutoCompleteResult.RESULT_NOMATCH;
        return Ci.nsIAutoCompleteResult.RESULT_SUCCESS;
    },
    get matchCount() {
        return this.entries.length;
    },

    getValueAt : function (index) {
        this._checkIndexBounds(index);
        return this.entries[index].text;
    },

    getLabelAt: function(index) {
        return getValueAt(index);
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

    getFinalCompleteValueAt : function (index) {
        return this.getValueAt(index);
    },

    removeValueAt : function (index, removeFromDB) {
        this._checkIndexBounds(index);

        let [removedEntry] = this.entries.splice(index, 1);

        if (removeFromDB) {
          this.formHistory.update({ op: "remove",
                                    fieldname: this.fieldName,
                                    value: removedEntry.text });
        }
    }
};


if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT &&
    Services.prefs.getBoolPref("browser.tabs.remote.desktopbehavior", false)) {
  
  
  let component = [FormAutoCompleteChild];
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
} else {
  let component = [FormAutoComplete];
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
}
