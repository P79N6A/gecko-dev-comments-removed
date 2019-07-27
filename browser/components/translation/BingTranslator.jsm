



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "BingTranslator" ];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Http.jsm");


const MAX_REQUEST_DATA = 5000; 
                               



const MAX_REQUEST_CHUNKS = 1000; 





const MAX_REQUESTS = 15;












this.BingTranslator = function(translationDocument, sourceLanguage, targetLanguage) {
  this.translationDocument = translationDocument;
  this.sourceLanguage = sourceLanguage;
  this.targetLanguage = targetLanguage;
  this._pendingRequests = 0;
  this._partialSuccess = false;
  this._serviceUnavailable = false;
  this._translatedCharacterCount = 0;
};

this.BingTranslator.prototype = {
  






  translate: function() {
    return Task.spawn(function *() {
      let currentIndex = 0;
      this._onFinishedDeferred = Promise.defer();

      
      
      for (let requestCount = 0; requestCount < MAX_REQUESTS; requestCount++) {
        
        
        
        
        yield CommonUtils.laterTickResolvingPromise();

        
        let request = this._generateNextTranslationRequest(currentIndex);

        
        
        let bingRequest = new BingRequest(request.data,
                                          this.sourceLanguage,
                                          this.targetLanguage);
        this._pendingRequests++;
        bingRequest.fireRequest().then(this._chunkCompleted.bind(this),
                                       this._chunkFailed.bind(this));

        currentIndex = request.lastIndex;
        if (request.finished) {
          break;
        }
      }

      return this._onFinishedDeferred.promise;
    }.bind(this));
  },

  



  _resetToken : function() {
    
    BingTokenManager._currentExpiryTime = 0;
  },

  







  _chunkCompleted: function(bingRequest) {
    if (this._parseChunkResult(bingRequest)) {
      this._partialSuccess = true;
      
      this._translatedCharacterCount += bingRequest.characterCount;
    }

    this._checkIfFinished();
  },

  









  _chunkFailed: function(aError) {
    if (aError instanceof Ci.nsIXMLHttpRequest &&
        [400, 401].indexOf(aError.status) != -1) {
      let body = aError.responseText;
      if (body && body.includes("TranslateApiException") &&
          (body.includes("balance") || body.includes("active state")))
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

  









  _parseChunkResult: function(bingRequest) {
    let results;
    try {
      let doc = bingRequest.networkRequest.responseXML;
      results = doc.querySelectorAll("TranslatedText");
    } catch (e) {
      return false;
    }

    let len = results.length;
    if (len != bingRequest.translationData.length) {
      
      
      
      return false;
    }

    let error = false;
    for (let i = 0; i < len; i++) {
      try {
        let result = results[i].firstChild.nodeValue;
        let root = bingRequest.translationData[i][0];

        if (root.isSimpleRoot) {
          
          
          result = result.replace(/&amp;/g, "&");
        }

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

      text = escapeXML(text);
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











function BingRequest(translationData, sourceLanguage, targetLanguage) {
  this.translationData = translationData;
  this.sourceLanguage = sourceLanguage;
  this.targetLanguage = targetLanguage;
  this.characterCount = 0;
}

BingRequest.prototype = {
  


  fireRequest: function() {
    return Task.spawn(function *(){
      
      let token = yield BingTokenManager.getToken();
      let auth = "Bearer " + token;

      
      let url = getUrlParam("https://api.microsofttranslator.com/v2/Http.svc/TranslateArray",
                            "browser.translation.bing.translateArrayURL");

      
      let headers = [["Content-type", "text/xml"], ["Authorization", auth]];

      
      let requestString =
        '<TranslateArrayRequest>' +
          '<AppId/>' +
          '<From>' + this.sourceLanguage + '</From>' +
          '<Options>' +
            '<ContentType xmlns="http://schemas.datacontract.org/2004/07/Microsoft.MT.Web.Service.V2">text/html</ContentType>' +
            '<ReservedFlags xmlns="http://schemas.datacontract.org/2004/07/Microsoft.MT.Web.Service.V2" />' +
          '</Options>' +
          '<Texts xmlns:s="http://schemas.microsoft.com/2003/10/Serialization/Arrays">';

      for (let [, text] of this.translationData) {
        requestString += '<s:string>' + text + '</s:string>';
        this.characterCount += text.length;
      }

      requestString += '</Texts>' +
          '<To>' + this.targetLanguage + '</To>' +
        '</TranslateArrayRequest>';

      
      let deferred = Promise.defer();
      let options = {
        onLoad: (function(responseText, xhr) {
          deferred.resolve(this);
        }).bind(this),
        onError: function(e, responseText, xhr) {
          deferred.reject(xhr);
        },
        postData: requestString,
        headers: headers
      };

      
      let request = httpRequest(url, options);

      
      request.overrideMimeType("text/xml");
      this.networkRequest = request;
      return deferred.promise;
    }.bind(this));
  }
};




let BingTokenManager = {
  _currentToken: null,
  _currentExpiryTime: 0,
  _pendingRequest: null,

  







  getToken: function() {
    if (this._pendingRequest) {
      return this._pendingRequest;
    }

    let remainingMs = this._currentExpiryTime - new Date();
    
    if (remainingMs > 60 * 1000) {
      return Promise.resolve(this._currentToken);
    }

    return this._getNewToken();
  },

  





  _getNewToken: function() {
    let url = getUrlParam("https://datamarket.accesscontrol.windows.net/v2/OAuth2-13",
                          "browser.translation.bing.authURL");
    let params = [
      ["grant_type", "client_credentials"],
      ["scope", "http://api.microsofttranslator.com"],
      ["client_id",
      getUrlParam("%BING_API_CLIENTID%", "browser.translation.bing.clientIdOverride")],
      ["client_secret",
      getUrlParam("%BING_API_KEY%", "browser.translation.bing.apiKeyOverride")]
    ];

    let deferred = Promise.defer();
    let options = {
      onLoad: function(responseText, xhr) {
        BingTokenManager._pendingRequest = null;
        try {
          let json = JSON.parse(responseText);

          if (json.error) {
            deferred.reject(json.error);
            return;
          }

          let token = json.access_token;
          let expires_in = json.expires_in;
          BingTokenManager._currentToken = token;
          BingTokenManager._currentExpiryTime = new Date(Date.now() + expires_in * 1000);
          deferred.resolve(token);
        } catch (e) {
          deferred.reject(e);
        }
      },
      onError: function(e, responseText, xhr) {
        BingTokenManager._pendingRequest = null;
        deferred.reject(e);
      },
      postData: params
    };

    this._pendingRequest = deferred.promise;
    let request = httpRequest(url, options);

    return deferred.promise;
  }
};




function escapeXML(aStr) {
  return aStr.toString()
             .replace(/&/g, "&amp;")
             .replace(/\"/g, "&quot;")
             .replace(/\'/g, "&apos;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;");
}





function getUrlParam(paramValue, prefName) {
  if (Services.prefs.getPrefType(prefName))
    paramValue = Services.prefs.getCharPref(prefName);
  paramValue = Services.urlFormatter.formatURL(paramValue);
  return paramValue;
}
