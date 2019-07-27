



"use strict";






function CaptureStreamTestHelper(width, height) {
  this.cout = document.createElement('canvas');
  if (width) {
    this.elemWidth = width;
  }
  if (height) {
    this.elemHeight = height;
  }
  this.cout.width = this.elemWidth;
  this.cout.height = this.elemHeight;
  document.body.appendChild(this.cout);
}

CaptureStreamTestHelper.prototype = {
  
  black: { data: [0, 0, 0, 255], name: "black" },
  green: { data: [0, 255, 0, 255], name: "green" },
  red: { data: [255, 0, 0, 255], name: "red" },

  
  elemWidth: 100,
  elemHeight: 100,

  
  requestFrame: function (video) {
    info("Requesting frame from " + video.id);
    video.mozSrcObject.requestFrame();
  },

  
  testPixel: function (video, refData, threshold) {
    var ctxout = this.cout.getContext('2d');
    ctxout.drawImage(video, 0, 0);
    var pixel = ctxout.getImageData(0, 0, 1, 1).data;
    return pixel.every((val, i) => Math.abs(val - refData[i]) <= threshold);
  },

  



  waitForPixel: function (video, refColor, threshold, infoString) {
    return new Promise(resolve => {
      info("Testing " + video.id + " against [" + refColor.data.join(',') + "]");
      CaptureStreamTestHelper2D.prototype.clear.call(this, this.cout);
      video.ontimeupdate = () => {
        if (this.testPixel(video, refColor.data, threshold)) {
          ok(true, video.id + " " + infoString);
          video.ontimeupdate = null;
          resolve();
        }
      };
    });
  },

  




  waitForPixelToTimeout: function (video, refColor, threshold, timeout, infoString) {
    return new Promise(resolve => {
      info("Waiting for " + video.id + " to time out after " + timeout +
           "ms against [" + refColor.data.join(',') + "] - " + refColor.name);
      CaptureStreamTestHelper2D.prototype.clear.call(this, this.cout);
      var startTime = video.currentTime;
      video.ontimeupdate = () => {
        if (this.testPixel(video, refColor.data, threshold)) {
          ok(false, video.id + " " + infoString);
          video.ontimeupdate = null;
          resolve();
        } else if (video.currentTime > startTime + (timeout / 1000.0)) {
          ok(true, video.id + " " + infoString);
          video.ontimeupdate = null;
          resolve();
        }
      };
    });
  },

  
  createAndAppendElement: function (type, id) {
    var e = document.createElement(type);
    e.id = id;
    e.width = this.elemWidth;
    e.height = this.elemHeight;
    if (type === 'video') {
      e.autoplay = true;
    }
    document.body.appendChild(e);
    return e;
  },
}


function CaptureStreamTestHelper2D(width, height) {
  CaptureStreamTestHelper.call(this, width, height);
}

CaptureStreamTestHelper2D.prototype = Object.create(CaptureStreamTestHelper.prototype);
CaptureStreamTestHelper2D.prototype.constructor = CaptureStreamTestHelper2D;


CaptureStreamTestHelper2D.prototype.clear = function(canvas) {
  var ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, canvas.width, canvas.height);
};


CaptureStreamTestHelper2D.prototype.drawColor = function(canvas, color) {
  var ctx = canvas.getContext('2d');
  var rgba = color.data.slice(); 
  info("Drawing color " + rgba.join(','));
  rgba[3] = rgba[3] / 255.0; 
  ctx.fillStyle = "rgba(" + rgba.join(',') + ")";

  
  ctx.fillRect(0, 0, canvas.width / 2, canvas.height / 2);
};


CaptureStreamTestHelper2D.prototype.testNotClean = function(canvas) {
  var ctx = canvas.getContext('2d');
  var error = "OK";
  try {
    var data = ctx.getImageData(0, 0, 1, 1);
  } catch(e) {
    error = e.name;
  }
  is(error, "SecurityError",
     "Canvas '" + canvas.id + "' should not be origin-clean");
};


function CaptureStreamTestHelperWebGL(width, height) {
  CaptureStreamTestHelper.call(this, width, height);
}

CaptureStreamTestHelperWebGL.prototype = Object.create(CaptureStreamTestHelper.prototype);
CaptureStreamTestHelperWebGL.prototype.constructor = CaptureStreamTestHelperWebGL;


CaptureStreamTestHelperWebGL.prototype.setFragmentColorLocation = function(colorLocation) {
  this.colorLocation = colorLocation;
};


CaptureStreamTestHelperWebGL.prototype.clearColor = function(canvas, color) {
  info("WebGL: clearColor(" + color.name + ")");
  var gl = canvas.getContext('webgl');
  var conv = color.data.map(i => i / 255.0);
  gl.clearColor(conv[0], conv[1], conv[2], conv[3]);
  gl.clear(gl.COLOR_BUFFER_BIT);
};


CaptureStreamTestHelperWebGL.prototype.drawColor = function(canvas, color) {
  info("WebGL: drawArrays(" + color.name + ")");
  var gl = canvas.getContext('webgl');
  var conv = color.data.map(i => i / 255.0);
  gl.uniform4f(this.colorLocation, conv[0], conv[1], conv[2], conv[3]);
  gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
};
