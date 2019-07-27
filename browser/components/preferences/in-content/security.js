



Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

var gSecurityPane = {
  _pane: null,

  


  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gSecurityPane));
    }

    this._pane = document.getElementById("paneSecurity");
    this._initMasterPasswordUI();

    setEventListener("addonExceptions", "command",
      gSecurityPane.showAddonExceptions);
    setEventListener("passwordExceptions", "command",
      gSecurityPane.showPasswordExceptions);
    setEventListener("useMasterPassword", "command",
      gSecurityPane.updateMasterPasswordButton);
    setEventListener("changeMasterPassword", "command",
      gSecurityPane.changeMasterPassword);
    setEventListener("showPasswords", "command",
      gSecurityPane.showPasswords);
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

    gSubDialog.open("chrome://browser/content/preferences/permissions.xul",
                    null, params);
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

    if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
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
    gSubDialog.open("chrome://passwordmgr/content/passwordManagerExceptions.xul");
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
      this._initMasterPasswordUI();
    }
    else {
      gSubDialog.open("chrome://mozapps/content/preferences/removemp.xul",
                      null, null, this._initMasterPasswordUI.bind(this));
    }
  },

  


  changeMasterPassword: function ()
  {
    gSubDialog.open("chrome://mozapps/content/preferences/changemp.xul",
                    "resizable=no", null, this._initMasterPasswordUI.bind(this));
  },

  



  showPasswords: function ()
  {
    gSubDialog.open("chrome://passwordmgr/content/passwordManager.xul");
  }

};
