





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "RESTRequest", "resource://services-common/rest.js");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils", "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts", "resource://gre/modules/FxAccounts.jsm");

let log = Log.repository.getLogger("readinglist.serverclient");

const OAUTH_SCOPE = "readinglist"; 

this.EXPORTED_SYMBOLS = [
  "ServerClient",
];







function objectToUTF8Json(obj) {
  
  return CommonUtils.encodeUTF8(JSON.stringify(obj));
}

function ServerClient(fxa = fxAccounts) {
  this.fxa = fxa;
}

ServerClient.prototype = {

  request(options) {
    return this._request(options.path, options.method, options.body, options.headers);
  },

  get serverURL() {
    return Services.prefs.getCharPref("readinglist.server");
  },

  _getURL(path) {
    let result = this.serverURL;
    
    
    if (result.endsWith("/")) {
      result = result.slice(0, -1);
    }
    return result + path;
  },

  
  _getToken() {
    
    
    return this.fxa.getOAuthToken({scope: OAUTH_SCOPE});
  },

  _removeToken(token) {
    return this.fxa.removeCachedOAuthToken({token});
  },

  
  _convertRestError(error) {
    return error; 
  },

  
  _convertJSError(error) {
    return error; 
  },

  


  _request: Task.async(function* (path, method, body, headers) {
    let token = yield this._getToken();
    let response = yield this._rawRequest(path, method, body, headers, token);
    log.debug("initial request got status ${status}", response);
    if (response.status == 401) {
      
      this._removeToken(token);
      token = yield this._getToken();
      response = yield this._rawRequest(path, method, body, headers, token);
      log.debug("retry of request got status ${status}", response);
    }
    return response;
  }),

  










  _rawRequest(path, method, body, headers, oauthToken) {
    return new Promise((resolve, reject) => {
      let url = this._getURL(path);
      log.debug("dispatching request to", url);
      let request = new RESTRequest(url);
      method = method.toUpperCase();

      request.setHeader("Accept", "application/json");
      request.setHeader("Content-Type", "application/json; charset=utf-8");
      request.setHeader("Authorization", "Bearer " + oauthToken);
      
      if (headers) {
        for (let [headerName, headerValue] in Iterator(headers)) {
          log.trace("Caller specified header: ${headerName}=${headerValue}", {headerName, headerValue});
          request.setHeader(headerName, headerValue);
        }
      }

      request.onComplete = error => {
        
        
        
        
        let response = request.response;
        if (response && response.headers) {
          let backoff = response.headers["backoff"] || response.headers["retry-after"];
          if (backoff) {
            log.info("Server requested backoff", backoff);
            Services.obs.notifyObservers(null, "readinglist:backoff-requested", backoff);
          }
        }
        if (error) {
          return reject(this._convertRestError(error));
        }

        log.debug("received response status: ${status} ${statusText}", response);
        
        let result = {
          status: response.status,
          headers: response.headers
        };
        try {
          if (response.body) {
            result.body = JSON.parse(response.body);
          }
        } catch (e) {
          log.debug("Response is not JSON. First 1024 chars: |${body}|",
                    { body: response.body.substr(0, 1024) });
          
          
          
        }

        resolve(result);
      }
      
      
      request.dispatch(method, objectToUTF8Json(body));
    });
  },
};
