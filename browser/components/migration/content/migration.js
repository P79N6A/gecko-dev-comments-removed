



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const kIMig = Ci.nsIBrowserProfileMigrator;
const kIPStartup = Ci.nsIProfileStartup;

Cu.import("resource:///modules/MigrationUtils.jsm");

var MigrationWizard = {
  _source: "",                  
  _itemsFlags: kIMig.ALL,       
  _selectedProfile: null,       
  _wiz: null,
  _migrator: null,
  _autoMigrate: null,

  init: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.addObserver(this, "Migration:Started", false);
    os.addObserver(this, "Migration:ItemBeforeMigrate", false);
    os.addObserver(this, "Migration:ItemAfterMigrate", false);
    os.addObserver(this, "Migration:ItemError", false);
    os.addObserver(this, "Migration:Ended", false);

    this._wiz = document.documentElement;

    if ("arguments" in window && window.arguments.length > 1) {
      this._source = window.arguments[0];
      this._migrator = window.arguments[1] instanceof kIMig ?
                       window.arguments[1] : null;
      this._autoMigrate = window.arguments[2].QueryInterface(kIPStartup);
      this._skipImportSourcePage = window.arguments[3];

      if (this._autoMigrate) {
        
        
        var nothing = document.getElementById("nothing");
        nothing.hidden = false;
      }
    }

    this.onImportSourcePageShow();
  },

  uninit: function ()
  {
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    os.removeObserver(this, "Migration:Started");
    os.removeObserver(this, "Migration:ItemBeforeMigrate");
    os.removeObserver(this, "Migration:ItemAfterMigrate");
    os.removeObserver(this, "Migration:ItemError");
    os.removeObserver(this, "Migration:Ended");
    MigrationUtils.finishMigration();
  },

  
  onImportSourcePageShow: function ()
  {
    this._wiz.canRewind = false;

    var selectedMigrator = null;

    
    var group = document.getElementById("importSourceGroup");
    for (var i = 0; i < group.childNodes.length; ++i) {
      var migratorKey = group.childNodes[i].id;
      if (migratorKey != "nothing") {
        var migrator = MigrationUtils.getMigrator(migratorKey);
        if (migrator) {
          
          
          if (!selectedMigrator || this._source == migratorKey)
            selectedMigrator = group.childNodes[i];
        } else {
          
          group.childNodes[i].hidden = true;
        }
      }
    }

    if (selectedMigrator)
      group.selectedItem = selectedMigrator;
    else {
      
      document.getElementById("noSources").hidden = false;

      this._wiz.canAdvance = false;

      document.getElementById("importBookmarks").hidden = true;
      document.getElementById("importAll").hidden = true;
    }

    
    if (this._migrator && this._skipImportSourcePage) {
      this._wiz.advance();
      this._wiz.canRewind = false;
    }
  },
  
  onImportSourcePageAdvanced: function ()
  {
    var newSource = document.getElementById("importSourceGroup").selectedItem.id;
    
    if (newSource == "nothing") {
      document.documentElement.cancel();
      return false;
    }
    
    if (!this._migrator || (newSource != this._source)) {
      
      this._migrator = MigrationUtils.getMigrator(newSource);

      this._itemsFlags = kIMig.ALL;
      this._selectedProfile = null;
    }
    this._source = newSource;

    
    var sourceProfiles = this._migrator.sourceProfiles;    
    if (sourceProfiles && sourceProfiles.length > 1) {
      this._wiz.currentPage.next = "selectProfile";
    }
    else {
      if (this._autoMigrate)
        this._wiz.currentPage.next = "homePageImport";
      else
        this._wiz.currentPage.next = "importItems";

      if (sourceProfiles && sourceProfiles.length == 1)
        this._selectedProfile = sourceProfiles[0];
      else
        this._selectedProfile = "";
    }
  },
  
  
  onSelectProfilePageShow: function ()
  {
    
    
    
    
      
    var profiles = document.getElementById("profiles");
    while (profiles.hasChildNodes()) 
      profiles.removeChild(profiles.firstChild);
    
    
    
    if (this._migrator) {
      var sourceProfiles = this._migrator.sourceProfiles;
      for (var i = 0; i < sourceProfiles.length; ++i) {
        var item = document.createElement("radio");
        item.id = sourceProfiles[i];
        item.setAttribute("label", sourceProfiles[i]);
        profiles.appendChild(item);
      }
    }
    
    profiles.selectedItem = this._selectedProfile ? document.getElementById(this._selectedProfile) : profiles.firstChild;
  },
  
  onSelectProfilePageRewound: function ()
  {
    var profiles = document.getElementById("profiles");
    this._selectedProfile = profiles.selectedItem.id;
  },
  
  onSelectProfilePageAdvanced: function ()
  {
    var profiles = document.getElementById("profiles");
    this._selectedProfile = profiles.selectedItem.id;
    
    
    if (this._autoMigrate)
      this._wiz.currentPage.next = "homePageImport";
  },
  
  
  onImportItemsPageShow: function ()
  {
    var dataSources = document.getElementById("dataSources");
    while (dataSources.hasChildNodes())
      dataSources.removeChild(dataSources.firstChild);

    var items = this._migrator.getMigrateData(this._selectedProfile, this._autoMigrate);
    for (var i = 0; i < 16; ++i) {
      var itemID = (items >> i) & 0x1 ? Math.pow(2, i) : 0;
      if (itemID > 0) {
        var checkbox = document.createElement("checkbox");
        checkbox.id = itemID;
        checkbox.setAttribute("label", 
          MigrationUtils.getLocalizedString(itemID + "_" + this._source));
        dataSources.appendChild(checkbox);
        if (!this._itemsFlags || this._itemsFlags & itemID)
          checkbox.checked = true;
      }
    }
  },

  onImportItemsPageRewound: function ()
  {
    this._wiz.canAdvance = true;
    this.onImportItemsPageAdvanced();
  },

  onImportItemsPageAdvanced: function ()
  {
    var dataSources = document.getElementById("dataSources");
    this._itemsFlags = 0;
    for (var i = 0; i < dataSources.childNodes.length; ++i) {
      var checkbox = dataSources.childNodes[i];
      if (checkbox.localName == "checkbox" && checkbox.checked)
        this._itemsFlags |= parseInt(checkbox.id);
    }
  },
  
  onImportItemCommand: function (aEvent)
  {
    var items = document.getElementById("dataSources");
    var checkboxes = items.getElementsByTagName("checkbox");

    var oneChecked = false;
    for (var i = 0; i < checkboxes.length; ++i) {
      if (checkboxes[i].checked) {
        oneChecked = true;
        break;
      }
    }

    this._wiz.canAdvance = oneChecked;
  },

  
  onHomePageMigrationPageShow: function ()
  {
    
    if (!this._autoMigrate) {
      this._wiz.advance();
      return;
    }

    var brandBundle = document.getElementById("brandBundle");
    
    
    try {
      var pageTitle = brandBundle.getString("homePageMigrationPageTitle");
      var pageDesc = brandBundle.getString("homePageMigrationDescription");
      var mainStr = brandBundle.getString("homePageSingleStartMain");
    }
    catch (e) {
      this._wiz.advance();
      return;
    }

    document.getElementById("homePageImport").setAttribute("label", pageTitle);
    document.getElementById("homePageImportDesc").setAttribute("value", pageDesc);

    this._wiz._adjustWizardHeader();

    var singleStart = document.getElementById("homePageSingleStart");
    singleStart.setAttribute("label", mainStr);
    singleStart.setAttribute("value", "DEFAULT");

    var source = null;
    switch (this._source) {
      case "ie":
        source = "sourceNameIE";
        break;
      case "safari":
        source = "sourceNameSafari";
        break;
      case "chrome":
        source = "sourceNameChrome";
        break;
      case "firefox":
        source = "sourceNameFirefox";
        break;
    }

    
    this._migrator.getMigrateData(this._selectedProfile, this._autoMigrate);

    var oldHomePageURL = this._migrator.sourceHomePageURL;

    if (oldHomePageURL && source) {
      var appName = MigrationUtils.getLocalizedString(source);
      var oldHomePageLabel =
        brandBundle.getFormattedString("homePageImport", [appName]);
      var oldHomePage = document.getElementById("oldHomePage");
      oldHomePage.setAttribute("label", oldHomePageLabel);
      oldHomePage.setAttribute("value", oldHomePageURL);
      oldHomePage.removeAttribute("hidden");
    }
    else {
      
      this._wiz.advance();
    }
  },

  onHomePageMigrationPageAdvanced: function ()
  {
    
    try {
      var radioGroup = document.getElementById("homePageRadiogroup");

      this._newHomePage = radioGroup.selectedItem.value;
    } catch(ex) {}
  },

  
  onMigratingPageShow: function ()
  {
    this._wiz.getButton("cancel").disabled = true;
    this._wiz.canRewind = false;
    this._wiz.canAdvance = false;
    
    
    if (this._autoMigrate)
      this._itemsFlags = this._migrator.getMigrateData(this._selectedProfile, this._autoMigrate);

    this._listItems("migratingItems");
    setTimeout(this.onMigratingMigrate, 0, this);
  },

  onMigratingMigrate: function (aOuter)
  {
    aOuter._migrator.migrate(aOuter._itemsFlags, aOuter._autoMigrate, aOuter._selectedProfile);
  },
  
  _listItems: function (aID)
  {
    var items = document.getElementById(aID);
    while (items.hasChildNodes())
      items.removeChild(items.firstChild);

    var brandBundle = document.getElementById("brandBundle");
    var itemID;
    for (var i = 0; i < 16; ++i) {
      var itemID = (this._itemsFlags >> i) & 0x1 ? Math.pow(2, i) : 0;
      if (itemID > 0) {
        var label = document.createElement("label");
        label.id = itemID + "_migrated";
        try {
          label.setAttribute("value",
            MigrationUtils.getLocalizedString(itemID + "_" + this._source));
          items.appendChild(label);
        }
        catch (e) {
          
          
          break;
        }
      }
    }
  },
  
  observe: function (aSubject, aTopic, aData)
  {
    switch (aTopic) {
    case "Migration:Started":
      break;
    case "Migration:ItemBeforeMigrate":
      var label = document.getElementById(aData + "_migrated");
      if (label)
        label.setAttribute("style", "font-weight: bold");
      break;
    case "Migration:ItemAfterMigrate":
      var label = document.getElementById(aData + "_migrated");
      if (label)
        label.removeAttribute("style");
      break;
    case "Migration:Ended":
      if (this._autoMigrate) {
        if (this._newHomePage) {
          try {
            
            var prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                                    .getService(Components.interfaces.nsIPrefService);
            var prefBranch = prefSvc.getBranch(null);

            if (this._newHomePage == "DEFAULT") {
              prefBranch.clearUserPref("browser.startup.homepage");
            }
            else {
              var str = Components.classes["@mozilla.org/supports-string;1"]
                                .createInstance(Components.interfaces.nsISupportsString);
              str.data = this._newHomePage;
              prefBranch.setComplexValue("browser.startup.homepage",
                                         Components.interfaces.nsISupportsString,
                                         str);
            }

            var dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                                   .getService(Components.interfaces.nsIProperties);
            var prefFile = dirSvc.get("ProfDS", Components.interfaces.nsIFile);
            prefFile.append("prefs.js");
            prefSvc.savePrefFile(prefFile);
          } catch(ex) { 
            dump(ex); 
          }
        }

        
        this._wiz.canAdvance = true;
        this._wiz.advance();

        setTimeout(close, 5000);
      }
      else {
        this._wiz.canAdvance = true;
        var nextButton = this._wiz.getButton("next");
        nextButton.click();
      }
      break;
    case "Migration:ItemError":
      var type = "undefined";
      switch (parseInt(aData)) {
      case Ci.nsIBrowserProfileMigrator.SETTINGS:
        type = "settings";
        break;
      case Ci.nsIBrowserProfileMigrator.COOKIES:
        type = "cookies";
        break;
      case Ci.nsIBrowserProfileMigrator.HISTORY:
        type = "history";
        break;
      case Ci.nsIBrowserProfileMigrator.FORMDATA:
        type = "form data";
        break;
      case Ci.nsIBrowserProfileMigrator.PASSWORDS:
        type = "passwords";
        break;
      case Ci.nsIBrowserProfileMigrator.BOOKMARKS:
        type = "bookmarks";
        break;
      case Ci.nsIBrowserProfileMigrator.OTHERDATA:
        type = "misc. data";
        break;
      }
      Cc["@mozilla.org/consoleservice;1"]
        .getService(Ci.nsIConsoleService)
        .logStringMessage("some " + type + " did not successfully migrate.");
      break;
    }
  },

  onDonePageShow: function ()
  {
    this._wiz.getButton("cancel").disabled = true;
    this._wiz.canRewind = false;
    this._listItems("doneItems");
  }
};
