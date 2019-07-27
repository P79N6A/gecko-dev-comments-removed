



this.EXPORTED_SYMBOLS = [ "FormAutoCompleteResult" ];

const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

this.FormAutoCompleteResult =
 function FormAutoCompleteResult(searchString,
                                 searchResult,
                                 defaultIndex,
                                 errorDescription,
                                 values,
                                 labels,
                                 comments,
                                 prevResult) {
  this.searchString = searchString;
  this._searchResult = searchResult;
  this._defaultIndex = defaultIndex;
  this._errorDescription = errorDescription;
  this._values = values;
  this._labels = labels;
  this._comments = comments;
  this._formHistResult = prevResult;

  if (prevResult) {
    this.entries = prevResult.wrappedJSObject.entries;
  } else {
    this.entries = [];
  }
}

FormAutoCompleteResult.prototype = {

  
  searchString: "",

  
  _searchResult: 0,

  
  _defaultIndex: 0,

  
  _errorDescription: "",

  



  _formHistResult: null,

  entries: null,

  get wrappedJSObject() {
    return this;
  },

  






  get searchResult() {
    return this._searchResult;
  },

  


  get defaultIndex() {
    return this._defaultIndex;
  },

  


  get errorDescription() {
    return this._errorDescription;
  },

  


  get matchCount() {
    return this._values.length;
  },

  _checkIndexBounds : function (index) {
    if (index < 0 || index >= this._values.length) {
      throw Components.Exception("Index out of range.", Cr.NS_ERROR_ILLEGAL_VALUE);
    }
  },

  




  getValueAt: function(index) {
    this._checkIndexBounds(index);
    return this._values[index];
  },

  getLabelAt: function(index) {
    this._checkIndexBounds(index);
    return this._labels[index];
  },

  




  getCommentAt: function(index) {
    this._checkIndexBounds(index);
    return this._comments[index];
  },

  




  getStyleAt: function(index) {
    this._checkIndexBounds(index);

    if (this._formHistResult && index < this._formHistResult.matchCount) {
      return "fromhistory";
    }

    if (!this._comments[index]) {
      return null;  
    }

    if (index == 0) {
      return "suggestfirst";  
    }

    return "suggesthint";   
  },

  




  getImageAt: function(index) {
    this._checkIndexBounds(index);
    return "";
  },

  




  getFinalCompleteValueAt: function(index) {
    return this.getValueAt(index);
  },

  



  removeValueAt: function(index, removeFromDatabase) {
    this._checkIndexBounds(index);
    
    
    
    if (removeFromDatabase && this._formHistResult &&
        index < this._formHistResult.matchCount) {
      
      this._formHistResult.removeValueAt(index, true);
    }
    this._values.splice(index, 1);
    this._labels.splice(index, 1);
    this._comments.splice(index, 1);
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteResult])
};
