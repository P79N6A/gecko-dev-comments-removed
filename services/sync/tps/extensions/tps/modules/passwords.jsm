




































 




var EXPORTED_SYMBOLS = ["Password", "DumpPasswords"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://gre/modules/Services.jsm");
CU.import("resource://tps/logger.jsm");

let nsLoginInfo = new Components.Constructor(
                      "@mozilla.org/login-manager/loginInfo;1",  
                      CI.nsILoginInfo,  
                      "init");  

var DumpPasswords = function TPS__Passwords__DumpPasswords() {
  let logins = Services.logins.getAllLogins();
  Logger.logInfo("\ndumping password list\n", true);
  for (var i = 0; i < logins.length; i++) {
    Logger.logInfo("* host=" + logins[i].hostname + ", submitURL=" + logins[i].formSubmitURL +
                   ", realm=" + logins[i].httpRealm + ", password=" + logins[i].password +
                   ", passwordField=" + logins[i].passwordField + ", username=" +
                   logins[i].username + ", usernameField=" + logins[i].usernameField, true);
  }
  Logger.logInfo("\n\nend password list\n", true);
};




function PasswordProps(props) {
  this.hostname = null;
  this.submitURL = null;
  this.realm = null;
  this.username = "";
  this.password = "";
  this.usernameField = "";
  this.passwordField = "";
  this.delete = false;

  for (var prop in props) {
    if (prop in this)
      this[prop] = props[prop];
  }
}




function Password(props) {
  this.props = new PasswordProps(props);
  if ("changes" in props) {
    this.updateProps = new PasswordProps(props);
    for (var prop in props.changes)
      if (prop in this.updateProps)
        this.updateProps[prop] = props.changes[prop];
  }
  else {
    this.updateProps = null;
  }
}




Password.prototype = {
  







  Create: function() {
    let login = new nsLoginInfo(this.props.hostname, this.props.submitURL,
                                this.props.realm, this.props.username,
                                this.props.password, 
                                this.props.usernameField,
                                this.props.passwordField);
    Services.logins.addLogin(login);
    login.QueryInterface(CI.nsILoginMetaInfo);
    return login.guid;               
  },

  







  Find: function() {
    let logins = Services.logins.findLogins({},
                                            this.props.hostname,
                                            this.props.submitURL,
                                            this.props.realm);
    for (var i = 0; i < logins.length; i++) {
      if (logins[i].username == this.props.username &&
          logins[i].password == this.props.password &&
          logins[i].usernameField == this.props.usernameField &&
          logins[i].passwordField == this.props.passwordField) {
        logins[i].QueryInterface(CI.nsILoginMetaInfo);
        return logins[i].guid;
      }
    }
    return -1;
  },

  








 
  Update: function() {
    let oldlogin = new nsLoginInfo(this.props.hostname, 
                                   this.props.submitURL,
                                   this.props.realm, 
                                   this.props.username,
                                   this.props.password, 
                                   this.props.usernameField,
                                   this.props.passwordField);
    let newlogin = new nsLoginInfo(this.updateProps.hostname, 
                                   this.updateProps.submitURL,
                                   this.updateProps.realm, 
                                   this.updateProps.username,
                                   this.updateProps.password, 
                                   this.updateProps.usernameField,
                                   this.updateProps.passwordField);
    Services.logins.modifyLogin(oldlogin, newlogin);
  },

  







  Remove: function() {
    let login = new nsLoginInfo(this.props.hostname, 
                                this.props.submitURL,
                                this.props.realm, 
                                this.props.username,
                                this.props.password, 
                                this.props.usernameField,
                                this.props.passwordField);
    Services.logins.removeLogin(login);
  },
};
