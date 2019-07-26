


































ArgGenerators = {





  deleteBuffer : {
    generate : function() { return [GL.createBuffer()]; },
    checkArgValidity : function(o) {
      GL.bindBuffer(GL.ARRAY_BUFFER, o);
      return GL.isBuffer(o);
    },
    cleanup : function(o) {
      GL.bindBuffer(GL.ARRAY_BUFFER, null);
      try { GL.deleteBuffer(o); } catch(e) {}
    }
  },
  deleteFramebuffer : {
    generate : function() { return [GL.createFramebuffer()]; },
    checkArgValidity : function(o) {
      GL.bindFramebuffer(GL.FRAMEBUFFER, o);
      return GL.isFramebuffer(o);
    },
    cleanup : function(o) {
      GL.bindFramebuffer(GL.FRAMEBUFFER, null);
      try { GL.deleteFramebuffer(o); } catch(e) {}
    }
  },
  deleteProgram : {
    generate : function() { return [GL.createProgram()]; },
    checkArgValidity : function(o) { return GL.isProgram(o); },
    cleanup : function(o) { try { GL.deleteProgram(o); } catch(e) {} }
  },
  deleteRenderbuffer : {
    generate : function() { return [GL.createRenderbuffer()]; },
    checkArgValidity : function(o) {
      GL.bindRenderbuffer(GL.RENDERBUFFER, o);
      return GL.isRenderbuffer(o);
    },
    cleanup : function(o) {
      GL.bindRenderbuffer(GL.RENDERBUFFER, null);
      try { GL.deleteRenderbuffer(o); } catch(e) {}
    }
  },
  deleteShader : {
    generate : function() { return [GL.createShader(shaderType.random())]; },
    checkArgValidity : function(o) { return GL.isShader(o); },
    cleanup : function(o) { try { GL.deleteShader(o); } catch(e) {} }
  },
  deleteTexture : {
    generate : function() { return [GL.createTexture()]; },
    checkArgValidity : function(o) {
      GL.bindTexture(GL.TEXTURE_2D, o);
      return GL.isTexture(o);
    },
    cleanup : function(o) {
      GL.bindTexture(GL.TEXTURE_2D, null);
      try { GL.deleteTexture(o); } catch(e) {}
    }
  },
  depthFunc : {
    generate : function() { return [depthFuncFunc.random()]; },
    checkArgValidity : function(f) { return depthFuncFunc.has(f); },
    teardown : function() { GL.depthFunc(GL.LESS); }
  },
  depthMask : {
    generate : function() { return [randomBool()]; },
    teardown : function() { GL.depthFunc(GL.TRUE); }
  },
  depthRange : {
    generate : function() { return [Math.random(), Math.random()]; },
    teardown : function() { GL.depthRange(0, 1); }
  },
  detachShader : {
    generate : function() {
      var p = GL.createProgram();
      var sh = GL.createShader(shaderType.random());
      GL.attachShader(p, sh);
      return [p, sh];
    },
    checkArgValidity : function(p, sh) {
      return GL.isProgram(p) && GL.isShader(sh) && GL.getAttachedShaders(p).has(sh);
    },
    cleanup : function(p, sh) {
      try {GL.deleteProgram(p);} catch(e) {}
      try {GL.deleteShader(sh);} catch(e) {}
    }
  },
  disable : {
    generate : function() { return [enableCap.random()]; },
    checkArgValidity : function(c) { return enableCap.has(c); },
    cleanup : function(c) { if (c == GL.DITHER) GL.enable(c); }
  },
  disableVertexAttribArray : {
    generate : function() { return [randomVertexAttribute()]; },
    checkArgValidity : function(v) { return isVertexAttribute(v); }
  },
  drawArrays : {}, 
  drawElements : {}, 



  enable : {
    generate : function() { return [enableCap.random()]; },
    checkArgValidity : function(c) { return enableCap.has(c); },
    cleanup : function(c) { if (c != GL.DITHER) GL.disable(c); }
  },
  enableVertexAttribArray : {
    generate : function() { return [randomVertexAttribute()]; },
    checkArgValidity : function(v) { return isVertexAttribute(castToInt(v)); },
    cleanup : function(v) { GL.disableVertexAttribArray(v); }
  },



  finish : {
    generate : function() { return []; }
  },
  flush : {
    generate : function() { return []; }
  },
  framebufferRenderbuffer : {}, 
  framebufferTexture2D : {}, 
  frontFace : {
    generate : function() { return [frontFaceMode.random()]; },
    checkArgValidity : function(c) { return frontFaceMode.has(c); },
    cleanup : function(c) { GL.frontFace(GL.CCW); }
  },



  generateMipmap : {
    setup : function() {
      var tex = GL.createTexture();
      var tex2 = GL.createTexture();
      GL.bindTexture(GL.TEXTURE_2D, tex);
      GL.bindTexture(GL.TEXTURE_CUBE_MAP, tex2);
      var pix = new Uint8Array(16*16*4);
      GL.texImage2D(GL.TEXTURE_2D, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
      GL.texImage2D(GL.TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL.RGBA, 16, 16, 0, GL.RGBA, GL.UNSIGNED_BYTE, pix);
    },
    generate : function() { return [bindTextureTarget.random()]; },
    checkArgValidity : function(t) { return bindTextureTarget.has(t); },
    teardown : function(tex, tex2) {
      GL.bindTexture(GL.TEXTURE_2D, null);
      GL.bindTexture(GL.TEXTURE_CUBE_MAP, null);
      GL.deleteTexture(tex);
      GL.deleteTexture(tex2);
    }
  },
  getActiveAttrib : {
  











  },
  getActiveUniform : {}, 
  getAttachedShaders : {
    setup : function() {
      var program = GL.createProgram();
      var s1 = GL.createShader(GL.VERTEX_SHADER);
      var s2 = GL.createShader(GL.FRAGMENT_SHADER);
      GL.attachShader(program, s1);
      GL.attachShader(program, s2);
      return [program, s1, s2];
    },
    generate : function(program, s1, s2) {
      return [program]
    },
    checkArgValidity : function(program) {
      return GL.isProgram(program);
    },
    teardown : function(program, s1, s2) {
      GL.deleteProgram(program);
      GL.deleteShader(s1);
      GL.deleteShader(s2);
    }
  }

};

