



"use strict";

const EXPORTED_SYMBOLS = [
  "TokenServerClient",
  "TokenServerClientError",
  "TokenServerClientNetworkError",
  "TokenServerClientServerError"
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");

const Prefs = new Preferences("services.common.tokenserverclient.");









function TokenServerClientError(message) {
  this.name = "TokenServerClientError";
  this.message = message || "Client error.";
}
TokenServerClientError.prototype = new Error();
TokenServerClientError.prototype.constructor = TokenServerClientError;







function TokenServerClientNetworkError(error) {
  this.name = "TokenServerClientNetworkError";
  this.error = error;
}
TokenServerClientNetworkError.prototype = new TokenServerClientError();
TokenServerClientNetworkError.prototype.constructor =
  TokenServerClientNetworkError;










function TokenServerClientServerError(message) {
  this.name = "TokenServerClientServerError";
  this.message = message || "Server error.";
}
TokenServerClientServerError.prototype = new TokenServerClientError();
TokenServerClientServerError.prototype.constructor =
  TokenServerClientServerError;
























function TokenServerClient() {
  this._log = Log4Moz.repository.getLogger("Common.TokenServerClient");
  this._log.level = Log4Moz.Level[Prefs.get("logger.level")];
}
TokenServerClient.prototype = {
  


  _log: null,

  







































  getTokenFromBrowserIDAssertion:
    function getTokenFromBrowserIDAssertion(url, assertion, cb) {
    if (!url) {
      throw new TokenServerClientError("url argument is not valid.");
    }

    if (!assertion) {
      throw new TokenServerClientError("assertion argument is not valid.");
    }

    if (!cb) {
      throw new TokenServerClientError("cb argument is not valid.");
    }

    this._log.debug("Beginning BID assertion exchange: " + url);

    let req = new RESTRequest(url);
    req.setHeader("accept", "application/json");
    req.setHeader("authorization", "Browser-ID " + assertion);
    let client = this;
    req.get(function onResponse(error) {
      if (error) {
        cb(new TokenServerClientNetworkError(error), null);
        return;
      }

      let self = this;
      function callCallback(error, result) {
        if (!cb) {
          self._log.warn("Callback already called! Did it throw?");
          return;
        }

        try {
          cb(error, result);
        } catch (ex) {
          self._log.warn("Exception when calling user-supplied callback: " +
                         CommonUtils.exceptionStr(ex));
        }

        cb = null;
      }

      try {
        client._processTokenResponse(this.response, callCallback);
      } catch (ex) {
        this._log.warn("Error processing token server response: " +
                       CommonUtils.exceptionStr(ex));

        let error = new TokenServerClientError(ex);
        error.response = this.response;
        callCallback(error, null);
      }
    });
  },

  







  _processTokenResponse: function processTokenResponse(response, cb) {
    this._log.debug("Got token response.");

    if (!response.success) {
      this._log.info("Non-200 response code to token request: " +
                     response.status);
      this._log.debug("Response body: " + response.body);
      let error = new TokenServerClientServerError("Non 200 response code: " +
                                                   response.status);
      error.response = response;
      cb(error, null);
      return;
    }

    let ct = response.headers["content-type"];
    if (ct != "application/json" && ct.indexOf("application/json;") != 0) {
      let error =  new TokenServerClientError("Unsupported media type: " + ct);
      error.response = response;
      cb(error, null);
      return;
    }

    let result;
    try {
      result = JSON.parse(response.body);
    } catch (ex) {
      let error = new TokenServerClientServerError("Invalid JSON returned " +
                                                   "from server.");
      error.response = response;
      cb(error, null);
      return;
    }

    for each (let k in ["id", "key", "api_endpoint", "uid"]) {
      if (!(k in result)) {
        let error = new TokenServerClientServerError("Expected key not " +
                                                     " present in result: " +
                                                     k);
        error.response = response;
        cb(error, null);
        return;
      }
    }

    this._log.debug("Successful token response: " + result.id);
    cb(null, {
      id:       result.id,
      key:      result.key,
      endpoint: result.api_endpoint,
      uid:      result.uid,
    });
  }
};
