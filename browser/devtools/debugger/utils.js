


"use strict";

const XULUtils = {
  









  addCommands: function(commandset, commands) {
    Object.keys(commands).forEach(name => {
      let node = document.createElement('command');
      node.id = name;
      
      
      node.setAttribute('oncommand', ' ');
      node.addEventListener('command', commands[name]);
      commandset.appendChild(node);
    });
  }
};


const SAMPLE_SIZE = 50; 
const INDENT_COUNT_THRESHOLD = 5; 
const CHARACTER_LIMIT = 250; 




const SourceUtils = {
  _labelsCache: new Map(), 
  _groupsCache: new Map(),
  _minifiedCache: new WeakMap(),

  






  isJavaScript: function(aUrl, aContentType = "") {
    return (aUrl && /\.jsm?$/.test(this.trimUrlQuery(aUrl))) ||
           aContentType.contains("javascript");
  },

  






  isMinified: Task.async(function*(sourceClient) {
    if (this._minifiedCache.has(sourceClient)) {
      return this._minifiedCache.get(sourceClient);
    }

    let [, text] = yield DebuggerController.SourceScripts.getText(sourceClient);
    let isMinified;
    let lineEndIndex = 0;
    let lineStartIndex = 0;
    let lines = 0;
    let indentCount = 0;
    let overCharLimit = false;

    
    text = text.replace(/\/\*[\S\s]*?\*\/|\/\/(.+|\n)/g, "");

    while (lines++ < SAMPLE_SIZE) {
      lineEndIndex = text.indexOf("\n", lineStartIndex);
      if (lineEndIndex == -1) {
         break;
      }
      if (/^\s+/.test(text.slice(lineStartIndex, lineEndIndex))) {
        indentCount++;
      }
      
      if ((lineEndIndex - lineStartIndex) > CHARACTER_LIMIT) {
        overCharLimit = true;
        break;
      }
      lineStartIndex = lineEndIndex + 1;
    }

    isMinified =
      ((indentCount / lines) * 100) < INDENT_COUNT_THRESHOLD || overCharLimit;

    this._minifiedCache.set(sourceClient, isMinified);
    return isMinified;
  }),

  




  clearCache: function() {
    this._labelsCache.clear();
    this._groupsCache.clear();
    this._minifiedCache.clear();
  },

  







  getSourceLabel: function(aUrl) {
    let cachedLabel = this._labelsCache.get(aUrl);
    if (cachedLabel) {
      return cachedLabel;
    }

    let sourceLabel = null;

    for (let name of Object.keys(KNOWN_SOURCE_GROUPS)) {
      if (aUrl.startsWith(KNOWN_SOURCE_GROUPS[name])) {
        sourceLabel = aUrl.substring(KNOWN_SOURCE_GROUPS[name].length);
      }
    }

    if (!sourceLabel) {
      sourceLabel = this.trimUrl(aUrl);
    }

    let unicodeLabel = NetworkHelper.convertToUnicode(unescape(sourceLabel));
    this._labelsCache.set(aUrl, unicodeLabel);
    return unicodeLabel;
  },

  








  getSourceGroup: function(aUrl) {
    let cachedGroup = this._groupsCache.get(aUrl);
    if (cachedGroup) {
      return cachedGroup;
    }

    try {
      
      var uri = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
    } catch (e) {
      
      return "";
    }

    let groupLabel = uri.prePath;

    for (let name of Object.keys(KNOWN_SOURCE_GROUPS)) {
      if (aUrl.startsWith(KNOWN_SOURCE_GROUPS[name])) {
        groupLabel = name;
      }
    }

    let unicodeLabel = NetworkHelper.convertToUnicode(unescape(groupLabel));
    this._groupsCache.set(aUrl, unicodeLabel)
    return unicodeLabel;
  },

  












  trimUrlLength: function(aUrl, aLength, aSection) {
    aLength = aLength || SOURCE_URL_DEFAULT_MAX_LENGTH;
    aSection = aSection || "end";

    if (aUrl.length > aLength) {
      switch (aSection) {
        case "start":
          return L10N.ellipsis + aUrl.slice(-aLength);
          break;
        case "center":
          return aUrl.substr(0, aLength / 2 - 1) + L10N.ellipsis + aUrl.slice(-aLength / 2 + 1);
          break;
        case "end":
          return aUrl.substr(0, aLength) + L10N.ellipsis;
          break;
      }
    }
    return aUrl;
  },

  







  trimUrlQuery: function(aUrl) {
    let length = aUrl.length;
    let q1 = aUrl.indexOf('?');
    let q2 = aUrl.indexOf('&');
    let q3 = aUrl.indexOf('#');
    let q = Math.min(q1 != -1 ? q1 : length,
                     q2 != -1 ? q2 : length,
                     q3 != -1 ? q3 : length);

    return aUrl.slice(0, q);
  },

  












  trimUrl: function(aUrl, aLabel, aSeq) {
    if (!(aUrl instanceof Ci.nsIURL)) {
      try {
        
        aUrl = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
      } catch (e) {
        
        return aUrl;
      }
    }
    if (!aSeq) {
      let name = aUrl.fileName;
      if (name) {
        
        

        
        
        aLabel = aUrl.fileName.replace(/\&.*/, "");
      } else {
        
        
        aLabel = "";
      }
      aSeq = 1;
    }

    
    if (aLabel && aLabel.indexOf("?") != 0) {
      
      
      if (!DebuggerView.Sources.getItemForAttachment(e => e.label == aLabel)) {
        return aLabel;
      }
    }

    
    if (aSeq == 1) {
      let query = aUrl.query;
      if (query) {
        return this.trimUrl(aUrl, aLabel + "?" + query, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 2) {
      let ref = aUrl.ref;
      if (ref) {
        return this.trimUrl(aUrl, aLabel + "#" + aUrl.ref, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 3) {
      let dir = aUrl.directory;
      if (dir) {
        return this.trimUrl(aUrl, dir.replace(/^\//, "") + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 4) {
      let host = aUrl.hostPort;
      if (host) {
        return this.trimUrl(aUrl, host + "/" + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq == 5) {
      return this.trimUrl(aUrl, aUrl.specIgnoringRef, aSeq + 1);
    }
    
    return aUrl.spec;
  }
};

