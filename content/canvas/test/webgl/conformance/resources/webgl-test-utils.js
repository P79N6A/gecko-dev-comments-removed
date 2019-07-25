



WebGLTestUtils = (function() {





var log = function(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
};





var simpleTextureVertexShader = '' +
  'attribute vec4 vPosition;\n' +
  'attribute vec2 texCoord0;\n' +
  'varying vec2 texCoord;\n' +
  'void main() {\n' +
  '    gl_Position = vPosition;\n' +
  '    texCoord = texCoord0;\n' +
  '}\n';





var simpleTextureFragmentShader = '' +
  'uniform sampler2D tex;\n' +
  'varying vec2 texCoord;\n' +
  'void main() {\n' +
  '    gl_FragColor = texture2D(tex, texCoord);\n' +
  '}\n';






var setupSimpleTextureVertexShader = function(gl) {
    return loadShader(gl, simpleTextureVertexShader, gl.VERTEX_SHADER, false);
};






var setupSimpleTextureFragmentShader = function(gl) {
    return loadShader(
        gl, simpleTextureFragmentShader, gl.FRAGMENT_SHADER, false);
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
  var program = gl.createProgram();
  gl.attachShader(program, vs);
  gl.attachShader(program, fs);
  gl.bindAttribLocation(program, opt_positionLocation, 'vPosition');
  gl.bindAttribLocation(program, opt_texcoordLocation, 'texCoord0');
  gl.linkProgram(program);

  
  var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!linked) {
      
      var error = gl.getProgramInfoLog (program);
      log("Error in program linking:"+error);

      gl.deleteProgram(program);
      gl.deleteProgram(fs);
      gl.deleteProgram(vs);

      return null;
  }

  gl.useProgram(program);
  return program;
};







var setupUnitQuad = function(gl, opt_positionLocation, opt_texcoordLocation) {
  opt_positionLocation = opt_positionLocation || 0;
  opt_texcoordLocation = opt_texcoordLocation || 1;

  var vertexObject = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vertexObject);
  gl.bufferData(gl.ARRAY_BUFFER, new WebGLFloatArray(
      [-1,1,0, 1,1,0, -1,-1,0,
       -1,-1,0, 1,1,0, 1,-1,0]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(opt_positionLocation);
  gl.vertexAttribPointer(opt_positionLocation, 3, gl.FLOAT, false, 0, 0);

  var vertexObject = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vertexObject);
  gl.bufferData(gl.ARRAY_BUFFER, new WebGLFloatArray(
      [0,0, 1,0, 0,1,
       0,1, 1,0, 1,1]), gl.STATIC_DRAW);
  gl.enableVertexAttribArray(opt_texcoordLocation);
  gl.vertexAttribPointer(opt_texcoordLocation, 2, gl.FLOAT, false, 0, 0);
};








var setupTexturedQuad = function(
    gl, opt_positionLocation, opt_texcoordLocation) {
  var program = setupSimpleTextureProgram(
      gl, opt_positionLocation, opt_texcoordLocation);
  setupUnitQuad(gl, opt_positionLocation, opt_texcoordLocation);
  return program;
};











var fillTexture = function(gl, tex, width, height, color, opt_level) {
  opt_level = opt_level || 0;
  var canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  var ctx2d = canvas.getContext('2d');
  ctx2d.fillStyle = "rgba(" + color[0] + "," + color[1] + "," + color[2] + "," + color[3] + ")";
  ctx2d.fillRect(0, 0, width, height);
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.texImage2D(gl.TEXTURE_2D, opt_level, canvas);
};










var createColoredTexture = function(gl, width, height, color) {
  var tex = gl.createTexture();
  fillTexture(gl, text, width, height, color);
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








var checkCanvas = function(gl, color, msg) {
  var width = gl.canvas.width;
  var height = gl.canvas.height;
  var buf = gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE);
  for (var i = 0; i < width * height; ++i) {
    var offset = i * 4;
    if (buf[offset + 0] != color[0] ||
        buf[offset + 1] != color[1] ||
        buf[offset + 2] != color[2] ||
        buf[offset + 3] != color[3]) {
      testFailed(msg);
      debug('expected: ' +
          color[0] + ', ' +
          color[1] + ', ' +
          color[2] + ', ' +
          color[3] + ' was: ' +
          buf[offset + 0] + ', ' +
          buf[offset + 1] + ', ' +
          buf[offset + 2] + ', ' +
          buf[offset + 3]);
      return;
    }
  }
  testPassed(msg);
};







var create3DContext = function(opt_canvas) {
  opt_canvas = opt_canvas || document.createElement("canvas");
  var context = null;
  try {
    context = opt_canvas.getContext("experimental-webgl");
  } catch(e) {}
  if (!context) {
    try {
      context = opt_canvas.getContext("webkit-3d");
    } catch(e) {}
  }
  if (!context) {
    try {
      context = opt_canvas.getContext("moz-webgl");
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
      log("createContextWrapperThatThrowsOnGLError: Error accessing " + i);
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






var glErrorShouldBe = function(gl, glError) {
  var err = gl.getError();
  if (err != glError) {
    testFailed("getError expected: " + getGLErrorAsString(gl, glError) + ". Was " + getGLErrorAsString(gl, err) + ".");
  } else {
    testPassed("getError was expected value: " + getGLErrorAsString(gl, glError) + ".");
  }
};






var linkProgram = function(gl, program) {
  
  gl.linkProgram(program);

  
  var linked = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!linked) {
    
    var error = gl.getProgramInfoLog (program);

    gl.deleteProgram(program);
    gl.deleteProgram(fragmentShader);
    gl.deleteProgram(vertexShader);

    testFailed("Error in program linking:" + error);
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

  gl.enable(gl.DEPTH_TEST);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

  gl.program = program;
  return gl;
};






var getShaderSource = function(file) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", file, false);
  xhr.send();
  return xhr.responseText;
};








var loadShader = function(gl, shaderSource, shaderType) {
  
  var shader = gl.createShader(shaderType);
  if (shader == null) {
    log("*** Error: unable to create shader '"+shaderSource+"'");
    return null;
  }

  
  gl.shaderSource(shader, shaderSource);

  
  gl.compileShader(shader);

  
  var compiled = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
  if (!compiled) {
    
    var error = gl.getShaderInfoLog(shader);
    log("*** Error compiling shader '"+shader+"':"+error);
    gl.deleteShader(shader);
    return null;
  }

  return shader;
}








var loadShaderFromFile = function(gl, file, type) {
  var shaderSource = getShaderSource(file);
  return loadShader(gl, shaderSource, type);
};









var loadShaderFromScript = function(gl, scriptId, opt_shaderType) {
  var shaderSource = "";

  var shaderScript = document.getElementById(scriptId);
  if (!shaderScript) {
    throw("*** Error: unknown script element" + scriptId);
  } else if (!opt_shaderType) {
    if (shaderScript.type == "x-shader/x-vertex") {
      opt_shaderType = gl.VERTEX_SHADER;
    } else if (shaderScript.type == "x-shader/x-fragment") {
      opt_shaderType = gl.FRAGMENT_SHADER;
    } else if (shaderType != gl.VERTEX_SHADER && shaderType != gl.FRAGMENT_SHADER) {
      throw("*** Error: unknown shader type");
      return null;
    }

    shaderSource = shaderScript.text;
  }

  return loadShader(
      gl, shaderSource, opt_shaderType ? opt_shaderType : shaderType);
};

var loadStandardProgram = function(gl) {
  var program = gl.createProgram();
  gl.attachShader(program, loadStandardVertexShader(gl));
  gl.attachShader(program, loadStandardFragmentShader(gl));
  linkProgram(gl, program);
  return program;
};








var loadProgramFromFile = function(gl, vertexShaderPath, fragmentShaderPath) {
  var program = gl.createProgram();
  gl.attachShader(
      program,
      loadShaderFromFile(gl, vertexShaderPath, gl.VERTEX_SHADER));
  gl.attachShader(
      program,
      loadShaderFromFile(gl, fragmentShaderPath, gl.FRAGMENT_SHADER));
  linkProgram(gl, program);
  return program;
};











var loadProgramFromScript = function loadProgramFromScript(
  gl, vertexScriptId, fragmentScriptId) {
  var program = gl.createProgram();
  gl.attachShader(
      program,
      loadShaderFromScript(gl, vertexScriptId, gl.VERTEX_SHADER));
  gl.attachShader(
      program,
      loadShaderFromScript(gl, fragmentScriptId,  gl.FRAGMENT_SHADER));
  linkProgram(gl, program);
  return program;
};

var loadStandardVertexShader = function(gl) {
  return loadShaderFromFile(
      gl, "resources/vertexShader.vert", gl.VERTEX_SHADER);
};

var loadStandardFragmentShader = function(gl) {
  return loadShaderFromfile(
      gl, "resources/fragmentShader.frag", gl.FRAGMENT_SHADER);
};

return {
    create3DContext: create3DContext,
    create3DContextWithWrapperThatThrowsOnGLError:
        create3DContextWithWrapperThatThrowsOnGLError,
    checkCanvas: checkCanvas,
    createColoredTexture: createColoredTexture,
    drawQuad: drawQuad,
    glErrorShouldBe: glErrorShouldBe,
    fillTexture: fillTexture,
    loadProgramFromFile: loadProgramFromFile,
    loadProgramFromScript: loadProgramFromScript,
    loadShader: loadShader,
    loadShaderFromFile: loadShaderFromFile,
    loadShaderFromScript: loadShaderFromScript,
    loadStandardProgram: loadStandardProgram,
    loadStandardVertexShader: loadStandardVertexShader,
    loadStandardFragmentShader: loadStandardFragmentShader,
    setupSimpleTextureFragmentShader: setupSimpleTextureFragmentShader,
    setupSimpleTextureProgram: setupSimpleTextureProgram,
    setupSimpleTextureVertexShader: setupSimpleTextureVertexShader,
    setupTexturedQuad: setupTexturedQuad,
    setupUnitQuad: setupUnitQuad,
    shouldGenerateGLError: shouldGenerateGLError,

    none: false
};

}());


