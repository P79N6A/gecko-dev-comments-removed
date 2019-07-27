




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");










function LoginManagerPrompter() {
}

LoginManagerPrompter.prototype = {

    classID : Components.ID("97d12931-abe2-11df-94e2-0800200c9a66"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerPrompter]),

    _factory       : null,
    _window        : null,
    _debug         : false, 

    __pwmgr : null, 
    get _pwmgr() {
        if (!this.__pwmgr)
            this.__pwmgr = Cc["@mozilla.org/login-manager;1"].
                           getService(Ci.nsILoginManager);
        return this.__pwmgr;
    },

    __promptService : null, 
    get _promptService() {
        if (!this.__promptService)
            this.__promptService =
                Cc["@mozilla.org/embedcomp/prompt-service;1"].
                getService(Ci.nsIPromptService2);
        return this.__promptService;
    },
    
    __strBundle : null, 
    get _strBundle() {
        if (!this.__strBundle) {
            var bunService = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService);
            this.__strBundle = bunService.createBundle(
                        "chrome://browser/locale/passwordmgr.properties");
            if (!this.__strBundle)
                throw "String bundle for Login Manager not present!";
        }

        return this.__strBundle;
    },

    __brandBundle : null, 
    get _brandBundle() {
        if (!this.__brandBundle) {
            var bunService = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService);
            this.__brandBundle = bunService.createBundle(
                        "chrome://branding/locale/brand.properties");
            if (!this.__brandBundle)
                throw "Branding string bundle not present!";
        }

        return this.__brandBundle;
    },


    __ellipsis : null,
    get _ellipsis() {
        if (!this.__ellipsis) {
            this.__ellipsis = "\u2026";
            try {
                this.__ellipsis = Services.prefs.getComplexValue(
                                    "intl.ellipsis", Ci.nsIPrefLocalizedString).data;
            } catch (e) { }
        }
        return this.__ellipsis;
    },


    




    log : function (message) {
        if (!this._debug)
            return;

        dump("Pwmgr Prompter: " + message + "\n");
        Services.console.logStringMessage("Pwmgr Prompter: " + message);
    },


    




    



    init : function (aWindow, aFactory) {
        this._window = aWindow;
        this._factory = aFactory || null;

        var prefBranch = Services.prefs.getBranch("signon.");
        this._debug = prefBranch.getBoolPref("debug");
        this.log("===== initialized =====");
    },


    setE10sData : function (aBrowser) {
      
      throw new Error("Not Yet Implemented");
    },


    



    promptToSavePassword : function (aLogin) {
        var notifyBox = this._getNotifyBox();
        if (notifyBox)
            this._showSaveLoginNotification(notifyBox, aLogin);
    },


    





    _showLoginNotification : function (aNotifyBox, aName, aText, aButtons) {
        var oldBar = aNotifyBox.getNotificationWithValue(aName);
        const priority = aNotifyBox.PRIORITY_INFO_MEDIUM;

        this.log("Adding new " + aName + " notification bar");
        var newBar = aNotifyBox.appendNotification(
                                aText, aName,
                                "chrome://browser/skin/images/infobar-key.png",
                                priority, aButtons);

        
        
        newBar.persistence++;

        
        
        
        
        newBar.timeout = Date.now() + 20000; 

        if (oldBar) {
            this.log("(...and removing old " + aName + " notification bar)");
            aNotifyBox.removeNotification(oldBar);
        }
    },


    







    _showSaveLoginNotification : function (aNotifyBox, aLogin) {
        
        
        
        
        var neverButtonText =
              this._getLocalizedString("notifyBarNotForThisSiteButtonText");
        var neverButtonAccessKey =
              this._getLocalizedString("notifyBarNotForThisSiteButtonAccessKey");
        var rememberButtonText =
              this._getLocalizedString("notifyBarRememberPasswordButtonText");
        var rememberButtonAccessKey =
              this._getLocalizedString("notifyBarRememberPasswordButtonAccessKey");

        var brandShortName =
              this._brandBundle.GetStringFromName("brandShortName");
        var displayHost = this._getShortDisplayHost(aLogin.hostname);
        var notificationText;
        if (aLogin.username) {
            var displayUser = this._sanitizeUsername(aLogin.username);
            notificationText  = this._getLocalizedString(
                                        "saveLoginText",
                                        [brandShortName, displayUser, displayHost]);
        } else {
            notificationText  = this._getLocalizedString(
                                        "saveLoginTextNoUsername",
                                        [brandShortName, displayHost]);
        }

        
        
        
        var pwmgr = this._pwmgr;


        var buttons = [
            
            {
                label:     rememberButtonText,
                accessKey: rememberButtonAccessKey,
                popup:     null,
                callback: function(aNotificationBar, aButton) {
                    pwmgr.addLogin(aLogin);
                }
            },

            
            {
                label:     neverButtonText,
                accessKey: neverButtonAccessKey,
                popup:     null,
                callback: function(aNotificationBar, aButton) {
                    pwmgr.setLoginSavingEnabled(aLogin.hostname, false);
                }
            }
        ];

        this._showLoginNotification(aNotifyBox, "password-save",
             notificationText, buttons);
    },


    







    promptToChangePassword : function (aOldLogin, aNewLogin) {
        var notifyBox = this._getNotifyBox();
        if (notifyBox)
            this._showChangeLoginNotification(notifyBox, aOldLogin, aNewLogin.password);
    },

    





    _showChangeLoginNotification : function (aNotifyBox, aOldLogin, aNewPassword) {
        var notificationText;
        if (aOldLogin.username)
            notificationText  = this._getLocalizedString(
                                          "passwordChangeText",
                                          [aOldLogin.username]);
        else
            notificationText  = this._getLocalizedString(
                                          "passwordChangeTextNoUser");

        var changeButtonText =
              this._getLocalizedString("notifyBarChangeButtonText");
        var changeButtonAccessKey =
              this._getLocalizedString("notifyBarChangeButtonAccessKey");
        var dontChangeButtonText =
              this._getLocalizedString("notifyBarDontChangeButtonText2");
        var dontChangeButtonAccessKey =
              this._getLocalizedString("notifyBarDontChangeButtonAccessKey");

        
        
        
        var self = this;

        var buttons = [
            
            {
                label:     changeButtonText,
                accessKey: changeButtonAccessKey,
                popup:     null,
                callback:  function(aNotificationBar, aButton) {
                    self._updateLogin(aOldLogin, aNewPassword);
                }
            },

            
            {
                label:     dontChangeButtonText,
                accessKey: dontChangeButtonAccessKey,
                popup:     null,
                callback:  function(aNotificationBar, aButton) {
                    
                }
            }
        ];

        this._showLoginNotification(aNotifyBox, "password-change",
             notificationText, buttons);
    },

    












    promptToChangePasswordWithUsernames : function (logins, count, aNewLogin) {
        const buttonFlags = Ci.nsIPrompt.STD_YES_NO_BUTTONS;

        var usernames = logins.map(function (l) l.username);
        var dialogText  = this._getLocalizedString("userSelectText");
        var dialogTitle = this._getLocalizedString("passwordChangeTitle");
        var selectedIndex = { value: null };

        
        
        var ok = this._promptService.select(null,
                                dialogTitle, dialogText,
                                usernames.length, usernames,
                                selectedIndex);
        if (ok) {
            
            var selectedLogin = logins[selectedIndex.value];
            this.log("Updating password for user " + selectedLogin.username);
            this._updateLogin(selectedLogin, aNewLogin.password);
        }
    },


    

    


    _updateLogin : function (login, newPassword) {
        var now = Date.now();
        var propBag = Cc["@mozilla.org/hash-property-bag;1"].
                      createInstance(Ci.nsIWritablePropertyBag);
        if (newPassword) {
            propBag.setProperty("password", newPassword);
            
            
            
            propBag.setProperty("timePasswordChanged", now);
        }
        propBag.setProperty("timeLastUsed", now);
        propBag.setProperty("timesUsedIncrement", 1);
        this._pwmgr.modifyLogin(login, propBag);
    },

    


    _getNotifyWindow: function () {
        try {
            
            var notifyWin = this._window.top;

            
            
            
            if (notifyWin.opener) {
                var chromeDoc = this._getChromeWindow(notifyWin).
                                     document.documentElement;
                var webnav = notifyWin.
                             QueryInterface(Ci.nsIInterfaceRequestor).
                             getInterface(Ci.nsIWebNavigation);

                
                
                
                
                if (chromeDoc.getAttribute("chromehidden") &&
                    webnav.sessionHistory.count == 1) {
                    this.log("Using opener window for notification bar.");
                    notifyWin = notifyWin.opener;
                }
            }

            return notifyWin;

        } catch (e) {
            
            this.log("Unable to get notify window");
            return null;
        }
    },

    




    _getChromeWindow: function (aWindow) {
        var chromeWin = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell)
                               .chromeEventHandler.ownerDocument.defaultView;
        return chromeWin;
    },

    





    _getNotifyBox : function () {
        let notifyBox = null;

        try {
            let notifyWin = this._getNotifyWindow();
            let windowID = notifyWin.QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;

            
            
            let chromeWin = this._getChromeWindow(notifyWin).wrappedJSObject;
            let browser = chromeWin.Browser.getBrowserForWindowId(windowID);

            notifyBox = chromeWin.getNotificationBox(browser);
        } catch (e) {
            Cu.reportError(e);
        }

        return notifyBox;
    },

    











 
    _getLocalizedString : function (key, formatArgs) {
        if (formatArgs)
            return this._strBundle.formatStringFromName(
                                        key, formatArgs, formatArgs.length);
        else
            return this._strBundle.GetStringFromName(key);
    },


    






    _sanitizeUsername : function (username) {
        if (username.length > 30) {
            username = username.substring(0, 30);
            username += this._ellipsis;
        }
        return username.replace(/['"]/g, "");
    },


    






    _getShortDisplayHost: function (aURIString) {
        var displayHost;

        var eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"].
                          getService(Ci.nsIEffectiveTLDService);
        var idnService = Cc["@mozilla.org/network/idn-service;1"].
                         getService(Ci.nsIIDNService);
        try {
            var uri = Services.io.newURI(aURIString, null, null);
            var baseDomain = eTLDService.getBaseDomain(uri);
            displayHost = idnService.convertToDisplayIDN(baseDomain, {});
        } catch (e) {
            this.log("_getShortDisplayHost couldn't process " + aURIString);
        }

        if (!displayHost)
            displayHost = aURIString;

        return displayHost;
    },

}; 


var component = [LoginManagerPrompter];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);

