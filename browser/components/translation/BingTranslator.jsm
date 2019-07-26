



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "BingTranslation" ];

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/rest.js");


const MAX_REQUEST_DATA = 5000; 
                               



const MAX_REQUEST_CHUNKS = 1000; 





const MAX_REQUESTS = 15;












this.BingTranslation = function(translationDocument, sourceLanguage, targetLanguage) {
  this.translationDocument = translationDocument;
  this.sourceLanguage = sourceLanguage;
  this.targetLanguage = targetLanguage;
  this._pendingRequests = 0;
  this._partialSuccess = false;
};

this.BingTranslation.prototype = {
  






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
        bingRequest.fireRequest().then(this._chunkCompleted.bind(this));

        currentIndex = request.lastIndex;
        if (request.finished) {
          break;
        }
      }

      return this._onFinishedDeferred.promise;
    }.bind(this));
  },

  







  _chunkCompleted: function(bingRequest) {
     this._pendingRequests--;
     if (bingRequest.requestSucceeded &&
         this._parseChunkResult(bingRequest)) {
       
       this._partialSuccess = true;
     }

    
    
    
    
    
    
    if (this._pendingRequests == 0) {
      if (this._partialSuccess) {
        this._onFinishedDeferred.resolve("success");
      } else {
        this._onFinishedDeferred.reject("failure");
      }
    }
  },

  









  _parseChunkResult: function(bingRequest) {
    let domParser = Cc["@mozilla.org/xmlextras/domparser;1"]
                      .createInstance(Ci.nsIDOMParser);

    let results;
    try {
      let doc = domParser.parseFromString(bingRequest.networkRequest
                                                     .response.body, "text/xml");
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
        bingRequest.translationData[i][0].parseResult(
          results[i].firstChild.nodeValue
        );
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
}

BingRequest.prototype = {
  


  fireRequest: function() {
    return Task.spawn(function *(){
      let token = yield BingTokenManager.getToken();
      let auth = "Bearer " + token;
      let request = new RESTRequest("https://api.microsofttranslator.com/v2/Http.svc/TranslateArray");
      request.setHeader("Content-type", "text/xml");
      request.setHeader("Authorization", auth);

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
      }

      requestString += '</Texts>' +
          '<To>' + this.targetLanguage + '</To>' +
        '</TranslateArrayRequest>';

      let utf8 = CommonUtils.encodeUTF8(requestString);

      let deferred = Promise.defer();
      request.post(utf8, function(err) {
        deferred.resolve(this);
      }.bind(this));

      this.networkRequest = request;
      return deferred.promise;
    }.bind(this));
  },

  





  get requestSucceeded() {
    return !this.networkRequest.error &&
            this.networkRequest.response.success;
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
    let request = new RESTRequest("https://datamarket.accesscontrol.windows.net/v2/OAuth2-13");
    request.setHeader("Content-type", "application/x-www-form-urlencoded");
    let params = [
      "grant_type=client_credentials",
      "scope=" + encodeURIComponent("http://api.microsofttranslator.com"),
      "client_id=",
      "client_secret="
    ];

    let deferred = Promise.defer();
    this._pendingRequest = deferred.promise;
    request.post(params.join("&"), function(err) {
      this._pendingRequest = null;

      if (err) {
        deferred.reject(err);
      }

      try {
        let json = JSON.parse(this.response.body);
        let token = json.access_token;
        let expires_in = json.expires_in;
        BingTokenManager._currentToken = token;
        BingTokenManager._currentExpiryTime = new Date(Date.now() + expires_in * 1000);
        deferred.resolve(token);
      } catch (e) {
        deferred.reject(e);
      }
    });

    return deferred.promise;
  }
};




function escapeXML(aStr) {
  return aStr.toString()
             .replace("&", "&amp;", "g")
             .replace('"', "&quot;", "g")
             .replace("'", "&apos;", "g")
             .replace("<", "&lt;", "g")
             .replace(">", "&gt;", "g");
}
