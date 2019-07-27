




const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function nsLoginInfo() {}

nsLoginInfo.prototype = {

  classID : Components.ID("{0f2f347c-1e4f-40cc-8efd-792dea70a85e}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsILoginInfo, Ci.nsILoginMetaInfo]),

  
  
  

  hostname      : null,
  formSubmitURL : null,
  httpRealm     : null,
  username      : null,
  password      : null,
  usernameField : null,
  passwordField : null,

  init : function (aHostname, aFormSubmitURL, aHttpRealm,
                   aUsername,      aPassword,
                   aUsernameField, aPasswordField) {
    this.hostname      = aHostname;
    this.formSubmitURL = aFormSubmitURL;
    this.httpRealm     = aHttpRealm;
    this.username      = aUsername;
    this.password      = aPassword;
    this.usernameField = aUsernameField;
    this.passwordField = aPasswordField;
  },

  matches : function (aLogin, ignorePassword) {
    if (this.hostname      != aLogin.hostname      ||
        this.httpRealm     != aLogin.httpRealm     ||
        this.username      != aLogin.username)
      return false;

    if (!ignorePassword && this.password != aLogin.password)
      return false;

    
    if (this.formSubmitURL != "" && aLogin.formSubmitURL != "" &&
        this.formSubmitURL != aLogin.formSubmitURL)
      return false;

    

    return true;
  },

  equals : function (aLogin) {
    if (this.hostname      != aLogin.hostname      ||
        this.formSubmitURL != aLogin.formSubmitURL ||
        this.httpRealm     != aLogin.httpRealm     ||
        this.username      != aLogin.username      ||
        this.password      != aLogin.password      ||
        this.usernameField != aLogin.usernameField ||
        this.passwordField != aLogin.passwordField)
      return false;

    return true;
  },

  clone : function() {
    let clone = Cc["@mozilla.org/login-manager/loginInfo;1"].
                createInstance(Ci.nsILoginInfo);
    clone.init(this.hostname, this.formSubmitURL, this.httpRealm,
               this.username, this.password,
               this.usernameField, this.passwordField);

    
    clone.QueryInterface(Ci.nsILoginMetaInfo);
    clone.guid = this.guid;
    clone.timeCreated = this.timeCreated;
    clone.timeLastUsed = this.timeLastUsed;
    clone.timePasswordChanged = this.timePasswordChanged;
    clone.timesUsed = this.timesUsed;

    return clone;
  },

  
  
  

  guid : null,
  timeCreated : null,
  timeLastUsed : null,
  timePasswordChanged : null,
  timesUsed : null

}; 

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([nsLoginInfo]);
