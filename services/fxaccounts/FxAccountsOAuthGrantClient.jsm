









this.EXPORTED_SYMBOLS = ["FxAccountsOAuthGrantClient", "FxAccountsOAuthGrantClientError"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://services-common/rest.js");

Cu.importGlobalProperties(["URL"]);

const AUTH_ENDPOINT = "/authorization";
const DESTROY_ENDPOINT = "/destroy";














this.FxAccountsOAuthGrantClient = function(options) {

  this._validateOptions(options);
  this.parameters = options;

  try {
    this.serverURL = new URL(this.parameters.serverURL);
  } catch (e) {
    throw new Error("Invalid 'serverURL'");
  }

  log.debug("FxAccountsOAuthGrantClient Initialized");
};

this.FxAccountsOAuthGrantClient.prototype = {

  







  getTokenFromAssertion: function (assertion, scope) {
    if (!assertion) {
      throw new Error("Missing 'assertion' parameter");
    }
    if (!scope) {
      throw new Error("Missing 'scope' parameter");
    }
    let params = {
      scope: scope,
      client_id: this.parameters.client_id,
      assertion: assertion,
      response_type: "token"
    };

    return this._createRequest(AUTH_ENDPOINT, "POST", params);
  },

  







  destroyToken: function (token) {
    if (!token) {
      throw new Error("Missing 'token' parameter");
    }
    let params = {
      token: token,
    };

    return this._createRequest(DESTROY_ENDPOINT, "POST", params);
  },

  






  _validateOptions: function (options) {
    if (!options) {
      throw new Error("Missing configuration options");
    }

    ["serverURL", "client_id"].forEach(option => {
      if (!options[option]) {
        throw new Error("Missing '" + option + "' parameter");
      }
    });
  },

  


  _Request: RESTRequest,

  











  _createRequest: function(path, method = "POST", params) {
    return new Promise((resolve, reject) => {
      let profileDataUrl = this.serverURL + path;
      let request = new this._Request(profileDataUrl);
      method = method.toUpperCase();

      request.setHeader("Accept", "application/json");
      request.setHeader("Content-Type", "application/json");

      request.onComplete = function (error) {
        if (error) {
          return reject(new FxAccountsOAuthGrantClientError({
            error: ERROR_NETWORK,
            errno: ERRNO_NETWORK,
            message: error.toString(),
          }));
        }

        let body = null;
        try {
          body = JSON.parse(request.response.body);
        } catch (e) {
          return reject(new FxAccountsOAuthGrantClientError({
            error: ERROR_PARSE,
            errno: ERRNO_PARSE,
            code: request.response.status,
            message: request.response.body,
          }));
        }

        
        if (request.response.success) {
          return resolve(body);
        }

        if (typeof body.errno === 'number') {
          
          body.errno += OAUTH_SERVER_ERRNO_OFFSET;
        } else if (body.errno) {
          body.errno = ERRNO_UNKNOWN_ERROR;
        }
        return reject(new FxAccountsOAuthGrantClientError(body));
      };

      if (method === "POST") {
        request.post(params);
      } else {
        
        return reject(new FxAccountsOAuthGrantClientError({
          error: ERROR_NETWORK,
          errno: ERRNO_NETWORK,
          code: ERROR_CODE_METHOD_NOT_ALLOWED,
          message: ERROR_MSG_METHOD_NOT_ALLOWED,
        }));
      }
    });
  },

};















this.FxAccountsOAuthGrantClientError = function(details) {
  details = details || {};

  this.name = "FxAccountsOAuthGrantClientError";
  this.code = details.code || null;
  this.errno = details.errno || ERRNO_UNKNOWN_ERROR;
  this.error = details.error || ERROR_UNKNOWN;
  this.message = details.message || null;
};







FxAccountsOAuthGrantClientError.prototype._toStringFields = function() {
  return {
    name: this.name,
    code: this.code,
    errno: this.errno,
    error: this.error,
    message: this.message,
  };
};






FxAccountsOAuthGrantClientError.prototype.toString = function() {
  return this.name + "(" + JSON.stringify(this._toStringFields()) + ")";
};
