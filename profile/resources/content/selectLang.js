










































function SelectListItem(listRef, itemValue)
{

  try {

    var selectedItem;

    if (itemValue) {
      var elements = listRef.getElementsByAttribute("value", itemValue);
      selectedItem = elements.item(0);
    }

    if (selectedItem)
    {
      listRef.selectedItem = selectedItem;
      return true;
    }
    else
      return false;
  } 

  catch(e)
  {
    return false;
  }

}


function Startup()
{
  var defaultLanguage;
  var languageList = document.getElementById("langList");
  var selectedLanguage = window.arguments.length ? window.arguments[0] : null;

  
  try
  {
    const nsIPrefLocalizedString = Components.interfaces.nsIPrefLocalizedString;
    var prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                               .getService(Components.interfaces.nsIPrefService)
                               .getBranch("general.useragent.");
    defaultLanguage = prefBranch.getComplexValue("locale", nsIPrefLocalizedString).data;
  }

  catch(e) 
  {}

  
  if (!SelectListItem(languageList, selectedLanguage))
    if (!SelectListItem(languageList, defaultLanguage))
      languageList.selectedIndex = 0;

}


function onAccept()
{
  
  var languageList = document.getElementById("langList");
  var selectedItem = languageList.selectedItems.length ? languageList.selectedItems[0] : null;

  if (selectedItem) {
    var langName = selectedItem.getAttribute("value");
    var langStore = opener.document.getElementById("profileLanguage");

    if (langStore)
      langStore.setAttribute("data", langName);
  }

  return true;
}
