











































var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
var profileManagerMode = "selection";
var set = null;


function CreateProfileWizard()
{
  window.openDialog('chrome://communicator/content/profile/createProfileWizard.xul',
                    '', 'centerscreen,chrome,modal,titlebar');
}


function CreateProfile( aProfName, aProfDir )
{
  AddItem(aProfName, "yes");
  var profileList = document.getElementById( "profiles" );
  profileList.view.selection.select(profileList.view.rowCount - 1);
  profileList.treeBoxObject.ensureRowIsVisible(profileList.currentIndex);
}


function RenameProfile()
{
  var lString, oldName, newName;
  var renameButton = document.getElementById("renbutton");
  if (renameButton.getAttribute("disabled") == "true" )
    return false;
  var profileList = document.getElementById( "profiles" );
  var selected = profileList.view.getItemAtIndex(profileList.currentIndex);
  var profilename = selected.getAttribute("profile_name");
  var errorMessage = null;
  if( selected.getAttribute("rowMigrate") == "no" ) {
    
    lString = gProfileManagerBundle.getString("migratebeforerename");
    lString = lString.replace(/\s*<html:br\/>/g,"\n");
    lString = lString.replace(/%brandShortName%/, gBrandBundle.getString("brandShortName"));
    var title = gProfileManagerBundle.getString("migratetitle");
    if (promptService.confirm(window, title, lString)) {
      var profileDir = profile.getProfileDir(profilename);
      if (profileDir) {
        profileDir = profileDir.QueryInterface( Components.interfaces.nsIFile );
        if (profileDir) {
          if (!profileDir.exists()) {
            errorMessage = gProfileManagerBundle.getString("sourceProfileDirMissing");
            var profileDirMissingTitle = gProfileManagerBundle.getString("sourceProfileDirMissingTitle");
            promptService.alert(window, profileDirMissingTitle, errorMessage);
            return false;
          }
        }
      }      
      profile.migrateProfile( profilename );
    }
    else
      return false;
  }
  else {
    oldName = selected.getAttribute("profile_name");
    newName = {value:oldName};
    var dialogTitle = gProfileManagerBundle.getString("renameprofiletitle");
    var msg = gProfileManagerBundle.getString("renameProfilePrompt");
    msg = msg.replace(/%oldProfileName%/gi, oldName);
    while (1) {
      var rv = promptService.prompt(window, dialogTitle, msg, newName, null, {value:0});
      if (rv) {
        newName = newName.value;

        
        if (newName == oldName)
          return false;

        errorMessage = checkProfileName(newName);
        if (errorMessage) {
          var profileNameInvalidTitle = gProfileManagerBundle.getString("profileNameInvalidTitle");
          promptService.alert(window, profileNameInvalidTitle, errorMessage);
          return false;
        }
        
        var migrate = selected.getAttribute("rowMigrate");
        try {
          profile.renameProfile(oldName, newName);
          selected.firstChild.firstChild.setAttribute( "label", newName );
          selected.setAttribute( "profile_name", newName );
        }
        catch(e) {
          lString = gProfileManagerBundle.getString("profileExists");
          var profileExistsTitle = gProfileManagerBundle.getString("profileExistsTitle");
          promptService.alert(window, profileExistsTitle, lString);
          continue;
        }
      }
      break;
    }
  }
  
  DoEnabling(); 
  return true; 
}


function ConfirmDelete()
{
  var deleteButton = document.getElementById("delbutton");
  if( deleteButton.getAttribute("disabled") == "true" )
    return;
  var profileList = document.getElementById( "profiles" );

  var selected = profileList.view.getItemAtIndex(profileList.currentIndex);
  var name = selected.getAttribute("profile_name");
  
  var dialogTitle = gProfileManagerBundle.getString("deletetitle");
  var dialogText;
  
  if( selected.getAttribute("rowMigrate") == "no" ) {
    var brandName = gBrandBundle.getString("brandShortName");
    dialogText = gProfileManagerBundle.getFormattedString("delete4xprofile", [brandName]);
    dialogText = dialogText.replace(/\s*<html:br\/>/g,"\n");
    
    if (promptService.confirm(window, dialogTitle, dialogText)) {
      profile.deleteProfile( name, false );
      profileList.removeChild( selected );
    }
    return;
  }
  else {
    var pathExists = true;
    try {
      var path = profile.getProfilePath(name);
    }
    catch (ex) {
      pathExists = false;
    }
    if (pathExists) {
      dialogText = gProfileManagerBundle.getFormattedString("deleteprofile", [path]);
      dialogText = dialogText.replace(/\s*<html:br\/>/g,"\n");
      var buttonPressed = promptService.confirmEx(window, dialogTitle, dialogText,
                              (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0) +
                              (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1) +
                              (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2),
                              gProfileManagerBundle.getString("dontDeleteFiles"),
                              null,
                              gProfileManagerBundle.getString("deleteFiles"),
                              null, {value:0});
      if (buttonPressed != 1)
          DeleteProfile(buttonPressed == 2);
    }
    else
      DeleteProfile(false);
  }
}


function DeleteProfile(deleteFiles)
{
  var profileList = document.getElementById("profiles");
  if (profileList.view.selection.count) {
    var selected = profileList.view.getItemAtIndex(profileList.currentIndex);
    var name = selected.getAttribute("profile_name");
    var previous = profileList.currentIndex - 1;

    try {
      profile.deleteProfile(name, deleteFiles);
      profileList.lastChild.removeChild(selected);

      if (previous) {
        profileList.view.selection.select(previous);
        profileList.treeBoxObject.ensureRowIsVisible(previous);
      }

      
      DoEnabling();
    }
    catch (ex) {
      dump("Exception during profile deletion.\n");
    }
  }
}

function ConfirmMigrateAll()
{
  var string = gProfileManagerBundle.getString("migrateallprofiles");
  var title = gProfileManagerBundle.getString("migrateallprofilestitle");
  if (promptService.confirm(window, title, string))
    return true;
  else 
    return false;
}

function SwitchProfileManagerMode()
{
  var prattleIndex  = null;
  var captionLine   = null;
  var buttonDisplay = null;
  var selItems      = [];
  
  if( profileManagerMode == "selection" )
  {
    prattleIndex = 1;                                       
    
    try {
      captionLine = gProfileManagerBundle.getString("pm_title"); 
    } catch(e) {
      captionLine = "Manage Profiles *";
    }
    
    var profileList = document.getElementById("profiles");
    profileList.focus();

    
    document.documentElement.getButton("extra2").hidden = true;
    profileManagerMode = "manager";                         
  } 
  else {
    prattleIndex = 0;
    try {
      captionLine = gProfileManagerBundle.getString("ps_title");
    } catch(e) {
      captionLine = "Select Profile *";
    }
    profileManagerMode = "selection";
  }

  
  var deck = document.getElementById( "prattle" );
  deck.setAttribute( "selectedIndex", prattleIndex )
    
  
  ChangeCaption( captionLine );
  
  set = !set;
}


function ChangeCaption( aCaption )
{
  var caption = document.getElementById( "header" );
  caption.setAttribute( "description", aCaption );
  document.title = aCaption;
}


function DoEnabling()
{
  var renbutton = document.getElementById( "renbutton" );
  var delbutton = document.getElementById( "delbutton" );
  var start     = document.documentElement.getButton( "accept" );
  
  var profileList = document.getElementById( "profiles" );
  if (profileList.view.selection.count == 0)
  {
    renbutton.setAttribute( "disabled", "true" );
    delbutton.setAttribute( "disabled", "true" );
    start.setAttribute( "disabled", "true" );
  }
  else {
    if( renbutton.getAttribute( "disabled" ) == "true" )
      renbutton.removeAttribute( "disabled", "true" );
    if( start.getAttribute( "disabled" ) == "true" )
      start.removeAttribute( "disabled", "true" );
    
    var canDelete = true;
    if (!gStartupMode) {  
      var selected = profileList.view.getItemAtIndex(profileList.currentIndex);
      var profileName = selected.getAttribute("profile_name");
      var currentProfile = profile.currentProfile;
      if (currentProfile && (profileName == currentProfile))
        canDelete = false;      
    }
    if (canDelete) {
      if ( delbutton.getAttribute( "disabled" ) == "true" )
        delbutton.removeAttribute( "disabled" );
    }
    else
      delbutton.setAttribute( "disabled", "true" );
      
  }
}


function HandleKeyEvent( aEvent )
{
  switch( aEvent.keyCode ) 
  {
  case 46:
    if( profileManagerMode != "manager" )
      return;
    ConfirmDelete();
    break;
  case KeyEvent.DOM_VK_F2:
    if( profileManagerMode != "manager" )
      return;
    RenameProfile();
    break;
  }
}

function HandleClickEvent( aEvent )
{
  if (aEvent.button == 0 && aEvent.target.parentNode.view.selection.count) {
    if (!onStart())
      return false;
    window.close();
    return true;
  }
  return false;
}

