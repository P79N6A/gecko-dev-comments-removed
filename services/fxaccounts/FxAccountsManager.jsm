










"use strict";

this.EXPORTED_SYMBOLS = ["FxAccountsManager"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/FxAccountsConsts.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsClient",
  "resource://gre/modules/FxAccountsClient.jsm");

this.FxAccountsManager = {

  
  
  _fxAccounts: fxAccounts,

  
  
  _activeSession: null,

  
  get _user() {
    if (!this._activeSession || !this._activeSession.email) {
      return null;
    }

    return {
      accountId: this._activeSession.email,
      verified: this._activeSession.verified
    }
  },

  _getError: function(aServerResponse) {
    if (!aServerResponse || !aServerResponse.error || !aServerResponse.error.errno) {
      return;
    }
    return SERVER_ERRNO_TO_ERROR[aServerResponse.error.errno];
  },

  _serverError: function(aServerResponse) {
    let error = this._getError({ error: aServerResponse });
    return Promise.reject({
      error: error ? error : ERROR_SERVER_ERROR,
      details: aServerResponse
    });
  },

  
  
  _createFxAccountsClient: function() {
    return new FxAccountsClient();
  },

  _signInSignUp: function(aMethod, aAccountId, aPassword) {
    if (Services.io.offline) {
      return Promise.reject({
        error: ERROR_OFFLINE
      });
    }

    if (!aAccountId) {
      return Promise.reject({
        error: ERROR_INVALID_ACCOUNTID
      });
    }

    if (!aPassword) {
      return Promise.reject({
        error: ERROR_INVALID_PASSWORD
      });
    }

    
    if (this._activeSession) {
      return Promise.reject({
        error: ERROR_ALREADY_SIGNED_IN_USER,
        details: {
          user: this._user
        }
      });
    }

    let client = this._createFxAccountsClient();
    return this._fxAccounts.getSignedInUser().then(
      user => {
        if (user) {
          return Promise.reject({
            error: ERROR_ALREADY_SIGNED_IN_USER,
            details: {
              user: user
            }
          });
        }
        return client[aMethod](aAccountId, aPassword);
      }
    ).then(
      user => {
        let error = this._getError(user);
        if (!user || !user.uid || !user.sessionToken || error) {
          return Promise.reject({
            error: error ? error : ERROR_INTERNAL_INVALID_USER,
            details: {
              user: user
            }
          });
        }

        
        user.email = aAccountId;
        return this._fxAccounts.setSignedInUser(user, false).then(
          () => {
            this._activeSession = user;
            return Promise.resolve({
              accountCreated: aMethod === "signUp",
              user: this._user
            });
          }
        );
      },
      reason => { return this._serverError(reason); }
    );
  },

  _getAssertion: function(aAudience) {
    return this._fxAccounts.getAssertion(aAudience);
  },

  _signOut: function() {
    if (!this._activeSession) {
      return Promise.resolve();
    }

    return this._fxAccounts.signOut(this._activeSession.sessionToken).then(
      () => {
        
        
        
        if (Services.io.offline) {
          this._activeSession = null;
          return Promise.resolve();
        }
        
        let client = this._createFxAccountsClient();
        return client.signOut(this._activeSession.sessionToken).then(
          result => {
            
            
            this._activeSession = null;
            let error = this._getError(result);
            if (error) {
              return Promise.reject({
                error: error,
                details: result
              });
            }
            return Promise.resolve();
          },
          reason => {
            
            
            this._activeSession = null;
            return this._serverError(reason);
          }
        );
      }
    );
  },

  

  signIn: function(aAccountId, aPassword) {
    return this._signInSignUp("signIn", aAccountId, aPassword);
  },

  signUp: function(aAccountId, aPassword) {
    return this._signInSignUp("signUp", aAccountId, aPassword);
  },

  signOut: function() {
    if (!this._activeSession) {
      
      
      return this.getAccount().then(
        result => {
          if (!result) {
            return Promise.resolve();
          }
          return this._signOut();
        }
      );
    }
    return this._signOut();
  },

  getAccount: function() {
    
    if (this._activeSession) {
      
      
      if (this._activeSession && !this._activeSession.verified &&
          !Services.io.offline) {
        return this.verificationStatus(this._activeSession);
      }

      return Promise.resolve(this._user);
    }

    
    return this._fxAccounts.getSignedInUser().then(
      user => {
        if (!user || !user.email) {
          return Promise.resolve(null);
        }

        this._activeSession = user;
        
        
        
        if (!user.verified && !Services.io.offline) {
          return this.verificationStatus(user);
        }

        return Promise.resolve(this._user);
      }
    );
  },

  queryAccount: function(aAccountId) {
    if (Services.io.offline) {
      return Promise.reject({
        error: ERROR_OFFLINE
      });
    }

    let deferred = Promise.defer();

    if (!aAccountId) {
      return Promise.reject({
        error: ERROR_INVALID_ACCOUNTID
      });
    }

    let client = this._createFxAccountsClient();
    return client.accountExists(aAccountId).then(
      result => {
        let error = this._getError(result);
        if (error) {
          return Promise.reject({
            error: error,
            details: result
          });
        }

        return Promise.resolve({
          registered: result
        });
      },
      reason => { this._serverError(reason); }
    );
  },

  verificationStatus: function() {
    if (!this._activeSession || !this._activeSession.sessionToken) {
      return Promise.reject({
        error: ERROR_NO_TOKEN_SESSION
      });
    }

    
    
    if (this._activeSession.verified) {
      return Promise.resolve(this._user);
    }

    if (Services.io.offline) {
      return Promise.reject({
        error: ERROR_OFFLINE
      });
    }

    let client = this._createFxAccountsClient();
    return client.recoveryEmailStatus(this._activeSession.sessionToken).then(
      data => {
        let error = this._getError(data);
        if (error) {
          return Promise.reject({
            error: error,
            details: data
          });
        }

        
        
        
        if (this._activeSession.verified != data.verified) {
          this._activeSession.verified = data.verified;
          return this._fxAccounts.setSignedInUser(this._activeSession).then(
            () => {
              return Promise.resolve(this._user);
            }
          );
        }
        return Promise.resolve(this._user);
      },
      reason => { return this._serverError(reason); }
    );
  },

  getAssertion: function(aAudience) {
    if (!aAudience) {
      return Promise.reject({
        error: ERROR_INVALID_AUDIENCE
      });
    }

    if (Services.io.offline) {
      return Promise.reject({
        error: ERROR_OFFLINE
      });
    }

    return this.getAccount().then(
      user => {
        if (user) {
          
          if (user.verified) {
            return this._getAssertion(aAudience);
          }

          return Promise.reject({
            error: ERROR_UNVERIFIED_ACCOUNT,
            details: {
              user: user
            }
          });
        }

        
        
        let ui = Cc["@mozilla.org/fxaccounts/fxaccounts-ui-glue;1"]
                   .createInstance(Ci.nsIFxAccountsUIGlue);
        return ui.signInFlow().then(
          result => {
            
            
            if (result && result.verified) {
              return this._getAssertion(aAudience);
            }
            return Promise.reject({
              error: ERROR_UNVERIFIED_ACCOUNT,
              details: {
                user: result
              }
            });
          },
          error => {
            return Promise.reject({
              error: ERROR_UI_ERROR,
              details: error
            });
          }
        );
      }
    );
  }
};
