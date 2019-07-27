GLSLGenerator = (function() {

var vertexShaderTemplate = [
  "attribute vec4 aPosition;",
  "",
  "varying vec4 vColor;",
  "",
  "$(extra)",
  "$(emu)",
  "",
  "void main()",
  "{",
  "   gl_Position = aPosition;",
  "   vec2 texcoord = vec2(aPosition.xy * 0.5 + vec2(0.5, 0.5));",
  "   vec4 color = vec4(",
  "       texcoord,",
  "       texcoord.x * texcoord.y,",
  "       (1.0 - texcoord.x) * texcoord.y * 0.5 + 0.5);",
  "   $(test)",
  "}"
].join("\n");

var fragmentShaderTemplate = [
  "#if defined(GL_ES)",
  "precision mediump float;",
  "#endif",
  "",
  "varying vec4 vColor;",
  "",
  "$(extra)",
  "$(emu)",
  "",
  "void main()",
  "{",
  "   $(test)",
  "}"
].join("\n");

var baseVertexShader = [
  "attribute vec4 aPosition;",
  "",
  "varying vec4 vColor;",
  "",
  "void main()",
  "{",
  "   gl_Position = aPosition;",
  "   vec2 texcoord = vec2(aPosition.xy * 0.5 + vec2(0.5, 0.5));",
  "   vColor = vec4(",
  "       texcoord,",
  "       texcoord.x * texcoord.y,",
  "       (1.0 - texcoord.x) * texcoord.y * 0.5 + 0.5);",
  "}"
].join("\n");

var baseFragmentShader = [
  "#if defined(GL_ES)",
  "precision mediump float;",
  "#endif",
  "varying vec4 vColor;",
  "",
  "void main()",
  "{",
  "   gl_FragColor = vColor;",
  "}"
].join("\n");

var types = [
  { type: "float",
    code: [
      "float $(func)_emu($(args)) {",
      "  return $(func)_base($(baseArgs));",
      "}"].join("\n")
  },
  { type: "vec2",
    code: [
      "vec2 $(func)_emu($(args)) {",
      "  return vec2(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)));",
      "}"].join("\n")
  },
  { type: "vec3",
    code: [
      "vec3 $(func)_emu($(args)) {",
      "  return vec3(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)),",
      "      $(func)_base($(baseArgsZ)));",
      "}"].join("\n")
  },
  { type: "vec4",
    code: [
      "vec4 $(func)_emu($(args)) {",
      "  return vec4(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)),",
      "      $(func)_base($(baseArgsZ)),",
      "      $(func)_base($(baseArgsW)));",
      "}"].join("\n")
  }
];

var bvecTypes = [
  { type: "bvec2",
    code: [
      "bvec2 $(func)_emu($(args)) {",
      "  return bvec2(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)));",
      "}"].join("\n")
  },
  { type: "bvec3",
    code: [
      "bvec3 $(func)_emu($(args)) {",
      "  return bvec3(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)),",
      "      $(func)_base($(baseArgsZ)));",
      "}"].join("\n")
  },
  { type: "bvec4",
    code: [
      "vec4 $(func)_emu($(args)) {",
      "  return bvec4(",
      "      $(func)_base($(baseArgsX)),",
      "      $(func)_base($(baseArgsY)),",
      "      $(func)_base($(baseArgsZ)),",
      "      $(func)_base($(baseArgsW)));",
      "}"].join("\n")
  }
];

var replaceRE = /\$\((\w+)\)/g;

var replaceParams = function(str) {
  var args = arguments;
  return str.replace(replaceRE, function(str, p1, offset, s) {
    for (var ii = 1; ii < args.length; ++ii) {
      if (args[ii][p1] !== undefined) {
        return args[ii][p1];
      }
    }
    throw "unknown string param '" + p1 + "'";
  });
};

var generateReferenceShader = function(
    shaderInfo, template, params, typeInfo, test) {
  var input = shaderInfo.input;
  var output = shaderInfo.output;
  var feature = params.feature;
  var testFunc = params.testFunc;
  var emuFunc = params.emuFunc || "";
  var extra = params.extra || '';
  var args = params.args || "$(type) value";
  var type = typeInfo.type;
  var typeCode = typeInfo.code;

  var baseArgs = params.baseArgs || "value$(field)";
  var baseArgsX = replaceParams(baseArgs, {field: ".x"});
  var baseArgsY = replaceParams(baseArgs, {field: ".y"});
  var baseArgsZ = replaceParams(baseArgs, {field: ".z"});
  var baseArgsW = replaceParams(baseArgs, {field: ".w"});
  var baseArgs = replaceParams(baseArgs, {field: ""});

  test = replaceParams(test, {
    input: input,
    output: output,
    func: feature + "_emu"
  });
  emuFunc = replaceParams(emuFunc, {
    func: feature
  });
  args = replaceParams(args, {
    type: type
  });
  typeCode = replaceParams(typeCode, {
    func: feature,
    type: type,
    args: args,
    baseArgs: baseArgs,
    baseArgsX: baseArgsX,
    baseArgsY: baseArgsY,
    baseArgsZ: baseArgsZ,
    baseArgsW: baseArgsW
  });
  var shader = replaceParams(template, {
    extra: extra,
    emu: emuFunc + "\n\n" + typeCode,
    test: test
  });
  return shader;
};

var generateTestShader = function(
    shaderInfo, template, params, test) {
  var input = shaderInfo.input;
  var output = shaderInfo.output;
  var feature = params.feature;
  var testFunc = params.testFunc;
  var extra = params.extra || '';

  test = replaceParams(test, {
    input: input,
    output: output,
    func: feature
  });
  var shader = replaceParams(template, {
    extra: extra,
    emu: '',
    test: test
  });
  return shader;
};

var runFeatureTest = function(params) {
  if (window.initNonKhronosFramework) {
    window.initNonKhronosFramework(false);
  }

  var wtu = WebGLTestUtils;
  var gridRes = params.gridRes;
  var vertexTolerance = params.tolerance || 0;
  var fragmentTolerance = vertexTolerance;
  if ('fragmentTolerance' in params)
    fragmentTolerance = params.fragmentTolerance || 0;

  description("Testing GLSL feature: " + params.feature);

  var width = 32;
  var height = 32;

  var console = document.getElementById("console");
  var canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  var gl = wtu.create3DContext(canvas);
  if (!gl) {
    testFailed("context does not exist");
    finishTest();
    return;
  }

  var canvas2d = document.createElement('canvas');
  canvas2d.width = width;
  canvas2d.height = height;
  var ctx = canvas2d.getContext("2d");
  var imgData = ctx.getImageData(0, 0, width, height);

  var shaderInfos = [
    { type: "vertex",
      input: "color",
      output: "vColor",
      vertexShaderTemplate: vertexShaderTemplate,
      fragmentShaderTemplate: baseFragmentShader,
      tolerance: vertexTolerance
    },
    { type: "fragment",
      input: "vColor",
      output: "gl_FragColor",
      vertexShaderTemplate: baseVertexShader,
      fragmentShaderTemplate: fragmentShaderTemplate,
      tolerance: fragmentTolerance
    }
  ];
  for (var ss = 0; ss < shaderInfos.length; ++ss) {
    var shaderInfo = shaderInfos[ss];
    var tests = params.tests;
    var testTypes = params.emuFuncs || (params.bvecTest ? bvecTypes : types);
    
    for (var ii = 0; ii < tests.length; ++ii) {
      var type = testTypes[ii];
      if (params.simpleEmu) {
        type = {
          type: type.type,
          code: params.simpleEmu
        };
      }
      debug("");
      var str = replaceParams(params.testFunc, {
        func: params.feature,
        type: type.type,
        arg0: type.type
      });
      debug("Testing: " + str + " in " + shaderInfo.type + " shader");

      var referenceVertexShaderSource = generateReferenceShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          params,
          type,
          tests[ii]);
      var referenceFragmentShaderSource = generateReferenceShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          params,
          type,
          tests[ii]);
      var testVertexShaderSource = generateTestShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          params,
          tests[ii]);
      var testFragmentShaderSource = generateTestShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          params,
          tests[ii]);

      debug("");
      wtu.addShaderSource(
          console, "reference vertex shader", referenceVertexShaderSource);
      wtu.addShaderSource(
          console, "reference fragment shader", referenceFragmentShaderSource);
      wtu.addShaderSource(
          console, "test vertex shader", testVertexShaderSource);
      wtu.addShaderSource(
          console, "test fragment shader", testFragmentShaderSource);
      debug("");

      var refData = draw(
          canvas, referenceVertexShaderSource, referenceFragmentShaderSource);
      var refImg = wtu.makeImage(canvas);
      if (ss == 0) {
        var testData = draw(
            canvas, testVertexShaderSource, referenceFragmentShaderSource);
      } else {
        var testData = draw(
            canvas, referenceVertexShaderSource, testFragmentShaderSource);
      }
      var testImg = wtu.makeImage(canvas);

      reportResults(refData, refImg, testData, testImg, shaderInfo.tolerance);
    }
  }

  finishTest();

  function reportResults(refData, refImage, testData, testImage, tolerance) {
    var same = true;
    for (var yy = 0; yy < height; ++yy) {
      for (var xx = 0; xx < width; ++xx) {
        var offset = (yy * width + xx) * 4;
        var imgOffset = ((height - yy - 1) * width + xx) * 4;
        imgData.data[imgOffset + 0] = 0;
        imgData.data[imgOffset + 1] = 0;
        imgData.data[imgOffset + 2] = 0;
        imgData.data[imgOffset + 3] = 255;
        if (Math.abs(refData[offset + 0] - testData[offset + 0]) > tolerance ||
            Math.abs(refData[offset + 1] - testData[offset + 1]) > tolerance ||
            Math.abs(refData[offset + 2] - testData[offset + 2]) > tolerance ||
            Math.abs(refData[offset + 3] - testData[offset + 3]) > tolerance) {
          imgData.data[imgOffset] = 255;
          same = false;
        }
      }
    }

    var diffImg = null;
    if (!same) {
      ctx.putImageData(imgData, 0, 0);
      diffImg = wtu.makeImage(canvas2d);
    }

    var div = document.createElement("div");
    div.className = "testimages";
    wtu.insertImage(div, "ref", refImg);
    wtu.insertImage(div, "test", testImg);
    if (diffImg) {
      wtu.insertImage(div, "diff", diffImg);
    }
    div.appendChild(document.createElement('br'));


    console.appendChild(div);

    if (!same) {
      testFailed("images are different");
    } else {
      testPassed("images are the same");
    }

    console.appendChild(document.createElement('hr'));
  }

  function draw(canvas, vsSource, fsSource) {
    var program = wtu.loadProgram(gl, vsSource, fsSource, testFailed);

    var posLoc = gl.getAttribLocation(program, "aPosition");
    WebGLTestUtils.setupQuad(gl, gridRes, posLoc);

    gl.useProgram(program);
    gl.clearColor(0, 0, 1, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.drawElements(gl.TRIANGLES, gridRes * gridRes * 6, gl.UNSIGNED_SHORT, 0);
    wtu.glErrorShouldBe(gl, gl.NO_ERROR, "no errors from draw");

    var img = new Uint8Array(width * height * 4);
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, img);
    return img;
  }

};

var runBasicTest = function(params) {
  if (window.initNonKhronosFramework) {
    window.initNonKhronosFramework(false);
  }

  var wtu = WebGLTestUtils;
  var gridRes = params.gridRes;
  var vertexTolerance = params.tolerance || 0;
  var fragmentTolerance = vertexTolerance;
  if ('fragmentTolerance' in params)
    fragmentTolerance = params.fragmentTolerance || 0;

  description("Testing : " + document.getElementsByTagName("title")[0].innerText);

  var width = 32;
  var height = 32;

  var console = document.getElementById("console");
  var canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  var gl = wtu.create3DContext(canvas);
  if (!gl) {
    testFailed("context does not exist");
    finishTest();
    return;
  }

  var canvas2d = document.createElement('canvas');
  canvas2d.width = width;
  canvas2d.height = height;
  var ctx = canvas2d.getContext("2d");
  var imgData = ctx.getImageData(0, 0, width, height);

  var shaderInfos = [
    { type: "vertex",
      input: "color",
      output: "vColor",
      vertexShaderTemplate: vertexShaderTemplate,
      fragmentShaderTemplate: baseFragmentShader,
      tolerance: vertexTolerance
    },
    { type: "fragment",
      input: "vColor",
      output: "gl_FragColor",
      vertexShaderTemplate: baseVertexShader,
      fragmentShaderTemplate: fragmentShaderTemplate,
      tolerance: fragmentTolerance
    }
  ];
  for (var ss = 0; ss < shaderInfos.length; ++ss) {
    var shaderInfo = shaderInfos[ss];
    var tests = params.tests;

    
    for (var ii = 0; ii < tests.length; ++ii) {
      var test = tests[ii];
      debug("");
      debug("Testing: " + test.name + " in " + shaderInfo.type + " shader");

      function genShader(shaderInfo, template, shader, subs) {
        shader = replaceParams(shader, subs, {
            input: shaderInfo.input,
            output: shaderInfo.output
          });
        shader = replaceParams(template, subs, {
            test: shader,
            emu: "",
            extra: ""
          });
        return shader;
      }

      var referenceVertexShaderSource = genShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          test.reference.shader,
          test.reference.subs);
      var referenceFragmentShaderSource = genShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          test.reference.shader,
          test.reference.subs);
      var testVertexShaderSource = genShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          test.test.shader,
          test.test.subs);
      var testFragmentShaderSource = genShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          test.test.shader,
          test.test.subs);

      debug("");
      wtu.addShaderSource(
          console, "reference vertex shader", referenceVertexShaderSource);
      wtu.addShaderSource(
          console, "reference fragment shader", referenceFragmentShaderSource);
      wtu.addShaderSource(
          console, "test vertex shader", testVertexShaderSource);
      wtu.addShaderSource(
          console, "test fragment shader", testFragmentShaderSource);
      debug("");

      var refData = draw(
          canvas, referenceVertexShaderSource, referenceFragmentShaderSource);
      var refImg = wtu.makeImage(canvas);
      if (ss == 0) {
        var testData = draw(
            canvas, testVertexShaderSource, referenceFragmentShaderSource);
      } else {
        var testData = draw(
            canvas, referenceVertexShaderSource, testFragmentShaderSource);
      }
      var testImg = wtu.makeImage(canvas);

      reportResults(refData, refImg, testData, testImg, shaderInfo.tolerance);
    }
  }

  finishTest();

  function reportResults(refData, refImage, testData, testImage, tolerance) {
    var same = true;
    for (var yy = 0; yy < height; ++yy) {
      for (var xx = 0; xx < width; ++xx) {
        var offset = (yy * width + xx) * 4;
        var imgOffset = ((height - yy - 1) * width + xx) * 4;
        imgData.data[imgOffset + 0] = 0;
        imgData.data[imgOffset + 1] = 0;
        imgData.data[imgOffset + 2] = 0;
        imgData.data[imgOffset + 3] = 255;
        if (Math.abs(refData[offset + 0] - testData[offset + 0]) > tolerance ||
            Math.abs(refData[offset + 1] - testData[offset + 1]) > tolerance ||
            Math.abs(refData[offset + 2] - testData[offset + 2]) > tolerance ||
            Math.abs(refData[offset + 3] - testData[offset + 3]) > tolerance) {
          imgData.data[imgOffset] = 255;
          same = false;
        }
      }
    }

    var diffImg = null;
    if (!same) {
      ctx.putImageData(imgData, 0, 0);
      diffImg = wtu.makeImage(canvas2d);
    }

    var div = document.createElement("div");
    div.className = "testimages";
    wtu.insertImage(div, "ref", refImg);
    wtu.insertImage(div, "test", testImg);
    if (diffImg) {
      wtu.insertImage(div, "diff", diffImg);
    }
    div.appendChild(document.createElement('br'));

    console.appendChild(div);

    if (!same) {
      testFailed("images are different");
    } else {
      testPassed("images are the same");
    }

    console.appendChild(document.createElement('hr'));
  }

  function draw(canvas, vsSource, fsSource) {
    var program = wtu.loadProgram(gl, vsSource, fsSource, testFailed);

    var posLoc = gl.getAttribLocation(program, "aPosition");
    WebGLTestUtils.setupQuad(gl, gridRes, posLoc);

    gl.useProgram(program);
    gl.clearColor(0, 0, 1, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.drawElements(gl.TRIANGLES, gridRes * gridRes * 6, gl.UNSIGNED_SHORT, 0);
    wtu.glErrorShouldBe(gl, gl.NO_ERROR, "no errors from draw");

    var img = new Uint8Array(width * height * 4);
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, img);
    return img;
  }

};

var runReferenceImageTest = function(params) {
  if (window.initNonKhronosFramework) {
    window.initNonKhronosFramework(false);
  }

  var wtu = WebGLTestUtils;
  var gridRes = params.gridRes;
  var vertexTolerance = params.tolerance || 0;
  var fragmentTolerance = vertexTolerance;
  if ('fragmentTolerance' in params)
    fragmentTolerance = params.fragmentTolerance || 0;

  description("Testing GLSL feature: " + params.feature);

  var width = 32;
  var height = 32;

  var console = document.getElementById("console");
  var canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  var gl = wtu.create3DContext(canvas, { antialias: false });
  if (!gl) {
    testFailed("context does not exist");
    finishTest();
    return;
  }

  var canvas2d = document.createElement('canvas');
  canvas2d.width = width;
  canvas2d.height = height;
  var ctx = canvas2d.getContext("2d");
  var imgData = ctx.getImageData(0, 0, width, height);

  var shaderInfos = [
    { type: "vertex",
      input: "color",
      output: "vColor",
      vertexShaderTemplate: vertexShaderTemplate,
      fragmentShaderTemplate: baseFragmentShader,
      tolerance: vertexTolerance
    },
    { type: "fragment",
      input: "vColor",
      output: "gl_FragColor",
      vertexShaderTemplate: baseVertexShader,
      fragmentShaderTemplate: fragmentShaderTemplate,
      tolerance: fragmentTolerance
    }
  ];
  for (var ss = 0; ss < shaderInfos.length; ++ss) {
    var shaderInfo = shaderInfos[ss];
    var tests = params.tests;
    var testTypes = params.emuFuncs || (params.bvecTest ? bvecTypes : types);
    
    for (var ii = 0; ii < tests.length; ++ii) {
      var type = testTypes[ii];
      var isVertex = (ss == 0);
      debug("");
      var str = replaceParams(params.testFunc, {
        func: params.feature,
        type: type.type,
        arg0: type.type
      });
      debug("Testing: " + str + " in " + shaderInfo.type + " shader");

      var referenceVertexShaderSource = generateReferenceShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          params,
          type,
          tests[ii].source);
      var referenceFragmentShaderSource = generateReferenceShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          params,
          type,
          tests[ii].source);
      var testVertexShaderSource = generateTestShader(
          shaderInfo,
          shaderInfo.vertexShaderTemplate,
          params,
          tests[ii].source);
      var testFragmentShaderSource = generateTestShader(
          shaderInfo,
          shaderInfo.fragmentShaderTemplate,
          params,
          tests[ii].source);
      var referenceTexture = generateReferenceTexture(
          gl,
          tests[ii].generator,
          isVertex ? gridRes : width,
          isVertex ? gridRes : height,
          isVertex);

      debug("");
      wtu.addShaderSource(
          console, "test vertex shader", testVertexShaderSource);
      wtu.addShaderSource(
          console, "test fragment shader", testFragmentShaderSource);
      debug("");
      var refData = drawReferenceImage(canvas, referenceTexture, isVertex);
      var refImg = wtu.makeImage(canvas);
      if (isVertex) {
        var testData = draw(
            canvas, testVertexShaderSource, referenceFragmentShaderSource);
      } else {
        var testData = draw(
            canvas, referenceVertexShaderSource, testFragmentShaderSource);
      }
      var testImg = wtu.makeImage(canvas);
      var testTolerance = shaderInfo.tolerance;
      
      if ('tolerance' in tests[ii])
        testTolerance = tests[ii].tolerance || 0;
      reportResults(refData, refImg, testData, testImg, testTolerance);
    }
  }

  finishTest();

  function reportResults(refData, refImage, testData, testImage, tolerance) {
    var same = true;
    for (var yy = 0; yy < height; ++yy) {
      for (var xx = 0; xx < width; ++xx) {
        var offset = (yy * width + xx) * 4;
        var imgOffset = ((height - yy - 1) * width + xx) * 4;
        imgData.data[imgOffset + 0] = 0;
        imgData.data[imgOffset + 1] = 0;
        imgData.data[imgOffset + 2] = 0;
        imgData.data[imgOffset + 3] = 255;
        if (Math.abs(refData[offset + 0] - testData[offset + 0]) > tolerance ||
            Math.abs(refData[offset + 1] - testData[offset + 1]) > tolerance ||
            Math.abs(refData[offset + 2] - testData[offset + 2]) > tolerance ||
            Math.abs(refData[offset + 3] - testData[offset + 3]) > tolerance) {
          console.appendChild(document.createTextNode('at (' + xx + ',' + yy + '): ref=(' +
                                                      refData[offset + 0] + ',' +
                                                      refData[offset + 1] + ',' +
                                                      refData[offset + 2] + ',' +
                                                      refData[offset + 3] + ')  test=(' +
                                                      testData[offset + 0] + ',' +
                                                      testData[offset + 1] + ',' +
                                                      testData[offset + 2] + ',' +
                                                      testData[offset + 3] + ')'));
          console.appendChild(document.createElement('br'));          



          imgData.data[imgOffset] = 255;
          same = false;
        }
      }
    }

    var diffImg = null;
    if (!same) {
      ctx.putImageData(imgData, 0, 0);
      diffImg = wtu.makeImage(canvas2d);
    }

    var div = document.createElement("div");
    div.className = "testimages";
    wtu.insertImage(div, "ref", refImg);
    wtu.insertImage(div, "test", testImg);
    if (diffImg) {
      wtu.insertImage(div, "diff", diffImg);
    }
    div.appendChild(document.createElement('br'));

    console.appendChild(div);

    if (!same) {
      testFailed("images are different");
    } else {
      testPassed("images are the same");
    }

    console.appendChild(document.createElement('hr'));
  }

  function draw(canvas, vsSource, fsSource) {
    var program = wtu.loadProgram(gl, vsSource, fsSource, testFailed);

    var posLoc = gl.getAttribLocation(program, "aPosition");
    WebGLTestUtils.setupQuad(gl, gridRes, posLoc);

    gl.useProgram(program);
    gl.clearColor(0, 0, 1, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.drawElements(gl.TRIANGLES, gridRes * gridRes * 6, gl.UNSIGNED_SHORT, 0);
    wtu.glErrorShouldBe(gl, gl.NO_ERROR, "no errors from draw");

    var img = new Uint8Array(width * height * 4);
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, img);
    return img;
  }

  function drawReferenceImage(canvas, texture, isVertex) {
    var program;
    if (isVertex) {
      var halfTexel = 0.5 / (1.0 + gridRes);
      program = WebGLTestUtils.setupTexturedQuadWithTexCoords(
        gl, [halfTexel, halfTexel], [1.0 - halfTexel, 1.0 - halfTexel]);
    } else {
      program = WebGLTestUtils.setupTexturedQuad(gl);
    }

    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, texture);
    var texLoc = gl.getUniformLocation(program, "tex");
    gl.uniform1i(texLoc, 0);
    wtu.drawQuad(gl);
    wtu.glErrorShouldBe(gl, gl.NO_ERROR, "no errors from draw");

    var img = new Uint8Array(width * height * 4);
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, img);
    return img;
  }

  
















  function generateReferenceTexture(
    gl,
    generator,
    width,
    height,
    isVertex) {

    
    
    function computeTexCoord(x) {
      return x * 0.5 + 0.5;
    }

    function computeColor(texCoordX, texCoordY) {
      return [ texCoordX,
               texCoordY,
               texCoordX * texCoordY,
               (1.0 - texCoordX) * texCoordY * 0.5 + 0.5 ];
    }

    function clamp(value, minVal, maxVal) {
      return Math.max(minVal, Math.min(value, maxVal));
    }

    
    
    
    function evaluateAtClipCoords(px, py, pixel) {
      var tcx = computeTexCoord(px);
      var tcy = computeTexCoord(py);

      var color = computeColor(tcx, tcy);

      var output = generator(color[0], color[1], color[2], color[3]);

      
      
      pixel[0] = clamp(Math.round(256 * output[0]), 0, 255);
      pixel[1] = clamp(Math.round(256 * output[1]), 0, 255);
      pixel[2] = clamp(Math.round(256 * output[2]), 0, 255);
      pixel[3] = clamp(Math.round(256 * output[3]), 0, 255);
    }

    function fillFragmentReference() {
      var data = new Uint8Array(4 * width * height);

      var horizTexel = 1.0 / width;
      var vertTexel = 1.0 / height;
      var halfHorizTexel = 0.5 * horizTexel;
      var halfVertTexel = 0.5 * vertTexel;

      var pixel = new Array(4);

      for (var yi = 0; yi < height; ++yi) {
        for (var xi = 0; xi < width; ++xi) {
          

          
          var px = -1.0 + 2.0 * (halfHorizTexel + xi * horizTexel);
          var py = -1.0 + 2.0 * (halfVertTexel + yi * vertTexel);

          evaluateAtClipCoords(px, py, pixel);
          var index = 4 * (width * yi + xi);
          data[index + 0] = pixel[0];
          data[index + 1] = pixel[1];
          data[index + 2] = pixel[2];
          data[index + 3] = pixel[3];
        }
      }

      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, data);
    }

    function fillVertexReference() {
      
      
      
      
      if (width != height) {
        throw "width and height must be equal";
      }

      var texSize = 1 + width;
      var data = new Uint8Array(4 * texSize * texSize);

      var step = 2.0 / width;

      var pixel = new Array(4);

      for (var yi = 0; yi < texSize; ++yi) {
        for (var xi = 0; xi < texSize; ++xi) {
          

          
          var px = -1.0 + (xi * step);
          var py = -1.0 + (yi * step);

          evaluateAtClipCoords(px, py, pixel);
          var index = 4 * (texSize * yi + xi);
          data[index + 0] = pixel[0];
          data[index + 1] = pixel[1];
          data[index + 2] = pixel[2];
          data[index + 3] = pixel[3];
        }
      }

      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, texSize, texSize, 0,
                    gl.RGBA, gl.UNSIGNED_BYTE, data);
    }

    
    
    

    var texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    if (isVertex) {
      fillVertexReference();
    } else {
      fillFragmentReference();
    }

    return texture;
  }
};

return {
  




























































  runFeatureTest: runFeatureTest,

  









































































  runBasicTest: runBasicTest,

  

















































  runReferenceImageTest: runReferenceImageTest,

  none: false
};

}());

