


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const events = require("sdk/event/core");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const protocol = require("devtools/server/protocol");
const {CallWatcherActor, CallWatcherFront} = require("devtools/server/actors/call-watcher");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils.js");

const {on, once, off, emit} = events;
const {method, custom, Arg, Option, RetVal} = protocol;

const CANVAS_CONTEXTS = [
  "CanvasRenderingContext2D",
  "WebGLRenderingContext"
];

const ANIMATION_GENERATORS = [
  "requestAnimationFrame",
  "mozRequestAnimationFrame"
];

const DRAW_CALLS = [
  
  "fill",
  "stroke",
  "clearRect",
  "fillRect",
  "strokeRect",
  "fillText",
  "strokeText",
  "drawImage",

  
  "clear",
  "drawArrays",
  "drawElements",
  "finish",
  "flush"
];

const INTERESTING_CALLS = [
  
  "save",
  "restore",

  
  "useProgram"
];

exports.register = function(handle) {
  handle.addTabActor(CanvasActor, "canvasActor");
  handle.addGlobalActor(CanvasActor, "canvasActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(CanvasActor);
  handle.removeGlobalActor(CanvasActor);
};








protocol.types.addType("uint32-array", {
  write: (v) => "[" + Array.join(v, ",") + "]",
  read: (v) => new Uint32Array(JSON.parse(v))
});




protocol.types.addDictType("snapshot-image", {
  index: "number",
  width: "number",
  height: "number",
  flipped: "boolean",
  pixels: "uint32-array"
});




protocol.types.addDictType("snapshot-overview", {
  calls: "array:function-call",
  thumbnails: "array:snapshot-image",
  screenshot: "snapshot-image"
});






let FrameSnapshotActor = protocol.ActorClass({
  typeName: "frame-snapshot",

  











  initialize: function(conn, { canvas, calls, screenshot }) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this._contentCanvas = canvas;
    this._functionCalls = calls;
    this._lastDrawCallScreenshot = screenshot;
  },

  


  getOverview: method(function() {
    return {
      calls: this._functionCalls,
      thumbnails: this._functionCalls.map(e => e._thumbnail).filter(e => !!e),
      screenshot: this._lastDrawCallScreenshot
    };
  }, {
    response: { overview: RetVal("snapshot-overview") }
  }),

  



  generateScreenshotFor: method(function(functionCall) {
    let caller = functionCall.details.caller;
    let global = functionCall.meta.global;

    let canvas = this._contentCanvas;
    let calls = this._functionCalls;
    let index = calls.indexOf(functionCall);

    
    
    
    let { replayContext, lastDrawCallIndex } = ContextUtils.replayAnimationFrame({
      contextType: global,
      canvas: canvas,
      calls: calls,
      first: 0,
      last: index
    });

    
    let dimensions = Math.min(CanvasFront.SCREENSHOT_HEIGHT_MAX, canvas.height);
    let screenshot;

    
    
    
    if (global == CallWatcherFront.CANVAS_WEBGL_CONTEXT) {
      screenshot = ContextUtils.getPixelsForWebGL(replayContext);
      replayContext.bindFramebuffer(replayContext.FRAMEBUFFER, null);
      screenshot.flipped = true;
    }
    
    else if (global == CallWatcherFront.CANVAS_2D_CONTEXT) {
      screenshot = ContextUtils.getPixelsFor2D(replayContext);
      screenshot.flipped = false;
    }

    screenshot.index = lastDrawCallIndex;
    return screenshot;
  }, {
    request: { call: Arg(0, "function-call") },
    response: { screenshot: RetVal("snapshot-image") }
  })
});




let FrameSnapshotFront = protocol.FrontClass(FrameSnapshotActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this._lastDrawCallScreenshot = null;
    this._cachedScreenshots = new WeakMap();
  },

  



  getOverview: custom(function() {
    return this._getOverview().then(data => {
      this._lastDrawCallScreenshot = data.screenshot;
      return data;
    });
  }, {
    impl: "_getOverview"
  }),

  



  generateScreenshotFor: custom(function(functionCall) {
    if (CanvasFront.ANIMATION_GENERATORS.has(functionCall.name)) {
      return promise.resolve(this._lastDrawCallScreenshot);
    }
    let cachedScreenshot = this._cachedScreenshots.get(functionCall);
    if (cachedScreenshot) {
      return cachedScreenshot;
    }
    let screenshot = this._generateScreenshotFor(functionCall);
    this._cachedScreenshots.set(functionCall, screenshot);
    return screenshot;
  }, {
    impl: "_generateScreenshotFor"
  })
});






let CanvasActor = exports.CanvasActor = protocol.ActorClass({
  typeName: "canvas",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._onContentFunctionCall = this._onContentFunctionCall.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  


  setup: method(function({ reload }) {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    this._callWatcher = new CallWatcherActor(this.conn, this.tabActor);
    this._callWatcher.onCall = this._onContentFunctionCall;
    this._callWatcher.setup({
      tracedGlobals: CANVAS_CONTEXTS,
      tracedFunctions: ANIMATION_GENERATORS,
      performReload: reload
    });
  }, {
    request: { reload: Option(0, "boolean") },
    oneway: true
  }),

  


  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    this._callWatcher.finalize();
    this._callWatcher = null;
  }, {
    oneway: true
  }),

  


  isInitialized: method(function() {
    return !!this._initialized;
  }, {
    response: { initialized: RetVal("boolean") }
  }),

  








  recordAnimationFrame: method(function() {
    if (this._callWatcher.isRecording()) {
      return this._currentAnimationFrameSnapshot.promise;
    }

    this._callWatcher.eraseRecording();
    this._callWatcher.resumeRecording();

    let deferred = this._currentAnimationFrameSnapshot = promise.defer();
    return deferred.promise;
  }, {
    response: { snapshot: RetVal("frame-snapshot") }
  }),

  



  _onContentFunctionCall: function(functionCall) {
    let { window, name, args } = functionCall.details;

    
    
    
    
    
    
    inplaceShallowCloneArrays(args, window);

    if (CanvasFront.ANIMATION_GENERATORS.has(name)) {
      this._handleAnimationFrame(functionCall);
      return;
    }
    if (CanvasFront.DRAW_CALLS.has(name) && this._animationStarted) {
      this._handleDrawCall(functionCall);
      return;
    }
  },

  


  _handleAnimationFrame: function(functionCall) {
    if (!this._animationStarted) {
      this._handleAnimationFrameBegin();
    } else {
      this._handleAnimationFrameEnd(functionCall);
    }
  },

  


  _handleAnimationFrameBegin: function() {
    this._callWatcher.eraseRecording();
    this._animationStarted = true;
  },

  


  _handleAnimationFrameEnd: function() {
    
    
    
    let functionCalls = this._callWatcher.pauseRecording();
    this._callWatcher.eraseRecording();

    
    
    let index = this._lastDrawCallIndex;
    let width = this._lastContentCanvasWidth;
    let height = this._lastContentCanvasHeight;
    let flipped = this._lastThumbnailFlipped;
    let pixels = ContextUtils.getPixelStorage()["32bit"];
    let lastDrawCallScreenshot = {
      index: index,
      width: width,
      height: height,
      flipped: flipped,
      pixels: pixels.subarray(0, width * height)
    };

    
    
    let frameSnapshot = new FrameSnapshotActor(this.conn, {
      canvas: this._lastDrawCallCanvas,
      calls: functionCalls,
      screenshot: lastDrawCallScreenshot
    });

    this._currentAnimationFrameSnapshot.resolve(frameSnapshot);
    this._currentAnimationFrameSnapshot = null;
    this._animationStarted = false;
  },

  



  _handleDrawCall: function(functionCall) {
    let functionCalls = this._callWatcher.pauseRecording();
    let caller = functionCall.details.caller;
    let global = functionCall.meta.global;

    let contentCanvas = this._lastDrawCallCanvas = caller.canvas;
    let index = this._lastDrawCallIndex = functionCalls.indexOf(functionCall);
    let w = this._lastContentCanvasWidth = contentCanvas.width;
    let h = this._lastContentCanvasHeight = contentCanvas.height;

    
    let dimensions = CanvasFront.THUMBNAIL_HEIGHT;
    let thumbnail;

    
    
    if (global == CallWatcherFront.CANVAS_WEBGL_CONTEXT) {
      
      
      let framebufferBinding = caller.getParameter(caller.FRAMEBUFFER_BINDING);
      if (framebufferBinding == null) {
        thumbnail = ContextUtils.getPixelsForWebGL(caller, 0, 0, w, h, dimensions);
        thumbnail.flipped = this._lastThumbnailFlipped = true;
        thumbnail.index = index;
      }
    } else if (global == CallWatcherFront.CANVAS_2D_CONTEXT) {
      thumbnail = ContextUtils.getPixelsFor2D(caller, 0, 0, w, h, dimensions);
      thumbnail.flipped = this._lastThumbnailFlipped = false;
      thumbnail.index = index;
    }

    functionCall._thumbnail = thumbnail;
    this._callWatcher.resumeRecording();
  }
});




let ContextUtils = {
  








  getWebGLContext: function(canvas) {
    return canvas.getContext("webgl") ||
           canvas.getContext("experimental-webgl");
  },

  


















  getPixelsForWebGL: function(gl,
    srcX = 0, srcY = 0,
    srcWidth = gl.canvas.width,
    srcHeight = gl.canvas.height,
    dstHeight = srcHeight)
  {
    let contentPixels = ContextUtils.getPixelStorage(srcWidth, srcHeight);
    let { "8bit": charView, "32bit": intView } = contentPixels;
    gl.readPixels(srcX, srcY, srcWidth, srcHeight, gl.RGBA, gl.UNSIGNED_BYTE, charView);
    return this.resizePixels(intView, srcWidth, srcHeight, dstHeight);
  },

  


















  getPixelsFor2D: function(ctx,
    srcX = 0, srcY = 0,
    srcWidth = ctx.canvas.width,
    srcHeight = ctx.canvas.height,
    dstHeight = srcHeight)
  {
    let { data } = ctx.getImageData(srcX, srcY, srcWidth, srcHeight);
    let { "32bit": intView } = ContextUtils.usePixelStorage(data.buffer);
    return this.resizePixels(intView, srcWidth, srcHeight, dstHeight);
  },

  














  resizePixels: function(srcPixels, srcWidth, srcHeight, dstHeight) {
    let screenshotRatio = dstHeight / srcHeight;
    let dstWidth = Math.floor(srcWidth * screenshotRatio);

    
    let dstPixels = new Array(dstWidth * dstHeight);

    
    
    let isTransparent = true;

    for (let dstX = 0; dstX < dstWidth; dstX++) {
      for (let dstY = 0; dstY < dstHeight; dstY++) {
        let srcX = Math.floor(dstX / screenshotRatio);
        let srcY = Math.floor(dstY / screenshotRatio);
        let cPos = srcX + srcWidth * srcY;
        let dPos = dstX + dstWidth * dstY;
        let color = dstPixels[dPos] = srcPixels[cPos];
        if (color) {
          isTransparent = false;
        }
      }
    }

    return {
      width: dstWidth,
      height: dstHeight,
      pixels: isTransparent ? [] : dstPixels
    };
  },

  



























  replayAnimationFrame: function({ contextType, canvas, calls, first, last }) {
    let w = canvas.width;
    let h = canvas.height;

    let replayCanvas;
    let replayContext;
    let customFramebuffer;
    let lastDrawCallIndex = -1;

    
    
    if (contextType == CallWatcherFront.CANVAS_WEBGL_CONTEXT) {
      replayCanvas = canvas;
      replayContext = this.getWebGLContext(replayCanvas);
      customFramebuffer = this.createBoundFramebuffer(replayContext, w, h);
    }
    
    else if (contextType == CallWatcherFront.CANVAS_2D_CONTEXT) {
      let contentDocument = canvas.ownerDocument;
      replayCanvas = contentDocument.createElement("canvas");
      replayCanvas.width = w;
      replayCanvas.height = h;
      replayContext = replayCanvas.getContext("2d");
      replayContext.clearRect(0, 0, w, h);
    }

    
    for (let i = first; i <= last; i++) {
      let { type, name, args } = calls[i].details;

      
      
      if (name == "bindFramebuffer" && args[1] == null) {
        replayContext.bindFramebuffer(replayContext.FRAMEBUFFER, customFramebuffer);
      } else {
        if (type == CallWatcherFront.METHOD_FUNCTION) {
          replayContext[name].apply(replayContext, args);
        } else if (type == CallWatcherFront.SETTER_FUNCTION) {
          replayContext[name] = args;
        } else {
          
        }
        if (CanvasFront.DRAW_CALLS.has(name)) {
          lastDrawCallIndex = i;
        }
      }
    }

    return {
      replayContext: replayContext,
      lastDrawCallIndex: lastDrawCallIndex
    };
  },

  













  getPixelStorage: function(w = 0, h = 0) {
    let storage = this._currentPixelStorage;
    if (storage && storage["32bit"].length >= w * h) {
      return storage;
    }
    return this.usePixelStorage(new ArrayBuffer(w * h * 4));
  },

  





  usePixelStorage: function(buffer) {
    let array8bit = new Uint8Array(buffer);
    let array32bit = new Uint32Array(buffer);
    return this._currentPixelStorage = {
      "8bit": array8bit,
      "32bit": array32bit
    };
  },

  












  createBoundFramebuffer: function(gl, width, height) {
    let framebuffer = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);

    
    
    let colorBuffer = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, colorBuffer);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);

    let depthBuffer = gl.createRenderbuffer();
    gl.bindRenderbuffer(gl.RENDERBUFFER, depthBuffer);
    gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, width, height);

    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, colorBuffer, 0);
    gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);

    gl.bindTexture(gl.TEXTURE_2D, null);
    gl.bindRenderbuffer(gl.RENDERBUFFER, null);

    return framebuffer;
  }
};




let CanvasFront = exports.CanvasFront = protocol.FrontClass(CanvasActor, {
  initialize: function(client, { canvasActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: canvasActor });
    this.manage(this);
  }
});




CanvasFront.CANVAS_CONTEXTS = new Set(CANVAS_CONTEXTS);
CanvasFront.ANIMATION_GENERATORS = new Set(ANIMATION_GENERATORS);
CanvasFront.DRAW_CALLS = new Set(DRAW_CALLS);
CanvasFront.INTERESTING_CALLS = new Set(INTERESTING_CALLS);
CanvasFront.THUMBNAIL_HEIGHT = 50; 
CanvasFront.SCREENSHOT_HEIGHT_MAX = 256; 
CanvasFront.INVALID_SNAPSHOT_IMAGE = {
  index: -1,
  width: 0,
  height: 0,
  pixels: []
};





function inplaceShallowCloneArrays(functionArguments, contentWindow) {
  let { Object, Array, ArrayBuffer } = contentWindow;

  functionArguments.forEach((arg, index, store) => {
    if (arg instanceof Array) {
      store[index] = arg.slice();
    }
    if (arg instanceof Object && arg.buffer instanceof ArrayBuffer) {
      store[index] = new arg.constructor(arg);
    }
  });
}
