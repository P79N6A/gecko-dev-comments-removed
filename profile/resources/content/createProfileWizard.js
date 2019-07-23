







































var gProfile = Components.classes["@mozilla.org/profile/manager;1"].getService(Components.interfaces.nsIProfileInternal);
var gProfileManagerBundle;


var gProfileRoot;


var gProfileDisplay;


function initWizard()
{ 
  gProfileManagerBundle = document.getElementById("bundle_profileManager");
    
  
  gProfileDisplay = document.getElementById("profileDisplay").firstChild;
  setDisplayToDefaultFolder();
}


function initSecondWizardPage() 
{
  var profileName = document.getElementById("profileName");
  profileName.select();
  profileName.focus();

  
  checkCurrentInput(profileName.value);
}

function setDisplayToDefaultFolder()
{
  setDisplayToFolder(gProfile.defaultProfileParentDir);
  document.getElementById("useDefault").disabled = true;
}

function setDisplayToFolder(profileRoot)
{
  var profileName = document.getElementById("profileName");
  profileName.focus();
  gProfileRoot = profileRoot;
}

function updateProfileDisplay()
{
  var currentProfileName = document.getElementById("profileName").value;
  var profilePathAndName = gProfileRoot.clone();

  profilePathAndName.append(currentProfileName);
  gProfileDisplay.data = profilePathAndName.path;
}



function showLangDialog()
{
  var languageCode = document.getElementById("profileLanguage").getAttribute("data");
  var regionCode = document.getElementById("profileRegion").getAttribute("data");
  window.openDialog("chrome://communicator/content/profile/selectLang.xul",
                    "", "centerscreen,modal,titlebar",
                    languageCode, regionCode);
}


function chooseProfileFolder()
{
  var newProfileRoot;
  
  try {
    var dirChooser = Components.classes["@mozilla.org/filepicker;1"].createInstance(Components.interfaces.nsIFilePicker);
    dirChooser.init(window, gProfileManagerBundle.getString("chooseFolder"), Components.interfaces.nsIFilePicker.modeGetFolder);
    dirChooser.appendFilters(Components.interfaces.nsIFilePicker.filterAll);
    if (dirChooser.show() == dirChooser.returnCancel)
      return;
    newProfileRoot = dirChooser.file;
  }
  catch(e) {
    
    return;
  }

  
  
  document.getElementById("useDefault").disabled = (newProfileRoot.equals(gProfile.defaultProfileParentDir));

  setDisplayToFolder(newProfileRoot);
  updateProfileDisplay();
}


function checkCurrentInput(currentInput)
{
  var finishButton = document.documentElement.getButton("finish");
  var finishText = document.getElementById("finishText");
  var canAdvance;

  var errorMessage = checkProfileName(currentInput);
  if (!errorMessage) {
    finishText.className = "";
    finishText.firstChild.data = gProfileManagerBundle.getString("profileFinishText");
    canAdvance = true;
  }
  else {
    finishText.className = "error";
    finishText.firstChild.data = errorMessage;
    canAdvance = false;
  }

  document.documentElement.canAdvance = canAdvance;
  finishButton.disabled = !canAdvance;

  updateProfileDisplay();
}



function checkProfileName(profileNameToCheck)
{
  
  if (!/\S/.test(profileNameToCheck))
    return gProfileManagerBundle.getString("profileNameEmpty");

  
  if (/([\\*:?<>|\/\"])/.test(profileNameToCheck))
    return gProfileManagerBundle.getFormattedString("invalidChar", [RegExp.$1]);

  
  if (gProfile.profileExists(profileNameToCheck))
    return gProfileManagerBundle.getString("profileExists");

  
  return "";
}


function enableNextButton()
{
  document.documentElement.canAdvance = true;
}

function onCancel()
{
  
  if (!window.opener)
    return true;

  try {
    gProfile.forgetCurrentProfile();
  }
  catch (ex) {
  }

  return true;
}

function onFinish() 
{
  var profileName = document.getElementById("profileName").value;
  var languageCode = document.getElementById("profileLanguage").getAttribute("data");
  var regionCode = document.getElementById("profileRegion").getAttribute("data");

  var proceed = processCreateProfileData(profileName, gProfileRoot, languageCode, regionCode);
  
  if (!proceed)
    return false;

  
  if (window.opener)
    
    window.opener.CreateProfile(profileName, gProfileRoot);
  else {
    
    gProfile.currentProfile = profileName;
    var dialogParams = window.arguments[0].QueryInterface(Components.interfaces.nsIDialogParamBlock);
    dialogParams.SetInt(0, 1); 
  }

  
  return true;
}


function processCreateProfileData(profileName, profileRoot, languageCode, regionCode)
{
  try {
    var profileLocation = profileRoot.clone();
    profileLocation.append(profileName);
    gProfile.createNewProfileWithLocales(profileName, profileRoot.path, languageCode, regionCode, profileLocation.exists());

    return true;
  }
  catch (e) {
    var profileCreationFailed = gProfileManagerBundle.getString("profileCreationFailed");
    var profileCreationFailedTitle = gProfileManagerBundle.getString("profileCreationFailedTitle");
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
    promptService.alert(window, profileCreationFailedTitle, profileCreationFailed);

    return false;
  }
}
