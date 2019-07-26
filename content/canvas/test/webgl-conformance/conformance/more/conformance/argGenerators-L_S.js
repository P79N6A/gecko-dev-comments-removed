


































ArgGenerators = {





  lineWidth : {
    generate : function() { return [randomLineWidth()]; },
    teardown : function() { GL.lineWidth(1); }
  },
  linkProgram : {}, 


  pixelStorei : {
    generate : function() {
      return [pixelStoreiPname.random(), pixelStoreiParam.random()];
    },
    checkArgValidity : function(pname, param) {
      return pixelStoreiPname.has(pname) && pixelStoreiParam.has(param);
    },
    teardown : function() {
      GL.pixelStorei(GL.PACK_ALIGNMENT, 4);
      GL.pixelStorei(GL.UNPACK_ALIGNMENT, 4);
    }
  },
  polygonOffset : {
    generate : function() { return [randomFloat(), randomFloat()]; },
    teardown : function() { GL.polygonOffset(0,0); }
  },



  readPixels : {}, 
  renderbufferStorage : {}, 



  sampleCoverage : {
    generate : function() { return [randomFloatFromRange(0,1), randomBool()] },
    teardown : function() { GL.sampleCoverage(1, false); }
  },
  scissor : {
    generate : function() {
      return [randomInt(3000)-1500, randomInt(3000)-1500, randomIntFromRange(0,3000), randomIntFromRange(0,3000)];
    },
    checkArgValidity : function(x,y,w,h) {
      return castToInt(w) >= 0 && castToInt(h) >= 0;
    },
    teardown : function() {
      GL.scissor(0,0,GL.canvas.width, GL.canvas.height);
    }
  },
  shaderSource : {}, 
  stencilFunc : {
    generate : function(){
      return [stencilFuncFunc.random(), randomInt(MaxStencilValue), randomInt(0xffffffff)];
    },
    checkArgValidity : function(func, ref, mask) {
      return stencilFuncFunc.has(func) && castToInt(ref) >= 0 && castToInt(ref) < MaxStencilValue;
    },
    teardown : function() {
      GL.stencilFunc(GL.ALWAYS, 0, 0xffffffff);
    }
  },
  stencilFuncSeparate : {
    generate : function(){
      return [cullFace.random(), stencilFuncFunc.random(), randomInt(MaxStencilValue), randomInt(0xffffffff)];
    },
    checkArgValidity : function(face, func, ref, mask) {
      return cullFace.has(face) && stencilFuncFunc.has(func) && castToInt(ref) >= 0 && castToInt(ref) < MaxStencilValue;
    },
    teardown : function() {
      GL.stencilFunc(GL.ALWAYS, 0, 0xffffffff);
    }
  },
  stencilMask : {
    generate : function() { return [randomInt(0xffffffff)]; },
    teardown : function() { GL.stencilMask(0xffffffff); }
  }

};
