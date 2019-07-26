



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = [ "BingTranslation" ];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");


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

  _parseChunkResult() {
    
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




function escapeXML(aStr) {
  return aStr.toString()
             .replace("&", "&amp;", "g")
             .replace('"', "&quot;", "g")
             .replace("'", "&apos;", "g")
             .replace("<", "&lt;", "g")
             .replace(">", "&gt;", "g");
}
