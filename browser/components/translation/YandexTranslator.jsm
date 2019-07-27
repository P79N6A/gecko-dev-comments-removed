



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "YandexTranslator" ];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Http.jsm");


const MAX_REQUEST_DATA = 5000; 
                               



const MAX_REQUEST_CHUNKS = 1000; 





const MAX_REQUESTS = 15;

const YANDEX_RETURN_CODE_OK = 200;

const YANDEX_ERR_KEY_INVALID               = 401; 
const YANDEX_ERR_KEY_BLOCKED               = 402; 
const YANDEX_ERR_DAILY_REQ_LIMIT_EXCEEDED  = 403; 
const YANDEX_ERR_DAILY_CHAR_LIMIT_EXCEEDED = 404; 
const YANDEX_ERR_TEXT_TOO_LONG             = 413; 
const YANDEX_ERR_UNPROCESSABLE_TEXT        = 422; 
const YANDEX_ERR_LANG_NOT_SUPPORTED        = 501; 


const YANDEX_PERMANENT_ERRORS = [
  YANDEX_ERR_KEY_INVALID,
  YANDEX_ERR_KEY_BLOCKED,
  YANDEX_ERR_DAILY_REQ_LIMIT_EXCEEDED,
  YANDEX_ERR_DAILY_CHAR_LIMIT_EXCEEDED,
];












this.YandexTranslator = function(translationDocument, sourceLanguage, targetLanguage) {
  this.translationDocument = translationDocument;
  this.sourceLanguage = sourceLanguage;
  this.targetLanguage = targetLanguage;
  this._pendingRequests = 0;
  this._partialSuccess = false;
  this._serviceUnavailable = false;
  this._translatedCharacterCount = 0;
};

this.YandexTranslator.prototype = {
  






  translate: function() {
    return Task.spawn(function *() {
      let currentIndex = 0;
      this._onFinishedDeferred = Promise.defer();

      
      
      for (let requestCount = 0; requestCount < MAX_REQUESTS; requestCount++) {
        
        
        
        
        yield CommonUtils.laterTickResolvingPromise();

        
        let request = this._generateNextTranslationRequest(currentIndex);

        
        
        let yandexRequest = new YandexRequest(request.data,
                                          this.sourceLanguage,
                                          this.targetLanguage);
        this._pendingRequests++;
        yandexRequest.fireRequest().then(this._chunkCompleted.bind(this),
                                       this._chunkFailed.bind(this));

        currentIndex = request.lastIndex;
        if (request.finished) {
          break;
        }
      }

      return this._onFinishedDeferred.promise;
    }.bind(this));
  },

  







  _chunkCompleted: function(yandexRequest) {
    if (this._parseChunkResult(yandexRequest)) {
      this._partialSuccess = true;
      
      this._translatedCharacterCount += yandexRequest.characterCount;
    }

    this._checkIfFinished();
  },

  









  _chunkFailed: function(aError) {
    if (aError instanceof Ci.nsIXMLHttpRequest) {
      let body = aError.responseText;
      let json = { code: 0 };
      try {
        json = JSON.parse(body);
      } catch (e) {}

      if (json.code && YANDEX_PERMANENT_ERRORS.indexOf(json.code) != -1)
        this._serviceUnavailable = true;
    }

    this._checkIfFinished();
  },

  




  _checkIfFinished: function() {
    
    
    
    
    
    
    if (--this._pendingRequests == 0) {
      if (this._partialSuccess) {
        this._onFinishedDeferred.resolve({
          characterCount: this._translatedCharacterCount
        });
      } else {
        let error = this._serviceUnavailable ? "unavailable" : "failure";
        this._onFinishedDeferred.reject(error);
      }
    }
  },

  








  _parseChunkResult: function(yandexRequest) {
    let results;
    try {
      let result = JSON.parse(yandexRequest.networkRequest.responseText);
      if (result.code != 200) {
        Services.console.logStringMessage("YandexTranslator: Result is " + result.code);
        return false;
      }
      results = result.text
    } catch (e) {
      return false;
    }

    let len = results.length;
    if (len != yandexRequest.translationData.length) {
      
      
      
      return false;
    }

    let error = false;
    for (let i = 0; i < len; i++) {
      try {
        let result = results[i];
        let root = yandexRequest.translationData[i][0];
        root.parseResult(result);
      } catch (e) { error = true; }
    }

    return !error;
  },

  






  _generateNextTranslationRequest: function(startIndex) {
    let currentDataSize = 0;
    let currentChunks = 0;
    let output = [];
    let rootsList = this.translationDocument.roots;

    for (let i = startIndex; i < rootsList.length; i++) {
      let root = rootsList[i];
      let text = this.translationDocument.generateTextForItem(root);
      if (!text) {
        continue;
      }

      let newCurSize = currentDataSize + text.length;
      let newChunks = currentChunks + 1;

      if (newCurSize > MAX_REQUEST_DATA ||
          newChunks > MAX_REQUEST_CHUNKS) {

        
        
        
        
        return {
          data: output,
          finished: false,
          lastIndex: i
        };
      }

      currentDataSize = newCurSize;
      currentChunks = newChunks;
      output.push([root, text]);
    }

    return {
      data: output,
      finished: true,
      lastIndex: 0
    };
  }
};











function YandexRequest(translationData, sourceLanguage, targetLanguage) {
  this.translationData = translationData;
  this.sourceLanguage = sourceLanguage;
  this.targetLanguage = targetLanguage;
  this.characterCount = 0;
}

YandexRequest.prototype = {
  


  fireRequest: function() {
    return Task.spawn(function *(){
      
      let url = getUrlParam("https://translate.yandex.net/api/v1.5/tr.json/translate",
                            "browser.translation.yandex.translateURLOverride");

      
      let apiKey = getUrlParam("%YANDEX_API_KEY%", "browser.translation.yandex.apiKeyOverride");
      let params = [
        ["key", apiKey],
        ["format", "html"],
        ["lang", this.sourceLanguage + "-" + this.targetLanguage],
      ];

      for (let [, text] of this.translationData) {
        params.push(["text", text]);
        this.characterCount += text.length;
      }

      
      let deferred = Promise.defer();
      let options = {
        onLoad: (function(responseText, xhr) {
          deferred.resolve(this);
        }).bind(this),
        onError: function(e, responseText, xhr) {
          deferred.reject(xhr);
        },
        postData: params
      };

      
      this.networkRequest = httpRequest(url, options);

      return deferred.promise;
    }.bind(this));
  }
};





function getUrlParam(paramValue, prefName) {
  if (Services.prefs.getPrefType(prefName))
    paramValue = Services.prefs.getCharPref(prefName);
  paramValue = Services.urlFormatter.formatURL(paramValue);
  return paramValue;
}
