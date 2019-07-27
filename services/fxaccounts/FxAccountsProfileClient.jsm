






 "use strict;"

this.EXPORTED_SYMBOLS = ["FxAccountsProfileClient", "FxAccountsProfileClientError"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/rest.js");

Cu.importGlobalProperties(["URL"]);












this.FxAccountsProfileClient = function(options) {
  if (!options || !options.serverURL) {
    throw new Error("Missing 'serverURL' configuration option");
  }

  this.fxa = options.fxa || fxAccounts;
  
  
  
  
  
  
  this.token = options.token;

  try {
    this.serverURL = new URL(options.serverURL);
  } catch (e) {
    throw new Error("Invalid 'serverURL'");
  }
  this.oauthOptions = {
    scope: "profile",
  };
  log.debug("FxAccountsProfileClient: Initialized");
};

this.FxAccountsProfileClient.prototype = {
  



  serverURL: null,

  


  _Request: RESTRequest,

  











  _createRequest: Task.async(function* (path, method = "GET") {
    let token = this.token;
    if (!token) {
      
      token = yield this.fxa.getOAuthToken(this.oauthOptions);
    }
    try {
      return (yield this._rawRequest(path, method, token));
    } catch (ex if ex instanceof FxAccountsProfileClientError && ex.code == 401) {
      
      if (this.token) {
        throw ex;
      }
      
      log.info("Fetching the profile returned a 401 - revoking our token and retrying");
      yield this.fxa.removeCachedOAuthToken({token});
      token = yield this.fxa.getOAuthToken(this.oauthOptions);
      
      
      try {
        return (yield this._rawRequest(path, method, token));
      } catch (ex if ex instanceof FxAccountsProfileClientError && ex.code == 401) {
        log.info("Retry fetching the profile still returned a 401 - revoking our token and failing");
        yield this.fxa.removeCachedOAuthToken({token});
        throw ex;
      }
    }
  }),

  












  _rawRequest: function(path, method, token) {
    return new Promise((resolve, reject) => {
      let profileDataUrl = this.serverURL + path;
      let request = new this._Request(profileDataUrl);
      method = method.toUpperCase();

      request.setHeader("Authorization", "Bearer " + token);
      request.setHeader("Accept", "application/json");

      request.onComplete = function (error) {
        if (error) {
          return reject(new FxAccountsProfileClientError({
            error: ERROR_NETWORK,
            errno: ERRNO_NETWORK,
            message: error.toString(),
          }));
        }

        let body = null;
        try {
          body = JSON.parse(request.response.body);
        } catch (e) {
          return reject(new FxAccountsProfileClientError({
            error: ERROR_PARSE,
            errno: ERRNO_PARSE,
            code: request.response.status,
            message: request.response.body,
          }));
        }

        
        if (request.response.success) {
          return resolve(body);
        } else {
          return reject(new FxAccountsProfileClientError({
            error: body.error || ERROR_UNKNOWN,
            errno: body.errno || ERRNO_UNKNOWN_ERROR,
            code: request.response.status,
            message: body.message || body,
          }));
        }
      };

      if (method === "GET") {
        request.get();
      } else {
        
        return reject(new FxAccountsProfileClientError({
          error: ERROR_NETWORK,
          errno: ERRNO_NETWORK,
          code: ERROR_CODE_METHOD_NOT_ALLOWED,
          message: ERROR_MSG_METHOD_NOT_ALLOWED,
        }));
      }
    });
  },

  






  fetchProfile: function () {
    log.debug("FxAccountsProfileClient: Requested profile");
    return this._createRequest("/profile", "GET");
  },

  






  fetchProfileImage: function () {
    log.debug("FxAccountsProfileClient: Requested avatar");
    return this._createRequest("/avatar", "GET");
  }
};















this.FxAccountsProfileClientError = function(details) {
  details = details || {};

  this.name = "FxAccountsProfileClientError";
  this.code = details.code || null;
  this.errno = details.errno || ERRNO_UNKNOWN_ERROR;
  this.error = details.error || ERROR_UNKNOWN;
  this.message = details.message || null;
};







FxAccountsProfileClientError.prototype._toStringFields = function() {
  return {
    name: this.name,
    code: this.code,
    errno: this.errno,
    error: this.error,
    message: this.message,
  };
};






FxAccountsProfileClientError.prototype.toString = function() {
  return this.name + "(" + JSON.stringify(this._toStringFields()) + ")";
};
