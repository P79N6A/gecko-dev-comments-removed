










"use strict";

this.EXPORTED_SYMBOLS = [
  "BagheeraClient",
  "BagheeraClientRequestResult",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");





this.BagheeraClientRequestResult = function BagheeraClientRequestResult() {
  this.transportSuccess = false;
  this.serverSuccess = false;
  this.request = null;
}

Object.freeze(BagheeraClientRequestResult.prototype);









this.BagheeraClient = function BagheeraClient(baseURI) {
  if (!baseURI) {
    throw new Error("baseURI argument must be defined.");
  }

  this._log = Log4Moz.repository.getLogger("Services.BagheeraClient");
  this._log.level = Log4Moz.Level["Debug"];

  this.baseURI = baseURI;

  if (!baseURI.endsWith("/")) {
    this.baseURI += "/";
  }
}

BagheeraClient.prototype = {
  







  _loadFlags: Ci.nsIRequest.LOAD_BYPASS_CACHE |
              Ci.nsIRequest.INHIBIT_CACHING |
              Ci.nsIRequest.LOAD_ANONYMOUS,

  DEFAULT_TIMEOUT_MSEC: 5 * 60 * 1000, 

  _RE_URI_IDENTIFIER: /^[a-zA-Z0-9_-]+$/,

  





















  uploadJSON: function uploadJSON(namespace, id, payload, deleteOldID=null) {
    if (!namespace) {
      throw new Error("namespace argument must be defined.");
    }

    if (!id) {
      throw new Error("id argument must be defined.");
    }

    if (!payload) {
      throw new Error("payload argument must be defined.");
    }

    let uri = this._submitURI(namespace, id);

    let data = payload;

    if (typeof(payload) == "object") {
      data = JSON.stringify(payload);
    }

    if (typeof(data) != "string") {
      throw new Error("Unknown type for payload: " + typeof(data));
    }

    this._log.info("Uploading data to " + uri);

    let request = new RESTRequest(uri);
    request.loadFlags = this._loadFlags;
    request.timeout = this.DEFAULT_TIMEOUT_MSEC;

    if (deleteOldID) {
      request.setHeader("X-Obsolete-Document", deleteOldID);
    }

    let deferred = Promise.defer();

    data = CommonUtils.convertString(data, "uncompressed", "deflate");
    
    request.setHeader("Content-Type", "application/json+zlib; charset=utf-8");

    this._log.info("Request body length: " + data.length);

    let result = new BagheeraClientRequestResult();
    result.namespace = namespace;
    result.id = id;

    request.onComplete = this._onComplete.bind(this, request, deferred, result);
    request.post(data);

    return deferred.promise;
  },

  









  deleteDocument: function deleteDocument(namespace, id) {
    let uri = this._submitURI(namespace, id);

    let request = new RESTRequest(uri);
    request.loadFlags = this._loadFlags;
    request.timeout = this.DEFAULT_TIMEOUT_MSEC;

    let result = new BagheeraClientRequestResult();
    result.namespace = namespace;
    result.id = id;
    let deferred = Promise.defer();

    request.onComplete = this._onComplete.bind(this, request, deferred, result);
    request.delete();

    return deferred.promise;
  },

  _submitURI: function _submitURI(namespace, id) {
    if (!this._RE_URI_IDENTIFIER.test(namespace)) {
      throw new Error("Illegal namespace name. Must be alphanumeric + [_-]: " +
                      namespace);
    }

    if (!this._RE_URI_IDENTIFIER.test(id)) {
      throw new Error("Illegal id value. Must be alphanumeric + [_-]: " + id);
    }

    return this.baseURI + "1.0/submit/" + namespace + "/" + id;
  },

  _onComplete: function _onComplete(request, deferred, result, error) {
    result.request = request;

    if (error) {
      this._log.info("Transport failure on request: " +
                     CommonUtils.exceptionStr(error));
      result.transportSuccess = false;
      deferred.resolve(result);
      return;
    }

    result.transportSuccess = true;

    let response = request.response;

    switch (response.status) {
      case 200:
      case 201:
        result.serverSuccess = true;
        break;

      default:
        result.serverSuccess = false;

        this._log.info("Received unexpected status code: " + response.status);
        this._log.debug("Response body: " + response.body);
    }

    deferred.resolve(result);
  },
};

Object.freeze(BagheeraClient.prototype);
