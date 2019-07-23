







































var gMisspelledWord;
var gSpellChecker = null;
var gAllowSelectWord = true;
var gPreviousReplaceWord = "";
var gFirstTime = true;
var gLastSelectedLang = null;
var gDictCount = 0;

function Startup()
{
  var sendMailMessageMode = false;

  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  
  gSpellChecker = Components.classes['@mozilla.org/editor/editorspellchecker;1'].createInstance(Components.interfaces.nsIEditorSpellCheck);
  if (!gSpellChecker)
  {
    dump("SpellChecker not found!!!\n");
    window.close();
    return;
  }

  
  try {
    
    var filterContractId;
    sendMailMessageMode = window.arguments[0];
    var skipBlockQuotes = window.arguments[1];
    var enableSelectionChecking = window.arguments[2];

    if (skipBlockQuotes)
      filterContractId = "@mozilla.org/editor/txtsrvfiltermail;1";
    else
      filterContractId = "@mozilla.org/editor/txtsrvfilter;1";

    gSpellChecker.setFilter(Components.classes[filterContractId].createInstance(Components.interfaces.nsITextServicesFilter));
    gSpellChecker.InitSpellChecker(editor, enableSelectionChecking);

  }
  catch(ex) {
   dump("*** Exception error: InitSpellChecker\n");
    window.close();
    return;
  }

  gDialog.MisspelledWordLabel = document.getElementById("MisspelledWordLabel");
  gDialog.MisspelledWord      = document.getElementById("MisspelledWord");
  gDialog.ReplaceButton       = document.getElementById("Replace");
  gDialog.IgnoreButton        = document.getElementById("Ignore");
  gDialog.StopButton          = document.getElementById("Stop");
  gDialog.CloseButton         = document.getElementById("Close");
  gDialog.ReplaceWordInput    = document.getElementById("ReplaceWordInput");
  gDialog.SuggestedList       = document.getElementById("SuggestedList");
  gDialog.LanguageMenulist    = document.getElementById("LanguageMenulist");

  
  

  var curLang;

  try {
    curLang = gSpellChecker.GetCurrentDictionary();
  } catch(ex) {
    curLang = "";
  }

  InitLanguageMenu(curLang);
  
  
  NextWord();

  
  
  if (sendMailMessageMode)
  {
    
    if (!gMisspelledWord)
    {
      onClose();
      return;
    }

    
    gDialog.CloseButton.hidden = true;
    gDialog.CloseButton = document.getElementById("Send");
    gDialog.CloseButton.hidden = false;
  }
  else
  {
    
    
    
    gDialog.StopButton.hidden = true;
  }

  
  
  
  gFirstTime = false;
}

function InitLanguageMenu(aCurLang)
{

  var o1 = {};
  var o2 = {};

  
  

  try
  {
    gSpellChecker.GetDictionaryList(o1, o2);
  }
  catch(ex)
  {
    dump("Failed to get DictionaryList!\n");
    return;
  }

  var dictList = o1.value;
  var count    = o2.value;

  
  
  if (gDictCount == count)
    return;

  
  gDictCount = count;

  
  

  
  var languageBundle = document.getElementById("languageBundle");
  var regionBundle = null;
  
  if (languageBundle)
    regionBundle = document.getElementById("regionBundle");
  
  var menuStr2;
  var isoStrArray;
  var langId;
  var langLabel;
  var i;

  for (i = 0; i < count; i++)
  {
    try
    {
      langId = dictList[i];
      isoStrArray = dictList[i].split("-");

      if (languageBundle && isoStrArray[0])
        langLabel = languageBundle.getString(isoStrArray[0].toLowerCase());

      if (regionBundle && langLabel && isoStrArray.length > 1 && isoStrArray[1])
      {
        menuStr2 = regionBundle.getString(isoStrArray[1].toLowerCase());
        if (menuStr2)
          langLabel += "/" + menuStr2;
      }

      if (langLabel && isoStrArray.length > 2 && isoStrArray[2])
        langLabel += " (" + isoStrArray[2] + ")";

      if (!langLabel)
        langLabel = langId;
    }
    catch (ex)
    {
      
      
      langLabel = langId;
    }
    dictList[i] = [langLabel, langId];
  }
  
  
  dictList.sort(
    function compareFn(a, b)
    {
      return a[0].localeCompare(b[0]);
    }
  );

  
  var languageMenuPopup = gDialog.LanguageMenulist.firstChild;
  while (languageMenuPopup.firstChild.localName != "menuseparator")
    languageMenuPopup.removeChild(languageMenuPopup.firstChild);

  var defaultItem = null;

  for (i = 0; i < count; i++)
  {
    var item = gDialog.LanguageMenulist.insertItemAt(i, dictList[i][0], dictList[i][1]);
    if (aCurLang && dictList[i][1] == aCurLang)
      defaultItem = item;
  }

  
  if (defaultItem)
  {
    gDialog.LanguageMenulist.selectedItem = defaultItem;
    gLastSelectedLang = defaultItem;
  }
}

function DoEnabling()
{
  if (!gMisspelledWord)
  {
    
    gDialog.MisspelledWord.setAttribute("value",GetString( gFirstTime ? "NoMisspelledWord" : "CheckSpellingDone"));

    gDialog.ReplaceButton.removeAttribute("default");
    gDialog.IgnoreButton.removeAttribute("default");

    gDialog.CloseButton.setAttribute("default","true");
    
    gDialog.CloseButton.focus();

    SetElementEnabledById("MisspelledWordLabel", false);
    SetElementEnabledById("ReplaceWordLabel", false);
    SetElementEnabledById("ReplaceWordInput", false);
    SetElementEnabledById("CheckWord", false);
    SetElementEnabledById("SuggestedListLabel", false);
    SetElementEnabledById("SuggestedList", false);
    SetElementEnabledById("Ignore", false);
    SetElementEnabledById("IgnoreAll", false);
    SetElementEnabledById("Replace", false);
    SetElementEnabledById("ReplaceAll", false);
    SetElementEnabledById("AddToDictionary", false);
  } else {
    SetElementEnabledById("MisspelledWordLabel", true);
    SetElementEnabledById("ReplaceWordLabel", true);
    SetElementEnabledById("ReplaceWordInput", true);
    SetElementEnabledById("CheckWord", true);
    SetElementEnabledById("SuggestedListLabel", true);
    SetElementEnabledById("SuggestedList", true);
    SetElementEnabledById("Ignore", true);
    SetElementEnabledById("IgnoreAll", true);
    SetElementEnabledById("AddToDictionary", true);

    gDialog.CloseButton.removeAttribute("default");
    SetReplaceEnable();
  }
}

function NextWord()
{
  gMisspelledWord = gSpellChecker.GetNextMisspelledWord();
  SetWidgetsForMisspelledWord();
}

function SetWidgetsForMisspelledWord()
{
  gDialog.MisspelledWord.setAttribute("value", TruncateStringAtWordEnd(gMisspelledWord, 30, true));


  
  gDialog.ReplaceWordInput.value = gMisspelledWord;
  gPreviousReplaceWord = gMisspelledWord;

  
  FillSuggestedList(gMisspelledWord);

  DoEnabling();

  if (gMisspelledWord)
    SetTextboxFocus(gDialog.ReplaceWordInput);
}

function CheckWord()
{
  var word = gDialog.ReplaceWordInput.value;
  if (word) 
  {
    if (gSpellChecker.CheckCurrentWord(word))
    {
      FillSuggestedList(word);
      SetReplaceEnable();
    } 
    else 
    {
      ClearListbox(gDialog.SuggestedList);
      var item = gDialog.SuggestedList.appendItem(GetString("CorrectSpelling"), "");
      if (item) item.setAttribute("disabled", "true");
      
      gAllowSelectWord = false;
    }
  }
}

function SelectSuggestedWord()
{
  if (gAllowSelectWord)
  {
    var selectedItem
    if (gDialog.SuggestedList.selectedItem)
    {
      var selValue = gDialog.SuggestedList.selectedItem.getAttribute("label");
      gDialog.ReplaceWordInput.value = selValue;
      gPreviousReplaceWord = selValue;
    }
    else
    {
      gDialog.ReplaceWordInput.value = gPreviousReplaceWord;
    }
    SetReplaceEnable();
  }
}

function ChangeReplaceWord()
{
  
  
  var saveAllow = gAllowSelectWord;
  gAllowSelectWord = false;

  
  var newIndex = -1;
  var newSelectedItem;
  var replaceWord = TrimString(gDialog.ReplaceWordInput.value);
  if (replaceWord)
  {
    for (var i = 0; i < gDialog.SuggestedList.getRowCount(); i++)
    {
      var item = gDialog.SuggestedList.getItemAtIndex(i);
      if (item.getAttribute("label") == replaceWord)
      {
        newSelectedItem = item;
        break;
      }
    }
  }
  gDialog.SuggestedList.selectedItem = newSelectedItem;

  gAllowSelectWord = saveAllow;

  
  gPreviousReplaceWord = gDialog.ReplaceWordInput.value;

  SetReplaceEnable();
}

function Ignore()
{
  NextWord();
}

function IgnoreAll()
{
  if (gMisspelledWord) {
    gSpellChecker.IgnoreWordAllOccurrences(gMisspelledWord);
  }
  NextWord();
}

function Replace(newWord)
{
  if (!newWord)
    return;

  if (gMisspelledWord && gMisspelledWord != newWord)
  {
    var editor = GetCurrentEditor();
    editor.beginTransaction();
    try {
      gSpellChecker.ReplaceWord(gMisspelledWord, newWord, false);
    } catch (e) {}
    editor.endTransaction();
  }
  NextWord();
}

function ReplaceAll()
{
  var newWord = gDialog.ReplaceWordInput.value;
  if (gMisspelledWord && gMisspelledWord != newWord)
  {
    var editor = GetCurrentEditor();
    editor.beginTransaction();
    try {
      gSpellChecker.ReplaceWord(gMisspelledWord, newWord, true);
    } catch (e) {}
    editor.endTransaction();
  }
  NextWord();
}

function AddToDictionary()
{
  if (gMisspelledWord) {
    gSpellChecker.AddWordToDictionary(gMisspelledWord);
  }
  NextWord();
}

function EditDictionary()
{
  window.openDialog("chrome://editor/content/EdDictionary.xul", "_blank", "chrome,close,titlebar,modal", "", gMisspelledWord);
}

function SelectLanguage()
{
  try {
    var item = gDialog.LanguageMenulist.selectedItem;
    if (item.value != "more-cmd") {
      gSpellChecker.SetCurrentDictionary(item.value);
      gLastSelectedLang = item;
    }
    else {
      var dictionaryUrl = getDictionaryURL();
                      
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                                .getService(Components.interfaces.nsIIOService);
      uri = ioService.newURI(dictionaryUrl, null, null);
      var protocolSvc = Components.classes["@mozilla.org/uriloader/external-protocol-service;1"]
                                  .getService(Components.interfaces.nsIExternalProtocolService);
      if (protocolSvc.isExposedProtocol(uri.scheme))
        opener.openDialog(getBrowserURL(), "_blank", "all,dialog=no", dictionaryUrl);
      else
        protocolSvc.loadUrl(uri);

      if (gLastSelectedLang)
        gDialog.LanguageMenulist.selectedItem = gLastSelectedLang;
    }
  } catch (ex) {
    dump(ex);
  }
}

function getDictionaryURL()
{
  var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                  .getService(Components.interfaces.nsIURLFormatter);
                  
  return formatter.formatURLPref("spellchecker.dictionaries.download.url");
}

function Recheck()
{
  
  try {
    var curLang = gSpellChecker.GetCurrentDictionary();
    gSpellChecker.UninitSpellChecker();
    gSpellChecker.InitSpellChecker(GetCurrentEditor(), false);
    gSpellChecker.SetCurrentDictionary(curLang);
    gMisspelledWord = gSpellChecker.GetNextMisspelledWord();
    SetWidgetsForMisspelledWord();
  } catch(ex) {
    dump(ex);
  }
}

function FillSuggestedList(misspelledWord)
{
  var list = gDialog.SuggestedList;

  
  gAllowSelectWord = false;
  ClearListbox(list);
  var item;

  if (misspelledWord.length > 0)
  {
    
    var count = 0;
    var firstWord = 0;
    do {
      var word = gSpellChecker.GetSuggestedWord();
      if (count==0)
        firstWord = word;
      if (word.length > 0)
      {
        list.appendItem(word, "");
        count++;
      }
    } while (word.length > 0);

    if (count == 0)
    {
      
      item = list.appendItem(GetString("NoSuggestedWords"));
      if (item) item.setAttribute("disabled", "true");
      gAllowSelectWord = false;
    } else {
      gAllowSelectWord = true;
      
      gDialog.SuggestedList.selectedIndex = 0;
    }
  } 
  else
  {
    item = list.appendItem("", "");
    if (item)
      item.setAttribute("disabled", "true");
  }
}

function SetReplaceEnable()
{
  
  var newWord = gDialog.ReplaceWordInput.value;
  var enable = newWord.length > 0 && newWord != gMisspelledWord;
  SetElementEnabledById("Replace", enable);
  SetElementEnabledById("ReplaceAll", enable);
  if (enable)
  {
    gDialog.ReplaceButton.setAttribute("default","true");
    gDialog.IgnoreButton.removeAttribute("default");
  }
  else
  {
    gDialog.IgnoreButton.setAttribute("default","true");
    gDialog.ReplaceButton.removeAttribute("default");
  }
}

function doDefault()
{
  if (gDialog.ReplaceButton.getAttribute("default") == "true")
    Replace();
  else if (gDialog.IgnoreButton.getAttribute("default") == "true")
    Ignore();
  else if (gDialog.CloseButton.getAttribute("default") == "true")
    onClose();

  return false;
}

function ExitSpellChecker()
{
  if (gSpellChecker)
  {
    try
    {
      var curLang = gSpellChecker.GetCurrentDictionary();
      gSpellChecker.UninitSpellChecker();
      if ("@mozilla.org/spellchecker;1" in Components.classes) {
        var spellChecker = Components.classes["@mozilla.org/spellchecker/hunspell;1"]
                                     .getService(Components.interfaces.mozISpellCheckingEngine);
        spellChecker.dictionary = curLang;
      }
      
      
      if (("InlineSpellCheckerUI" in window.opener) &&
          window.opener.InlineSpellCheckerUI.enabled)
        window.opener.InlineSpellCheckerUI.mInlineSpellChecker.spellCheckRange(null);
    }
    finally
    {
      gSpellChecker = null;
    }
  }
}

function CancelSpellCheck()
{
  ExitSpellChecker();

  
  window.opener.cancelSendMessage = true;
  return true;
}

function onClose()
{
  ExitSpellChecker();

  window.opener.cancelSendMessage = false;
  window.close();
}
