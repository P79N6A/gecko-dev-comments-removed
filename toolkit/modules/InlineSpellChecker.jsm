



this.EXPORTED_SYMBOLS = [ "InlineSpellChecker",
                          "SpellCheckHelper" ];
var gLanguageBundle;
var gRegionBundle;
const MAX_UNDO_STACK_DEPTH = 1;

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

this.InlineSpellChecker = function InlineSpellChecker(aEditor) {
  this.init(aEditor);
  this.mAddedWordStack = []; 
}

InlineSpellChecker.prototype = {
  
  init: function(aEditor)
  {
    this.uninit();
    this.mEditor = aEditor;
    try {
      this.mInlineSpellChecker = this.mEditor.getInlineSpellChecker(true);
      
    } catch(e) {
      this.mInlineSpellChecker = null;
    }
  },

  initFromRemote: function(aSpellInfo)
  {
    if (this.mRemote)
      throw new Error("Unexpected state");
    this.uninit();

    if (!aSpellInfo)
      return;
    this.mInlineSpellChecker = this.mRemote = new RemoteSpellChecker(aSpellInfo);
    this.mOverMisspelling = aSpellInfo.overMisspelling;
    this.mMisspelling = aSpellInfo.misspelling;
  },

  
  uninit: function()
  {
    if (this.mRemote) {
      this.mRemote.uninit();
      this.mRemote = null;
    }

    this.mEditor = null;
    this.mInlineSpellChecker = null;
    this.mOverMisspelling = false;
    this.mMisspelling = "";
    this.mMenu = null;
    this.mSpellSuggestions = [];
    this.mSuggestionItems = [];
    this.mDictionaryMenu = null;
    this.mDictionaryNames = [];
    this.mDictionaryItems = [];
    this.mWordNode = null;
  },

  
  
  initFromEvent: function(rangeParent, rangeOffset)
  {
    this.mOverMisspelling = false;

    if (!rangeParent || !this.mInlineSpellChecker)
      return;

    var selcon = this.mEditor.selectionController;
    var spellsel = selcon.getSelection(selcon.SELECTION_SPELLCHECK);
    if (spellsel.rangeCount == 0)
      return; 

    var range = this.mInlineSpellChecker.getMisspelledWord(rangeParent,
                                                          rangeOffset);
    if (! range)
      return; 

    this.mMisspelling = range.toString();
    this.mOverMisspelling = true;
    this.mWordNode = rangeParent;
    this.mWordOffset = rangeOffset;
  },

  
  
  get canSpellCheck()
  {
    
    
    if (this.mRemote)
      return this.mRemote.canSpellCheck;
    return this.mInlineSpellChecker != null;
  },

  get initialSpellCheckPending() {
    if (this.mRemote) {
      return this.mRemote.spellCheckPending;
    }
    return !!(this.mInlineSpellChecker &&
              !this.mInlineSpellChecker.spellChecker &&
              this.mInlineSpellChecker.spellCheckPending);
  },

  
  get enabled()
  {
    if (this.mRemote)
      return this.mRemote.enableRealTimeSpell;
    return (this.mInlineSpellChecker &&
            this.mInlineSpellChecker.enableRealTimeSpell);
  },
  set enabled(isEnabled)
  {
    if (this.mRemote)
      this.mRemote.setSpellcheckUserOverride(isEnabled);
    else if (this.mInlineSpellChecker)
      this.mEditor.setSpellcheckUserOverride(isEnabled);
  },

  
  get overMisspelling()
  {
    return this.mOverMisspelling;
  },

  
  
  addSuggestionsToMenu: function(menu, insertBefore, maxNumber)
  {
    if (!this.mRemote && (!this.mInlineSpellChecker || !this.mOverMisspelling))
      return 0; 

    var spellchecker = this.mRemote || this.mInlineSpellChecker.spellChecker;
    try {
      if (!this.mRemote && !spellchecker.CheckCurrentWord(this.mMisspelling))
        return 0;  
    } catch(e) {
        return 0;
    }

    this.mMenu = menu;
    this.mSpellSuggestions = [];
    this.mSuggestionItems = [];
    for (var i = 0; i < maxNumber; i ++) {
      var suggestion = spellchecker.GetSuggestedWord();
      if (! suggestion.length)
        break;
      this.mSpellSuggestions.push(suggestion);

      var item = menu.ownerDocument.createElement("menuitem");
      this.mSuggestionItems.push(item);
      item.setAttribute("label", suggestion);
      item.setAttribute("value", suggestion);
      
      
      var callback = function(me, val) { return function(evt) { me.replaceMisspelling(val); } };
      item.addEventListener("command", callback(this, i), true);
      item.setAttribute("class", "spell-suggestion");
      menu.insertBefore(item, insertBefore);
    }
    return this.mSpellSuggestions.length;
  },

  
  
  clearSuggestionsFromMenu: function()
  {
    for (var i = 0; i < this.mSuggestionItems.length; i ++) {
      this.mMenu.removeChild(this.mSuggestionItems[i]);
    }
    this.mSuggestionItems = [];
  },

  sortDictionaryList: function(list) {
    var sortedList = [];
    for (var i = 0; i < list.length; i ++) {
      sortedList.push({"id": list[i],
                       "label": this.getDictionaryDisplayName(list[i])});
    }
    sortedList.sort(function(a, b) {
      if (a.label < b.label)
        return -1;
      if (a.label > b.label)
        return 1;
      return 0;
    });

    return sortedList;
  },

  
  
  addDictionaryListToMenu: function(menu, insertBefore)
  {
    this.mDictionaryMenu = menu;
    this.mDictionaryNames = [];
    this.mDictionaryItems = [];

    if (!this.enabled)
      return 0;

    var list;
    var curlang = "";
    if (this.mRemote) {
      list = this.mRemote.dictionaryList;
      curlang = this.mRemote.currentDictionary;
    }
    else if (this.mInlineSpellChecker) {
      var spellchecker = this.mInlineSpellChecker.spellChecker;
      var o1 = {}, o2 = {};
      spellchecker.GetDictionaryList(o1, o2);
      list = o1.value;
      var listcount = o2.value;
      try {
        curlang = spellchecker.GetCurrentDictionary();
      } catch(e) {}
    }

    var sortedList = this.sortDictionaryList(list);

    for (var i = 0; i < sortedList.length; i ++) {
      this.mDictionaryNames.push(sortedList[i].id);
      var item = menu.ownerDocument.createElement("menuitem");
      item.setAttribute("id", "spell-check-dictionary-" + sortedList[i].id);
      item.setAttribute("label", sortedList[i].label);
      item.setAttribute("type", "radio");
      this.mDictionaryItems.push(item);
      if (curlang == sortedList[i].id) {
        item.setAttribute("checked", "true");
      } else {
        var callback = function(me, val) {
          return function(evt) {
            me.selectDictionary(val);
          }
        };
        item.addEventListener("command", callback(this, i), true);
      }
      if (insertBefore)
        menu.insertBefore(item, insertBefore);
      else
        menu.appendChild(item);
    }
    return list.length;
  },

  
  getDictionaryDisplayName: function(dictionaryName) {
    try {
      
      let languageTagMatch = /^([a-z]{2,3}|[a-z]{4}|[a-z]{5,8})(?:[-_]([a-z]{4}))?(?:[-_]([A-Z]{2}|[0-9]{3}))?((?:[-_](?:[a-z0-9]{5,8}|[0-9][a-z0-9]{3}))*)(?:[-_][a-wy-z0-9](?:[-_][a-z0-9]{2,8})+)*(?:[-_]x(?:[-_][a-z0-9]{1,8})+)?$/i;
      var [languageTag, languageSubtag, scriptSubtag, regionSubtag, variantSubtags] = dictionaryName.match(languageTagMatch);
    } catch(e) {
      
      return dictionaryName;
    }

    if (!gLanguageBundle) {
      
      var bundleService = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                    .getService(Components.interfaces.nsIStringBundleService);
      gLanguageBundle = bundleService.createBundle(
          "chrome://global/locale/languageNames.properties");
      gRegionBundle = bundleService.createBundle(
          "chrome://global/locale/regionNames.properties");
    }

    var displayName = "";

    
    try {
      displayName += gLanguageBundle.GetStringFromName(languageSubtag.toLowerCase());
    } catch(e) {
      displayName += languageSubtag.toLowerCase(); 
    }

    
    if (regionSubtag) {
      displayName += " (";

      try {
        displayName += gRegionBundle.GetStringFromName(regionSubtag.toLowerCase());
      } catch(e) {
        displayName += regionSubtag.toUpperCase(); 
      }

      displayName += ")";
    }

    
    if (scriptSubtag) {
      displayName += " / ";

      
      displayName += scriptSubtag; 
    }

    
    if (variantSubtags)
      
      displayName += " (" + variantSubtags.substr(1).split(/[-_]/).join(" / ") + ")"; 

    return displayName;
  },

  
  
  clearDictionaryListFromMenu: function()
  {
    for (var i = 0; i < this.mDictionaryItems.length; i ++) {
      this.mDictionaryMenu.removeChild(this.mDictionaryItems[i]);
    }
    this.mDictionaryItems = [];
  },

  
  selectDictionary: function(index)
  {
    if (this.mRemote) {
      this.mRemote.selectDictionary(index);
      return;
    }
    if (! this.mInlineSpellChecker || index < 0 || index >= this.mDictionaryNames.length)
      return;
    var spellchecker = this.mInlineSpellChecker.spellChecker;
    spellchecker.SetCurrentDictionary(this.mDictionaryNames[index]);
    this.mInlineSpellChecker.spellCheckRange(null); 
  },

  
  replaceMisspelling: function(index)
  {
    if (this.mRemote) {
      this.mRemote.replaceMisspelling(index);
      return;
    }
    if (! this.mInlineSpellChecker || ! this.mOverMisspelling)
      return;
    if (index < 0 || index >= this.mSpellSuggestions.length)
      return;
    this.mInlineSpellChecker.replaceWord(this.mWordNode, this.mWordOffset,
                                         this.mSpellSuggestions[index]);
  },

  
  toggleEnabled: function()
  {
    if (this.mRemote)
      this.mRemote.toggleEnabled();
    else
      this.mEditor.setSpellcheckUserOverride(!this.mInlineSpellChecker.enableRealTimeSpell);
  },

  
  addToDictionary: function()
  {
    
    if (this.mAddedWordStack.length == MAX_UNDO_STACK_DEPTH)
      this.mAddedWordStack.shift();

    this.mAddedWordStack.push(this.mMisspelling);
    if (this.mRemote)
      this.mRemote.addToDictionary();
    else {
      this.mInlineSpellChecker.addWordToDictionary(this.mMisspelling);
    }
  },
  
  undoAddToDictionary: function()
  {
    if (this.mAddedWordStack.length > 0)
    {
      var word = this.mAddedWordStack.pop();
      if (this.mRemote)
        this.mRemote.undoAddToDictionary(word);
      else
        this.mInlineSpellChecker.removeWordFromDictionary(word);
    }
  },
  canUndo : function()
  {
    
    return (this.mAddedWordStack.length > 0);
  },
  ignoreWord: function()
  {
    if (this.mRemote)
      this.mRemote.ignoreWord();
    else
      this.mInlineSpellChecker.ignoreWord(this.mMisspelling);
  }
};

var SpellCheckHelper = {
  
  EDITABLE: 0x1,

  
  INPUT: 0x2,

  
  TEXTAREA: 0x4,

  
  TEXTINPUT: 0x8,

  
  KEYWORD: 0x10,

  
  
  CONTENTEDITABLE: 0x20,

  
  NUMERIC: 0x40,

  isTargetAKeywordField(aNode, window) {
    if (!(aNode instanceof window.HTMLInputElement))
      return false;

    var form = aNode.form;
    if (!form || aNode.type == "password")
      return false;

    var method = form.method.toUpperCase();

    
    
    
    
    
    
    
    
    
    
    return (method == "GET" || method == "") ||
           (form.enctype != "text/plain") && (form.enctype != "multipart/form-data");
  },

  
  getComputedStyle(aElem, aProp) {
    return aElem.ownerDocument
                .defaultView
                .getComputedStyle(aElem, "").getPropertyValue(aProp);
  },

  isEditable(element, window) {
    var flags = 0;
    if (element instanceof window.HTMLInputElement) {
      flags |= this.INPUT;

      if (element.mozIsTextField(false) || element.type == "number") {
        flags |= this.TEXTINPUT;

        if (element.type == "number") {
          flags |= this.NUMERIC;
        }

        
        if (!element.readOnly &&
            (element.type == "text" || element.type == "search")) {
          flags |= this.EDITABLE;
        }
        if (this.isTargetAKeywordField(element, window))
          flags |= this.KEYWORD;
      }
    } else if (element instanceof window.HTMLTextAreaElement) {
      flags |= this.TEXTINPUT | this.TEXTAREA;
      if (!element.readOnly) {
        flags |= this.EDITABLE;
      }
    }

    if (!(flags & this.EDITABLE)) {
      var win = element.ownerDocument.defaultView;
      if (win) {
        var isEditable = false;
        try {
          var editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebNavigation)
                                  .QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIEditingSession);
          if (editingSession.windowIsEditable(win) &&
              this.getComputedStyle(element, "-moz-user-modify") == "read-write") {
            isEditable = true;
          }
        }
        catch(ex) {
          
        }

        if (isEditable)
          flags |= this.CONTENTEDITABLE;
      }
    }

    return flags;
  },
};

function RemoteSpellChecker(aSpellInfo) {
  this._spellInfo = aSpellInfo;
  this._suggestionGenerator = null;
}

RemoteSpellChecker.prototype = {
  get canSpellCheck() { return this._spellInfo.canSpellCheck; },
  get spellCheckPending() { return this._spellInfo.initialSpellCheckPending; },
  get overMisspelling() { return this._spellInfo.overMisspelling; },
  get enableRealTimeSpell() { return this._spellInfo.enableRealTimeSpell; },

  GetSuggestedWord() {
    if (!this._suggestionGenerator) {
      this._suggestionGenerator = (function*(spellInfo) {
        for (let i of spellInfo.spellSuggestions)
          yield i;
      })(this._spellInfo);
    }

    let next = this._suggestionGenerator.next();
    if (next.done) {
      this._suggestionGenerator = null;
      return "";
    }
    return next.value;
  },

  get currentDictionary() { return this._spellInfo.currentDictionary },
  get dictionaryList() { return this._spellInfo.dictionaryList.slice(); },

  selectDictionary(index) {
    this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:selectDictionary",
                                            { index });
  },

  replaceMisspelling(index) {
    this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:replaceMisspelling",
                                            { index });
  },

  toggleEnabled() { this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:toggleEnabled", {}); },
  addToDictionary() {
    
    
    
    
    
    
    

    let dictionary = Cc["@mozilla.org/spellchecker/personaldictionary;1"]
                       .getService(Ci.mozIPersonalDictionary);
    dictionary.addWord(this._spellInfo.misspelling, "");

    this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:recheck", {});
  },
  undoAddToDictionary(word) {
    let dictionary = Cc["@mozilla.org/spellchecker/personaldictionary;1"]
                       .getService(Ci.mozIPersonalDictionary);
    dictionary.removeWord(word, "");

    this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:recheck", {});
  },
  ignoreWord() {
    let dictionary = Cc["@mozilla.org/spellchecker/personaldictionary;1"]
                       .getService(Ci.mozIPersonalDictionary);
    dictionary.ignoreWord(this._spellInfo.misspelling);

    this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:recheck", {});
  },
  uninit() { this._spellInfo.target.sendAsyncMessage("InlineSpellChecker:uninit", {}); }
};
