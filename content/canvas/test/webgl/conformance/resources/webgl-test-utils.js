



WebGLTestUtils = (function() {





var log = function(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
};





var error = function(msg) {
  if (window.console) {
    if (window.console.error) {
      window.console.error(msg);
    }
    else if (window.console.log) {
      window.console.log(msg);
    }
  }
};




var loggingOff = function() {
  log = function() {};
  error = function() {};
};







var glEnumToString = function(gl, value) {
  for (var p in gl) {
    if (gl[p] == value) {
      return p;
    }
  }
  return "0x" + value.toString(16);
};

var lastError = "";





var getLastError = function() {
  return lastError;
};







var endsWith = function(haystack, needle) {
  return haystack.substr(haystack.length - needle.length) === needle;
};







var startsWith = function(haystack, needle) {
  return haystack.substr(0, needle.length) === needle;
};





var simpleTextureVertexShader = [
  'attribute vec4 vPosition;',
  'attribute vec2 texCoord0;',
  'varying vec2 texCoord;',
  'void main() {',
  '    gl_Position = vPosition;',
  '    texCoord = texCoord0;',
  '}'].join('\n');





var simpleTextureFragmentShader = [
  'precision mediump float;',
  'uniform sampler2D tex;',
  'varying vec2 texCoord;',
  'void main() {',
  '    gl_FragData[0] = texture2D(tex, texCoord);',
  '}'].join('\n');






var setupSimpleTextureVertexShader = function(gl) {
    return loadShader(gl, simpleTextureVertexShader, gl.VERTEX_SHADER);
};






var setupSimpleTextureFragmentShader = function(gl) {
    return loadShader(
        gl, simpleTextureFragmentShader, gl.FRAGMENT_SHADER);
};








var setupProgram = function(gl, shaders, opt_attribs, opt_locations) {
  var program = gl.createProgram();
  for (var ii = 0; ii < shaders.length; ++ii) {
    gl.attachShader(program, shaders[ii]);
  }
  if (opt_attribs) {
    for (var ii = 0; ii < opt_attribs.length; ++ii) {
      gl.bindAttribLocation(
          program,
          opt_locations ? opt_locations[ii] : ii,
          opt_attribs[ii]);
    }
  }
  gl.linkProgram(program);

  
  var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!linked) {
      
      lastError = gl.getProgramInfoLog (program);
      error("Error in program linking:" + lastError);

      gl.deleteProgram(program);
      return null;
  }

  gl.useProgram(program);
  return program;
};








var setupSimpleTextureProgram = function(
    gl, opt_positionLocation, opt_texcoordLocation) {
  opt_positionLocation = opt_positionLocation || 0;
  opt_texcoordLocation = opt_texcoordLocation || 1;
  var vs = setupSimpleTextureVertexShader(gl);
  var fs = setupSimpleTextureFragmentShader(gl);
  if (!vs || !fs) {
    return null;
  }
  var program = setupProgram(
      gl,
      [vs, fs],
      ['vPosition', 'texCoord0'],
      [opt_positionLocation, opt_texcoordLocation]);
  if (!program) {
    gl.deleteShader(fs);
    gl.deleteShader(vs);
  }
  gl.useProgram(program);
  return program;
};









var setupUnitQuad = function(gl, opt_positionLocation, opt_texcoordLocation) {
  opt_positionLocation = opt_positionLocation || 0;
  opt_texcoordLocation = opt_texcoordLocation || 1;
  var objects = [];

  var vertexObject = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vertexObject);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
       1.0,  1.0, 0.0,
      -1.0,  1.0, 0.0,
      -1.0, -1.0, 0.0,
       1.0,  1.0, 0.0,
      -1.0, -1.0, 0.0,
       1.0, -1.0, 0.0]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(opt_positionLocation);
  gl.vertexAttribPointer(opt_positionLocation, 3, gl.FLOAT, false, 0, 0);
  objects.push(vertexObject);

  var vertexObject = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vertexObject);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
      1.0, 1.0,
      0.0, 1.0,
      0.0, 0.0,
      1.0, 1.0,
      0.0, 0.0,
      1.0, 0.0]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(opt_texcoordLocation);
  gl.vertexAttribPointer(opt_texcoordLocation, 2, gl.FLOAT, false, 0, 0);
  objects.push(vertexObject);
  return objects;
};








var setupTexturedQuad = function(
    gl, opt_positionLocation, opt_texcoordLocation) {
  var program = setupSimpleTextureProgram(
      gl, opt_positionLocation, opt_texcoordLocation);
  setupUnitQuad(gl, opt_positionLocation, opt_texcoordLocation);
  return program;
};







var setupQuad = function (
    gl, gridRes, opt_positionLocation, opt_flipOddTriangles) {
  var positionLocation = opt_positionLocation || 0;
  var objects = [];

  var vertsAcross = gridRes + 1;
  var numVerts = vertsAcross * vertsAcross;
  var positions = new Float32Array(numVerts * 3);
  var indices = new Uint16Array(6 * gridRes * gridRes);

  var poffset = 0;

  for (var yy = 0; yy <= gridRes; ++yy) {
    for (var xx = 0; xx <= gridRes; ++xx) {
      positions[poffset + 0] = -1 + 2 * xx / gridRes;
      positions[poffset + 1] = -1 + 2 * yy / gridRes;
      positions[poffset + 2] = 0;

      poffset += 3;
    }
  }

  var tbase = 0;
  for (var yy = 0; yy < gridRes; ++yy) {
    var index = yy * vertsAcross;
    for (var xx = 0; xx < gridRes; ++xx) {
      indices[tbase + 0] = index + 0;
      indices[tbase + 1] = index + 1;
      indices[tbase + 2] = index + vertsAcross;
      indices[tbase + 3] = index + vertsAcross;
      indices[tbase + 4] = index + 1;
      indices[tbase + 5] = index + vertsAcross + 1;

      if (opt_flipOddTriangles) {
        indices[tbase + 4] = index + vertsAcross + 1;
        indices[tbase + 5] = index + 1;
      }

      index += 1;
      tbase += 6;
    }
  }

  var buf = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, buf);
  gl.bufferData(gl.ARRAY_BUFFER, positions, gl.STATIC_DRAW);
  gl.enableVertexAttribArray(positionLocation);
  gl.vertexAttribPointer(positionLocation, 3, gl.FLOAT, false, 0, 0);
  objects.push(buf);

  var buf = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buf);
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);
  objects.push(buf);

  return objects;
};











var fillTexture = function(gl, tex, width, height, color, opt_level) {
  opt_level = opt_level || 0;
  var numPixels = width * height;
  var size = numPixels * 4;
  var buf = new Uint8Array(size);
  for (var ii = 0; ii < numPixels; ++ii) {
    var off = ii * 4;
    buf[off + 0] = color[0];
    buf[off + 1] = color[1];
    buf[off + 2] = color[2];
    buf[off + 3] = color[3];
  }
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.texImage2D(
      gl.TEXTURE_2D, opt_level, gl.RGBA, width, height, 0,
      gl.RGBA, gl.UNSIGNED_BYTE, buf);
  };










var createColoredTexture = function(gl, width, height, color) {
  var tex = gl.createTexture();
  fillTexture(gl, tex, width, height, color);
  return tex;
};








var drawQuad = function(gl, opt_color) {
  opt_color = opt_color || [255, 255, 255, 255];
  gl.clearColor(
      opt_color[0] / 255,
      opt_color[1] / 255,
      opt_color[2] / 255,
      opt_color[3] / 255);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.drawArrays(gl.TRIANGLES, 0, 6);
};














var checkCanvasRect = function(gl, x, y, width, height, color, msg, errorRange) {
  errorRange = errorRange || 0;
  var buf = new Uint8Array(width * height * 4);
  gl.readPixels(x, y, width, height, gl.RGBA, gl.UNSIGNED_BYTE, buf);
  for (var i = 0; i < width * height; ++i) {
    var offset = i * 4;
    for (var j = 0; j < color.length; ++j) {
      if (Math.abs(buf[offset + j] - color[j]) > errorRange) {
        var was = buf[offset + 0].toString();
        for (j = 1; j < color.length; ++j) {
          was += "," + buf[offset + j];
        }

        var cv = document.createElement('canvas');
        cv.height = height;
        cv.width = width;
        var ctx = cv.getContext('2d');
        ctx.fillStyle="rgba(" + color[0] + ", " + color[1] + ", " + color[2] + ", 255)";
        ctx.fillRect(0, 0, width, height);
        testFailedRender(msg, ctx, buf, width, height);

        debug('at (' + (i % width) + ', ' + Math.floor(i / width) +
              ') expected: ' + color + ' was ' + was);
        return;
      }
    }
  }
  testPassed(msg);
};










var checkCanvas = function(gl, color, msg, errorRange) {
  checkCanvasRect(gl, 0, 0, gl.canvas.width, gl.canvas.height, color, msg, errorRange);
};









var loadTexture = function(gl, url, callback) {
    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    var image = new Image();
    image.onload = function() {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
        callback(image);
    };
    image.src = url;
    return texture;
};







var create3DContext = function(opt_canvas, opt_attributes) {
  opt_canvas = opt_canvas || document.createElement("canvas");
  var context = null;
  try {
    context = opt_canvas.getContext("webgl", opt_attributes);
  } catch(e) {}
  if (!context) {
    try {
      context = opt_canvas.getContext("experimental-webgl", opt_attributes);
    } catch(e) {}
  }
  if (!context) {
    testFailed("Unable to fetch WebGL rendering context for Canvas");
  }
  return context;
}







var getGLErrorAsString = function(gl, err) {
  if (err === gl.NO_ERROR) {
    return "NO_ERROR";
  }
  for (var name in gl) {
    if (gl[name] === err) {
      return name;
    }
  }
  return err.toString();
};








var createGLErrorWrapper = function(context, fname) {
  return function() {
    var rv = context[fname].apply(context, arguments);
    var err = context.getError();
    if (err != 0)
      throw "GL error " + getGLErrorAsString(err) + " in " + fname;
    return rv;
  };
};







function create3DContextWithWrapperThatThrowsOnGLError(canvas) {
  var context = create3DContext(canvas);
  var wrap = {};
  for (var i in context) {
    try {
      if (typeof context[i] == 'function') {
        wrap[i] = createGLErrorWrapper(context, i);
      } else {
        wrap[i] = context[i];
      }
    } catch (e) {
      error("createContextWrapperThatThrowsOnGLError: Error accessing " + i);
    }
  }
  wrap.getError = function() {
      return context.getError();
  };
  return wrap;
};







var shouldGenerateGLError = function(gl, glError, evalStr) {
  var exception;
  try {
    eval(evalStr);
  } catch (e) {
    exception = e;
  }
  if (exception) {
    testFailed(evalStr + " threw exception " + exception);
  } else {
    var err = gl.getError();
    if (err != glError) {
      testFailed(evalStr + " expected: " + getGLErrorAsString(gl, glError) + ". Was " + getGLErrorAsString(gl, err) + ".");
    } else {
      testPassed(evalStr + " was expected value: " + getGLErrorAsString(gl, glError) + ".");
    }
  }
};







var glErrorShouldBe = function(gl, glError, opt_msg) {
  opt_msg = opt_msg || "";
  var err = gl.getError();
  if (err != glError) {
    testFailed("getError expected: " + getGLErrorAsString(gl, glError) +
               ". Was " + getGLErrorAsString(gl, err) + " : " + opt_msg);
  } else {
    testPassed("getError was expected value: " +
                getGLErrorAsString(gl, glError) + " : " + opt_msg);
  }
};







var linkProgram = function(gl, program, opt_errorCallback) {
  
  gl.linkProgram(program);

  
  var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!linked) {
    
    var error = gl.getProgramInfoLog (program);

    testFailed("Error in program linking:" + error);

    gl.deleteProgram(program);
  }
};













var setupWebGLWithShaders = function(
   canvasName, vshader, fshader, attribs) {
  var canvas = document.getElementById(canvasName);
  var gl = create3DContext(canvas);
  if (!gl) {
    testFailed("No WebGL context found");
  }

  
  var vertexShader = loadShaderFromScript(gl, vshader);
  var fragmentShader = loadShaderFromScript(gl, fshader);

  if (!vertexShader || !fragmentShader) {
    return null;
  }

  
  program = gl.createProgram();

  if (!program) {
    return null;
  }

  
  gl.attachShader (program, vertexShader);
  gl.attachShader (program, fragmentShader);

  
  for (var i in attribs) {
    gl.bindAttribLocation (program, i, attribs[i]);
  }

  linkProgram(gl, program);

  gl.useProgram(program);

  gl.clearColor(0,0,0,1);
  gl.clearDepth(1);

  gl.enable(gl.DEPTH_TEST);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

  gl.program = program;
  return gl;
};







var loadTextFileAsync = function(url, callback) {
  log ("loading: " + url);
  var error = 'loadTextFileSynchronous failed to load url "' + url + '"';
  var request;
  if (window.XMLHttpRequest) {
    request = new XMLHttpRequest();
    if (request.overrideMimeType) {
      request.overrideMimeType('text/plain');
    }
  } else {
    throw 'XMLHttpRequest is disabled';
  }
  try {
    request.open('GET', url, true);
    request.onreadystatechange = function() {
      if (request.readyState == 4) {
        var text = '';
        
        
        
        
        var success = request.status == 200 || request.status == 0;
        if (success) {
          text = request.responseText;
        }
        log("loaded: " + url);
        callback(success, text);
      }
    };
    request.send(null);
  } catch (e) {
    log("failed to load: " + url);
    callback(false, '');
  }
};










var getFileListAsync = function(url, callback) {
  var files = [];

  var getFileListImpl = function(url, callback) {
    var files = [];
    if (url.substr(url.length - 4) == '.txt') {
      loadTextFileAsync(url, function() {
        return function(success, text) {
          if (!success) {
            callback(false, '');
            return;
          }
          var lines = text.split('\n');
          var prefix = '';
          var lastSlash = url.lastIndexOf('/');
          if (lastSlash >= 0) {
            prefix = url.substr(0, lastSlash + 1);
          }
          var fail = false;
          var count = 1;
          var index = 0;
          for (var ii = 0; ii < lines.length; ++ii) {
            var str = lines[ii].replace(/^\s\s*/, '').replace(/\s\s*$/, '');
            if (str.length > 4 &&
                str[0] != '#' &&
                str[0] != ";" &&
                str.substr(0, 2) != "//") {
              var names = str.split(/ +/);
              new_url = prefix + str;
              if (names.length == 1) {
                new_url = prefix + str;
                ++count;
                getFileListImpl(new_url, function(index) {
                  return function(success, new_files) {
                    log("got files: " + new_files.length);
                    if (success) {
                      files[index] = new_files;
                    }
                    finish(success);
                  };
                }(index++));
              } else {
                var s = "";
                var p = "";
                for (var jj = 0; jj < names.length; ++jj) {
                  s += p + prefix + names[jj];
                  p = " ";
                }
                files[index++] = s;
              }
            }
          }
          finish(true);

          function finish(success) {
            if (!success) {
              fail = true;
            }
            --count;
            log("count: " + count);
            if (!count) {
              callback(!fail, files);
            }
          }
        }
      }());

    } else {
      files.push(url);
      callback(true, files);
    }
  };

  getFileListImpl(url, function(success, files) {
    
    var flat = [];
    flatten(files);
    function flatten(files) {
      for (var ii = 0; ii < files.length; ++ii) {
        var value = files[ii];
        if (typeof(value) == "string") {
          flat.push(value);
        } else {
          flatten(value);
        }
      }
    }
    callback(success, flat);
  });
};






var readFile = function(file) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", file, false);
  xhr.send();
  return xhr.responseText.replace(/\r/g, "");
};

var readFileList = function(url) {
  var files = [];
  if (url.substr(url.length - 4) == '.txt') {
    var lines = readFile(url).split('\n');
    var prefix = '';
    var lastSlash = url.lastIndexOf('/');
    if (lastSlash >= 0) {
      prefix = url.substr(0, lastSlash + 1);
    }
    for (var ii = 0; ii < lines.length; ++ii) {
      var str = lines[ii].replace(/^\s\s*/, '').replace(/\s\s*$/, '');
      if (str.length > 4 &&
          str[0] != '#' &&
          str[0] != ";" &&
          str.substr(0, 2) != "//") {
        var names = str.split(/ +/);
        if (names.length == 1) {
          new_url = prefix + str;
          files = files.concat(readFileList(new_url));
        } else {
          var s = "";
          var p = "";
          for (var jj = 0; jj < names.length; ++jj) {
            s += p + prefix + names[jj];
            p = " ";
          }
          files.push(s);
        }
      }
    }
  } else {
    files.push(url);
  }
  return files;
};









var loadShader = function(gl, shaderSource, shaderType, opt_errorCallback) {
  var errFn = opt_errorCallback || error;
  
  var shader = gl.createShader(shaderType);
  if (shader == null) {
    errFn("*** Error: unable to create shader '"+shaderSource+"'");
    return null;
  }

  
  gl.shaderSource(shader, shaderSource);
  var err = gl.getError();
  if (err != gl.NO_ERROR) {
    errFn("*** Error loading shader '" + shader + "':" + glEnumToString(gl, err));
    return null;
  }

  
  gl.compileShader(shader);

  
  var compiled = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
  if (!compiled) {
    
    lastError = gl.getShaderInfoLog(shader);
    errFn("*** Error compiling shader '" + shader + "':" + lastError);
    gl.deleteShader(shader);
    return null;
  }

  return shader;
}









var loadShaderFromFile = function(gl, file, type, opt_errorCallback) {
  var shaderSource = readFile(file);
  return loadShader(gl, shaderSource, type, opt_errorCallback);
};










var loadShaderFromScript = function(
    gl, scriptId, opt_shaderType, opt_errorCallback) {
  var shaderSource = "";
  var shaderType;
  var shaderScript = document.getElementById(scriptId);
  if (!shaderScript) {
    throw("*** Error: unknown script element" + scriptId);
  }
  shaderSource = shaderScript.text;

  if (!opt_shaderType) {
    if (shaderScript.type == "x-shader/x-vertex") {
      shaderType = gl.VERTEX_SHADER;
    } else if (shaderScript.type == "x-shader/x-fragment") {
      shaderType = gl.FRAGMENT_SHADER;
    } else if (shaderType != gl.VERTEX_SHADER && shaderType != gl.FRAGMENT_SHADER) {
      throw("*** Error: unknown shader type");
      return null;
    }
  }

  return loadShader(
      gl, shaderSource, opt_shaderType ? opt_shaderType : shaderType,
      opt_errorCallback);
};

var loadStandardProgram = function(gl) {
  var program = gl.createProgram();
  gl.attachShader(program, loadStandardVertexShader(gl));
  gl.attachShader(program, loadStandardFragmentShader(gl));
  linkProgram(gl, program);
  return program;
};









var loadProgramFromFile = function(
    gl, vertexShaderPath, fragmentShaderPath, opt_errorCallback) {
  var program = gl.createProgram();
  gl.attachShader(
      program,
      loadShaderFromFile(
          gl, vertexShaderPath, gl.VERTEX_SHADER, opt_errorCallback));
  gl.attachShader(
      program,
      loadShaderFromFile(
          gl, fragmentShaderPath, gl.FRAGMENT_SHADER, opt_errorCallback));
  linkProgram(gl, program, opt_errorCallback);
  return program;
};












var loadProgramFromScript = function loadProgramFromScript(
    gl, vertexScriptId, fragmentScriptId, opt_errorCallback) {
  var program = gl.createProgram();
  gl.attachShader(
      program,
      loadShaderFromScript(
          gl, vertexScriptId, gl.VERTEX_SHADER, opt_errorCallback));
  gl.attachShader(
      program,
      loadShaderFromScript(
          gl, fragmentScriptId,  gl.FRAGMENT_SHADER, opt_errorCallback));
  linkProgram(gl, program, opt_errorCallback);
  return program;
};










var loadProgram = function(
    gl, vertexShader, fragmentShader, opt_errorCallback) {
  var program = gl.createProgram();
  gl.attachShader(
      program,
      loadShader(
          gl, vertexShader, gl.VERTEX_SHADER, opt_errorCallback));
  gl.attachShader(
      program,
      loadShader(
          gl, fragmentShader, gl.FRAGMENT_SHADER, opt_errorCallback));
  linkProgram(gl, program, opt_errorCallback);
  return program;
};

var basePath;
var getBasePath = function() {
  if (!basePath) {
    var expectedBase = "webgl-test-utils.js";
    var scripts = document.getElementsByTagName('script');
    for (var script, i = 0; script = scripts[i]; i++) {
      var src = script.src;
      var l = src.length;
      if (src.substr(l - expectedBase.length) == expectedBase) {
        basePath = src.substr(0, l - expectedBase.length);
      }
    }
  }
  return basePath;
};

var loadStandardVertexShader = function(gl) {
  return loadShaderFromFile(
      gl, getBasePath() + "vertexShader.vert", gl.VERTEX_SHADER);
};

var loadStandardFragmentShader = function(gl) {
  return loadShaderFromFile(
      gl, getBasePath() + "fragmentShader.frag", gl.FRAGMENT_SHADER);
};







var loadImageAsync = function(url, callback) {
  var img = document.createElement('img');
  img.onload = function() {
    callback(img);
  };
  img.src = url;
};







var loadImagesAsync = function(urls, callback) {
  var count = 1;
  var images = { };
  function countDown() {
    --count;
    if (count == 0) {
      callback(images);
    }
  }
  function imageLoaded(url) {
    return function(img) {
      images[url] = img;
      countDown();
    }
  }
  for (var ii = 0; ii < urls.length; ++ii) {
    ++count;
    loadImageAsync(urls[ii], imageLoaded(urls[ii]));
  }
  countDown();
};

var getUrlArguments = function() {
  var args = {};
  try {
    var s = window.location.href;
    var q = s.indexOf("?");
    var e = s.indexOf("#");
    if (e < 0) {
      e = s.length;
    }
    var query = s.substring(q + 1, e);
    var pairs = query.split("&");
    for (var ii = 0; ii < pairs.length; ++ii) {
      var keyValue = pairs[ii].split("=");
      var key = keyValue[0];
      var value = decodeURIComponent(keyValue[1]);
      args[key] = value;
    }
  } catch (e) {
    throw "could not parse url";
  }
  return args;
};

return {
  create3DContext: create3DContext,
  create3DContextWithWrapperThatThrowsOnGLError:
    create3DContextWithWrapperThatThrowsOnGLError,
  checkCanvas: checkCanvas,
  checkCanvasRect: checkCanvasRect,
  createColoredTexture: createColoredTexture,
  drawQuad: drawQuad,
  endsWith: endsWith,
  getFileListAsync: getFileListAsync,
  getLastError: getLastError,
  getUrlArguments: getUrlArguments,
  glEnumToString: glEnumToString,
  glErrorShouldBe: glErrorShouldBe,
  fillTexture: fillTexture,
  loadImageAsync: loadImageAsync,
  loadImagesAsync: loadImagesAsync,
  loadProgram: loadProgram,
  loadProgramFromFile: loadProgramFromFile,
  loadProgramFromScript: loadProgramFromScript,
  loadShader: loadShader,
  loadShaderFromFile: loadShaderFromFile,
  loadShaderFromScript: loadShaderFromScript,
  loadStandardProgram: loadStandardProgram,
  loadStandardVertexShader: loadStandardVertexShader,
  loadStandardFragmentShader: loadStandardFragmentShader,
  loadTextFileAsync: loadTextFileAsync,
  loadTexture: loadTexture,
  log: log,
  loggingOff: loggingOff,
  error: error,
  setupProgram: setupProgram,
  setupQuad: setupQuad,
  setupSimpleTextureFragmentShader: setupSimpleTextureFragmentShader,
  setupSimpleTextureProgram: setupSimpleTextureProgram,
  setupSimpleTextureVertexShader: setupSimpleTextureVertexShader,
  setupTexturedQuad: setupTexturedQuad,
  setupUnitQuad: setupUnitQuad,
  setupWebGLWithShaders: setupWebGLWithShaders,
  startsWith: startsWith,
  shouldGenerateGLError: shouldGenerateGLError,
  readFile: readFile,
  readFileList: readFileList,

  none: false
};

}());


