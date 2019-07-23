









































var gDialogParams;
var gStartupMode; 
var gProfileManagerBundle;
var gBrandBundle;
var profile     = Components.classes["@mozilla.org/profile/manager;1"].getService();
if (profile)
  profile       = profile.QueryInterface(Components.interfaces.nsIProfileInternal);

var Registry;

function StartUp()
{
  gDialogParams = window.arguments[0].
                    QueryInterface(Components.interfaces.nsIDialogParamBlock);
  gStartupMode = (gDialogParams.GetString(0) == "startup");
  
  gProfileManagerBundle = document.getElementById("bundle_profileManager");
  gBrandBundle = document.getElementById("bundle_brand");

  if (gStartupMode) {
    document.documentElement.setAttribute("buttonlabelcancel",
      document.documentElement.getAttribute("buttonlabelexit"));
    document.documentElement.setAttribute("buttonlabelaccept",
      document.documentElement.getAttribute("buttonlabelstart"));
  }

  if(window.location && window.location.search && window.location.search == "?manage=true" )
    SwitchProfileManagerMode();
  else {  
    
    var introTextItem = document.getElementById("intro");
    var insertText = document.documentElement.getAttribute("buttonlabelaccept");
    var introText = gProfileManagerBundle.getFormattedString(
                      gStartupMode ? "intro_start" : "intro_switch",
                      [insertText]);
    introTextItem.textContent = introText;
  }

  var dirServ = Components.classes['@mozilla.org/file/directory_service;1']
                          .getService(Components.interfaces.nsIProperties);

  
  
  
  var regFile = dirServ.get("AppRegF", Components.interfaces.nsIFile);

  Registry = Components.classes['@mozilla.org/registry;1'].createInstance();
  Registry = Registry.QueryInterface(Components.interfaces.nsIRegistry);
  Registry.open(regFile);

  loadElements();
  highlightCurrentProfile();

  var offlineState = document.getElementById("offlineState");
  if (gStartupMode) {
    
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].
                      getService(Components.interfaces.nsIIOService);
    offlineState.checked = ioService.offline;
  }
  else {
    
    offlineState.hidden = true;
  }

  var autoSelectLastProfile = document.getElementById("autoSelectLastProfile");
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefBranch);
  if (prefs.getBoolPref("profile.manage_only_at_launch"))
    autoSelectLastProfile.hidden = true;
  else
    autoSelectLastProfile.checked = profile.startWithLastUsedProfile;

  var profileList = document.getElementById("profiles");
  profileList.focus();

  DoEnabling();
}


function highlightCurrentProfile()
{
  try {
    var currentProfile = profile.currentProfile;
    if( !currentProfile )
      return;
    var profileList = document.getElementById( "profiles" );
    var currentProfileItem = profileList.getElementsByAttribute("profile_name", currentProfile).item(0);
    if( currentProfileItem ) {
      var currentProfileIndex = profileList.view.getIndexOfItem(currentProfileItem);
      profileList.view.selection.select( currentProfileIndex );
      profileList.treeBoxObject.ensureRowIsVisible( currentProfileIndex );
    }
  }
  catch(e) {
    dump("*** failed to select current profile in list\n");
  }
}



function AddItem(aName, aMigrated)
{
  var tree = document.getElementById("profiles");
  var treeitem = document.createElement("treeitem");
  var treerow = document.createElement("treerow");
  var treecell = document.createElement("treecell");
  treecell.setAttribute("label", aName);
  treecell.setAttribute("properties", "rowMigrate-" + aMigrated);
  treeitem.setAttribute("profile_name", aName);
  treeitem.setAttribute("rowMigrate", aMigrated);
  treerow.appendChild(treecell);
  treeitem.appendChild(treerow);
  tree.lastChild.appendChild(treeitem);
}



function loadElements()
{
  try {
    var profileRoot = Registry.getKey(Registry.Common, "Profiles");
    var regEnum = Registry.enumerateSubtrees( profileRoot );

    
    
    
    regEnum.first();
    while (true)
    {
      var node = regEnum.currentItem();
      node = node.QueryInterface(Components.interfaces.nsIRegistryNode);

      if ( node.name == "" )
        break;

      var migrated = Registry.getString( node.key, "migrated" );

      AddItem(node.name, migrated);

      regEnum.next();
    }
  }
  catch (e) {}
}




function onStart()
{
  var profileList = document.getElementById("profiles");
  var selected = profileList.view.getItemAtIndex(profileList.currentIndex);
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].
    getService(Components.interfaces.nsIPromptService);

  var profilename = selected.getAttribute("profile_name");
  if( selected.getAttribute("rowMigrate") == "no" ) {
    var lString = gProfileManagerBundle.getString("migratebeforestart");
    lString = lString.replace(/\s*<html:br\/>/g,"\n");
    lString = lString.replace(/%brandShortName%/gi,
                              gBrandBundle.getString("brandShortName"));
    var title = gProfileManagerBundle.getString("migratetitle");

    if (promptService.confirm(window, title, lString)) {
      var profileDir = profile.getProfileDir(profilename);
      if (profileDir) {
        profileDir = profileDir.QueryInterface( Components.interfaces.nsIFile );
        if (profileDir) {
          if (!profileDir.exists()) {
            var errorMessage = gProfileManagerBundle.getString("sourceProfileDirMissing");
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

  
  if (gStartupMode) {
    var offlineState = document.getElementById("offlineState");
    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService2);
    if (offlineState.checked != ioService.offline) {
      ioService.manageOfflineStatus = false;
      ioService.offline = offlineState.checked;
    }
  }

  var autoSelectLastProfile = document.getElementById("autoSelectLastProfile");
  if (!autoSelectLastProfile.hidden)
    profile.startWithLastUsedProfile = autoSelectLastProfile.checked;

  try {
    profile.currentProfile = profilename;
  }
  catch (ex) {
	  var brandName = gBrandBundle.getString("brandShortName");    
    var message;
    var fatalError = false;
    switch (ex.result) {
      case Components.results.NS_ERROR_FILE_ACCESS_DENIED:
        message = gProfileManagerBundle.getFormattedString("profDirLocked", [brandName, profilename]);
        message = message.replace(/\s*<html:br\/>/g,"\n");
        break;
      case Components.results.NS_ERROR_FILE_NOT_FOUND:
        message = gProfileManagerBundle.getFormattedString("profDirMissing", [brandName, profilename]);
        message = message.replace(/\s*<html:br\/>/g,"\n");
        break;
      case Components.results.NS_ERROR_ABORT:
        message = gProfileManagerBundle.getFormattedString("profileSwitchFailed", [brandName, profilename, brandName, brandName]);
        message = message.replace(/\s*<html:br\/>/g,"\n");
        fatalError = true;
        break;
      default:
        message = ex.message;
        break;
  }
      promptService.alert(window, null, message);

      if (fatalError)
      {
        var appStartup = Components.classes["@mozilla.org/seamonkey/app-startup;1"]
                                   .getService(Components.interfaces.nsIAppStartup);
        appStartup.quit(Components.interfaces.nsIAppStartup.eForceQuit);
      }

      return false;
  }
  
  gDialogParams.SetInt(0, 1); 
    
  return true;
}



function onExit()
{
  gDialogParams.SetInt(0, 0); 
  return true;
}
