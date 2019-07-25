



var gSecurityPane = {
  _pane: null,

  


  init: function ()
  {
    this._pane = document.getElementById("paneSecurity");
    this._initMasterPasswordUI();
  },

  

  








  



  readWarnAddonInstall: function ()
  {
    var warn = document.getElementById("xpinstall.whitelist.required");
    var exceptions = document.getElementById("addonExceptions");

    exceptions.disabled = !warn.value;

    
    return undefined;
  },

  


  showAddonExceptions: function ()
  {
    var bundlePrefs = document.getElementById("bundlePreferences");

    var params = this._addonParams;
    if (!params.windowTitle || !params.introText) {
      params.windowTitle = bundlePrefs.getString("addons_permissions_title");
      params.introText = bundlePrefs.getString("addonspermissionstext");
    }

    openDialog("chrome://browser/content/preferences/permissions.xul",
               "Browser:Permissions",
               "model=yes",
               params);
  },

  


  _addonParams:
    {
      blockVisible: false,
      sessionVisible: false,
      allowVisible: true,
      prefilledHost: "",
      permissionType: "install"
    },

  

  






  




  readSavePasswords: function ()
  {
    var pref = document.getElementById("signon.rememberSignons");
    var excepts = document.getElementById("passwordExceptions");

    const Cc = Components.classes, Ci = Components.interfaces;
    var pbs = Cc["@mozilla.org/privatebrowsing;1"].
              getService(Ci.nsIPrivateBrowsingService);

    if (pbs.autoStarted) {
      document.getElementById("savePasswords").disabled = true;
      excepts.disabled = true;
      return false;
    } else {
      excepts.disabled = !pref.value;
      
      return undefined;
    }
  },

  



  showPasswordExceptions: function ()
  {
    openDialog("chrome://passwordmgr/content/passwordManagerExceptions.xul",
               "Toolkit:PasswordManagerExceptions",
               "model=yes",
               null);
  },

  





  _initMasterPasswordUI: function ()
  {
    var noMP = !this._masterPasswordSet();

    var button = document.getElementById("changeMasterPassword");
    button.disabled = noMP;

    var checkbox = document.getElementById("useMasterPassword");
    checkbox.checked = !noMP;
  },

  


  _masterPasswordSet: function ()
  {
    var secmodDB = Cc["@mozilla.org/security/pkcs11moduledb;1"].
                   getService(Ci.nsIPKCS11ModuleDB);
    var slot = secmodDB.findSlotByName("");
    if (slot) {
      var status = slot.status;
      var hasMP = status != Ci.nsIPKCS11Slot.SLOT_UNINITIALIZED &&
                  status != Ci.nsIPKCS11Slot.SLOT_READY;
      return hasMP;
    } else {
      
      return false;
    }
  },

  




  updateMasterPasswordButton: function ()
  {
    var checkbox = document.getElementById("useMasterPassword");
    var button = document.getElementById("changeMasterPassword");
    button.disabled = !checkbox.checked;

    
    
    
    
    
    if (!checkbox.checked)
      this._removeMasterPassword();
    else
      this.changeMasterPassword();

    this._initMasterPasswordUI();
  },

  




  _removeMasterPassword: function ()
  {
    var secmodDB = Cc["@mozilla.org/security/pkcs11moduledb;1"].
                   getService(Ci.nsIPKCS11ModuleDB);
    if (secmodDB.isFIPSEnabled) {
      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                          getService(Ci.nsIPromptService);
      var bundle = document.getElementById("bundlePreferences");
      promptService.alert(window,
                          bundle.getString("pw_change_failed_title"),
                          bundle.getString("pw_change2empty_in_fips_mode"));
    }
    else {
      openDialog("chrome://mozapps/content/preferences/removemp.xul",
                 "Toolkit:RemoveMasterPassword", "modal=yes", null);
    }
    this._initMasterPasswordUI();
  },

  


  changeMasterPassword: function ()
  {
    openDialog("chrome://mozapps/content/preferences/changemp.xul",
               "Toolkit:ChangeMasterPassword", "modal=yes", null);
    this._initMasterPasswordUI();
  },

  



  showPasswords: function ()
  {
    openDialog("chrome://passwordmgr/content/passwordManager.xul",
               "Toolkit:PasswordManager",
               "modal=yes", null);
  }

};
