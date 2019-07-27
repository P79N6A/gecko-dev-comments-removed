



"use strict";

const {utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["BagheeraServer"];

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://testing-common/httpd.js");











this.BagheeraServer = function BagheeraServer() {
  this._log = Log.repository.getLogger("metrics.BagheeraServer");

  this.server = new HttpServer();
  this.namespaces = {};

  this.allowAllNamespaces = false;
}

BagheeraServer.prototype = {
  






  hasNamespace: function hasNamespace(ns) {
    return ns in this.namespaces;
  },

  








  hasDocument: function hasDocument(ns, id) {
    let namespace = this.namespaces[ns];

    if (!namespace) {
      return false;
    }

    return id in namespace;
  },

  










  getDocument: function getDocument(ns, id) {
    let namespace = this.namespaces[ns];

    if (!namespace) {
      return null;
    }

    return namespace[id];
  },

  









  setDocument: function setDocument(ns, id, payload) {
    let namespace = this.namespaces[ns];

    if (!namespace) {
      if (!this.allowAllNamespaces) {
        throw new Error("Namespace does not exist: " + ns);
      }

      this.createNamespace(ns);
      namespace = this.namespaces[ns];
    }

    namespace[id] = payload;
  },

  







  createNamespace: function createNamespace(ns) {
    if (ns in this.namespaces) {
      throw new Error("Namespace already exists: " + ns);
    }

    this.namespaces[ns] = {};
  },

  start: function start(port=-1) {
    this.server.registerPrefixHandler("/", this._handleRequest.bind(this));
    this.server.start(port);
    let i = this.server.identity;

    this.serverURI = i.primaryScheme + "://" + i.primaryHost + ":" +
                     i.primaryPort + "/";
    this.port = i.primaryPort;
  },

  stop: function stop(cb) {
    let handler = {onStopped: cb};

    this.server.stop(handler);
  },

  


  _handleRequest: function _handleRequest(request, response) {
    let path = request.path;
    this._log.info("Received request: " + request.method + " " + path + " " +
                   "HTTP/" + request.httpVersion);

    try {
      if (path.startsWith("/1.0/submit/")) {
        return this._handleV1Submit(request, response,
                                    path.substr("/1.0/submit/".length));
      } else {
        throw HTTP_404;
      }
    } catch (ex) {
      if (ex instanceof HttpError) {
        this._log.info("HttpError thrown: " + ex.code + " " + ex.description);
      } else {
        this._log.warn("Exception processing request: " +
                       CommonUtils.exceptionStr(ex));
      }

      throw ex;
    }
  },

  


  _handleV1Submit: function _handleV1Submit(request, response, rest) {
    if (!rest.length) {
      throw HTTP_404;
    }

    let namespace;
    let index = rest.indexOf("/");
    if (index == -1) {
      namespace = rest;
      rest = "";
    } else {
      namespace = rest.substr(0, index);
      rest = rest.substr(index + 1);
    }

    this._handleNamespaceSubmit(namespace, rest, request, response);
  },

  _handleNamespaceSubmit: function _handleNamespaceSubmit(namespace, rest,
                                                          request, response) {
    if (!this.hasNamespace(namespace)) {
      if (!this.allowAllNamespaces) {
        this._log.info("Request to unknown namespace: " + namespace);
        throw HTTP_404;
      }

      this.createNamespace(namespace);
    }

    if (!rest) {
      this._log.info("No ID defined.");
      throw HTTP_404;
    }

    let id = rest;
    if (id.includes("/")) {
      this._log.info("URI has too many components.");
      throw HTTP_404;
    }

    if (request.method == "POST") {
      return this._handleNamespaceSubmitPost(namespace, id, request, response);
    }

    if (request.method == "DELETE") {
      return this._handleNamespaceSubmitDelete(namespace, id, request, response);
    }

    this._log.info("Unsupported HTTP method on namespace handler: " +
                   request.method);
    response.setHeader("Allow", "POST,DELETE");
    throw HTTP_405;
  },

  _handleNamespaceSubmitPost:
    function _handleNamespaceSubmitPost(namespace, id, request, response) {

    this._log.info("Handling data upload for " + namespace + ":" + id);

    let requestBody = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
    this._log.info("Raw body length: " + requestBody.length);

    if (!request.hasHeader("Content-Type")) {
      this._log.info("Request does not have Content-Type header.");
      throw HTTP_400;
    }

    const ALLOWED_TYPES = [
      
      "application/json; charset=utf-8",
      "application/json+zlib; charset=utf-8",
    ];

    let ct = request.getHeader("Content-Type");
    if (ALLOWED_TYPES.indexOf(ct) == -1) {
      this._log.info("Unknown media type: " + ct);
      
      throw HTTP_415;
    }

    if (ct.startsWith("application/json+zlib")) {
      this._log.debug("Uncompressing entity body with deflate.");
      requestBody = CommonUtils.convertString(requestBody, "deflate",
                                              "uncompressed");
    }

    requestBody = CommonUtils.decodeUTF8(requestBody);

    this._log.debug("HTTP request body: " + requestBody);

    let doc;
    try {
      doc = JSON.parse(requestBody);
    } catch(ex) {
      this._log.info("JSON parse error.");
      throw HTTP_400;
    }

    this.namespaces[namespace][id] = doc;

    if (request.hasHeader("X-Obsolete-Document")) {
      let obsolete = request.getHeader("X-Obsolete-Document");
      this._log.info("Deleting from X-Obsolete-Document header: " + obsolete);
      for (let obsolete_id of obsolete.split(",")) {
        delete this.namespaces[namespace][obsolete_id];
      }
    }

    response.setStatusLine(request.httpVersion, 201, "Created");
    response.setHeader("Content-Type", "text/plain");

    let body = id;
    response.bodyOutputStream.write(body, body.length);
  },

  _handleNamespaceSubmitDelete:
    function _handleNamespaceSubmitDelete(namespace, id, request, response) {

    delete this.namespaces[namespace][id];

    let body = id;
    response.bodyOutputStream.write(body, body.length);
  },
};

Object.freeze(BagheeraServer.prototype);
