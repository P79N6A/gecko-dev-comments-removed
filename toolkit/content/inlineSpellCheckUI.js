# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Inline spellcheck code
#
# The Initial Developer of the Original Code is
# Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Brett Wilson <brettw@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

var InlineSpellCheckerUI = {
  mOverMisspelling: false,
  mMisspelling: "",
  mMenu: null,
  mSpellSuggestions: [], 
  mSuggestionItems: [],  
  mDictionaryMenu: null,
  mDictionaryNames: [], 
  mDictionaryItems: [],
  mLanguageBundle: null, 
  mRegionBundle: null, 

  
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

    if (! this.mInlineSpellChecker)
      return;

    var selcon = this.mEditor.selectionController;
    var spellsel = selcon.getSelection(selcon.SELECTION_SPELLCHECK);
    if (spellsel.rangeCount == 0)
      return; 

    var range = this.mInlineSpellChecker.getMispelledWord(rangeParent,
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

      var item = document.createElement("menuitem");
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

    if (! this.mLanguageBundle) {
      
      var bundleService = Components.classes["@mozilla.org/intl/stringbundle;1"]
                                    .getService(Components.interfaces.nsIStringBundleService);
      this.mLanguageBundle = bundleService.createBundle(
          "chrome://global/locale/languageNames.properties");
      this.mRegionBundle = bundleService.createBundle(
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
      if (this.mLanguageBundle && isoStrArray[0]) {
        try {
          displayName = this.mLanguageBundle.GetStringFromName(isoStrArray[0].toLowerCase());
        } catch(e) {} 
        if (this.mRegionBundle && isoStrArray[1]) {
          try {
            displayName += " / " + this.mRegionBundle.GetStringFromName(isoStrArray[1].toLowerCase());
          } catch(e) {} 
          if (isoStrArray[2])
            displayName += " (" + isoStrArray[2] + ")";
        }
      }

      
      if (displayName.length == 0)
        displayName = list[i];

      this.mDictionaryNames.push(list[i]);
      var item = document.createElement("menuitem");
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
