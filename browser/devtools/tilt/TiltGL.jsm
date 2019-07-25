







































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const WEBGL_CONTEXT_NAME = "experimental-webgl";

Cu.import("resource:///modules/devtools/TiltMath.jsm");
Cu.import("resource:///modules/devtools/TiltUtils.jsm");

let EXPORTED_SYMBOLS = ["TiltGL"];




let TiltGL = {};











TiltGL.Renderer = function TGL_Renderer(aCanvas, onError, onLoad)
{
  


  this.context = TiltGL.create3DContext(aCanvas);

  
  if (!this.context) {
    TiltUtils.Output.alert("Firefox", TiltUtils.L10n.get("initTilt.error"));
    TiltUtils.Output.error(TiltUtils.L10n.get("initWebGL.error"));

    if ("function" === typeof onError) {
      onError();
    }
    return;
  }

  
  this.context.clearColor(0, 0, 0, 0);
  this.context.clearDepth(1);

  


  this.width = aCanvas.width;
  this.height = aCanvas.height;

  


  this.mvMatrix = mat4.identity(mat4.create());

  


  this.projMatrix = mat4.identity(mat4.create());

  



  this._fillColor = [];

  



  this._strokeColor = [];

  


  this._strokeWeightValue = 0;

  


  this._colorShader = new TiltGL.Program(this.context, {
    vs: TiltGL.ColorShader.vs,
    fs: TiltGL.ColorShader.fs,
    attributes: ["vertexPosition"],
    uniforms: ["mvMatrix", "projMatrix", "fill"]
  });

  
  this.Program =
    TiltGL.Program.bind(TiltGL.Program, this.context);
  this.VertexBuffer =
    TiltGL.VertexBuffer.bind(TiltGL.VertexBuffer, this.context);
  this.IndexBuffer =
    TiltGL.IndexBuffer.bind(TiltGL.IndexBuffer, this.context);
  this.Texture =
    TiltGL.Texture.bind(TiltGL.Texture, this.context);

  
  this.defaults();

  
  if ("function" === typeof onLoad) {
    onLoad();
  }
};

TiltGL.Renderer.prototype = {

  


  clear: function TGLR_clear()
  {
    let gl = this.context;
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  },

  






  depthTest: function TGLR_depthTest(aEnabledFlag)
  {
    let gl = this.context;

    if (aEnabledFlag) {
      gl.enable(gl.DEPTH_TEST);
    } else {
      gl.disable(gl.DEPTH_TEST);
    }
  },

  





  stencilTest: function TGLR_stencilTest(aEnabledFlag)
  {
    let gl = this.context;

    if (aEnabledFlag) {
      gl.enable(gl.STENCIL_TEST);
    } else {
      gl.disable(gl.STENCIL_TEST);
    }
  },

  





  cullFace: function TGLR_cullFace(aModeFlag)
  {
    let gl = this.context;

    switch (aModeFlag) {
      case "front":
        gl.enable(gl.CULL_FACE);
        gl.cullFace(gl.FRONT);
        break;
      case "back":
        gl.enable(gl.CULL_FACE);
        gl.cullFace(gl.BACK);
        break;
      case "both":
        gl.enable(gl.CULL_FACE);
        gl.cullFace(gl.FRONT_AND_BACK);
        break;
      default:
        gl.disable(gl.CULL_FACE);
    }
  },

  





  frontFace: function TGLR_frontFace(aModeFlag)
  {
    let gl = this.context;

    switch (aModeFlag) {
      case "cw":
        gl.frontFace(gl.CW);
        break;
      case "ccw":
        gl.frontFace(gl.CCW);
        break;
    }
  },

  






  blendMode: function TGLR_blendMode(aModeFlag)
  {
    let gl = this.context;

    switch (aModeFlag) {
      case "alpha":
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
        break;
      case "add":
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
        break;
      default:
        gl.disable(gl.BLEND);
    }
  },

  











  useColorShader: function TGLR_useColorShader(
    aVerticesBuffer, aColor, aMvMatrix, aProjMatrix)
  {
    let program = this._colorShader;

    
    program.use();

    
    program.bindVertexBuffer("vertexPosition", aVerticesBuffer);
    program.bindUniformMatrix("mvMatrix", aMvMatrix || this.mvMatrix);
    program.bindUniformMatrix("projMatrix", aProjMatrix || this.projMatrix);
    program.bindUniformVec4("fill", aColor || this._fillColor);
  },

  







  drawVertices: function TGLR_drawVertices(aDrawMode, aCount)
  {
    this.context.drawArrays(aDrawMode, 0, aCount);
  },

  








  drawIndexedVertices: function TGLR_drawIndexedVertices(
    aDrawMode, aIndicesBuffer)
  {
    let gl = this.context;

    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, aIndicesBuffer._ref);
    gl.drawElements(aDrawMode, aIndicesBuffer.numItems, gl.UNSIGNED_SHORT, 0);
  },

  







  fill: function TGLR_fill(aColor, aMultiplyAlpha)
  {
    let fill = this._fillColor;

    fill[0] = aColor[0];
    fill[1] = aColor[1];
    fill[2] = aColor[2];
    fill[3] = aColor[3] * (aMultiplyAlpha || 1);
  },

  







  stroke: function TGLR_stroke(aColor, aMultiplyAlpha)
  {
    let stroke = this._strokeColor;

    stroke[0] = aColor[0];
    stroke[1] = aColor[1];
    stroke[2] = aColor[2];
    stroke[3] = aColor[3] * (aMultiplyAlpha || 1);
  },

  





  strokeWeight: function TGLR_strokeWeight(aWeight)
  {
    if (this._strokeWeightValue !== aWeight) {
      this._strokeWeightValue = aWeight;
      this.context.lineWidth(aWeight);
    }
  },

  



  perspective: function TGLR_perspective()
  {
    let fov = 45;
    let w = this.width;
    let h = this.height;
    let x = w / 2;
    let y = h / 2;
    let z = y / Math.tan(TiltMath.radians(fov) / 2);
    let aspect = w / h;
    let znear = z / 10;
    let zfar = z * 10;

    mat4.perspective(fov, aspect, znear, zfar, this.projMatrix, -1);
    mat4.translate(this.projMatrix, [-x, -y, -z]);
    mat4.identity(this.mvMatrix);
  },

  


  ortho: function TGLR_ortho()
  {
    mat4.ortho(0, this.width, this.height, 0, -1, 1, this.projMatrix);
    mat4.identity(this.mvMatrix);
  },

  



  projection: function TGLR_projection(aMatrix)
  {
    mat4.set(aMatrix, this.projMatrix);
    mat4.identity(this.mvMatrix);
  },

  



  origin: function TGLR_origin()
  {
    mat4.identity(this.mvMatrix);
  },

  





  transform: function TGLR_transform(aMatrix)
  {
    mat4.multiply(this.mvMatrix, aMatrix);
  },

  









  translate: function TGLR_translate(x, y, z)
  {
    mat4.translate(this.mvMatrix, [x, y, z || 0]);
  },

  











  rotate: function TGLR_rotate(angle, x, y, z)
  {
    mat4.rotate(this.mvMatrix, angle, [x, y, z]);
  },

  





  rotateX: function TGLR_rotateX(aAngle)
  {
    mat4.rotateX(this.mvMatrix, aAngle);
  },

  





  rotateY: function TGLR_rotateY(aAngle)
  {
    mat4.rotateY(this.mvMatrix, aAngle);
  },

  





  rotateZ: function TGLR_rotateZ(aAngle)
  {
    mat4.rotateZ(this.mvMatrix, aAngle);
  },

  









  scale: function TGLR_scale(x, y, z)
  {
    mat4.scale(this.mvMatrix, [x, y, z || 1]);
  },

  














  lerp: function TGLR_lerp(aMat, aMat2, aLerp, aDamping, aBalance)
  {
    if (aLerp < 0 || aLerp > 1) {
      return;
    }

    
    let f = Math.pow(1 - Math.pow(aLerp, aDamping || 1), 1 / aBalance || 1);

    
    for (let i = 0, len = this.projMatrix.length; i < len; i++) {
      aMat[i] = aMat[i] + f * (aMat2[i] - aMat[i]);
    }
  },

  


  defaults: function TGLR_defaults()
  {
    this.depthTest(true);
    this.stencilTest(false);
    this.cullFace(false);
    this.frontFace("ccw");
    this.blendMode("alpha");
    this.fill([1, 1, 1, 1]);
    this.stroke([0, 0, 0, 1]);
    this.strokeWeight(1);
    this.perspective();
    this.origin();
  },

  













  quad: function TGLR_quad(aV0, aV1, aV2, aV3)
  {
    let gl = this.context;
    let fill = this._fillColor;
    let stroke = this._strokeColor;
    let vert = new TiltGL.VertexBuffer(gl, [aV0[0], aV0[1], aV0[2] || 0,
                                            aV1[0], aV1[1], aV1[2] || 0,
                                            aV2[0], aV2[1], aV2[2] || 0,
                                            aV3[0], aV3[1], aV3[2] || 0], 3);

    
    this.useColorShader(vert, fill);
    this.drawVertices(gl.TRIANGLE_FAN, vert.numItems);

    this.useColorShader(vert, stroke);
    this.drawVertices(gl.LINE_LOOP, vert.numItems);

    TiltUtils.destroyObject(vert);
  },

  


  finalize: function TGLR_finalize()
  {
    if (this.context) {
      TiltUtils.destroyObject(this._colorShader);
    }
  }
};













TiltGL.VertexBuffer = function TGL_VertexBuffer(
  aContext, aElementsArray, aItemSize, aNumItems)
{
  


  this._context = aContext;

  


  this._ref = null;

  


  this.components = null;

  


  this.itemSize = 0;
  this.numItems = 0;

  
  if (aElementsArray) {
    this.initBuffer(aElementsArray, aItemSize, aNumItems);
  }
};

TiltGL.VertexBuffer.prototype = {

  










  initBuffer: function TGLVB_initBuffer(aElementsArray, aItemSize, aNumItems)
  {
    let gl = this._context;

    
    aNumItems = aNumItems || aElementsArray.length / aItemSize;

    
    this.components = new Float32Array(aElementsArray);

    
    this._ref = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this._ref);
    gl.bufferData(gl.ARRAY_BUFFER, this.components, gl.STATIC_DRAW);

    
    this.itemSize = aItemSize;
    this.numItems = aNumItems;
  },

  


  finalize: function TGLVB_finalize()
  {
    if (this._context) {
      this._context.deleteBuffer(this._ref);
    }
  }
};











TiltGL.IndexBuffer = function TGL_IndexBuffer(
  aContext, aElementsArray, aNumItems)
{
  


  this._context = aContext;

  


  this._ref = null;

  


  this.components = null;

  


  this.itemSize = 0;
  this.numItems = 0;

  
  if (aElementsArray) {
    this.initBuffer(aElementsArray, aNumItems);
  }
};

TiltGL.IndexBuffer.prototype = {

  









  initBuffer: function TGLIB_initBuffer(aElementsArray, aNumItems)
  {
    let gl = this._context;

    
    aNumItems = aNumItems || aElementsArray.length;

    
    this.components = new Uint16Array(aElementsArray);

    
    this._ref = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this._ref);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, this.components, gl.STATIC_DRAW);

    
    this.itemSize = 1;
    this.numItems = aNumItems;
  },

  


  finalize: function TGLIB_finalize()
  {
    if (this._context) {
      this._context.deleteBuffer(this._ref);
    }
  }
};











TiltGL.Program = function(aContext, aProperties)
{
  
  aProperties = aProperties || {};

  


  this._context = aContext;

  


  this._ref = null;

  


  this._id = -1;

  



  this._attributes = null;
  this._uniforms = null;

  
  if (aProperties.vs && aProperties.fs) {
    this.initProgram(aProperties);
  }
};

TiltGL.Program.prototype = {

  









  initProgram: function TGLP_initProgram(aProperties)
  {
    this._ref = TiltGL.ProgramUtils.create(this._context, aProperties);

    
    this._id = this._ref.id;
    this._attributes = this._ref.attributes;
    this._uniforms = this._ref.uniforms;

    
    delete this._ref.id;
    delete this._ref.attributes;
    delete this._ref.uniforms;
  },

  





  use: function TGLP_use()
  {
    let id = this._id;
    let utils = TiltGL.ProgramUtils;

    
    if (utils._activeProgram !== id) {
      utils._activeProgram = id;

      
      this._context.useProgram(this._ref);

      
      if (utils._enabledAttributes < this._attributes.length) {
        utils._enabledAttributes = this._attributes.length;

        
        for (let i in this._attributes) {
          if (this._attributes.hasOwnProperty(i)) {
            this._context.enableVertexAttribArray(this._attributes[i]);
          }
        }
      }
    }
  },

  







  bindVertexBuffer: function TGLP_bindVertexBuffer(aAtribute, aBuffer)
  {
    
    let gl = this._context;
    let attr = this._attributes[aAtribute];
    let size = aBuffer.itemSize;

    gl.bindBuffer(gl.ARRAY_BUFFER, aBuffer._ref);
    gl.vertexAttribPointer(attr, size, gl.FLOAT, false, 0, 0);
  },

  







  bindUniformMatrix: function TGLP_bindUniformMatrix(aUniform, m)
  {
    this._context.uniformMatrix4fv(this._uniforms[aUniform], false, m);
  },

  







  bindUniformVec4: function TGLP_bindUniformVec4(aUniform, v)
  {
    this._context.uniform4fv(this._uniforms[aUniform], v);
  },

  







  bindUniformFloat: function TGLP_bindUniformFloat(aUniform, f)
  {
    this._context.uniform1f(this._uniforms[aUniform], f);
  },

  







  bindTexture: function TGLP_bindTexture(aSampler, aTexture)
  {
    let gl = this._context;

    gl.uniform1i(this._uniforms[aSampler], 0);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, aTexture._ref);
  },

  


  finalize: function TGLP_finalize()
  {
    if (this._context) {
      this._context.useProgram(null);
      this._context.deleteProgram(this._ref);
    }
  }
};




TiltGL.ProgramUtils = {

  













  create: function TGLPU_create(aContext, aProperties)
  {
    
    aProperties = aProperties || {};

    
    let vertShader = this.compile(aContext, aProperties.vs, "vertex");
    let fragShader = this.compile(aContext, aProperties.fs, "fragment");
    let program = this.link(aContext, vertShader, fragShader);

    aContext.deleteShader(vertShader);
    aContext.deleteShader(fragShader);

    return this.cache(aContext, aProperties, program);
  },

  











  compile: function TGLPU_compile(aContext, aShaderSource, aShaderType)
  {
    let gl = aContext, shader, status;

    
    if ("string" !== typeof aShaderSource || aShaderSource.length < 1) {
      TiltUtils.Output.error(
        TiltUtils.L10n.get("compileShader.source.error"));
      return null;
    }

    
    if (aShaderType === "vertex") {
      shader = gl.createShader(gl.VERTEX_SHADER);
    } else if (aShaderType === "fragment") {
      shader = gl.createShader(gl.FRAGMENT_SHADER);
    } else {
      TiltUtils.Output.error(
        TiltUtils.L10n.format("compileShader.type.error", [aShaderSource]));
      return null;
    }

    
    gl.shaderSource(shader, aShaderSource);
    gl.compileShader(shader);

    
    shader.src = aShaderSource;

    
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      status = gl.getShaderInfoLog(shader);

      TiltUtils.Output.error(
        TiltUtils.L10n.format("compileShader.compile.error", [status]));
      return null;
    }

    
    return shader;
  },

  











  link: function TGLPU_link(aContext, aVertShader, aFragShader)
  {
    let gl = aContext, program, status;

    
    program = gl.createProgram();

    
    gl.attachShader(program, aVertShader);
    gl.attachShader(program, aFragShader);
    gl.linkProgram(program);

    
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      status = gl.getProgramInfoLog(program);

      TiltUtils.Output.error(
        TiltUtils.L10n.format("linkProgram.error", [status]));
      return null;
    }

    
    program.id = this._count++;

    return program;
  },

  













  cache: function TGLPU_cache(aContext, aProperties, aProgram)
  {
    
    aProperties = aProperties || {};

    
    aProgram.attributes = {};
    aProgram.uniforms = {};

    Object.defineProperty(aProgram.attributes, "length",
      { value: 0, writable: true, enumerable: false, configurable: true });

    Object.defineProperty(aProgram.uniforms, "length",
      { value: 0, writable: true, enumerable: false, configurable: true });


    let attr = aProperties.attributes;
    let unif = aProperties.uniforms;

    if (attr) {
      for (let i = 0, len = attr.length; i < len; i++) {
        
        let param = attr[i];
        let loc = aContext.getAttribLocation(aProgram, param);

        if ("number" === typeof loc && loc > -1) {
          
          
          if (aProgram.attributes[param] === undefined) {
            aProgram.attributes[param] = loc;
            aProgram.attributes.length++;
          }
        }
      }
    }

    if (unif) {
      for (let i = 0, len = unif.length; i < len; i++) {
        
        let param = unif[i];
        let loc = aContext.getUniformLocation(aProgram, param);

        if ("object" === typeof loc && loc) {
          
          
          if (aProgram.uniforms[param] === undefined) {
            aProgram.uniforms[param] = loc;
            aProgram.uniforms.length++;
          }
        }
      }
    }

    return aProgram;
  },

  


  _count: 0,

  


  _activeProgram: -1,

  


  _enabledAttributes: -1
};



















TiltGL.Texture = function(aContext, aProperties)
{
  
  aProperties = aProperties || {};

  


  this._context = aContext;

  


  this._ref = null;

  


  this._id = -1;

  



  this.width = -1;
  this.height = -1;

  


  this.loaded = false;

  
  if ("object" === typeof aProperties.source) {
    this.initTexture(aProperties);
  } else {
    TiltUtils.Output.error(
      TiltUtils.L10n.get("initTexture.source.error"));
  }
};

TiltGL.Texture.prototype = {

  

















  initTexture: function TGLT_initTexture(aProperties)
  {
    this._ref = TiltGL.TextureUtils.create(this._context, aProperties);

    
    this._id = this._ref.id;
    this.width = this._ref.width;
    this.height = this._ref.height;
    this.loaded = true;

    
    delete this._ref.id;
    delete this._ref.width;
    delete this._ref.height;
    delete this.onload;
  },

  


  finalize: function TGLT_finalize()
  {
    if (this._context) {
      this._context.deleteTexture(this._ref);
    }
  }
};




TiltGL.TextureUtils = {

  





















  create: function TGLTU_create(aContext, aProperties)
  {
    
    aProperties = aProperties || {};

    if (!aProperties.source) {
      return null;
    }

    let gl = aContext;
    let width = aProperties.source.width;
    let height = aProperties.source.height;
    let format = gl[aProperties.format || "RGB"];

    
    let source = this.resizeImageToPowerOfTwo(aProperties);

    
    let texture = gl.createTexture();

    
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, format, format, gl.UNSIGNED_BYTE, source);
    this.setTextureParams(gl, aProperties);

    
    gl.bindTexture(gl.TEXTURE_2D, null);

    
    texture.width = width;
    texture.height = height;

    
    texture.id = this._count++;

    return texture;
  },

  








  setTextureParams: function TGLTU_setTextureParams(aContext, aProperties)
  {
    
    aProperties = aProperties || {};

    let gl = aContext;
    let minFilter = gl.TEXTURE_MIN_FILTER;
    let magFilter = gl.TEXTURE_MAG_FILTER;
    let wrapS = gl.TEXTURE_WRAP_S;
    let wrapT = gl.TEXTURE_WRAP_T;

    
    if (aProperties.texture) {
      gl.bindTexture(gl.TEXTURE_2D, aProperties.texture.ref);
    }

    
    if ("nearest" === aProperties.minFilter) {
      gl.texParameteri(gl.TEXTURE_2D, minFilter, gl.NEAREST);
    } else if ("linear" === aProperties.minFilter && aProperties.mipmap) {
      gl.texParameteri(gl.TEXTURE_2D, minFilter, gl.LINEAR_MIPMAP_LINEAR);
    } else {
      gl.texParameteri(gl.TEXTURE_2D, minFilter, gl.LINEAR);
    }

    
    if ("nearest" === aProperties.magFilter) {
      gl.texParameteri(gl.TEXTURE_2D, magFilter, gl.NEAREST);
    } else {
      gl.texParameteri(gl.TEXTURE_2D, magFilter, gl.LINEAR);
    }

    
    if ("repeat" === aProperties.wrapS) {
      gl.texParameteri(gl.TEXTURE_2D, wrapS, gl.REPEAT);
    } else {
      gl.texParameteri(gl.TEXTURE_2D, wrapS, gl.CLAMP_TO_EDGE);
    }

    
    if ("repeat" === aProperties.wrapT) {
      gl.texParameteri(gl.TEXTURE_2D, wrapT, gl.REPEAT);
    } else {
      gl.texParameteri(gl.TEXTURE_2D, wrapT, gl.CLAMP_TO_EDGE);
    }

    
    if (aProperties.mipmap) {
      gl.generateMipmap(gl.TEXTURE_2D);
    }
  },

  










  createContentImage: function TGLTU_createContentImage(
    aContentWindow, aMaxImageSize)
  {
    
    let size = TiltUtils.DOM.getContentWindowDimensions(aContentWindow);

    
    let canvas = TiltUtils.DOM.initCanvas(null);
    canvas.width = TiltMath.clamp(size.width, 0, aMaxImageSize);
    canvas.height = TiltMath.clamp(size.height, 0, aMaxImageSize);

    
    let ctx = canvas.getContext("2d");
    ctx.drawWindow(aContentWindow, 0, 0, canvas.width, canvas.height, "#fff");

    return canvas;
  },

  
















  resizeImageToPowerOfTwo: function TGLTU_resizeImageToPowerOfTwo(aProperties)
  {
    
    aProperties = aProperties || {};

    if (!aProperties.source) {
      return null;
    }

    let isPowerOfTwoWidth = TiltMath.isPowerOfTwo(aProperties.source.width);
    let isPowerOfTwoHeight = TiltMath.isPowerOfTwo(aProperties.source.height);

    
    if (!aProperties.resize || (isPowerOfTwoWidth && isPowerOfTwoHeight)) {
      return aProperties.source;
    }

    
    let width = TiltMath.nextPowerOfTwo(aProperties.source.width);
    let height = TiltMath.nextPowerOfTwo(aProperties.source.height);

    
    let canvas = TiltUtils.DOM.initCanvas(null, {
      width: width,
      height: height
    });

    let ctx = canvas.getContext("2d");

    
    if (aProperties.fill) {
      ctx.fillStyle = aProperties.fill;
      ctx.fillRect(0, 0, width, height);
    }

    
    ctx.drawImage(aProperties.source, 0, 0, width, height);

    
    if (aProperties.stroke) {
      ctx.strokeStyle = aProperties.stroke;
      ctx.lineWidth = aProperties.strokeWeight;
      ctx.strokeRect(0, 0, width, height);
    }

    return canvas;
  },

  


  _count: 0
};









TiltGL.ColorShader = {

  


  vs: [
    "attribute vec3 vertexPosition;",

    "uniform mat4 mvMatrix;",
    "uniform mat4 projMatrix;",

    "void main() {",
    "    gl_Position = projMatrix * mvMatrix * vec4(vertexPosition, 1.0);",
    "}"
  ].join("\n"),

  


  fs: [
    "#ifdef GL_ES",
    "precision lowp float;",
    "#endif",

    "uniform vec4 fill;",

    "void main() {",
    "    gl_FragColor = fill;",
    "}"
  ].join("\n")
};







TiltGL.isWebGLSupported = function TGL_isWebGLSupported()
{
  let supported = false;

  try {
    let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);
    let angle = gfxInfo.FEATURE_WEBGL_ANGLE;
    let opengl = gfxInfo.FEATURE_WEBGL_OPENGL;

    
    supported = gfxInfo.getFeatureStatus(angle) === gfxInfo.FEATURE_NO_INFO ||
                gfxInfo.getFeatureStatus(opengl) === gfxInfo.FEATURE_NO_INFO;
  } catch(e) {
    TiltUtils.Output.error(e.message);
  } finally {
    return supported;
  }
};











TiltGL.create3DContext = function TGL_create3DContext(aCanvas, aFlags)
{
  TiltGL.clearCache();

  
  let context = null;

  try {
    context = aCanvas.getContext(WEBGL_CONTEXT_NAME, aFlags);
  } catch(e) {
    TiltUtils.Output.error(e.message);
  } finally {
    return context;
  }
};




TiltGL.clearCache = function TGL_clearCache()
{
  TiltGL.ProgramUtils._activeProgram = -1;
  TiltGL.ProgramUtils._enabledAttributes = -1;
};
