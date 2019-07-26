



"use strict";

module.metadata = {
  "stability": "stable"
};

const { ns } = require("./core/namespace");
const { emit } = require("./event/core");
const { merge } = require("./util/object");
const { stringify } = require("./querystring");
const { EventTarget } = require("./event/target");
const { Class } = require("./core/heritage");
const { XMLHttpRequest } = require("./net/xhr");
const apiUtils = require("./deprecated/api-utils");

const response = ns();
const request = ns();



const { validateOptions, validateSingleOption } = new OptionsValidator({
  url: {
    
    is:  ["string"]
  },
  headers: {
    map: function (v) v || {},
    is:  ["object"],
  },
  content: {
    map: function (v) v || null,
    is:  ["string", "object", "null"],
  },
  contentType: {
    map: function (v) v || "application/x-www-form-urlencoded",
    is:  ["string"],
  },
  overrideMimeType: {
    map: function(v) v || null,
    is: ["string", "null"],
  }
});

const REUSE_ERROR = "This request object has been used already. You must " +
                    "create a new one to make a new request."



function runRequest(mode, target) {
  let source = request(target)
  let { xhr, url, content, contentType, headers, overrideMimeType } = source;

  
  
  if (xhr)
    throw new Error(REUSE_ERROR);

  xhr = source.xhr = new XMLHttpRequest();

  
  
  let data = stringify(content);
  
  if (mode == "GET" && data)
    url = url + (/\?/.test(url) ? "&" : "?") + data;

  
  xhr.open(mode, url);

  xhr.forceAllowThirdPartyCookie();

  
  xhr.setRequestHeader("Content-Type", contentType);

  
  Object.keys(headers).forEach(function(name) {
    xhr.setRequestHeader(name, headers[name]);
  });

  
  if (overrideMimeType)
    xhr.overrideMimeType(overrideMimeType);

  
  xhr.onreadystatechange = function onreadystatechange() {
    if (xhr.readyState === 4) {
      let response = Response(xhr);
      source.response = response;
      emit(target, 'complete', response);
    }
  };

  
  
  xhr.send(mode !== "GET" ? data : null);
}

const Request = Class({
  extends: EventTarget,
  initialize: function initialize(options) {
    
    
    
    EventTarget.prototype.initialize.call(this, options);

    
    merge(request(this), validateOptions(options));
  },
  get url() { return request(this).url; },
  set url(value) { request(this).url = validateSingleOption('url', value); },
  get headers() { return request(this).headers; },
  set headers(value) {
    return request(this).headers = validateSingleOption('headers', value);
  },
  get content() { return request(this).content; },
  set content(value) {
    request(this).content = validateSingleOption('content', value);
  },
  get contentType() { return request(this).contentType; },
  set contentType(value) {
    request(this).contentType = validateSingleOption('contentType', value);
  },
  get response() { return request(this).response; },
  get: function() {
    runRequest('GET', this);
    return this;
  },
  post: function() {
    runRequest('POST', this);
    return this;
  },
  put: function() {
    runRequest('PUT', this);
    return this;
  }
});
exports.Request = Request;

const Response = Class({
  initialize: function initialize(request) {
    response(this).request = request;
  },
  get text() response(this).request.responseText,
  get xml() {
    throw new Error("Sorry, the 'xml' property is no longer available. " +
                    "see bug 611042 for more information.");
  },
  get status() response(this).request.status,
  get statusText() response(this).request.statusText,
  get json() {
    try {
      return JSON.parse(this.text);
    } catch(error) {
      return null;
    }
  },
  get headers() {
    let headers = {}, lastKey;
    
    
    let rawHeaders = response(this).request.getAllResponseHeaders() || "";
    rawHeaders.split("\n").forEach(function (h) {
      
      
      if (!h.length) {
        return;
      }

      let index = h.indexOf(":");
      
      
      let key = h.substring(0, index).trim(),
          val = h.substring(index + 1).trim();

      
      
      
      
      if (key) {
        headers[key] = val;
        lastKey = key;
      }
      else {
        headers[lastKey] += "\n" + val;
      }
    });
    return headers;
  }
});



function OptionsValidator(rules) {
  return {
    validateOptions: function (options) {
      return apiUtils.validateOptions(options, rules);
    },
    validateSingleOption: function (field, value) {
      
      
      let singleRule = {};
      if (field in rules) {
        singleRule[field] = rules[field];
      }
      let singleOption = {};
      singleOption[field] = value;
      
      return apiUtils.validateOptions(singleOption, singleRule)[field];
    }
  };
}
