




































var gPublishSiteData;
var gPublishDataChanged = false;
var gDefaultSiteIndex = -1;
var gDefaultSiteName;
var gPreviousDefaultSite;
var gPreviousTitle;
var gSettingsChanged = false;
var gSiteDataChanged = false;
var gAddNewSite = false;
var gCurrentSiteIndex = -1;
var gPasswordManagerOn = true;


function Startup()
{
  if (!GetCurrentEditor())
  {
    window.close();
    return;
  }

  gDialog.SiteList            = document.getElementById("SiteList");
  gDialog.SiteNameInput       = document.getElementById("SiteNameInput");
  gDialog.PublishUrlInput     = document.getElementById("PublishUrlInput");
  gDialog.BrowseUrlInput      = document.getElementById("BrowseUrlInput");
  gDialog.UsernameInput       = document.getElementById("UsernameInput");
  gDialog.PasswordInput       = document.getElementById("PasswordInput");
  gDialog.SavePassword        = document.getElementById("SavePassword");
  gDialog.SetDefaultButton    = document.getElementById("SetDefaultButton");
  gDialog.RemoveSiteButton    = document.getElementById("RemoveSiteButton");
  gDialog.OkButton            = document.documentElement.getButton("accept");

  gPublishSiteData = GetPublishSiteData();
  gDefaultSiteName = GetDefaultPublishSiteName();
  gPreviousDefaultSite = gDefaultSiteName;

  gPasswordManagerOn = GetBoolPref("signon.rememberSignons");
  gDialog.SavePassword.disabled = !gPasswordManagerOn;

  InitDialog();

  SetWindowLocation();
}

function InitDialog()
{
  
  if (!gPublishSiteData)
  {
    AddNewSite();
  }
  else
  {
    FillSiteList();

    
    
    

    SetTextboxFocus(gDialog.SiteNameInput);
  }
}

function FillSiteList()
{
  
  gIsSelecting = true;
  ClearListbox(gDialog.SiteList);
  gIsSelecting = false;
  gDefaultSiteIndex = -1;

  
  var count = gPublishSiteData.length;
  for (var i = 0; i < count; i++)
  {
    var name = gPublishSiteData[i].siteName;
    var item = gDialog.SiteList.appendItem(name);
    SetPublishItemStyle(item);
    if (name == gDefaultSiteName)
      gDefaultSiteIndex = i;
  }
}

function SetPublishItemStyle(item)
{
  
  if (item)
  {
    if (item.getAttribute("label") == gDefaultSiteName)
      item.setAttribute("class", "bold");
    else
      item.removeAttribute("class");
  }
}

function AddNewSite()
{
  
  if (!ApplyChanges())
    return;

  
  InitSiteSettings(-1);
  gAddNewSite = true;

  SetTextboxFocus(gDialog.SiteNameInput);
}

function RemoveSite()
{
  if (!gPublishSiteData)
    return;

  var index = gDialog.SiteList.selectedIndex;
  var item;
  if (index != -1)
  {
    item = gDialog.SiteList.selectedItems[0];
    var nameToRemove = item.getAttribute("label");

    
    gPublishSiteData.splice(index, 1);
    
    gDialog.SiteList.clearSelection();
    gDialog.SiteList.removeItemAt(index);

    
    if (index >= gPublishSiteData.length)
      index--;
    InitSiteSettings(index);

    if (nameToRemove == gDefaultSiteName)
    {
      
      
      SetDefault();
    }
    gSiteDataChanged = true;
  }
}

function SetDefault()
{
  if (!gPublishSiteData)
    return;

  var index = gDialog.SiteList.selectedIndex;
  if (index != -1)
  {
    gDefaultSiteIndex = index;
    gDefaultSiteName = gPublishSiteData[index].siteName;
    
    
    var item = gDialog.SiteList.firstChild;
    while (item)
    {
      SetPublishItemStyle(item);
      item = item.nextSibling;
    }
  }
}



var gIsSelecting = false;

function SelectSiteList()
{
  if (gIsSelecting)
    return;

  gIsSelecting = true;
  var newIndex = gDialog.SiteList.selectedIndex;

  
  if (!ApplyChanges())
    return;

  InitSiteSettings(newIndex);

  gIsSelecting = false;
}


function SetSelectedSiteIndex(index)
{
  gIsSelecting = true;
  gDialog.SiteList.selectedIndex = index;
  gIsSelecting = false;
}

function InitSiteSettings(selectedSiteIndex)
{  
  
  gCurrentSiteIndex = selectedSiteIndex;
  
  SetSelectedSiteIndex(selectedSiteIndex);
  var haveData = (gPublishSiteData && selectedSiteIndex != -1);

  gDialog.SiteNameInput.value = haveData ? gPublishSiteData[selectedSiteIndex].siteName : "";
  gDialog.PublishUrlInput.value = haveData ? gPublishSiteData[selectedSiteIndex].publishUrl : "";
  gDialog.BrowseUrlInput.value = haveData ? gPublishSiteData[selectedSiteIndex].browseUrl : "";
  gDialog.UsernameInput.value = haveData ? gPublishSiteData[selectedSiteIndex].username : "";

  var savePassord = haveData && gPasswordManagerOn;
  gDialog.PasswordInput.value = savePassord ? gPublishSiteData[selectedSiteIndex].password : "";
  gDialog.SavePassword.checked = savePassord ? gPublishSiteData[selectedSiteIndex].savePassword : false;

  gDialog.SetDefaultButton.disabled = !haveData;
  gDialog.RemoveSiteButton.disabled = !haveData;
  gSettingsChanged = false;
}

function onInputSettings()
{
  
  
  gSettingsChanged = true;
}

function ApplyChanges()
{
  if (gSettingsChanged && !UpdateSettings())
  {
    
    SetSelectedSiteIndex(gCurrentSiteIndex);
    return false;
  }
  return true;
}

function UpdateSettings()
{
  
  var newName = TrimString(gDialog.SiteNameInput.value);
  if (!newName)
  {
    ShowInputErrorMessage(GetString("MissingSiteNameError"), gDialog.SiteNameInput);
    return false;
  }
  if (PublishSiteNameExists(newName, gPublishSiteData, gCurrentSiteIndex))
  {
    ShowInputErrorMessage(GetString("DuplicateSiteNameError").replace(/%name%/, newName));            
    SetTextboxFocus(gDialog.SiteNameInput);
    return false;
  }

  var newUrl = FormatUrlForPublishing(gDialog.PublishUrlInput.value);
  if (!newUrl)
  {
    ShowInputErrorMessage(GetString("MissingPublishUrlError"), gDialog.PublishUrlInput);
    return false;
  }

  
  var newSiteData = false;

  if (!gPublishSiteData)
  {
    
    gPublishSiteData = new Array(1);
    gCurrentSiteIndex = 0;
    newSiteData = true;
  }
  else if (gCurrentSiteIndex == -1)
  {
    
    
    
    gCurrentSiteIndex = gPublishSiteData.length;
    newSiteData = true;
  }
    
  if (newSiteData)
  {
    
    gPublishSiteData[gCurrentSiteIndex] = {};
    gPublishSiteData[gCurrentSiteIndex].docDir = "";
    gPublishSiteData[gCurrentSiteIndex].otherDir = "";
    gPublishSiteData[gCurrentSiteIndex].dirList = [""];
    gPublishSiteData[gCurrentSiteIndex].previousSiteName = newName;
  }

  gPublishSiteData[gCurrentSiteIndex].siteName = newName;
  gPublishSiteData[gCurrentSiteIndex].publishUrl = newUrl;
  gPublishSiteData[gCurrentSiteIndex].browseUrl = FormatUrlForPublishing(gDialog.BrowseUrlInput.value);
  gPublishSiteData[gCurrentSiteIndex].username = TrimString(gDialog.UsernameInput.value);
  gPublishSiteData[gCurrentSiteIndex].password= gDialog.PasswordInput.value;
  gPublishSiteData[gCurrentSiteIndex].savePassword = gDialog.SavePassword.checked;

  if (gCurrentSiteIndex == gDefaultSiteIndex)
    gDefaultSiteName = newName;

  
  if (gPublishSiteData.length == 1 && !gDefaultSiteName)
  {
    gDefaultSiteName = gPublishSiteData[0].siteName;
    gDefaultSiteIndex = 0;
  }

  FillSiteList();

  
  SetSelectedSiteIndex(gCurrentSiteIndex);

  
  gSiteDataChanged = true;
  
  
  gSettingsChanged = false;
  gAddNewSite = false;

  return true;
}

function onAccept()
{
  
  if (!ApplyChanges())
    return false;

  if (gSiteDataChanged)
  {
    
    SavePublishSiteDataToPrefs(gPublishSiteData, gDefaultSiteName);
  }
  else if (gPreviousDefaultSite != gDefaultSiteName)
  {
    
    SetDefaultSiteName(gDefaultSiteName);
  }

  SaveWindowLocation();

  return true;
}
