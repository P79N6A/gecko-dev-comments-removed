




































function nsPluginInstallerWizard(){

  
  this.mPluginRequestArray = new Object();
  
  this.mPluginRequestArrayLength = 0;

  
  
  
  this.mPluginInfoArray = new Object();
  this.mPluginInfoArrayLength = 0;

  
  this.mPluginNotFoundArray = new Object();
  this.mPluginNotFoundArrayLength = 0;

  
  this.mPluginLicenseArray = new Array();

  
  this.pluginsToInstallNum = 0;

  this.mTab = null;
  this.mSuccessfullPluginInstallation = 0;

  
  
  

  if ("arguments" in window) {
    for (var item in window.arguments[0].plugins){
      this.mPluginRequestArray[window.arguments[0].plugins[item].mimetype] =
        new nsPluginRequest(window.arguments[0].plugins[item]);

      this.mPluginRequestArrayLength++;
    }

    this.mTab = window.arguments[0].tab;
  }

  this.WSPluginCounter = 0;
  this.licenseAcceptCounter = 0;

  this.prefBranch = null;
}

nsPluginInstallerWizard.prototype.getPluginData = function (){
  
  this.WSPluginCounter = 0;

  
  var rdfUpdater = new nsRDFItemUpdater(this.getOS(), this.getChromeLocale());

  for (item in this.mPluginRequestArray) {
    rdfUpdater.checkForPlugin(this.mPluginRequestArray[item]);
  }
}



nsPluginInstallerWizard.prototype.pluginInfoReceived = function (aPluginInfo){
  this.WSPluginCounter++;

  if (aPluginInfo && (aPluginInfo.pid != -1) ) {
    
    this.mPluginInfoArray[aPluginInfo.pid] = new PluginInfo(aPluginInfo);
    this.mPluginInfoArrayLength++;
  } else {
    this.mPluginNotFoundArray[aPluginInfo.requestedMimetype] = new PluginInfo(aPluginInfo);
    this.mPluginNotFoundArrayLength++;
  }

  var progressMeter = document.getElementById("ws_request_progress");

  if (progressMeter.getAttribute("mode") == "undetermined")
    progressMeter.setAttribute("mode", "determined");

  progressMeter.setAttribute("value",
      ((this.WSPluginCounter / this.mPluginRequestArrayLength) * 100) + "%");

  if (this.WSPluginCounter == this.mPluginRequestArrayLength) {
    
    if (this.mPluginInfoArrayLength == 0) {
      this.advancePage("lastpage", true, false, false);
    } else {
      
      this.advancePage(null, true, false, true);
    }
  } else {
    
  }
}

nsPluginInstallerWizard.prototype.showPluginList = function (){
  var myPluginList = document.getElementById("pluginList");
  var hasPluginWithInstallerUI = false;

  
  for (var run = myPluginList.childNodes.length; run > 0; run--)
    myPluginList.removeChild(myPluginList.childNodes.item(run));

  this.pluginsToInstallNum = 0;

  for (pluginInfoItem in this.mPluginInfoArray){
    

    var pluginInfo = this.mPluginInfoArray[pluginInfoItem];

    
    var myCheckbox = document.createElement("checkbox");
    myCheckbox.setAttribute("checked", "true");
    myCheckbox.setAttribute("oncommand", "gPluginInstaller.toggleInstallPlugin('" + pluginInfo.pid + "', this)");
    
    myCheckbox.setAttribute("label", pluginInfo.name + " " + (pluginInfo.version ? pluginInfo.version : ""));
    myCheckbox.setAttribute("src", pluginInfo.IconUrl);

    myPluginList.appendChild(myCheckbox);

    if (pluginInfo.InstallerShowsUI == "true")
      hasPluginWithInstallerUI = true;

    
    this.pluginsToInstallNum++;
  }

  if (hasPluginWithInstallerUI)
    document.getElementById("installerUI").hidden = false;

  this.canAdvance(true);
  this.canRewind(false);
}

nsPluginInstallerWizard.prototype.toggleInstallPlugin = function (aPid, aCheckbox) {
  this.mPluginInfoArray[aPid].toBeInstalled = aCheckbox.checked;

  
  this.pluginsToInstallNum = 0;
  for (pluginInfoItem in this.mPluginInfoArray){
    if (this.mPluginInfoArray[pluginInfoItem].toBeInstalled)
      this.pluginsToInstallNum++;
  }

  if (this.pluginsToInstallNum > 0)
    this.canAdvance(true);
  else
    this.canAdvance(false);
}

nsPluginInstallerWizard.prototype.canAdvance = function (aBool){
  document.getElementById("plugin-installer-wizard").canAdvance = aBool;
}

nsPluginInstallerWizard.prototype.canRewind = function (aBool){
  document.getElementById("plugin-installer-wizard").canRewind = aBool;
}

nsPluginInstallerWizard.prototype.canCancel = function (aBool){
  document.documentElement.getButton("cancel").disabled = !aBool;
}

nsPluginInstallerWizard.prototype.showLicenses = function (){
  this.canAdvance(false);
  this.canRewind(false);

  
  
  for (pluginInfoItem in this.mPluginInfoArray){
    var myPluginInfoItem = this.mPluginInfoArray[pluginInfoItem];
    if (myPluginInfoItem.toBeInstalled && myPluginInfoItem.licenseURL && (myPluginInfoItem.licenseURL != ""))
      this.mPluginLicenseArray.push(myPluginInfoItem.pid);
  }

  if (this.mPluginLicenseArray.length == 0) {
    
    this.advancePage(null, true, false, false);
  } else {
    this.licenseAcceptCounter = 0;

    
    var docShell = document.getElementById("licenseIFrame").docShell;
    var iiReq = docShell.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
    var webProgress = iiReq.getInterface(Components.interfaces.nsIWebProgress);
    webProgress.addProgressListener(gPluginInstaller.progressListener,
                                    Components.interfaces.nsIWebProgress.NOTIFY_ALL);

    this.showLicense();
  }
}

nsPluginInstallerWizard.prototype.enableNext = function (){
  
  
  if (gPluginInstaller.pluginsToInstallNum > 1)
    gPluginInstaller.canAdvance(true);

  document.getElementById("licenseRadioGroup1").disabled = false;
  document.getElementById("licenseRadioGroup2").disabled = false;
}

const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
nsPluginInstallerWizard.prototype.progressListener = {
  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if ((aStateFlags & nsIWebProgressListener.STATE_STOP) &&
       (aStateFlags & nsIWebProgressListener.STATE_IS_NETWORK)) {
      
      gPluginInstaller.enableNext();
    }
  },

  onProgressChange : function(aWebProgress, aRequest, aCurSelfProgress,
                              aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {},
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
  {},

  QueryInterface : function(aIID)
  {
     if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
         aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
         aIID.equals(Components.interfaces.nsISupports))
       return this;
     throw Components.results.NS_NOINTERFACE;
   }
}

nsPluginInstallerWizard.prototype.showLicense = function (){
  var pluginInfo = this.mPluginInfoArray[this.mPluginLicenseArray[this.licenseAcceptCounter]];

  this.canAdvance(false);

  loadFlags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
  document.getElementById("licenseIFrame").webNavigation.loadURI(pluginInfo.licenseURL, loadFlags, null, null, null);

  document.getElementById("pluginLicenseLabel").firstChild.nodeValue = 
    this.getFormattedString("pluginLicenseAgreement.label", [pluginInfo.name]);

  document.getElementById("licenseRadioGroup1").disabled = true;
  document.getElementById("licenseRadioGroup2").disabled = true;
  document.getElementById("licenseRadioGroup").selectedIndex = 
    pluginInfo.licenseAccepted ? 0 : 1;
}

nsPluginInstallerWizard.prototype.showNextLicense = function (){
  var rv = true;

  if (this.mPluginLicenseArray.length > 0) {
    this.storeLicenseRadioGroup();

    this.licenseAcceptCounter++;

    if (this.licenseAcceptCounter < this.mPluginLicenseArray.length) {
      this.showLicense();

      rv = false;
      this.canRewind(true);
    }
  }

  return rv;
}

nsPluginInstallerWizard.prototype.showPreviousLicense = function (){
  this.storeLicenseRadioGroup();
  this.licenseAcceptCounter--;

  if (this.licenseAcceptCounter > 0)
    this.canRewind(true);
  else
    this.canRewind(false);

  this.showLicense();
  
  
  return false;
}

nsPluginInstallerWizard.prototype.storeLicenseRadioGroup = function (){
  var pluginInfo = this.mPluginInfoArray[this.mPluginLicenseArray[this.licenseAcceptCounter]];
  pluginInfo.licenseAccepted = !document.getElementById("licenseRadioGroup").selectedIndex;
}

nsPluginInstallerWizard.prototype.licenseRadioGroupChange = function(aAccepted) {
  
  if (this.pluginsToInstallNum == 1)
    this.canAdvance(aAccepted);
}

nsPluginInstallerWizard.prototype.advancePage = function (aPageId, aCanAdvance, aCanRewind, aCanCancel){
  this.canAdvance(true);
  document.getElementById("plugin-installer-wizard").advance(aPageId);

  this.canAdvance(aCanAdvance);
  this.canRewind(aCanRewind);
  this.canCancel(aCanCancel);
}

nsPluginInstallerWizard.prototype.startPluginInstallation = function (){
  this.canAdvance(false);
  this.canRewind(false);

  
  
  

  var pluginURLArray = new Array();
  var pluginPidArray = new Array();

  for (pluginInfoItem in this.mPluginInfoArray){
    var pluginItem = this.mPluginInfoArray[pluginInfoItem];

    
    
    if (pluginItem.toBeInstalled && pluginItem.XPILocation && pluginItem.licenseAccepted) {
      pluginURLArray.push(pluginItem.XPILocation);
      pluginPidArray.push(pluginItem.pid);
    }
  }

  if (pluginURLArray.length > 0)
    PluginInstallService.startPluginInsallation(pluginURLArray, pluginPidArray);
  else
    this.advancePage(null, true, false, false);
}








nsPluginInstallerWizard.prototype.pluginInstallationProgress = function (aPid, aProgress, aError) {

  var statMsg = null;
  var pluginInfo = gPluginInstaller.mPluginInfoArray[aPid];

  switch (aProgress) {

    case 0:
      statMsg = this.getFormattedString("pluginInstallation.download.start", [pluginInfo.name]);
      break;

    case 1:
      statMsg = this.getFormattedString("pluginInstallation.download.finish", [pluginInfo.name]);
      break;

    case 2:
      statMsg = this.getFormattedString("pluginInstallation.install.start", [pluginInfo.name]);
      break;

    case 3:
      if (aError) {
        statMsg = this.getFormattedString("pluginInstallation.install.error", [pluginInfo.name, aError]);
        pluginInfo.error = aError;
      } else {
        statMsg = this.getFormattedString("pluginInstallation.install.finish", [pluginInfo.name]);
        pluginInfo.error = null;
      }
      break;

    case 4:
      statMsg = this.getString("pluginInstallation.complete");
      break;
  }

  if (statMsg)
    document.getElementById("plugin_install_progress_message").value = statMsg;

  if (aProgress == 4) {
    this.advancePage(null, true, false, false);
  }
}

nsPluginInstallerWizard.prototype.pluginInstallationProgressMeter = function (aPid, aValue, aMaxValue){
  var progressElm = document.getElementById("plugin_install_progress");

  if (progressElm.getAttribute("mode") == "undetermined")
    progressElm.setAttribute("mode", "determined");
  
  progressElm.setAttribute("value", Math.ceil((aValue / aMaxValue) * 100) + "%")
}

nsPluginInstallerWizard.prototype.addPluginResultRow = function (aImgSrc, aName, aNameTooltip, aStatus, aStatusTooltip, aManualUrl){
  var myRows = document.getElementById("pluginResultList");

  var myRow = document.createElement("row");
  myRow.setAttribute("align", "center");

  
  var myImage = document.createElement("image");
  myImage.setAttribute("src", aImgSrc);
  myImage.setAttribute("height", "16px");
  myImage.setAttribute("width", "16px");
  myRow.appendChild(myImage)

  
  var myLabel = document.createElement("label");
  myLabel.setAttribute("value", aName);
  if (aNameTooltip)
    myLabel.setAttribute("tooltiptext", aNameTooltip);
  myRow.appendChild(myLabel);

  if (aStatus) {
    myLabel = document.createElement("label");
    myLabel.setAttribute("value", aStatus);
    myRow.appendChild(myLabel);
  }

  
  if (aManualUrl) {
    var myButton = document.createElement("button");

    var manualInstallLabel = this.getString("pluginInstallationSummary.manualInstall.label");
    var manualInstallTooltip = this.getString("pluginInstallationSummary.manualInstall.tooltip");

    myButton.setAttribute("label", manualInstallLabel);
    myButton.setAttribute("tooltiptext", manualInstallTooltip);

    myRow.appendChild(myButton);

    
    if (aManualUrl)
      myButton.addEventListener("command", function() { gPluginInstaller.loadURL(aManualUrl) }, false);
  }

  myRows.appendChild(myRow);
}

nsPluginInstallerWizard.prototype.showPluginResults = function (){
  var notInstalledList = "?action=missingplugins";
  var needsRestart = false;
  var myRows = document.getElementById("pluginResultList");

  
  for (var run = myRows.childNodes.length; run--; run > 0)
    myRows.removeChild(myRows.childNodes.item(run));

  for (pluginInfoItem in this.mPluginInfoArray){
    

    var myPluginItem = this.mPluginInfoArray[pluginInfoItem];

    if (myPluginItem.toBeInstalled) {
      var statusMsg;
      var statusTooltip;
      if (myPluginItem.error){
        statusMsg = this.getString("pluginInstallationSummary.failed");
        statusTooltip = myPluginItem.error;
        notInstalledList += "&mimetype=" + pluginInfoItem;
      } else if (!myPluginItem.licenseAccepted) {
        statusMsg = this.getString("pluginInstallationSummary.licenseNotAccepted");
      } else if (!myPluginItem.XPILocation) {
        statusMsg = this.getString("pluginInstallationSummary.notAvailable");
        notInstalledList += "&mimetype=" + pluginInfoItem;
      } else {
        this.mSuccessfullPluginInstallation++;
        statusMsg = this.getString("pluginInstallationSummary.success");

        
        if (myPluginItem.needsRestart)
          needsRestart = true;
      }

      
      var manualUrl;
      if ((myPluginItem.error || !myPluginItem.XPILocation) && (myPluginItem.manualInstallationURL || this.mPluginRequestArray[myPluginItem.requestedMimetype].pluginsPage)){
        manualUrl = myPluginItem.manualInstallationURL ? myPluginItem.manualInstallationURL : this.mPluginRequestArray[myPluginItem.requestedMimetype].pluginsPage;
      }

      this.addPluginResultRow(
          myPluginItem.IconUrl, 
          myPluginItem.name + " " + (myPluginItem.version ? myPluginItem.version : ""),
          null,
          statusMsg, 
          statusTooltip,
          manualUrl);
    }
  }

  
  for (pluginInfoItem in this.mPluginNotFoundArray){
    var pluginRequest = this.mPluginRequestArray[pluginInfoItem];

    
    if (pluginRequest) {
      this.addPluginResultRow(
          "",
          this.getFormattedString("pluginInstallation.unknownPlugin", [pluginInfoItem]),
          null,
          null,
          null,
          pluginRequest.pluginsPage);
    }

    notInstalledList += "&mimetype=" + pluginInfoItem;
  }

  
  if (this.mPluginInfoArrayLength == 0) {
    var noPluginsFound = this.getString("pluginInstallation.noPluginsFound");
    document.getElementById("pluginSummaryDescription").setAttribute("value", noPluginsFound);
  } else if (this.mSuccessfullPluginInstallation == 0) {
    
    var noPluginsInstalled = this.getString("pluginInstallation.noPluginsInstalled");
    document.getElementById("pluginSummaryDescription").setAttribute("value", noPluginsInstalled);
  }

  document.getElementById("pluginSummaryRestartNeeded").hidden = !needsRestart;

  var app = Components.classes["@mozilla.org/xre/app-info;1"]
                      .getService(Components.interfaces.nsIXULAppInfo);

  
  notInstalledList +=
    "&appID=" + app.ID +
    "&appVersion=" + app.platformBuildID +
    "&clientOS=" + this.getOS() +
    "&chromeLocale=" + this.getChromeLocale();

  document.getElementById("moreInfoLink").addEventListener("click", function() { gPluginInstaller.loadURL("https://pfs.mozilla.org/plugins/" + notInstalledList) }, false);

  this.canAdvance(true);
  this.canRewind(false);
  this.canCancel(false);
}

nsPluginInstallerWizard.prototype.loadURL = function (aUrl){
  
  
  
  
  var pluginPagePrincipal =
    window.opener.content.document.nodePrincipal;

  const nsIScriptSecurityManager =
    Components.interfaces.nsIScriptSecurityManager;
  var secMan = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                         .getService(nsIScriptSecurityManager);

  secMan.checkLoadURIStrWithPrincipal(pluginPagePrincipal, aUrl,
    nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);

  window.opener.open(aUrl);
}

nsPluginInstallerWizard.prototype.getString = function (aName){
  return document.getElementById("pluginWizardString").getString(aName);
}

nsPluginInstallerWizard.prototype.getFormattedString = function (aName, aArray){
  return document.getElementById("pluginWizardString").getFormattedString(aName, aArray);
}

nsPluginInstallerWizard.prototype.getOS = function (){
  var httpService = Components.classes["@mozilla.org/network/protocol;1?name=http"]
                              .getService(Components.interfaces.nsIHttpProtocolHandler);
  return httpService.oscpu;
}

nsPluginInstallerWizard.prototype.getChromeLocale = function (){
  var chromeReg = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                            .getService(Components.interfaces.nsIXULChromeRegistry);
  return chromeReg.getSelectedLocale("global");
}

nsPluginInstallerWizard.prototype.getPrefBranch = function (){
  if (!this.prefBranch)
    this.prefBranch = Components.classes["@mozilla.org/preferences-service;1"]
                                .getService(Components.interfaces.nsIPrefBranch);
  return this.prefBranch;
}
function nsPluginRequest(aPlugRequest){
  this.mimetype = encodeURI(aPlugRequest.mimetype);
  this.pluginsPage = aPlugRequest.pluginsPage;
}

function PluginInfo(aResult) {
  this.name = aResult.name;
  this.pid = aResult.pid;
  this.version = aResult.version;
  this.IconUrl = aResult.IconUrl;
  this.XPILocation = aResult.XPILocation;
  this.InstallerShowsUI = aResult.InstallerShowsUI;
  this.manualInstallationURL = aResult.manualInstallationURL;
  this.requestedMimetype = aResult.requestedMimetype;
  this.licenseURL = aResult.licenseURL;
  this.needsRestart = (aResult.needsRestart == "true");

  this.error = null;
  this.toBeInstalled = true;

  
  this.licenseAccepted = this.licenseURL ? false : true;
}

var gPluginInstaller;

function wizardInit(){
  gPluginInstaller = new nsPluginInstallerWizard();
  gPluginInstaller.canAdvance(false);
  gPluginInstaller.getPluginData();
}

function wizardFinish(){
  
  if ((gPluginInstaller.mSuccessfullPluginInstallation > 0) &&
      (gPluginInstaller.mPluginInfoArray.length != 0) &&
      gPluginInstaller.mTab) {
    
    gPluginInstaller.mTab.missingPlugins = null;
    
    window.opener.gMissingPluginInstaller.closeNotification();
    
    window.opener.getBrowser().reloadTab(gPluginInstaller.mTab);
  }

  return true;
}
