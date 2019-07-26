


































ArgGenerators = {





  checkFramebufferStatus : {
    generate : function() {
      return [Math.random() > 0.5 ? null : GL.createFramebuffer()];
    },
    checkArgValidity : function(fbo) {
      if (fbo != null)
        GL.bindFramebuffer(GL.FRAMEBUFFER, fbo);
      return fbo == null || GL.isFramebuffer(fbo);
    },
    cleanup : function(fbo){
      GL.bindFramebuffer(GL.FRAMEBUFFER, null);
      if (fbo != null)
        try{ GL.deleteFramebuffer(fbo); } catch(e) {}
    }
  },
  clear : {
    generate : function() { return [clearMask.random()]; },
    checkArgValidity : function(mask) { return clearMask.has(mask); }
  },
  clearColor : {
    generate : function() { return randomColor(); },
    teardown : function() { GL.clearColor(0,0,0,0); }
  },
  clearDepth : {
    generate : function() { return [Math.random()]; },
    teardown : function() { GL.clearDepth(1); }
  },
  clearStencil : {
    generate : function() { return [randomStencil()]; },
    teardown : function() { GL.clearStencil(0); }
  },
  colorMask : {
    generate : function() {
      return [randomBool(), randomBool(), randomBool(), randomBool()];
    },
    teardown : function() { GL.colorMask(true, true, true, true); }
  },
  compileShader : {}, 
  copyTexImage2D : {}, 
  copyTexSubImage2D : {}, 
  createBuffer : {
    generate : function() { return []; },
    returnValueCleanup : function(o) { GL.deleteBuffer(o); }
  },
  createFramebuffer : {
    generate : function() { return []; },
    returnValueCleanup : function(o) { GL.deleteFramebuffer(o); }
  },
  createProgram : {
    generate : function() { return []; },
    returnValueCleanup : function(o) { GL.deleteProgram(o); }
  },
  createRenderbuffer : {
    generate : function() { return []; },
    returnValueCleanup : function(o) { GL.deleteRenderbuffer(o); }
  },
  createShader : {
    generate : function() { return [shaderType.random()]; },
    checkArgValidity : function(t) { return shaderType.has(t); },
    returnValueCleanup : function(o) { GL.deleteShader(o); }
  },
  createTexture : {
    generate : function() { return []; },
    returnValueCleanup : function(o) { GL.deleteTexture(o); }
  },
  cullFace : {
    generate : function() { return [cullFace.random()]; },
    checkArgValidity : function(f) { return cullFace.has(f); },
    teardown : function() { GL.cullFace(GL.BACK); }
  }

};
