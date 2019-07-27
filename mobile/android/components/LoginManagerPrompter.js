




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
    _window       : null,
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
                        "chrome://passwordmgr/locale/passwordmgr.properties");
            if (!this.__strBundle)
                throw "String bundle for Login Manager not present!";
        }

        return this.__strBundle;
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
        throw new Error("This should be filled in when Android is multiprocess");
    },

    



    promptToSavePassword : function (aLogin) {
        this._showSaveLoginNotification(aLogin);
    },


    





    _showLoginNotification : function (aName, aText, aButtons) {
        this.log("Adding new " + aName + " notification bar");
        let notifyWin = this._window.top;
        let chromeWin = this._getChromeWindow(notifyWin).wrappedJSObject;
        let browser = chromeWin.BrowserApp.getBrowserForWindow(notifyWin);
        let tabID = chromeWin.BrowserApp.getTabForBrowser(browser).id;

        
        

        
        
        
        

        let options = {
            persistWhileVisible: true,
            timeout: Date.now() + 10000
        }

        var nativeWindow = this._getNativeWindow();
        if (nativeWindow)
            nativeWindow.doorhanger.show(aText, aName, aButtons, tabID, options);
    },


    







    _showSaveLoginNotification : function (aLogin) {
        var displayHost = this._getShortDisplayHost(aLogin.hostname);
        var notificationText;
        if (aLogin.username) {
            var displayUser = this._sanitizeUsername(aLogin.username);
            notificationText  = this._getLocalizedString("savePassword", [displayUser, displayHost]);
        } else {
            notificationText  = this._getLocalizedString("savePasswordNoUser", [displayHost]);
        }

        
        
        
        var pwmgr = this._pwmgr;

        var buttons = [
            {
                label: this._getLocalizedString("saveButton"),
                callback: function() {
                    pwmgr.addLogin(aLogin);
                }
            },
            {
                label: this._getLocalizedString("dontSaveButton"),
                callback: function() {
                    
                }
            }
        ];

        this._showLoginNotification("password-save", notificationText, buttons);
    },

    







    promptToChangePassword : function (aOldLogin, aNewLogin) {
        this._showChangeLoginNotification(aOldLogin, aNewLogin.password);
    },

    





    _showChangeLoginNotification : function (aOldLogin, aNewPassword) {
        var notificationText;
        if (aOldLogin.username) {
            let displayUser = this._sanitizeUsername(aOldLogin.username);
            notificationText  = this._getLocalizedString("updatePassword", [displayUser]);
        } else {
            notificationText  = this._getLocalizedString("updatePasswordNoUser");
        }

        
        
        
        var self = this;

        var buttons = [
            {
                label: this._getLocalizedString("updateButton"),
                callback:  function() {
                    self._updateLogin(aOldLogin, aNewPassword);
                }
            },
            {
                label: this._getLocalizedString("dontUpdateButton"),
                callback:  function() {
                    
                }
            }
        ];

        this._showLoginNotification("password-change", notificationText, buttons);
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

    




    _getChromeWindow: function (aWindow) {
        var chromeWin = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell)
                               .chromeEventHandler.ownerDocument.defaultView;
        return chromeWin;
    },

    





    _getNativeWindow : function () {
        let nativeWindow = null;
        try {
            let notifyWin = this._window.top;
            let chromeWin = this._getChromeWindow(notifyWin).wrappedJSObject;
            if (chromeWin.NativeWindow) {
                nativeWindow = chromeWin.NativeWindow;
            } else {
                Cu.reportError("NativeWindow not available on window");
            }

        } catch (e) {
            
            Cu.reportError("No NativeWindow available: " + e);
        }
        return nativeWindow;
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


    







    _getFormattedHostname : function (aURI) {
        var uri;
        if (aURI instanceof Ci.nsIURI) {
            uri = aURI;
        } else {
            uri = Services.io.newURI(aURI, null, null);
        }
        var scheme = uri.scheme;

        var hostname = scheme + "://" + uri.host;

        
        
        port = uri.port;
        if (port != -1) {
            var handler = Services.io.getProtocolHandler(scheme);
            if (port != handler.defaultPort)
                hostname += ":" + port;
        }

        return hostname;
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

