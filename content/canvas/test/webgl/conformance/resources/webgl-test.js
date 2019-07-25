
























function webglTestLog(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
  if (document.getElementById("console")) {
    var log = document.getElementById("console");
    log.innerHTML += msg + "<br>";
  }
}






function create3DContext(canvas, attributes)
{
    if (!canvas)
        canvas = document.createElement("canvas");
    var context = null;
    try {
        context = canvas.getContext("webgl", attributes);
    } catch(e) {}
    if (!context) {
        try {
            context = canvas.getContext("experimental-webgl", attributes);
        } catch(e) {}
    }
    if (!context) {
        throw "Unable to fetch WebGL rendering context for Canvas";
    }
    return context;
}

function createGLErrorWrapper(context, fname) {
    return function() {
        var rv = context[fname].apply(context, arguments);
        var err = context.getError();
        if (err != 0)
            throw "GL error " + err + " in " + fname;
        return rv;
    };
}

function create3DContextWithWrapperThatThrowsOnGLError(canvas, attributes) {
  var context = create3DContext(canvas, attributes);
  
  var wrap = {};
  for (var i in context) {
    try {
      if (typeof context[i] == 'function') {
        wrap[i] = createGLErrorWrapper(context, i);
      } else {
        wrap[i] = context[i];
      }
    } catch (e) {
      webglTestLog("createContextWrapperThatThrowsOnGLError: Error accessing " + i);
    }
  }
  wrap.getError = function() {
      return context.getError();
  };
  return wrap;
}

function getGLErrorAsString(ctx, err) {
  if (err === ctx.NO_ERROR) {
    return "NO_ERROR";
  }
  for (var name in ctx) {
    if (ctx[name] === err) {
      return name;
    }
  }
  return "0x" + err.toString(16);
}


function shouldGenerateGLError(ctx, glErrors, evalStr) {
  if (!glErrors.length) {
    glErrors = [glErrors];
  }
  var exception;
  try {
    eval(evalStr);
  } catch (e) {
    exception = e;
  }
  if (exception) {
    testFailed(evalStr + " threw exception " + exception);
  } else {
    var err = ctx.getError();
    if (glErrors.indexOf(err) < 0) {
      var errStrs = [];
      for (var ii = 0; ii < glErrors.length; ++ii) {
        errStrs.push(getGLErrorAsString(ctx, glErrors[ii]));
      }
      testFailed(evalStr + " expected: " + errStrs.join(" or ") + ". Was " + getGLErrorAsString(ctx, err) + ".");
    } else {
      testPassed(evalStr + " generated expected GL error: " + getGLErrorAsString(ctx, err) + ".");
    }
  }
}









function glErrorShouldBe(gl, glErrors, opt_msg) {
  if (!glErrors.length) {
    glErrors = [glErrors];
  }
  opt_msg = opt_msg || "";
  var err = gl.getError();
  var ndx = glErrors.indexOf(err);
  if (ndx < 0) {
    if (glErrors.length == 1) {
      testFailed("getError expected: " + getGLErrorAsString(gl, glErrors[0]) +
                 ". Was " + getGLErrorAsString(gl, err) + " : " + opt_msg);
    } else {
      var errs = [];
      for (var ii = 0; ii < glErrors.length; ++ii) {
        errs.push(getGLErrorAsString(gl, glErrors[ii]));
      }
      testFailed("getError expected one of: [" + errs.join(", ") +
                 "]. Was " + getGLErrorAsString(gl, err) + " : " + opt_msg);
    }
  } else {
    testPassed("getError was expected value: " +
                getGLErrorAsString(gl, err) + " : " + opt_msg);
  }
};








function createProgram(gl, vshaders, fshaders, attribs)
{
  if (typeof(vshaders) == "string")
    vshaders = [vshaders];
  if (typeof(fshaders) == "string")
    fshaders = [fshaders];

  var shaders = [];
  var i;

  for (i = 0; i < vshaders.length; ++i) {
    var shader = loadShader(gl, vshaders[i], gl.VERTEX_SHADER);
    if (!shader)
      return null;
    shaders.push(shader);
  }

  for (i = 0; i < fshaders.length; ++i) {
    var shader = loadShader(gl, fshaders[i], gl.FRAGMENT_SHADER);
    if (!shader)
      return null;
    shaders.push(shader);
  }

  var prog = gl.createProgram();
  for (i = 0; i < shaders.length; ++i) {
    gl.attachShader(prog, shaders[i]);
  }

  if (attribs) {
    for (var i = 0; i < attribs.length; ++i) {
      gl.bindAttribLocation(prog, i, attribs[i]);
    }
  }

  gl.linkProgram(prog);

  
  var linked = gl.getProgramParameter(prog, gl.LINK_STATUS);
  if (!linked) {
    
    var error = gl.getProgramInfoLog(prog);
    webglTestLog("Error in program linking:" + error);

    gl.deleteProgram(prog);
    for (i = 0; i < shaders.length; ++i)
      gl.deleteShader(shaders[i]);
    return null;
  }

  return prog;
}
















function initWebGL(canvasName, vshader, fshader, attribs, clearColor, clearDepth, contextAttribs)
{
    var canvas = document.getElementById(canvasName);
    var gl = create3DContext(canvas, contextAttribs);
    if (!gl) {
        alert("No WebGL context found");
        return null;
    }

    
    gl.program = createProgram(gl, vshader, fshader, attribs);
    if (!gl.program)
        return null;

    gl.useProgram(gl.program);

    gl.clearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    gl.clearDepth(clearDepth);

    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    return gl;
}






function getShaderSource(file)
{
    var xhr = new XMLHttpRequest();
    xhr.open("GET", file, false);
    xhr.send();
    return xhr.responseText;
}










function loadShader(ctx, shaderId, shaderType, isFile)
{
    var shaderSource = "";

    if (isFile)
        shaderSource = getShaderSource(shaderId);
    else {
        var shaderScript = document.getElementById(shaderId);
        if (!shaderScript) {
            shaderSource = shaderId;
        } else {
            if (shaderScript.type == "x-shader/x-vertex") {
                shaderType = ctx.VERTEX_SHADER;
            } else if (shaderScript.type == "x-shader/x-fragment") {
                shaderType = ctx.FRAGMENT_SHADER;
            } else if (shaderType != ctx.VERTEX_SHADER && shaderType != ctx.FRAGMENT_SHADER) {
                webglTestLog("*** Error: unknown shader type");
                return null;
            }

            shaderSource = shaderScript.text;
        }
    }

    
    var shader = ctx.createShader(shaderType);
    if (shader == null) {
        webglTestLog("*** Error: unable to create shader '"+shaderId+"'");
        return null;
    }

    
    ctx.shaderSource(shader, shaderSource);

    
    ctx.compileShader(shader);

    
    var compiled = ctx.getShaderParameter(shader, ctx.COMPILE_STATUS);
    if (!compiled) {
        
        var error = ctx.getShaderInfoLog(shader);
        webglTestLog("*** Error compiling shader '"+shader+"':"+error);
        ctx.deleteShader(shader);
        return null;
    }

    return shader;
}

function loadShaderFromFile(ctx, file, type)
{
    return loadShader(ctx, file, type, true);
}

function loadShaderFromScript(ctx, script)
{
    return loadShader(ctx, script, 0, false);
}

function loadStandardProgram(context) {
    var program = context.createProgram();
    context.attachShader(program, loadStandardVertexShader(context));
    context.attachShader(program, loadStandardFragmentShader(context));
    context.linkProgram(program);
    return program;
}

function loadProgram(context, vertexShaderPath, fragmentShaderPath, isFile) {
    isFile = (isFile === undefined) ? true : isFile;
    var program = context.createProgram();
    context.attachShader(program, loadShader(context, vertexShaderPath, context.VERTEX_SHADER, isFile));
    context.attachShader(program, loadShader(context, fragmentShaderPath, context.FRAGMENT_SHADER, isFile));
    context.linkProgram(program);
    return program;
}

var getBasePathForResources = function() {
  var expectedBase = "webgl-test.js";
  var scripts = document.getElementsByTagName('script');
  for (var script, i = 0; script = scripts[i]; i++) {
    var src = script.src;
    var l = src.length;
    if (src.substr(l - expectedBase.length) == expectedBase) {
      return src.substr(0, l - expectedBase.length);
    }
  }
  throw 'oops';
};


function loadStandardVertexShader(context) {
    return loadShader(
        context,
        getBasePathForResources() + "vertexShader.vert",
        context.VERTEX_SHADER,
        true);
}

function loadStandardFragmentShader(context) {
    return loadShader(
        context,
        getBasePathForResources() + "fragmentShader.frag",
        context.FRAGMENT_SHADER,
        true);
}













function makeBox(ctx)
{
    
    
    
    
    
    
    
    
    
    
    var vertices = new Float32Array(
        [  1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,    
           1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,    
           1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,    
          -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,    
          -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,    
           1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 ]   
    );

    
    var normals = new Float32Array(
        [  0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,     
           1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,     
           0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,     
          -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,     
           0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,     
           0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1 ]    
       );


    
    var texCoords = new Float32Array(
        [  1, 1,   0, 1,   0, 0,   1, 0,    
           0, 1,   0, 0,   1, 0,   1, 1,    
           1, 0,   1, 1,   0, 1,   0, 0,    
           1, 1,   0, 1,   0, 0,   1, 0,    
           0, 0,   1, 0,   1, 1,   0, 1,    
           0, 0,   1, 0,   1, 1,   0, 1 ]   
       );

    
    var indices = new Uint8Array(
        [  0, 1, 2,   0, 2, 3,    
           4, 5, 6,   4, 6, 7,    
           8, 9,10,   8,10,11,    
          12,13,14,  12,14,15,    
          16,17,18,  16,18,19,    
          20,21,22,  20,22,23 ]   
      );

    var retval = { };

    retval.normalObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.normalObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, normals, ctx.STATIC_DRAW);

    retval.texCoordObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.texCoordObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, texCoords, ctx.STATIC_DRAW);

    retval.vertexObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.vertexObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, vertices, ctx.STATIC_DRAW);

    ctx.bindBuffer(ctx.ARRAY_BUFFER, 0);

    retval.indexObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, retval.indexObject);
    ctx.bufferData(ctx.ELEMENT_ARRAY_BUFFER, indices, ctx.STATIC_DRAW);
    ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, 0);

    retval.numIndices = indices.length;

    return retval;
}














function makeSphere(ctx, radius, lats, longs)
{
    var geometryData = [ ];
    var normalData = [ ];
    var texCoordData = [ ];
    var indexData = [ ];

    for (var latNumber = 0; latNumber <= lats; ++latNumber) {
        for (var longNumber = 0; longNumber <= longs; ++longNumber) {
            var theta = latNumber * Math.PI / lats;
            var phi = longNumber * 2 * Math.PI / longs;
            var sinTheta = Math.sin(theta);
            var sinPhi = Math.sin(phi);
            var cosTheta = Math.cos(theta);
            var cosPhi = Math.cos(phi);

            var x = cosPhi * sinTheta;
            var y = cosTheta;
            var z = sinPhi * sinTheta;
            var u = 1-(longNumber/longs);
            var v = latNumber/lats;

            normalData.push(x);
            normalData.push(y);
            normalData.push(z);
            texCoordData.push(u);
            texCoordData.push(v);
            geometryData.push(radius * x);
            geometryData.push(radius * y);
            geometryData.push(radius * z);
        }
    }

    longs += 1;
    for (var latNumber = 0; latNumber < lats; ++latNumber) {
        for (var longNumber = 0; longNumber < longs; ++longNumber) {
            var first = (latNumber * longs) + (longNumber % longs);
            var second = first + longs;
            indexData.push(first);
            indexData.push(second);
            indexData.push(first+1);

            indexData.push(second);
            indexData.push(second+1);
            indexData.push(first+1);
        }
    }

    var retval = { };

    retval.normalObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.normalObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, new Float32Array(normalData), ctx.STATIC_DRAW);

    retval.texCoordObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.texCoordObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, new Float32Array(texCoordData), ctx.STATIC_DRAW);

    retval.vertexObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ARRAY_BUFFER, retval.vertexObject);
    ctx.bufferData(ctx.ARRAY_BUFFER, new Float32Array(geometryData), ctx.STATIC_DRAW);

    retval.numIndices = indexData.length;
    retval.indexObject = ctx.createBuffer();
    ctx.bindBuffer(ctx.ELEMENT_ARRAY_BUFFER, retval.indexObject);
    ctx.bufferData(ctx.ELEMENT_ARRAY_BUFFER, new Uint16Array(indexData), ctx.STREAM_DRAW);

    return retval;
}














function loadObj(ctx, url)
{
    var obj = { loaded : false };
    obj.ctx = ctx;
    var req = new XMLHttpRequest();
    req.obj = obj;
    req.onreadystatechange = function () { processLoadObj(req) };
    req.open("GET", url, true);
    req.send(null);
    return obj;
}

function processLoadObj(req)
{
    webglTestLog("req="+req)
    
    if (req.readyState == 4) {
        doLoadObj(req.obj, req.responseText);
    }
}

function doLoadObj(obj, text)
{
    vertexArray = [ ];
    normalArray = [ ];
    textureArray = [ ];
    indexArray = [ ];

    var vertex = [ ];
    var normal = [ ];
    var texture = [ ];
    var facemap = { };
    var index = 0;

    var lines = text.split("\n");
    for (var lineIndex in lines) {
        var line = lines[lineIndex].replace(/[ \t]+/g, " ").replace(/\s\s*$/, "");

        
        if (line[0] == "#")
            continue;

        var array = line.split(" ");
        if (array[0] == "v") {
            
            vertex.push(parseFloat(array[1]));
            vertex.push(parseFloat(array[2]));
            vertex.push(parseFloat(array[3]));
        }
        else if (array[0] == "vt") {
            
            texture.push(parseFloat(array[1]));
            texture.push(parseFloat(array[2]));
        }
        else if (array[0] == "vn") {
            
            normal.push(parseFloat(array[1]));
            normal.push(parseFloat(array[2]));
            normal.push(parseFloat(array[3]));
        }
        else if (array[0] == "f") {
            
            if (array.length != 4) {
                webglTestLog("*** Error: face '"+line+"' not handled");
                continue;
            }

            for (var i = 1; i < 4; ++i) {
                if (!(array[i] in facemap)) {
                    
                    var f = array[i].split("/");
                    var vtx, nor, tex;

                    if (f.length == 1) {
                        vtx = parseInt(f[0]) - 1;
                        nor = vtx;
                        tex = vtx;
                    }
                    else if (f.length = 3) {
                        vtx = parseInt(f[0]) - 1;
                        tex = parseInt(f[1]) - 1;
                        nor = parseInt(f[2]) - 1;
                    }
                    else {
                        webglTestLog("*** Error: did not understand face '"+array[i]+"'");
                        return null;
                    }

                    
                    var x = 0;
                    var y = 0;
                    var z = 0;
                    if (vtx * 3 + 2 < vertex.length) {
                        x = vertex[vtx*3];
                        y = vertex[vtx*3+1];
                        z = vertex[vtx*3+2];
                    }
                    vertexArray.push(x);
                    vertexArray.push(y);
                    vertexArray.push(z);

                    
                    x = 0;
                    y = 0;
                    if (tex * 2 + 1 < texture.length) {
                        x = texture[tex*2];
                        y = texture[tex*2+1];
                    }
                    textureArray.push(x);
                    textureArray.push(y);

                    
                    x = 0;
                    y = 0;
                    z = 1;
                    if (nor * 3 + 2 < normal.length) {
                        x = normal[nor*3];
                        y = normal[nor*3+1];
                        z = normal[nor*3+2];
                    }
                    normalArray.push(x);
                    normalArray.push(y);
                    normalArray.push(z);

                    facemap[array[i]] = index++;
                }

                indexArray.push(facemap[array[i]]);
            }
        }
    }

    
    obj.normalObject = obj.ctx.createBuffer();
    obj.ctx.bindBuffer(obj.ctx.ARRAY_BUFFER, obj.normalObject);
    obj.ctx.bufferData(obj.ctx.ARRAY_BUFFER, new Float32Array(normalArray), obj.ctx.STATIC_DRAW);

    obj.texCoordObject = obj.ctx.createBuffer();
    obj.ctx.bindBuffer(obj.ctx.ARRAY_BUFFER, obj.texCoordObject);
    obj.ctx.bufferData(obj.ctx.ARRAY_BUFFER, new Float32Array(textureArray), obj.ctx.STATIC_DRAW);

    obj.vertexObject = obj.ctx.createBuffer();
    obj.ctx.bindBuffer(obj.ctx.ARRAY_BUFFER, obj.vertexObject);
    obj.ctx.bufferData(obj.ctx.ARRAY_BUFFER, new Float32Array(vertexArray), obj.ctx.STATIC_DRAW);

    obj.numIndices = indexArray.length;
    obj.indexObject = obj.ctx.createBuffer();
    obj.ctx.bindBuffer(obj.ctx.ELEMENT_ARRAY_BUFFER, obj.indexObject);
    obj.ctx.bufferData(obj.ctx.ELEMENT_ARRAY_BUFFER, new Uint16Array(indexArray), obj.ctx.STREAM_DRAW);

    obj.loaded = true;
}






function loadImageTexture(ctx, url)
{
    var texture = ctx.createTexture();
    texture.image = new Image();
    texture.image.onload = function() { doLoadImageTexture(ctx, texture.image, texture) }
    texture.image.src = url;
    return texture;
}

function doLoadImageTexture(ctx, image, texture)
{
    ctx.enable(ctx.TEXTURE_2D);
    ctx.bindTexture(ctx.TEXTURE_2D, texture);
    ctx.texImage2D(ctx.TEXTURE_2D, 0, ctx.RGBA, ctx.RGBA, ctx.UNSIGNED_BYTE, image);
    ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MAG_FILTER, ctx.LINEAR);
    ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_MIN_FILTER, ctx.LINEAR_MIPMAP_LINEAR);
    ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_WRAP_S, ctx.CLAMP_TO_EDGE);
    ctx.texParameteri(ctx.TEXTURE_2D, ctx.TEXTURE_WRAP_T, ctx.CLAMP_TO_EDGE);
    ctx.generateMipmap(ctx.TEXTURE_2D)
    ctx.bindTexture(ctx.TEXTURE_2D, 0);
}








Framerate = function(id)
{
    this.numFramerates = 10;
    this.framerateUpdateInterval = 500;
    this.id = id;

    this.renderTime = -1;
    this.framerates = [ ];
    self = this;
    var fr = function() { self.updateFramerate() }
    setInterval(fr, this.framerateUpdateInterval);
}

Framerate.prototype.updateFramerate = function()
{
    var tot = 0;
    for (var i = 0; i < this.framerates.length; ++i)
        tot += this.framerates[i];

    var framerate = tot / this.framerates.length;
    framerate = Math.round(framerate);
    document.getElementById(this.id).innerHTML = "Framerate:"+framerate+"fps";
}

Framerate.prototype.snapshot = function()
{
    if (this.renderTime < 0)
        this.renderTime = new Date().getTime();
    else {
        var newTime = new Date().getTime();
        var t = newTime - this.renderTime;
        var framerate = 1000/t;
        this.framerates.push(framerate);
        while (this.framerates.length > this.numFramerates)
            this.framerates.shift();
        this.renderTime = newTime;
    }
}
