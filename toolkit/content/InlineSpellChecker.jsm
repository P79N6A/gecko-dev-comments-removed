




































var EXPORTED_SYMBOLS = [ "InlineSpellChecker" ];
var gLanguageBundle;
var gRegionBundle;

function InlineSpellChecker(aEditor) {
  this.init(aEditor);
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

  
  uninit: function()
  {
    this.mInlineSpellChecker = null;
    this.mOverMisspelling = false;
    this.mMisspelling = "";
    this.mMenu = null;
    this.mSpellSuggestions = [];
    this.mSuggestionItems = [];
    this.mDictionaryMenu = null;
    this.mDictionaryNames = [];
    this.mDictionaryItems = [];
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
    
    
    return (this.mInlineSpellChecker != null);
  },

  
  get enabled()
  {
    return (this.mInlineSpellChecker &&
            this.mInlineSpellChecker.enableRealTimeSpell);
  },
  set enabled(isEnabled)
  {
    if (this.mInlineSpellChecker)
      this.mEditor.setSpellcheckUserOverride(isEnabled);
  },

  
  get overMisspelling()
  {
    return this.mOverMisspelling;
  },

  
  
  addSuggestionsToMenu: function(menu, insertBefore, maxNumber)
  {
    if (! this.mInlineSpellChecker || ! this.mOverMisspelling)
      return 0; 

    var spellchecker = this.mInlineSpellChecker.spellChecker;
    if (! spellchecker.CheckCurrentWord(this.mMisspelling))
      return 0;  

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

  
  
  addDictionaryListToMenu: function(menu, insertBefore)
  {
    this.mDictionaryMenu = menu;
    this.mDictionaryNames = [];
    this.mDictionaryItems = [];

    if (! gLanguageBundle) {
      
      var bundleService = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                    .getService(Components.interfaces.nsIStringBundleService);
      gLanguageBundle = bundleService.createBundle(
          "chrome://global/locale/languageNames.properties");
      gRegionBundle = bundleService.createBundle(
          "chrome://global/locale/regionNames.properties");
    }

    if (! this.mInlineSpellChecker || ! this.enabled)
      return 0;
    var spellchecker = this.mInlineSpellChecker.spellChecker;
    var o1 = {}, o2 = {};
    spellchecker.GetDictionaryList(o1, o2);
    var list = o1.value;
    var listcount = o2.value;
    var curlang = spellchecker.GetCurrentDictionary();
    var isoStrArray;

    for (var i = 0; i < list.length; i ++) {
      
      isoStrArray = list[i].split("-");
      var displayName = "";
      if (gLanguageBundle && isoStrArray[0]) {
        try {
          displayName = gLanguageBundle.GetStringFromName(isoStrArray[0].toLowerCase());
        } catch(e) {} 
        if (gRegionBundle && isoStrArray[1]) {
          try {
            displayName += " / " + gRegionBundle.GetStringFromName(isoStrArray[1].toLowerCase());
          } catch(e) {} 
          if (isoStrArray[2])
            displayName += " (" + isoStrArray[2] + ")";
        }
      }

      
      if (displayName.length == 0)
        displayName = list[i];

      this.mDictionaryNames.push(list[i]);
      var item = menu.ownerDocument.createElement("menuitem");
      item.setAttribute("label", displayName);
      item.setAttribute("type", "checkbox");
      this.mDictionaryItems.push(item);
      if (curlang == list[i]) {
        item.setAttribute("checked", "true");
      } else {
        var callback = function(me, val) { return function(evt) { me.selectDictionary(val); } };
        item.addEventListener("command", callback(this, i), true);
      }
      if (insertBefore)
        menu.insertBefore(item, insertBefore);
      else
        menu.appendChild(item);
    }
    return list.length;
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
    if (! this.mInlineSpellChecker || index < 0 || index >= this.mDictionaryNames.length)
      return;
    var spellchecker = this.mInlineSpellChecker.spellChecker;
    spellchecker.SetCurrentDictionary(this.mDictionaryNames[index]);
    spellchecker.saveDefaultDictionary();
    this.mInlineSpellChecker.spellCheckRange(null); 
  },

  
  replaceMisspelling: function(index)
  {
    if (! this.mInlineSpellChecker || ! this.mOverMisspelling)
      return;
    if (index < 0 || index >= this.mSpellSuggestions.length)
      return;
    this.mInlineSpellChecker.replaceWord(this.mWordNode, this.mWordOffset,
                                         this.mSpellSuggestions[index]);
  },

  
  toggleEnabled: function()
  {
    this.mEditor.setSpellcheckUserOverride(!this.mInlineSpellChecker.enableRealTimeSpell);
  },

  
  addToDictionary: function()
  {
    this.mInlineSpellChecker.addWordToDictionary(this.mMisspelling);
  },
  ignoreWord: function()
  {
    this.mInlineSpellChecker.ignoreWord(this.mMisspelling);
  }
};
