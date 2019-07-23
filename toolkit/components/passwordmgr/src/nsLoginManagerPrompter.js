




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");









function LoginManagerPromptFactory() {}

LoginManagerPromptFactory.prototype = {

    classDescription : "LoginManagerPromptFactory",
    contractID : "@mozilla.org/passwordmanager/authpromptfactory;1",
    classID : Components.ID("{749e62f4-60ae-4569-a8a2-de78b649660e}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIPromptFactory]),

    getPrompt : function (aWindow, aIID) {
        var prompt = new LoginManagerPrompter().QueryInterface(aIID);
        prompt.init(aWindow);
        return prompt;
    }
}; 

















function LoginManagerPrompter() {}

LoginManagerPrompter.prototype = {

    classDescription : "LoginManagerPrompter",
    contractID : "@mozilla.org/login-manager/prompter;1",
    classID : Components.ID("{8aa66d77-1bbb-45a6-991e-b8f47751c291}"),
    QueryInterface : XPCOMUtils.generateQI(
                        [Ci.nsIAuthPrompt2, Ci.nsILoginManagerPrompter]),

    _window        : null,
    _debug         : false, 

    __pwmgr : null, 
    get _pwmgr() {
        if (!this.__pwmgr)
            this.__pwmgr = Cc["@mozilla.org/login-manager;1"].
                           getService(Ci.nsILoginManager);
        return this.__pwmgr;
    },

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"].
                                getService(Ci.nsIConsoleService);
        return this.__logService;
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


    




    log : function (message) {
        if (!this._debug)
            return;

        dump("Pwmgr Prompter: " + message + "\n");
        this._logService.logStringMessage("Pwmgr Prompter: " + message);
    },




    




    








    promptAuth : function (aChannel, aLevel, aAuthInfo) {
        var selectedLogin = null;
        var checkbox = { value : false };
        var checkboxLabel = null;
        var epicfail = false;

        try {

            this.log("===== promptAuth called =====");

            
            
            
            var notifyBox = this._getNotifyBox();
            if (notifyBox)
                this._removeSaveLoginNotification(notifyBox);

            var [hostname, httpRealm] = this._getAuthTarget(aChannel, aAuthInfo);


            
            var foundLogins = this._pwmgr.findLogins({},
                                        hostname, null, httpRealm);

            
            if (foundLogins.length > 0) {
                selectedLogin = foundLogins[0];
                this._SetAuthInfo(aAuthInfo, selectedLogin.username,
                                             selectedLogin.password);
                checkbox.value = true;
            }

            var canRememberLogin = this._pwmgr.getLoginSavingEnabled(hostname);
        
            
            if (canRememberLogin && !notifyBox)
                checkboxLabel = this._getLocalizedString("rememberPassword");
        } catch (e) {
            
            epicfail = true;
            Components.utils.reportError("LoginManagerPrompter: " +
                "Epic fail in promptAuth: " + e + "\n");
        }

        var ok = this._promptService.promptAuth(this._window, aChannel,
                                aLevel, aAuthInfo, checkboxLabel, checkbox);
        if (epicfail)
            return ok;

        try {
            
            
            
            
            var rememberLogin = notifyBox ? canRememberLogin : checkbox.value;

            if (ok && rememberLogin) {
                var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                               createInstance(Ci.nsILoginInfo);
                newLogin.init(hostname, null, httpRealm,
                              aAuthInfo.username, aAuthInfo.password,
                              "", "");

                
                
                if (!selectedLogin ||
                    aAuthInfo.username != selectedLogin.username) {

                    
                    this.log("New login seen for " + aAuthInfo.username +
                             " @ " + hostname + " (" + httpRealm + ")");
                    if (notifyBox)
                        this._showSaveLoginNotification(notifyBox, newLogin);
                    else
                        this._pwmgr.addLogin(newLogin);

                } else if (selectedLogin &&
                           aAuthInfo.password != selectedLogin.password) {

                    this.log("Updating password for " + aAuthInfo.username +
                             " @ " + hostname + " (" + httpRealm + ")");
                    
                    this._pwmgr.modifyLogin(foundLogins[0], newLogin);

                } else {
                    this.log("Login unchanged, no further action needed.");
                    return ok;
                }
            }
        } catch (e) {
            Components.utils.reportError("LoginManagerPrompter: " +
                "Fail2 in promptAuth: " + e + "\n");
        }

        return ok;
    },

    asyncPromptAuth : function () {
        return NS_ERROR_NOT_IMPLEMENTED;
    },




    




    



    init : function (aWindow) {
        this._window = aWindow;

        var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                         getService(Ci.nsIPrefService).getBranch("signon.");
        this._debug = prefBranch.getBoolPref("debug");
        this.log("===== initialized =====");
    },


    



    promptToSavePassword : function (aLogin) {
        var notifyBox = this._getNotifyBox();

        if (notifyBox)
            this._showSaveLoginNotification(notifyBox, aLogin);
        else
            this._showSaveLoginDialog(aLogin);
    },


    







    _showSaveLoginNotification : function (aNotifyBox, aLogin) {

        
        
        
        
        var neverButtonText =
              this._getLocalizedString("notifyBarNeverForSiteButtonText");
        var neverButtonAccessKey =
              this._getLocalizedString("notifyBarNeverForSiteButtonAccessKey");
        var rememberButtonText =
              this._getLocalizedString("notifyBarRememberButtonText");
        var rememberButtonAccessKey =
              this._getLocalizedString("notifyBarRememberButtonAccessKey");

        var brandShortName =
              this._brandBundle.GetStringFromName("brandShortName");
        var notificationText  = this._getLocalizedString(
                                        "savePasswordText", [brandShortName]);

        
        
        
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


        var oldBar = aNotifyBox.getNotificationWithValue("password-save");
        const priority = aNotifyBox.PRIORITY_INFO_MEDIUM;

        this.log("Adding new save-password notification bar");
        var newBar = aNotifyBox.appendNotification(
                                notificationText, "password-save",
                                null, priority, buttons);

        
        
        newBar.persistence++;

        
        
        
        
        newBar.timeout = Date.now() + 20000; 

        if (oldBar) {
            this.log("(...and removing old save-password notification bar)");
            aNotifyBox.removeNotification(oldBar);
        }
    },


    



    _removeSaveLoginNotification : function (aNotifyBox) {

        var oldBar = aNotifyBox.getNotificationWithValue("password-save");

        if (oldBar) {
            this.log("Removing save-password notification bar.");
            aNotifyBox.removeNotification(oldBar);
        }
    },


    






    _showSaveLoginDialog : function (aLogin) {
        const buttonFlags = Ci.nsIPrompt.BUTTON_POS_1_DEFAULT +
            (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
            (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1) +
            (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_2);

        var brandShortName =
                this._brandBundle.GetStringFromName("brandShortName");

        var dialogText         = this._getLocalizedString(
                                        "savePasswordText", [brandShortName]);
        var dialogTitle        = this._getLocalizedString(
                                        "savePasswordTitle");
        var neverButtonText    = this._getLocalizedString(
                                        "neverForSiteButtonText");
        var rememberButtonText = this._getLocalizedString(
                                        "rememberButtonText");
        var notNowButtonText   = this._getLocalizedString(
                                        "notNowButtonText");

        this.log("Prompting user to save/ignore login");
        var userChoice = this._promptService.confirmEx(this._window,
                                            dialogTitle, dialogText,
                                            buttonFlags, rememberButtonText,
                                            notNowButtonText, neverButtonText,
                                            null, {});
        
        
        
        
        if (userChoice == 2) {
            this.log("Disabling " + aLogin.hostname + " logins by request.");
            this._pwmgr.setLoginSavingEnabled(aLogin.hostname, false);
        } else if (userChoice == 0) {
            this.log("Saving login for " + aLogin.hostname);
            this._pwmgr.addLogin(aLogin);
        } else {
            
            this.log("Ignoring login.");
        }
    },


    







    promptToChangePassword : function (aOldLogin, aNewLogin) {
        const buttonFlags = Ci.nsIPrompt.STD_YES_NO_BUTTONS;

        var dialogText;
        if (aOldLogin.username)
            dialogText  = this._getLocalizedString(
                                    "passwordChangeText",
                                    [aOldLogin.username]);
        else
            dialogText  = this._getLocalizedString(
                                    "passwordChangeTextNoUser");

        var dialogTitle = this._getLocalizedString(
                                    "passwordChangeTitle");

        
        var ok = !this._promptService.confirmEx(this._window,
                                dialogTitle, dialogText, buttonFlags,
                                null, null, null,
                                null, {});
        if (ok) {
            this.log("Updating password for user " + aOldLogin.username);
            this._pwmgr.modifyLogin(aOldLogin, aNewLogin);
        }
    },


    












    promptToChangePasswordWithUsernames : function (logins, count, aNewLogin) {
        const buttonFlags = Ci.nsIPrompt.STD_YES_NO_BUTTONS;

        var usernames = logins.map(function (l) l.username);
        var dialogText  = this._getLocalizedString("userSelectText");
        var dialogTitle = this._getLocalizedString("passwordChangeTitle");
        var selectedIndex = { value: null };

        
        
        var ok = this._promptService.select(this._window,
                                dialogTitle, dialogText,
                                usernames.length, usernames,
                                selectedIndex);
        if (ok) {
            
            

            var selectedLogin = logins[selectedIndex.value];

            this.log("Updating password for user " + selectedLogin.username);

            aNewLogin.username      = selectedLogin.username;
            aNewLogin.usernameField = selectedLogin.usernameField;

            this._pwmgr.modifyLogin(selectedLogin, aNewLogin);
        }
    },




    




    





    _getNotifyBox : function () {
        try {
            
            var notifyWindow = this._window.top

            
            
            
            if (notifyWindow.opener) {
                var chromeWin = notifyWindow
                                    .QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIWebNavigation)
                                    .QueryInterface(Ci.nsIDocShellTreeItem)
                                    .rootTreeItem
                                    .QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIDOMWindow);
                var chromeDoc = chromeWin.document.documentElement;

                
                
                if (chromeDoc.getAttribute("chromehidden")) {
                    this.log("Using opener window for notification bar.");
                    notifyWindow = notifyWindow.opener;
                }
            }


            
            
            var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
                     getService(Ci.nsIWindowMediator);
            var enumerator = wm.getEnumerator("navigator:browser");
            var tabbrowser = null;
            var foundBrowser = null;

            while (!foundBrowser && enumerator.hasMoreElements()) {
                var win = enumerator.getNext();
                tabbrowser = win.getBrowser(); 
                foundBrowser = tabbrowser.getBrowserForDocument(
                                                  notifyWindow.document);
            }

            
            if (foundBrowser)
                return tabbrowser.getNotificationBox(foundBrowser)

        } catch (e) {
            
            this.log("No notification box available: " + e)
        }

        return null;
    },


    











 
    _getLocalizedString : function (key, formatArgs) {
        if (formatArgs)
            return this._strBundle.formatStringFromName(
                                        key, formatArgs, formatArgs.length);
        else
            return this._strBundle.GetStringFromName(key);
    },


    





    _getFormattedHostname : function (aURI) {
        var scheme = aURI.scheme;

        var hostname = scheme + "://" + aURI.host;

        
        
        port = aURI.port;
        if (port != -1) {
            var ioService = Cc["@mozilla.org/network/io-service;1"].
                            getService(Ci.nsIIOService);
            var handler = ioService.getProtocolHandler(scheme);
            if (port != handler.defaultPort)
                hostname += ":" + port;
        }

        return hostname;
    },


    





    _getAuthTarget : function (aChannel, aAuthInfo) {
        var hostname, realm;

        
        
        if (aAuthInfo.flags & Ci.nsIAuthInformation.AUTH_PROXY) {
            this.log("getAuthTarget is for proxy auth");
            if (!(aChannel instanceof Ci.nsIProxiedChannel))
                throw "proxy auth needs nsIProxiedChannel";

            var info = aChannel.proxyInfo;
            if (!info)
                throw "proxy auth needs nsIProxyInfo";

            
            
            var idnService = Cc["@mozilla.org/network/idn-service;1"].
                             getService(Ci.nsIIDNService);
            hostname = "moz-proxy://" +
                        idnService.convertUTF8toACE(info.host) +
                        ":" + info.port;
            realm = aAuthInfo.realm;
            if (!realm)
                realm = hostname;

            return [hostname, realm];
        }

        hostname = this._getFormattedHostname(aChannel.URI);

        
        
        
        realm = aAuthInfo.realm;
        if (!realm)
            realm = hostname;

        return [hostname, realm];
    },

    




    _SetAuthInfo : function (aAuthInfo, username, password) {
        var flags = aAuthInfo.flags;
        if (flags & Ci.nsIAuthInformation.NEED_DOMAIN) {
            
            var idx = username.indexOf("\\");
            if (idx == -1) {
                aAuthInfo.username = username;
            } else {
                aAuthInfo.domain   =  username.substring(0, idx);
                aAuthInfo.username =  username.substring(idx+1);
            }
        } else {
            aAuthInfo.username = username;
        }
        aAuthInfo.password = password;
    }

}; 

var component = [LoginManagerPromptFactory, LoginManagerPrompter];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}


