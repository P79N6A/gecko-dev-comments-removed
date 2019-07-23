





































var gSpellChecker;
var gWordToAdd;

function Startup()
{
  if (!GetCurrentEditor())
  {
    window.close();
    return;
  }
  
  if ("gSpellChecker" in window.opener && window.opener.gSpellChecker)
    gSpellChecker = window.opener.gSpellChecker;

  if (!gSpellChecker)
  {
    dump("SpellChecker not found!!!\n");
    window.close();
    return;
  }
  
  gWordToAdd = window.arguments[1];
  
  gDialog.WordInput = document.getElementById("WordInput");
  gDialog.DictionaryList = document.getElementById("DictionaryList");
  
  gDialog.WordInput.value = gWordToAdd;
  FillDictionaryList();

  
  SelectWordToAddInList();
  SetTextboxFocus(gDialog.WordInput);
}

function ValidateWordToAdd()
{
  gWordToAdd = TrimString(gDialog.WordInput.value);
  if (gWordToAdd.length > 0)
  {
    return true;
  } else {
    return false;
  }
}    

function SelectWordToAddInList()
{
  for (var i = 0; i < gDialog.DictionaryList.getRowCount(); i++)
  {

    var wordInList = gDialog.DictionaryList.getItemAtIndex(i);
    if (wordInList && gWordToAdd == wordInList.label)
    {
      gDialog.DictionaryList.selectedIndex = i;
      break;
    }
  }
}

function AddWord()
{
  if (ValidateWordToAdd())
  {
    try {
      gSpellChecker.AddWordToDictionary(gWordToAdd);
    }
    catch (e) {
      dump("Exception occured in gSpellChecker.AddWordToDictionary\nWord to add probably already existed\n");
    }

    
    FillDictionaryList();

    SelectWordToAddInList();
    gDialog.WordInput.value = "";
  }
}

function ReplaceWord()
{
  if (ValidateWordToAdd())
  {
    var selItem = gDialog.DictionaryList.selectedItem;
    if (selItem)
    {
      try {
        gSpellChecker.RemoveWordFromDictionary(selItem.label);
      } catch (e) {}

      try {
        
        gSpellChecker.AddWordToDictionary(gWordToAdd);

        
        
        selItem.label = gWordToAdd; 
      } catch (e) {
        
        dump("Exception occured adding word in ReplaceWord\n");
        FillDictionaryList();
        SelectWordToAddInList();
      }
    }
  }
}

function RemoveWord()
{
  var selIndex = gDialog.DictionaryList.selectedIndex;
  if (selIndex >= 0)
  {
    var word = gDialog.DictionaryList.selectedItem.label;

    
    gDialog.DictionaryList.removeItemAt(selIndex);

    
    try {
      
      gSpellChecker.RemoveWordFromDictionary(word);
    }
    catch (e)
    {
      dump("Failed to remove word from dictionary\n");
    }

    ResetSelectedItem(selIndex);
  }
}

function FillDictionaryList()
{
  var selIndex = gDialog.DictionaryList.selectedIndex;

  
  ClearListbox(gDialog.DictionaryList);

  
  gSpellChecker.GetPersonalDictionary()

  var haveList = false;

  
  do {
    var word = gSpellChecker.GetPersonalDictionaryWord();
    if (word != "")
    {
      gDialog.DictionaryList.appendItem(word, "");
      haveList = true;
    }
  } while (word != "");
  
  
  
  if (!haveList)
      gDialog.DictionaryList.appendItem("", "");

  ResetSelectedItem(selIndex);
}

function ResetSelectedItem(index)
{
  var lastIndex = gDialog.DictionaryList.getRowCount() - 1;
  if (index > lastIndex)
    index = lastIndex;

  
  
  if (index == -1 && lastIndex >= 0)
    index = 0;

  gDialog.DictionaryList.selectedIndex = index;
}

function onClose()
{
  return true;
}
