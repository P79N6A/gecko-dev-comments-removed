


"use strict";

const { Cu, Ci, Cc } = require("chrome");
const { defer, all, resolve } = require("sdk/core/promise");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});

XPCOMUtils.defineLazyGetter(this, "appInfo", function() {
  return Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
});

XPCOMUtils.defineLazyModuleGetter(this, "ViewHelpers",
  "resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyGetter(this, "L10N", function() {
  return new ViewHelpers.L10N("chrome://browser/locale/devtools/har.properties");
});

XPCOMUtils.defineLazyGetter(this, "NetworkHelper", function() {
  return devtools.require("devtools/toolkit/webconsole/network-helper");
});

const HAR_VERSION = "1.1";




















var HarBuilder = function(options) {
  this._options = options;
  this._pageMap = [];
}

HarBuilder.prototype = {
  

  







  build: function() {
    this.promises = [];

    
    let log = this.buildLog();

    
    let items = this._options.items;
    for (let i=0; i<items.length; i++) {
      let file = items[i].attachment;
      log.entries.push(this.buildEntry(log, file));
    }

    
    
    let { resolve, promise } = defer();
    all(this.promises).then(results => resolve({ log: log }));

    return promise;
  },

  

  buildLog: function() {
    return {
      version: HAR_VERSION,
      creator: {
        name: appInfo.name,
        version: appInfo.version
      },
      browser: {
        name: appInfo.name,
        version: appInfo.version
      },
      pages: [],
      entries: [],
    }
  },

  buildPage: function(file) {
    let page = {};

    
    
    page.startedDateTime = 0;
    page.id = "page_" + this._options.id;
    page.title = this._options.title;

    return page;
  },

  getPage: function(log, file) {
    let id = this._options.id;
    let page = this._pageMap[id];
    if (page) {
      return page;
    }

    this._pageMap[id] = page = this.buildPage(file);
    log.pages.push(page);

    return page;
  },

  buildEntry: function(log, file) {
    let page = this.getPage(log, file);

    let entry = {};
    entry.pageref = page.id;
    entry.startedDateTime = dateToJSON(new Date(file.startedMillis));
    entry.time = file.endedMillis - file.startedMillis;

    entry.request = this.buildRequest(file);
    entry.response = this.buildResponse(file);
    entry.cache = this.buildCache(file);
    entry.timings = file.eventTimings ? file.eventTimings.timings : {};

    if (file.remoteAddress) {
      entry.serverIPAddress = file.remoteAddress;
    }

    if (file.remotePort) {
      entry.connection = file.remotePort + "";
    }

    
    if (!page.startedDateTime) {
      page.startedDateTime = entry.startedDateTime;
      page.pageTimings = this.buildPageTimings(page, file);
    }

    return entry;
  },

  buildPageTimings: function(page, file) {
    
    let timings = {
      onContentLoad: -1,
      onLoad: -1
    };

    return timings;
  },

  buildRequest: function(file) {
    let request = {
      bodySize: 0
    };

    request.method = file.method;
    request.url = file.url;
    request.httpVersion = file.httpVersion;

    request.headers = this.buildHeaders(file.requestHeaders);
    request.cookies = this.buildCookies(file.requestCookies);

    request.queryString = NetworkHelper.parseQueryString(
      NetworkHelper.nsIURL(file.url).query) || [];

    request.postData = this.buildPostData(file);

    request.headersSize = file.requestHeaders.headersSize;

    
    
    if (file.requestPostData) {
      this.fetchData(file.requestPostData.postData.text).then(value => {
        request.bodySize = value.length;
      });
    }

    return request;
  },

  





  buildHeaders: function(input) {
    if (!input) {
      return [];
    }

    return this.buildNameValuePairs(input.headers);
  },

  buildCookies: function(input) {
    if (!input) {
      return [];
    }

    return this.buildNameValuePairs(input.cookies);
  },

  buildNameValuePairs: function(entries) {
    let result = [];

    
    
    if (!entries) {
      return result;
    }

    
    entries.forEach(entry => {
      this.fetchData(entry.value).then(value => {
        result.push({
          name: entry.name,
          value: value
        });
      });
    })

    return result;
  },

  buildPostData: function(file) {
    let postData = {
      mimeType: findValue(file.requestHeaders.headers, "content-type"),
      params: [],
      text: ""
    };

    if (!file.requestPostData) {
      return postData;
    }

    if (file.requestPostData.postDataDiscarded) {
      postData.comment = L10N.getStr("har.requestBodyNotIncluded");
      return postData;
    }

    
    this.fetchData(file.requestPostData.postData.text).then(value => {
      postData.text = value;

      
      if (isURLEncodedFile(file, value)) {
        postData.mimeType = "application/x-www-form-urlencoded";

        
        this._options.view._getFormDataSections(file.requestHeaders,
          file.requestHeadersFromUploadStream,
          file.requestPostData).then(formDataSections => {
            formDataSections.forEach(section => {
              let paramsArray = NetworkHelper.parseQueryString(section);
              if (paramsArray) {
                postData.params = [...postData.params, ...paramsArray];
              }
            });
          });
      }
    });

    return postData;
  },

  buildResponse: function(file) {
    let response = {
      status: 0
    };

    
    if (file.status) {
      response.status = parseInt(file.status);
    }

    response.statusText = file.statusText || "";
    response.httpVersion = file.httpVersion;

    response.headers = this.buildHeaders(file.responseHeaders);
    response.cookies = this.buildCookies(file.responseCookies);

    response.content = this.buildContent(file);
    response.redirectURL = findValue(file.responseHeaders.headers, "Location");
    response.headersSize = file.responseHeaders.headersSize;
    response.bodySize = file.transferredSize || -1;

    return response;
  },

  buildContent: function(file) {
    let content = {
      mimeType: file.mimeType,
      size: -1
    };

    if (file.responseContent && file.responseContent.content) {
      content.size = file.responseContent.content.size;
    }

    if (!this._options.includeResponseBodies ||
        file.responseContent.contentDiscarded) {
      content.comment = L10N.getStr("har.responseBodyNotIncluded");
      return content;
    }

    if (file.responseContent) {
      let text = file.responseContent.content.text;
      let promise = this.fetchData(text).then(value => {
        content.text = value;
      });
    }

    return content;
  },

  buildCache: function(file) {
    let cache = {};

    if (!file.fromCache) {
      return cache;
    }

    
    

    if (file.cacheEntry) {
      cache.afterRequest = this.buildCacheEntry(file.cacheEntry);
    } else {
      cache.afterRequest = null;
    }

    return cache;
  },

  buildCacheEntry: function(cacheEntry) {
    let cache = {};

    cache.expires = findValue(cacheEntry, "Expires");
    cache.lastAccess = findValue(cacheEntry, "Last Fetched");
    cache.eTag = "";
    cache.hitCount = findValue(cacheEntry, "Fetch Count");

    return cache;
  },

  getBlockingEndTime: function(file) {
    if (file.resolveStarted && file.connectStarted) {
      return file.resolvingTime;
    }

    if (file.connectStarted) {
      return file.connectingTime;
    }

    if (file.sendStarted) {
      return file.sendingTime;
    }

    return (file.sendingTime > file.startTime) ?
      file.sendingTime : file.waitingForTime;
  },

  

  fetchData: function(string) {
    let promise = this._options.getString(string).then(value => {
      return value;
    });

    
    
    this.promises.push(promise);

    return promise;
  }
}






function isURLEncodedFile(file, text) {
  let contentType = "content-type: application/x-www-form-urlencoded"
  if (text && text.toLowerCase().indexOf(contentType) != -1) {
    return true;
  }

  
  
  
  
  let value = findValue(file.requestHeaders.headers, "content-type");
  if (value && value.indexOf("application/x-www-form-urlencoded") == 0) {
    return true;
  }

  return false;
}





function findValue(arr, name) {
  name = name.toLowerCase();
  let result = arr.find(entry => entry.name.toLowerCase() == name);
  return result ? result.value : "";
}















function dateToJSON(date) {
  function f(n, c) {
    if (!c) {
      c = 2;
    }
    let s = new String(n);
    while (s.length < c) {
      s = "0" + s;
    }
    return s;
  }

  let result = date.getFullYear() + '-' +
    f(date.getMonth() + 1) + '-' +
    f(date.getDate()) + 'T' +
    f(date.getHours()) + ':' +
    f(date.getMinutes()) + ':' +
    f(date.getSeconds()) + '.' +
    f(date.getMilliseconds(), 3);

  let offset = date.getTimezoneOffset();
  let positive = offset > 0;

  
  offset = Math.abs(offset);
  let offsetHours = Math.floor(offset / 60);
  let offsetMinutes = Math.floor(offset % 60);
  let prettyOffset = (positive > 0 ? "-" : "+") + f(offsetHours) +
    ":" + f(offsetMinutes);

  return result + prettyOffset;
}


exports.HarBuilder = HarBuilder;
