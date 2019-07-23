










































const C = Components.classes;
const I = Components.interfaces;

const ToolkitProfileService = "@mozilla.org/toolkit/profile-service;1";
const PromptService = "@mozilla.org/embedcomp/prompt-service;1";

var gDialogParams;
var gProfileManagerBundle;
var gBrandBundle;
var gProfileService;
var gPromptService;

function startup()
{
  try {
    gDialogParams = window.arguments[0].
      QueryInterface(I.nsIDialogParamBlock);

    gProfileService = C[ToolkitProfileService].getService(I.nsIToolkitProfileService);

    gProfileManagerBundle = document.getElementById("bundle_profileManager");
    gBrandBundle = document.getElementById("bundle_brand");

    gPromptService = C[PromptService].getService(I.nsIPromptService);

    document.documentElement.centerWindowOnScreen();

    var profilesElement = document.getElementById("profiles");

    var profileList = gProfileService.profiles;
    while (profileList.hasMoreElements()) {
      var profile = profileList.getNext().QueryInterface(I.nsIToolkitProfile);

      var listitem = profilesElement.appendItem(profile.name, "");

      var tooltiptext =
        gProfileManagerBundle.getFormattedString("profileTooltip", [profile.name, profile.rootDir.path]);
      listitem.setAttribute("tooltiptext", tooltiptext);
      listitem.setAttribute("class", "listitem-iconic");
      listitem.profile = profile;
      try {
        if (profile === gProfileService.selectedProfile) {
          setTimeout(function(a) {
            profilesElement.ensureElementIsVisible(a);
            profilesElement.selectItem(a);
          }, 0, listitem);
        }
      }
      catch(e) { }
    }

    var autoSelectLastProfile = document.getElementById("autoSelectLastProfile");
    autoSelectLastProfile.checked = gProfileService.startWithLastProfile;
    profilesElement.focus();
  }
  catch(e) {
    window.close();
    throw (e);
  }
}

function acceptDialog()
{
  var appName = gBrandBundle.getString("brandShortName");

  var profilesElement = document.getElementById("profiles");
  var selectedProfile = profilesElement.selectedItem;
  if (!selectedProfile) {
    var pleaseSelectTitle = gProfileManagerBundle.getString("pleaseSelectTitle");
    var pleaseSelect =
      gProfileManagerBundle.getFormattedString("pleaseSelect", [appName]);
    gPromptService.alert(window, pleaseSelectTitle, pleaseSelect);

    return false;
  }

  var profileLock;

  try {
    profileLock = selectedProfile.profile.lock({ value: null });
  }
  catch (e) {
    var lockedTitle = gProfileManagerBundle.getString("profileLockedTitle");
    var locked =
      gProfileManagerBundle.getFormattedString("profileLocked2", [appName, selectedProfile.profile.name, appName]);
    gPromptService.alert(window, lockedTitle, locked);

    return false;
  }
  gDialogParams.objects.insertElementAt(profileLock.nsIProfileLock, 0, false);

  var autoSelectLastProfile = document.getElementById("autoSelectLastProfile");
  gProfileService.startWithLastProfile = autoSelectLastProfile.checked;
  gProfileService.selectedProfile = selectedProfile.profile;

  
  gProfileService.startOffline = document.getElementById("offlineState").checked;

  gDialogParams.SetInt(0, 1);

  return true;
}


function onProfilesKey(aEvent)
{
  switch( aEvent.keyCode ) 
  {
  case KeyEvent.DOM_VK_DELETE:
    ConfirmDelete();
    break;
  case KeyEvent.DOM_VK_F2:
    RenameProfile();
    break;
  }
}

function onProfilesDblClick(aEvent)
{
  if(aEvent.target.localName == "listitem")
    document.documentElement.acceptDialog();
}


function CreateProfileWizard()
{
  window.openDialog('chrome://mozapps/content/profile/createProfileWizard.xul',
                    '', 'centerscreen,chrome,modal,titlebar', gProfileService);
}




function CreateProfile(aProfile)
{
  var profilesElement = document.getElementById("profiles");

  var listitem = profilesElement.appendItem(aProfile.name, "");

  var tooltiptext =
    gProfileManagerBundle.getFormattedString("profileTooltip", [aProfile.name, aProfile.rootDir.path]);
  listitem.setAttribute("tooltiptext", tooltiptext);
  listitem.setAttribute("class", "listitem-iconic");
  listitem.profile = aProfile;

  profilesElement.ensureElementIsVisible(listitem);
  profilesElement.selectItem(listitem);
}


function RenameProfile()
{
  var profilesElement = document.getElementById("profiles");
  var selectedItem = profilesElement.selectedItem;
  if (!selectedItem) {
    return false;
  }

  var selectedProfile = selectedItem.profile;

  var oldName = selectedProfile.name;
  var newName = {value: oldName};

  var dialogTitle = gProfileManagerBundle.getString("renameProfileTitle");
  var msg =
    gProfileManagerBundle.getFormattedString("renameProfilePrompt", [oldName]);

  if (gPromptService.prompt(window, dialogTitle, msg, newName, null, {value:0})) {
    newName = newName.value;

    
    if (newName == oldName)
      return false;

    try {
      selectedProfile.name = newName;
    }
    catch (e) {
      var alTitle = gProfileManagerBundle.getString("profileNameInvalidTitle");
      var alMsg = gProfileManagerBundle.getFormattedString("profileNameInvalid", [newName]);
      gPromptService.alert(window, alTitle, alMsg);
      return false;
    }

    selectedItem.label = newName;
    var tiptext = gProfileManagerBundle.
                  getFormattedString("profileTooltip",
                                     [newName, selectedProfile.rootDir.path]);
    selectedItem.setAttribute("tooltiptext", tiptext);

    return true;
  }

  return false;
}

function ConfirmDelete()
{
  var deleteButton = document.getElementById("delbutton");
  var profileList = document.getElementById( "profiles" );

  var selectedItem = profileList.selectedItem;
  if (!selectedItem) {
    return false;
  }

  var selectedProfile = selectedItem.profile;
  var deleteFiles = false;

  if (selectedProfile.rootDir.exists()) {
    var dialogTitle = gProfileManagerBundle.getString("deleteTitle");
    var dialogText =
      gProfileManagerBundle.getFormattedString("deleteProfileConfirm",
                                               [selectedProfile.rootDir.path]);

    var buttonPressed = gPromptService.confirmEx(window, dialogTitle, dialogText,
                          (gPromptService.BUTTON_TITLE_IS_STRING * gPromptService.BUTTON_POS_0) +
                          (gPromptService.BUTTON_TITLE_CANCEL * gPromptService.BUTTON_POS_1) +
                          (gPromptService.BUTTON_TITLE_IS_STRING * gPromptService.BUTTON_POS_2),
                          gProfileManagerBundle.getString("dontDeleteFiles"),
                          null,
                          gProfileManagerBundle.getString("deleteFiles"),
                          null, {value:0});
    if (buttonPressed == 1)
      return false;

    if (buttonPressed == 2)
      deleteFiles = true;
  }
  
  selectedProfile.remove(deleteFiles);
  selectedItem.parentNode.removeChild(selectedItem);

  return true;
}
