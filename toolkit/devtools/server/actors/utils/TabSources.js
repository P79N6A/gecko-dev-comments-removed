



"use strict";

const { Ci, Cu } = require("chrome");
const Services = require("Services");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dbg_assert, fetch } = DevToolsUtils;
const EventEmitter = require("devtools/toolkit/event-emitter");
const { OriginalLocation, GeneratedLocation, getOffsetColumn } = require("devtools/server/actors/common");
const { resolve } = Promise;

loader.lazyRequireGetter(this, "SourceActor", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "isEvalSource", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "SourceMapConsumer", "source-map", true);
loader.lazyRequireGetter(this, "SourceMapGenerator", "source-map", true);





function TabSources(threadActor, allowSourceFn=() => true) {
  EventEmitter.decorate(this);

  this._thread = threadActor;
  this._useSourceMaps = true;
  this._autoBlackBox = true;
  this._anonSourceMapId = 1;
  this.allowSource = source => {
    return !isHiddenSource(source) && allowSourceFn(source);
  }

  this.blackBoxedSources = new Set();
  this.prettyPrintedSources = new Map();

  
  this._sourceMaps = new Map();
  
  this._sourceMapCache = Object.create(null);
  
  this._sourceActors = new Map();
  
  this._sourceMappedSourceActors = Object.create(null);
}






const MINIFIED_SOURCE_REGEXP = /\bmin\.js$/;

TabSources.prototype = {
  


  reconfigure: function(options) {
    if('useSourceMaps' in options) {
      this._useSourceMaps = options.useSourceMaps;
    }

    if('autoBlackBox' in options) {
      this._autoBlackBox = options.autoBlackBox;
    }

    this.reset();
  },

  






  reset: function(opts={}) {
    this._sourceActors = new Map();
    this._sourceMaps = new Map();
    this._sourceMappedSourceActors = Object.create(null);

    if(opts.sourceMaps) {
      this._sourceMapCache = Object.create(null);
    }
  },

  















  source: function  ({ source, originalUrl, generatedSource,
              isInlineSource, contentType }) {
    dbg_assert(source || (originalUrl && generatedSource),
               "TabSources.prototype.source needs an originalUrl or a source");

    if (source) {
      
      

      if (!this.allowSource(source)) {
        return null;
      }

      
      
      
      
      
      if (source.url in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[source.url];
      }

      if (isInlineSource) {
        
        
        
        
        originalUrl = source.url;
        source = null;
      }
      else if (this._sourceActors.has(source)) {
        return this._sourceActors.get(source);
      }
    }
    else if (originalUrl) {
      
      
      
      
      for (let [source, actor] of this._sourceActors) {
        if (source.url === originalUrl) {
          return actor;
        }
      }

      if (originalUrl in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[originalUrl];
      }
    }

    let actor = new SourceActor({
      thread: this._thread,
      source: source,
      originalUrl: originalUrl,
      generatedSource: generatedSource,
      contentType: contentType
    });

    let sourceActorStore = this._thread.sourceActorStore;
    var id = sourceActorStore.getReusableActorId(source, originalUrl);
    if (id) {
      actor.actorID = id;
    }

    this._thread.threadLifetimePool.addActor(actor);
    sourceActorStore.setReusableActorId(source, originalUrl, actor.actorID);

    if (this._autoBlackBox && this._isMinifiedURL(actor.url)) {
      this.blackBox(actor.url);
    }

    if (source) {
      this._sourceActors.set(source, actor);
    }
    else {
      this._sourceMappedSourceActors[originalUrl] = actor;
    }

    this._emitNewSource(actor);
    return actor;
  },

  _emitNewSource: function(actor) {
    if(!actor.source) {
      
      
      
      this.emit('newSource', actor);
    }
    else {
      
      
      
      
      
      
      
      
      this.fetchSourceMap(actor.source).then(map => {
        if(!map) {
          this.emit('newSource', actor);
        }
      });
    }
  },

  getSourceActor: function(source) {
    if (source.url in this._sourceMappedSourceActors) {
      return this._sourceMappedSourceActors[source.url];
    }

    if (this._sourceActors.has(source)) {
      return this._sourceActors.get(source);
    }

    throw new Error('getSource: could not find source actor for ' +
                    (source.url || 'source'));
  },

  getSourceActorByURL: function(url) {
    if (url) {
      for (let [source, actor] of this._sourceActors) {
        if (source.url === url) {
          return actor;
        }
      }

      if (url in this._sourceMappedSourceActors) {
        return this._sourceMappedSourceActors[url];
      }
    }

    throw new Error('getSourceByURL: could not find source for ' + url);
  },

  







  _isMinifiedURL: function (aURL) {
    try {
      let url = Services.io.newURI(aURL, null, null)
                           .QueryInterface(Ci.nsIURL);
      return MINIFIED_SOURCE_REGEXP.test(url.fileName);
    } catch (e) {
      
      
      return MINIFIED_SOURCE_REGEXP.test(aURL);
    }
  },

  








  createNonSourceMappedActor: function (aSource) {
    
    
    
    
    
    
    let url = isEvalSource(aSource) ? null : aSource.url;
    let spec = { source: aSource };

    
    
    
    
    
    
    if (url) {
      try {
        let urlInfo = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
        if (urlInfo.fileExtension === "html" || urlInfo.fileExtension === "xml") {
          spec.isInlineSource = true;
        }
        else if (urlInfo.fileExtension === "js") {
          spec.contentType = "text/javascript";
        }
      } catch(ex) {
        

        
        
        if (url.indexOf("javascript:") === 0) {
          spec.contentType = "text/javascript";
        }
      }
    }
    else {
      
      spec.contentType = "text/javascript";
    }

    return this.source(spec);
  },

  










  _createSourceMappedActors: function (aSource) {
    if (!this._useSourceMaps || !aSource.sourceMapURL) {
      return resolve(null);
    }

    return this.fetchSourceMap(aSource)
      .then(map => {
        if (map) {
          return [
            this.source({ originalUrl: s, generatedSource: aSource })
            for (s of map.sources)
          ].filter(isNotNull);
        }
        return null;
      });
  },

  









  createSourceActors: function(aSource) {
    return this._createSourceMappedActors(aSource).then(actors => {
      let actor = this.createNonSourceMappedActor(aSource);
      return (actors || [actor]).filter(isNotNull);
    });
  },

  









  fetchSourceMap: function (aSource) {
    if (this._sourceMaps.has(aSource)) {
      return this._sourceMaps.get(aSource);
    }
    else if (!aSource || !aSource.sourceMapURL) {
      return resolve(null);
    }

    let sourceMapURL = aSource.sourceMapURL;
    if (aSource.url) {
      sourceMapURL = this._normalize(sourceMapURL, aSource.url);
    }
    let result = this._fetchSourceMap(sourceMapURL, aSource.url);

    
    
    this._sourceMaps.set(aSource, result);
    return result;
  },

  




  getSourceMap: function(aSource) {
    return resolve(this._sourceMaps.get(aSource));
  },

  



  setSourceMap: function(aSource, aMap) {
    this._sourceMaps.set(aSource, resolve(aMap));
  },

  












  _fetchSourceMap: function (aAbsSourceMapURL, aSourceURL) {
    if (!this._useSourceMaps) {
      return resolve(null);
    }
    else if (this._sourceMapCache[aAbsSourceMapURL]) {
      return this._sourceMapCache[aAbsSourceMapURL];
    }

    let fetching = fetch(aAbsSourceMapURL, { loadFromCache: false })
      .then(({ content }) => {
        let map = new SourceMapConsumer(content);
        this._setSourceMapRoot(map, aAbsSourceMapURL, aSourceURL);
        return map;
      })
      .then(null, error => {
        if (!DevToolsUtils.reportingDisabled) {
          DevToolsUtils.reportException("TabSources.prototype._fetchSourceMap", error);
        }
        return null;
      });
    this._sourceMapCache[aAbsSourceMapURL] = fetching;
    return fetching;
  },

  


  _setSourceMapRoot: function (aSourceMap, aAbsSourceMapURL, aScriptURL) {
    const base = this._dirname(
      aAbsSourceMapURL.indexOf("data:") === 0
        ? aScriptURL
        : aAbsSourceMapURL);
    aSourceMap.sourceRoot = aSourceMap.sourceRoot
      ? this._normalize(aSourceMap.sourceRoot, base)
      : base;
  },

  _dirname: function (aPath) {
    return Services.io.newURI(
      ".", null, Services.io.newURI(aPath, null, null)).spec;
  },

  














  clearSourceMapCache: function(aSourceMapURL, opts = { hard: false }) {
    let oldSm = this._sourceMapCache[aSourceMapURL];

    if (opts.hard) {
      delete this._sourceMapCache[aSourceMapURL];
    }

    if (oldSm) {
      
      for (let [source, sm] of this._sourceMaps.entries()) {
        if (sm === oldSm) {
          this._sourceMaps.delete(source);
        }
      }
    }
  },

  













  setSourceMapHard: function(aSource, aUrl, aMap) {
    let url = aUrl;
    if (!url) {
      
      
      
      
      
      
      
      url = "internal://sourcemap" + (this._anonSourceMapId++) + '/';
    }
    aSource.sourceMapURL = url;

    
    
    this._sourceMapCache[url] = resolve(aMap);
  },

  








  getFrameLocation: function (aFrame) {
    if (!aFrame || !aFrame.script) {
      return new GeneratedLocation();
    }
    return new GeneratedLocation(
      this.createNonSourceMappedActor(aFrame.script.source),
      aFrame.script.getOffsetLine(aFrame.offset),
      getOffsetColumn(aFrame.offset, aFrame.script)
    );
  },

  






  getOriginalLocation: function (generatedLocation) {
    let {
      generatedSourceActor,
      generatedLine,
      generatedColumn
    } = generatedLocation;
    let source = generatedSourceActor.source;
    let url = source ? source.url : generatedSourceActor._originalUrl;

    
    
    
    
    
    return this.fetchSourceMap(source).then(map => {
      if (map) {
        let {
          source: originalUrl,
          line: originalLine,
          column: originalColumn,
          name: originalName
        } = map.originalPositionFor({
          line: generatedLine,
          column: generatedColumn == null ? Infinity : generatedColumn
        });

        
        
        
        
        
        
        
        return new OriginalLocation(
          originalUrl ? this.source({
            originalUrl: originalUrl,
            generatedSource: source
          }) : null,
          originalLine,
          originalColumn,
          originalName
        );
      }

      
      return OriginalLocation.fromGeneratedLocation(generatedLocation);
    });
  },

  getAllGeneratedLocations: function (originalLocation) {
    let {
      originalSourceActor,
      originalLine,
      originalColumn
    } = originalLocation;

    let source = originalSourceActor.source ||
                 originalSourceActor.generatedSource;

    return this.fetchSourceMap(source).then((map) => {
      if (map) {
        map.computeColumnSpans();

        return map.allGeneratedPositionsFor({
          source: originalSourceActor.url,
          line: originalLine,
          column: originalColumn
        }).map(({ line, column, lastColumn }) => {
          return new GeneratedLocation(
            this.createNonSourceMappedActor(source),
            line,
            column,
            lastColumn
          );
        });
      }

      return [GeneratedLocation.fromOriginalLocation(originalLocation)];
    });
  },


  








  getGeneratedLocation: function (originalLocation) {
    let { originalSourceActor } = originalLocation;

    
    
    
    
    let source = originalSourceActor.source || originalSourceActor.generatedSource;

    
    return this.fetchSourceMap(source).then((map) => {
      if (map) {
        let {
          originalLine,
          originalColumn
        } = originalLocation;

        let {
          line: generatedLine,
          column: generatedColumn
        } = map.generatedPositionFor({
          source: originalSourceActor.url,
          line: originalLine,
          column: originalColumn == null ? 0 : originalColumn,
          bias: SourceMapConsumer.LEAST_UPPER_BOUND
        });

        return new GeneratedLocation(
          this.createNonSourceMappedActor(source),
          generatedLine,
          generatedColumn
        );
      }

      return GeneratedLocation.fromOriginalLocation(originalLocation);
    });
  },

  






  isBlackBoxed: function (aURL) {
    return this.blackBoxedSources.has(aURL);
  },

  





  blackBox: function (aURL) {
    this.blackBoxedSources.add(aURL);
  },

  





  unblackBox: function (aURL) {
    this.blackBoxedSources.delete(aURL);
  },

  





  isPrettyPrinted: function (aURL) {
    return this.prettyPrintedSources.has(aURL);
  },

  





  prettyPrint: function (aURL, aIndent) {
    this.prettyPrintedSources.set(aURL, aIndent);
  },

  


  prettyPrintIndent: function (aURL) {
    return this.prettyPrintedSources.get(aURL);
  },

  





  disablePrettyPrint: function (aURL) {
    this.prettyPrintedSources.delete(aURL);
  },

  


  _normalize: function (...aURLs) {
    dbg_assert(aURLs.length > 1, "Should have more than 1 URL");
    let base = Services.io.newURI(aURLs.pop(), null, null);
    let url;
    while ((url = aURLs.pop())) {
      base = Services.io.newURI(url, null, base);
    }
    return base.spec;
  },

  iter: function () {
    let actors = Object.keys(this._sourceMappedSourceActors).map(k => {
      return this._sourceMappedSourceActors[k];
    });
    for (let actor of this._sourceActors.values()) {
      if (!this._sourceMaps.has(actor.source)) {
        actors.push(actor);
      }
    }
    return actors;
  }
};





function isHiddenSource(aSource) {
  
  return aSource.text === '() {\n}';
}




function isNotNull(aThing) {
  return aThing !== null;
}

exports.TabSources = TabSources;
exports.isHiddenSource = isHiddenSource;
