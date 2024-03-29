



"use strict";

this.EXPORTED_SYMBOLS = ["SignInToWebsiteUX"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "IdentityService",
                                  "resource://gre/modules/identity/Identity.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["SignInToWebsiteUX"].concat(aMessageArgs));
}

this.SignInToWebsiteUX = {

  init: function SignInToWebsiteUX_init() {

    Services.obs.addObserver(this, "identity-request", false);
    Services.obs.addObserver(this, "identity-auth", false);
    Services.obs.addObserver(this, "identity-auth-complete", false);
    Services.obs.addObserver(this, "identity-login-state-changed", false);
  },

  uninit: function SignInToWebsiteUX_uninit() {
    Services.obs.removeObserver(this, "identity-request");
    Services.obs.removeObserver(this, "identity-auth");
    Services.obs.removeObserver(this, "identity-auth-complete");
    Services.obs.removeObserver(this, "identity-login-state-changed");
  },

  observe: function SignInToWebsiteUX_observe(aSubject, aTopic, aData) {
    log("observe: received", aTopic, "with", aData, "for", aSubject);
    let options = null;
    if (aSubject) {
      options = aSubject.wrappedJSObject;
    }
    switch(aTopic) {
      case "identity-request":
        this.requestLogin(options);
        break;
      case "identity-auth":
        this._openAuthenticationUI(aData, options);
        break;
      case "identity-auth-complete":
        this._closeAuthenticationUI(aData);
        break;
      case "identity-login-state-changed":
        let emailAddress = aData;
        if (emailAddress) {
          this._removeRequestUI(options);
          this._showLoggedInUI(emailAddress, options);
        } else {
          this._removeLoggedInUI(options);
        }
        break;
      default:
        Logger.reportError("SignInToWebsiteUX", "Unknown observer notification:", aTopic);
        break;
    }
  },

  


  requestLogin: function SignInToWebsiteUX_requestLogin(aOptions) {
    let windowID = aOptions.rpId;
    log("requestLogin", aOptions);
    let [chromeWin, browserEl] = this._getUIForWindowID(windowID);

    
    let message = aOptions.origin;
    let mainAction = {
      label: chromeWin.gNavigatorBundle.getString("identity.next.label"),
      accessKey: chromeWin.gNavigatorBundle.getString("identity.next.accessKey"),
      callback: function() {}, 
    };
    let options = {
      identity: {
        origin: aOptions.origin,
      },
    };
    let secondaryActions = [];

    
    for (let opt in aOptions) {
      options.identity[opt] = aOptions[opt];
    }
    log("requestLogin: rpId: ", options.identity.rpId);

    chromeWin.PopupNotifications.show(browserEl, "identity-request", message,
                                      "identity-notification-icon", mainAction,
                                      [], options);
  },

  


  getIdentitiesForSite: function SignInToWebsiteUX_getIdentitiesForSite(aOrigin) {
    return IdentityService.RP.getIdentitiesForSite(aOrigin);
  },

  


  selectIdentity: function SignInToWebsiteUX_selectIdentity(aRpId, aIdentity) {
    log("selectIdentity: rpId: ", aRpId, " identity: ", aIdentity);
    IdentityService.selectIdentity(aRpId, aIdentity);
  },

  

  


  _getUIForWindowID: function(aWindowID) {
    let content = Services.wm.getOuterWindowWithId(aWindowID);
    if (content) {
      let browser = content.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell).chromeEventHandler;
      let chromeWin = browser.ownerDocument.defaultView;
      return [chromeWin, browser];
    }

    Logger.reportError("SignInToWebsiteUX", "no content");
    return [null, null];
  },

  




  _openAuthenticationUI: function _openAuthenticationUI(aAuthURI, aContext) {
    
    let chromeWin = Services.wm.getMostRecentWindow('navigator:browser');
    let features = "chrome=false,width=640,height=480,centerscreen,location=yes,resizable=yes,scrollbars=yes,status=yes";
    log("aAuthURI: ", aAuthURI);
    let authWin = Services.ww.openWindow(chromeWin, "about:blank", "", features, null);
    let windowID = authWin.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
    log("authWin outer id: ", windowID);

    let provId = aContext.provId;
    
    IdentityService.IDP.setAuthenticationFlow(windowID, provId);

    authWin.location = aAuthURI;
  },

  _closeAuthenticationUI: function _closeAuthenticationUI(aAuthId) {
    log("_closeAuthenticationUI:", aAuthId);
    let [chromeWin, browserEl] = this._getUIForWindowID(aAuthId);
    if (chromeWin)
      chromeWin.close();
    else
      Logger.reportError("SignInToWebsite", "Could not close window with ID", aAuthId);
  },

  


  _showLoggedInUI: function _showLoggedInUI(aIdentity, aContext) {
    let windowID = aContext.rpId;
    log("_showLoggedInUI for ", windowID);
    let [chromeWin, browserEl] = this._getUIForWindowID(windowID);

    let message = chromeWin.gNavigatorBundle.getFormattedString("identity.loggedIn.description",
                                                          [aIdentity]);
    let mainAction = {
      label: chromeWin.gNavigatorBundle.getString("identity.loggedIn.signOut.label"),
      accessKey: chromeWin.gNavigatorBundle.getString("identity.loggedIn.signOut.accessKey"),
      callback: function() {
        log("sign out callback fired");
        IdentityService.RP.logout(windowID);
      },
    };
    let secondaryActions = [];
    let options = {
      dismissed: true,
    };
    let loggedInNot = chromeWin.PopupNotifications.show(browserEl, "identity-logged-in", message,
                                                  "identity-notification-icon", mainAction,
                                                  secondaryActions, options);
    loggedInNot.rpId = windowID;
  },

  


  _removeLoggedInUI: function _removeLoggedInUI(aContext) {
    let windowID = aContext.rpId;
    log("_removeLoggedInUI for ", windowID);
    if (!windowID)
      throw "_removeLoggedInUI: Invalid RP ID";
    let [chromeWin, browserEl] = this._getUIForWindowID(windowID);

    let loggedInNot = chromeWin.PopupNotifications.getNotification("identity-logged-in", browserEl);
    if (loggedInNot)
      chromeWin.PopupNotifications.remove(loggedInNot);
  },

  


  _removeRequestUI: function _removeRequestUI(aContext) {
    let windowID = aContext.rpId;
    log("_removeRequestUI for ", windowID);
    let [chromeWin, browserEl] = this._getUIForWindowID(windowID);

    let requestNot = chromeWin.PopupNotifications.getNotification("identity-request", browserEl);
    if (requestNot)
      chromeWin.PopupNotifications.remove(requestNot);
  },

};
