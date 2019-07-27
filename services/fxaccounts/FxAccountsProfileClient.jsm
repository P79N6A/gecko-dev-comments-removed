







this.EXPORTED_SYMBOLS = ["FxAccountsProfileClient", "FxAccountsProfileClientError"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://services-common/rest.js");

Cu.importGlobalProperties(["URL"]);












this.FxAccountsProfileClient = function(options) {
  if (!options || !options.serverURL || !options.token) {
    throw new Error("Missing 'serverURL' or 'token' configuration option");
  }

  try {
    this.serverURL = new URL(options.serverURL);
  } catch (e) {
    throw new Error("Invalid 'serverURL'");
  }
  this.token = options.token;
  log.debug("FxAccountsProfileClient: Initialized");
};

this.FxAccountsProfileClient.prototype = {
  



  serverURL: null,

  



  token: null,

  


  _Request: RESTRequest,

  











  _createRequest: function(path, method = "GET") {
    return new Promise((resolve, reject) => {
      let profileDataUrl = this.serverURL + path;
      let request = new this._Request(profileDataUrl);
      method = method.toUpperCase();

      request.setHeader("Authorization", "Bearer " + this.token);
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
          return reject(new FxAccountsProfileClientError(body));
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
  }
};















var FxAccountsProfileClientError = function (details) {
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
