



"use strict";





















this.EXPORTED_SYMBOLS = ["HawkClient"];

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-common/hawkrequest.js");
Cu.import("resource://gre/modules/Promise.jsm");












this.HawkClient = function(host) {
  this.host = host;

  
  
  this._localtimeOffsetMsec = 0;
}

this.HawkClient.prototype = {

  








  _constructError: function(restResponse, errorString) {
    let errorObj = {
      error: errorString,
      message: restResponse.statusText,
      code: restResponse.status,
      errno: restResponse.status
    };
    let retryAfter = restResponse.headers && restResponse.headers["retry-after"];
    retryAfter = retryAfter ? parseInt(retryAfter) : retryAfter;
    if (retryAfter) {
      errorObj.retryAfter = retryAfter;
    }
    return errorObj;
  },

  














  _updateClockOffset: function(dateString) {
    try {
      let serverDateMsec = Date.parse(dateString);
      this._localtimeOffsetMsec = serverDateMsec - this.now();
      log.debug("Clock offset vs " + this.host + ": " + this._localtimeOffsetMsec);
    } catch(err) {
      log.warn("Bad date header in server response: " + dateString);
    }
  },

  






  get localtimeOffsetMsec() {
    return this._localtimeOffsetMsec;
  },

  


  now: function() {
    return Date.now();
  },

  
















  request: function(path, method, credentials=null, payloadObj={}, retryOK=true) {
    method = method.toLowerCase();

    let deferred = Promise.defer();
    let uri = this.host + path;
    let self = this;

    function onComplete(error) {
      let restResponse = this.response;
      let status = restResponse.status;

      log.debug("(Response) code: " + status +
                " - Status text: " + restResponse.statusText,
                " - Response text: " + restResponse.body);

      if (error) {
        
        
        return deferred.reject(self._constructError(restResponse, error));
      }

      self._updateClockOffset(restResponse.headers["date"]);

      if (status === 401 && retryOK) {
        
        
        log.debug("Received 401 for " + path + ": retrying");
        return deferred.resolve(
            self.request(path, method, credentials, payloadObj, false));
      }

      
      
      
      
      
      

      let jsonResponse = {};
      try {
        jsonResponse = JSON.parse(restResponse.body);
      } catch(notJSON) {}

      let okResponse = (200 <= status && status < 300);
      if (!okResponse || jsonResponse.error) {
        if (jsonResponse.error) {
          return deferred.reject(jsonResponse);
        }
        return deferred.reject(self._constructError(restResponse, "Request failed"));
      }
      
      
      deferred.resolve(this.response.body);
    };

    let extra = {
      now: this.now(),
      localtimeOffsetMsec: this.localtimeOffsetMsec,
    };

    let request = this.newHAWKAuthenticatedRESTRequest(uri, credentials, extra);
    if (method == "post" || method == "put") {
      request[method](payloadObj, onComplete);
    } else {
      request[method](onComplete);
    }

    return deferred.promise;
  },

  
  newHAWKAuthenticatedRESTRequest: function(uri, credentials, extra) {
    return new HAWKAuthenticatedRESTRequest(uri, credentials, extra);
  },
}
