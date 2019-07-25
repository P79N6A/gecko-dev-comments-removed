



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const EXPORTED_SYMBOLS = [
  "AITCServer10User",
  "AITCServer10Server",
];

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/utils.js");







function AITCServer10User() {
  this._log = Log4Moz.repository.getLogger("Services.Common.AITCServer");
  this.apps = {};
}
AITCServer10User.prototype = {
  appRecordProperties: {
    origin:        true,
    manifestPath:  true,
    installOrigin: true,
    installedAt:   true,
    modifiedAt:    true,
    receipts:      true,
    name:          true,
    hidden:        true,
  },

  requiredAppProperties: [
    "origin",
    "manifestPath",
    "installOrigin",
    "installedAt",
    "modifiedAt",
    "name",
    "receipts",
  ],

  





  getApps: function getApps(minimal) {
    let result;

    for (let id in this.apps) {
      let app = this.apps[id];

      if (!minimal) {
        yield app;
        continue;
      }

      yield {origin: app.origin, modifiedAt: app.modifiedAt};
    }
  },

  getAppByID: function getAppByID(id) {
    return this.apps[id];
  },

  




  addApp: function addApp(app) {
    for (let k in app) {
      if (!(k in this.appRecordProperties)) {
        throw new Error("Unexpected property in app record: " + k);
      }
    }

    for each (let k in this.requiredAppProperties) {
      if (!(k in app)) {
        throw new Error("Required property not in app record: " + k);
      }
    }

    this.apps[this.originToID(app.origin)] = app;
  },

  


  hasAppID: function hasAppID(id) {
    return id in this.apps;
  },

  


  deleteAppWithID: function deleteAppWithID(id) {
    delete this.apps[id];
  },

  


  originToID: function originToID(origin) {
    let hash = CryptoUtils.UTF8AndSHA1(origin);
    return CommonUtils.encodeBase64URL(hash, false);
  },
};











function AITCServer10Server() {
  this._log = Log4Moz.repository.getLogger("Services.Common.AITCServer");

  this.server = new HttpServer();
  this.port = null;
  this.users = {};
  this.autoCreateUsers = false;
  this.mockStatus = {
    code: null,
    method: null
  };
  this.onRequest = null;

  this._appsAppHandlers = {
    GET:    this._appsAppGetHandler,
    PUT:    this._appsAppPutHandler,
    DELETE: this._appsAppDeleteHandler,
  };
}
AITCServer10Server.prototype = {
  ID_REGEX: /^[a-zA-Z0-9_-]{27}$/,
  VERSION_PATH: "/1.0/",

  


  get url() {
    
    return "http://localhost:" + this.port + this.VERSION_PATH;
  },

  


  start: function start(port) {
    if (!port) {
      throw new Error("port argument must be specified.");
    }

    this.port = port;

    this.server.registerPrefixHandler(this.VERSION_PATH,
                                      this._generalHandler.bind(this));
    this.server.start(port);
  },

  




  stop: function stop(cb) {
    let handler = {onStopped: cb};

    this.server.stop(handler);
  },

  createUser: function createUser(username) {
    if (username in this.users) {
      throw new Error("User already exists: " + username);
    }

    this._log.info("Registering user: " + username);

    this.users[username] = new AITCServer10User();
    this.server.registerPrefixHandler(this.VERSION_PATH + username + "/",
                                      this._userHandler.bind(this, username));

    return this.users[username];
  },

  





  getUser: function getUser(username) {
    if (!(username in this.users)) {
      throw new Error("user is not present in server: " + username);
    }

    return this.users[username];
  },
  
  


  _respondWithMockStatus: function _respondWithMockStatus(request, response) {
    response.setStatusLine(request.httpVersion, this.mockStatus.code,
      this.mockStatus.method);
    this._onRequest();
  },

  _onRequest: function _onRequest() {
    if (typeof this.onRequest === 'function') {
      this.onRequest();
    }
  },

  



  _generalHandler: function _generalHandler(request, response) {
    if (this.mockStatus.code && this.mockStatus.method) {
      this._respondWithMockStatus(request, response);
      return;
    }
    this._onRequest();
    let path = request.path;
    this._log.info("Request: " + request.method + " " + path);

    if (path.indexOf(this.VERSION_PATH) != 0) {
      throw new Error("generalHandler invoked improperly.");
    }

    let rest = request.path.substr(this.VERSION_PATH.length);
    if (!rest.length) {
      throw HTTP_404;
    }

    if (!this.autoCreateUsers) {
      throw HTTP_404;
    }

    let username;
    let index = rest.indexOf("/");
    if (index == -1) {
      username = rest;
    } else {
      username = rest.substr(0, index);
    }

    this.createUser(username);
    this._userHandler(username, request, response);
  },

  




  _userHandler: function _userHandler(username, request, response) {
    if (this.mockStatus.code && this.mockStatus.method) {
      this._respondWithMockStatus(request, response);
      return;
    }
    this._onRequest();
    this._log.info("Request: " + request.method + " " + request.path);
    let path = request.path;
    let prefix = this.VERSION_PATH + username + "/";

    if (path.indexOf(prefix) != 0) {
      throw new Error("userHandler invoked improperly.");
    }

    let user = this.users[username];
    if (!user) {
      throw new Error("User handler should not have been invoked for an " +
                      "unknown user!");
    }

    let requestTime = Date.now();
    response.dispatchTime = requestTime;
    response.setHeader("X-Timestamp", "" + requestTime);

    let handler;
    let remaining = path.substr(prefix.length);

    if (remaining == "apps" || remaining == "apps/") {
      this._log.info("Dispatching to apps index handler.");
      handler = this._appsIndexHandler.bind(this, user, request, response);
    } else if (!remaining.indexOf("apps/")) {
      let id = remaining.substr("apps/".length);

      this._log.info("Dispatching to app handler.");
      handler = this._appsAppHandler.bind(this, user, id, request, response);
    } else if (remaining == "devices" || !remaining.indexOf("devices/")) {
      this._log.info("Dispatching to devices handler.");
      handler = this._devicesHandler.bind(this, user,
                                          remaining.substr("devices".length),
                                          request, response);
    } else {
      throw HTTP_404;
    }

    try {
      handler();
    } catch (ex) {
      if (ex instanceof HttpError) {
        response.setStatusLine(request.httpVersion, ex.code, ex.description);
        return;
      }

      this._log.warn("Exception when processing request: " +
                     CommonUtils.exceptionStr(ex));
      throw ex;
    }
  },

  _appsIndexHandler: function _appsIndexHandler(user, request, response) {
    if (request.method != "GET") {
      response.setStatusLine(request.httpVersion, 405, "Method Not Allowed");
      response.setHeader("Accept", "GET");

      return;
    }

    let options = this._getQueryStringParams(request);
    for (let key in options) {
      let value = options[key];

      switch (key) {
        case "after":
          let time = parseInt(value, 10);
          if (isNaN(time)) {
            throw HTTP_400;
          }

          options.after = time;
          break;

        case "full":
          
          break;

        default:
          this._log.info("Unknown query string parameter: " + key);
          throw HTTP_400;
      }
    }

    let apps = [];
    let newest = 0;
    for each (let app in user.getApps(!("full" in options))) {
      if (app.modifiedAt > newest) {
        newest = app.modifiedAt;
      }

      if ("after" in options && app.modifiedAt <= options.after) {
        continue;
      }

      apps.push(app);
    }

    if (request.hasHeader("X-If-Modified-Since")) {
      let modified = parseInt(request.getHeader("X-If-Modified-Since"), 10);
      if (modified >= newest) {
        response.setStatusLine(request.httpVersion, 304, "Not Modified");
        return;
      }
    }

    let body = JSON.stringify({apps: apps});
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("X-Last-Modified", "" + newest);
    response.setHeader("Content-Type", "application/json");
    response.bodyOutputStream.write(body, body.length);
  },

  _appsAppHandler: function _appAppHandler(user, id, request, response) {
    if (!(request.method in this._appsAppHandlers)) {
      response.setStatusLine(request.httpVersion, 405, "Method Not Allowed");
      response.setHeader("Accept", Object.keys(this._appsAppHandlers).join(","));

      return;
    }

    let handler = this._appsAppHandlers[request.method];
    return handler.call(this, user, id, request, response);
  },

  _appsAppGetHandler: function _appsAppGetHandler(user, id, request, response) {
    if (!user.hasAppID(id)) {
      throw HTTP_404;
    }

    let app = user.getAppByID(id);

    if (request.hasHeader("X-If-Modified-Since")) {
      let modified = parseInt(request.getHeader("X-If-Modified-Since"), 10);

      this._log.debug("Client time: " + modified + "; Server time: " +
                      app.modifiedAt);

      if (modified >= app.modifiedAt) {
        response.setStatusLine(request.httpVersion, 304, "Not Modified");
        return;
      }
    }

    let body = JSON.stringify(app);
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("X-Last-Modified", "" + response.dispatchTime);
    response.setHeader("Content-Type", "application/json");
    response.bodyOutputStream.write(body, body.length);
  },

  _appsAppPutHandler: function _appsAppPutHandler(user, id, request, response) {
    if (!request.hasHeader("Content-Type")) {
      this._log.info("Request does not have Content-Type header.");
      throw HTTP_400;
    }

    let ct = request.getHeader("Content-Type");
    if (ct != "application/json" && ct.indexOf("application/json;") !== 0) {
      this._log.info("Unknown media type: " + ct);
      
      throw HTTP_415;
    }

    let requestBody = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
    this._log.debug("Request body: " + requestBody);
    if (requestBody.length > 8192) {
      this._log.info("Request body too long: " + requestBody.length);
      throw HTTP_413;
    }

    let hadApp = user.hasAppID(id);

    let app;
    try {
      app = JSON.parse(requestBody);
    } catch (e) {
      this._log.info("JSON parse error.");
      throw HTTP_400;
    }

    
    if (user.originToID(app.origin) != id) {
      this._log.warn("URL ID and origin mismatch. URL: " + id + "; Record: " +
                     user.originToID(app.origin));
      throw HTTP_403;
    }

    if (request.hasHeader("X-If-Unmodified-Since") && hadApp) {
      let modified = parseInt(request.getHeader("X-If-Unmodified-Since"), 10);
      let existing = user.getAppByID(id);

      if (existing.modifiedAt > modified) {
        this._log.info("Server modified after client.");
        throw HTTP_412;
      }
    }

    try {
      app.modifiedAt = response.dispatchTime;

      if (hadApp) {
        app.installedAt = user.getAppByID(id).installedAt;
      } else {
        app.installedAt = response.dispatchTime;
      }

      user.addApp(app);
    } catch (e) {
      this._log.info("Error adding app: " + CommonUtils.exceptionStr(e));
      throw HTTP_400;
    }

    let code = 201;
    let status = "Created";

    if (hadApp) {
      code = 204;
      status = "No Content";
    }

    response.setHeader("X-Last-Modified", "" + response.dispatchTime);
    response.setStatusLine(request.httpVersion, code, status);
  },

  _appsAppDeleteHandler: function _appsAppDeleteHandler(user, id, request,
                                                        response) {
    if (!user.hasAppID(id)) {
      throw HTTP_404;
    }

    let existing = user.getAppByID(id);
    if (request.hasHeader("X-If-Unmodified-Since")) {
      let modified = parseInt(request.getHeader("X-If-Unmodified-Since"), 10);

      if (existing.modifiedAt > modified) {
        throw HTTP_412;
      }
    }

    user.deleteAppWithID(id);

    response.setHeader("X-Last-Modified", "" + response.dispatchTime);
    response.setStatusLine(request.httpVersion, 204, "No Content");
  },

  _devicesHandler: function _devicesHandler(user, path, request, response) {
    
    
    response.setHeader("Content-Type", "application/json");
    let body = JSON.stringify({devices: []});

    response.setStatusLine(request.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
  },

  
  _getQueryStringParams: function _getQueryStringParams(request) {
    let params = {};
    for each (let chunk in request.queryString.split("&")) {
      if (!chunk) {
        continue;
      }

      let parts = chunk.split("=");
      
      if (parts.length == 1) {
        params[parts[0]] = "";
      } else {
        params[parts[0]] = parts[1];
      }
    }

    return params;
  },
};

