





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

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
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIAuthPrompt,
                                            Ci.nsIAuthPrompt2,
                                            Ci.nsILoginManagerPrompter]),

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


    __ioService: null, 
    get _ioService() {
        if (!this.__ioService)
            this.__ioService = Cc["@mozilla.org/network/io-service;1"].
                               getService(Ci.nsIIOService);
        return this.__ioService;
    },


    




    log : function (message) {
        if (!this._debug)
            return;

        dump("Pwmgr Prompter: " + message + "\n");
        this._logService.logStringMessage("Pwmgr Prompter: " + message);
    },




    


    





    prompt : function (aDialogTitle, aText, aPasswordRealm,
                       aSavePassword, aDefaultText, aResult) {
        if (aSavePassword != Ci.nsIAuthPrompt.SAVE_PASSWORD_NEVER)
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        this.log("===== prompt() called =====");

        if (aDefaultText) {
            aResult.value = aDefaultText;
        }

        return this._promptService.prompt(this._window,
               aDialogTitle, aText, aResult, null, {});
    },


    





    promptUsernameAndPassword : function (aDialogTitle, aText, aPasswordRealm,
                                         aSavePassword, aUsername, aPassword) {
        if (aSavePassword == Ci.nsIAuthPrompt.SAVE_PASSWORD_FOR_SESSION)
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        var selectedLogin = null;
        var foundLogins;
        var checkBox = { value : false };
        var checkBoxLabel = null;
        var hostname = this._getFormattedHostname(aPasswordRealm);

        this.log("===== promptUsernameAndPassword() called =====");

        var canRememberLogin = (aSavePassword ==
                                Ci.nsIAuthPrompt.SAVE_PASSWORD_PERMANENTLY) &&
                               this._pwmgr.getLoginSavingEnabled(hostname);

        
        if (canRememberLogin)
            checkBoxLabel = this._getLocalizedString("rememberPassword");

        if (!aUsername.value && !aPassword.value) {
            
            foundLogins = this._pwmgr.findLogins({}, hostname, null,
                                                 aPasswordRealm);

            
            
            if (foundLogins.length > 0) {
                selectedLogin = foundLogins[0];
                
                
                aUsername.value = foundLogins[0].username;
                aPassword.value = foundLogins[0].password;
            }
        }

        var ok = this._promptService.promptUsernameAndPassword(this._window,
                    aDialogTitle, aText, aUsername, aPassword,
                    checkBoxLabel, checkBox);

        if (ok && checkBox.value) {
            var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                           createInstance(Ci.nsILoginInfo);
            newLogin.init(hostname, null, aPasswordRealm,
                          aUsername.value, aPassword.value,
                          "", "");

            
            
            if (!selectedLogin || username != selectedLogin.username) {
                
                this.log("New login seen for " + aPasswordRealm);

                this._pwmgr.addLogin(newLogin);
            } else if (selectedLogin &&
                       password != selectedLogin.password) {
                
                this.log("Updating password for  " + aPasswordRealm);
                this._pwmgr.modifyLogin(foundLogins[0], newLogin);
            } else {
                this.log("Login unchanged, no further action needed.");
                return ok;
            }
        }

        return ok;
    },


    









    promptPassword : function (aDialogTitle, aText, aPasswordRealm,
                                         aSavePassword, aPassword) {
        if (aSavePassword == Ci.nsIAuthPrompt.SAVE_PASSWORD_FOR_SESSION)
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        var checkBox = { value : false };
        var checkBoxLabel = null;
        var [hostname, username, pathname] = this._decomposeURI(aPasswordRealm);
        var newRealm = hostname + pathname;

        this.log("===== promptPassword called() =====");

        var canRememberLogin = (aSavePassword ==
                                Ci.nsIAuthPrompt.SAVE_PASSWORD_PERMANENTLY) &&
                               this._pwmgr.getLoginSavingEnabled(hostname);

        
        if (canRememberLogin)
            checkBoxLabel = this._getLocalizedString("rememberPassword");

        if (!aPassword.value) {
            
            var foundLogins = this._pwmgr.findLogins({}, hostname, null,

                                                     newRealm);

            var i;
            
            
            
            
            for (i = 0; i < foundLogins.length; ++i) {
                if (foundLogins[i].username == username) {
                    aPassword.value = foundLogins[i].password;
                    
                    return true;
                }
            }
        }

        var ok = this._promptService.promptPassword(this._window, aDialogTitle,
                                                    aText, aPassword,
                                                    checkBoxLabel, checkBox);

        if (ok && checkBox.value) {
            var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                           createInstance(Ci.nsILoginInfo);
            newLogin.init(hostname, null, newRealm, username,
                          aPassword.value, "", "");

            this.log("New login seen for " + newRealm);

            this._pwmgr.addLogin(newLogin);
        }

        return ok;
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
            var [username, password] = this._GetAuthInfo(aAuthInfo);

            
            
            
            
            var rememberLogin = notifyBox ? canRememberLogin : checkbox.value;

            if (ok && rememberLogin) {
                var newLogin = Cc["@mozilla.org/login-manager/loginInfo;1"].
                               createInstance(Ci.nsILoginInfo);
                newLogin.init(hostname, null, httpRealm,
                              username, password, "", "");

                
                
                if (!selectedLogin || username != selectedLogin.username) {

                    
                    this.log("New login seen for " + username +
                             " @ " + hostname + " (" + httpRealm + ")");
                    if (notifyBox)
                        this._showSaveLoginNotification(notifyBox, newLogin);
                    else
                        this._pwmgr.addLogin(newLogin);

                } else if (selectedLogin &&
                           password != selectedLogin.password) {

                    this.log("Updating password for " + username +
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


    





    _showLoginNotification : function (aNotifyBox, aName, aText, aButtons) {
        var oldBar = aNotifyBox.getNotificationWithValue(aName);
        const priority = aNotifyBox.PRIORITY_INFO_MEDIUM;

        this.log("Adding new " + aName + " notification bar");
        var newBar = aNotifyBox.appendNotification(
                                aText, aName,
                                "chrome://mozapps/skin/passwordmgr/key.png",
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
              this._getLocalizedString("notifyBarNeverForSiteButtonText");
        var neverButtonAccessKey =
              this._getLocalizedString("notifyBarNeverForSiteButtonAccessKey");
        var rememberButtonText =
              this._getLocalizedString("notifyBarRememberButtonText");
        var rememberButtonAccessKey =
              this._getLocalizedString("notifyBarRememberButtonAccessKey");
        var notNowButtonText =
              this._getLocalizedString("notifyBarNotNowButtonText");
        var notNowButtonAccessKey =
              this._getLocalizedString("notifyBarNotNowButtonAccessKey");

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
            },

            
            {
                label:     notNowButtonText,
                accessKey: notNowButtonAccessKey,
                popup:     null,
                callback:  function() {  } 
            }
        ];

        this._showLoginNotification(aNotifyBox, "password-save",
             notificationText, buttons);
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
        var notifyBox = this._getNotifyBox();

        if (notifyBox)
            this._showChangeLoginNotification(notifyBox, aOldLogin, aNewLogin);
        else
            this._showChangeLoginDialog(aOldLogin, aNewLogin);
    },


    





    _showChangeLoginNotification : function (aNotifyBox, aOldLogin, aNewLogin) {
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
              this._getLocalizedString("notifyBarDontChangeButtonText");
        var dontChangeButtonAccessKey =
              this._getLocalizedString("notifyBarDontChangeButtonAccessKey");

        
        
        
        var pwmgr = this._pwmgr;

        var buttons = [
            
            {
                label:     changeButtonText,
                accessKey: changeButtonAccessKey,
                popup:     null,
                callback:  function(aNotificationBar, aButton) {
                    pwmgr.modifyLogin(aOldLogin, aNewLogin);
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


    





    _showChangeLoginDialog : function (aOldLogin, aNewLogin) {
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
                var webnav = notifyWindow
                                    .QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIWebNavigation);
                var chromeWin = webnav
                                    .QueryInterface(Ci.nsIDocShellTreeItem)
                                    .rootTreeItem
                                    .QueryInterface(Ci.nsIInterfaceRequestor)
                                    .getInterface(Ci.nsIDOMWindow);
                var chromeDoc = chromeWin.document.documentElement;

                
                
                
                
                if (chromeDoc.getAttribute("chromehidden") &&
                    webnav.sessionHistory.count == 1) {
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
        var uri;
        if (aURI instanceof Ci.nsIURI) {
            uri = aURI;
        } else {
            uri = this._ioService.newURI(aURI, null, null);
        }
        var scheme = uri.scheme;

        var hostname = scheme + "://" + uri.host;

        
        
        port = uri.port;
        if (port != -1) {
            var handler = this._ioService.getProtocolHandler(scheme);
            if (port != handler.defaultPort)
                hostname += ":" + port;
        }

        return hostname;
    },

    



    _decomposeURI: function (aURIString) {
        var uri = this._ioService.newURI(aURIString, null, null);
        var pathname = "";

        if (uri.path != "/")
            pathname = uri.path;

        return [this._getFormattedHostname(uri), uri.username, pathname];
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


    






    _GetAuthInfo : function (aAuthInfo) {
        var username, password;

        var flags = aAuthInfo.flags;
        if (flags & Ci.nsIAuthInformation.NEED_DOMAIN && aAuthInfo.domain)
            username = aAuthInfo.domain + "\\" + aAuthInfo.username;
        else
            username = aAuthInfo.username;

        password = aAuthInfo.password;

        return [username, password];
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
