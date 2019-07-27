


































ArgGenerators = {





  bindBuffer : {
    generate : function(buf) {
      return [bufferTarget.random(), GL.createBuffer()];
    },
    checkArgValidity : function(target, buf) {
      if (!bufferTarget.has(target))
        return false;
      GL.bindBuffer(target, buf);
      return GL.isBuffer(buf);
    },
    cleanup : function(t, buf, m) {
      GL.deleteBuffer(buf);
    }
  },
  bindFramebuffer : {
    generate : function() {
      return [GL.FRAMEBUFFER, Math.random() > 0.5 ? null : GL.createFramebuffer()];
    },
    checkArgValidity : function(target, fbo) {
      if (target != GL.FRAMEBUFFER)
        return false;
      if (fbo != null)
          GL.bindFramebuffer(target, fbo);
      return (fbo == null || GL.isFramebuffer(fbo));
    },
    cleanup : function(target, fbo) {
      GL.bindFramebuffer(target, null);
      if (fbo)
        GL.deleteFramebuffer(fbo);
    }
  },
  bindRenderbuffer : {
    generate : function() {
      return [GL.RENDERBUFFER, Math.random() > 0.5 ? null : GL.createRenderbuffer()];
    },
    checkArgValidity : function(target, rbo) {
      if (target != GL.RENDERBUFFER)
        return false;
      if (rbo != null)
        GL.bindRenderbuffer(target, rbo);
      return (rbo == null || GL.isRenderbuffer(rbo));
    },
    cleanup : function(target, rbo) {
      GL.bindRenderbuffer(target, null);
      if (rbo)
        GL.deleteRenderbuffer(rbo);
    }
  },
  bindTexture : {
    generate : function() {
      return [bindTextureTarget.random(), Math.random() > 0.5 ? null : GL.createTexture()];
    },
    checkArgValidity : function(target, o) {
      if (!bindTextureTarget.has(target))
        return false;
      if (o != null)
        GL.bindTexture(target, o);
      return (o == null || GL.isTexture(o));
    },
    cleanup : function(target, o) {
      GL.bindTexture(target, null);
      if (o)
        GL.deleteTexture(o);
    }
  },
  blendColor : {
    generate : function() { return randomColor(); },
    teardown : function() { GL.blendColor(0,0,0,0); }
  },
  blendEquation : {
    generate : function() { return [blendEquationMode.random()]; },
    checkArgValidity : function(o) { return blendEquationMode.has(o); },
    teardown : function() { GL.blendEquation(GL.FUNC_ADD); }
  },
  blendEquationSeparate : {
    generate : function() {
      return [blendEquationMode.random(), blendEquationMode.random()];
    },
    checkArgValidity : function(o,p) {
      return blendEquationMode.has(o) && blendEquationMode.has(p);
    },
    teardown : function() { GL.blendEquationSeparate(GL.FUNC_ADD, GL.FUNC_ADD); }
  },
  blendFunc : {
    generate : function() {
      return [blendFuncSfactor.random(), blendFuncDfactor.random()];
    },
    checkArgValidity : function(s,d) {
      return blendFuncSfactor.has(s) && blendFuncDfactor.has(d);
    },
    teardown : function() { GL.blendFunc(GL.ONE, GL.ZERO); }
  },
  blendFuncSeparate : {
    generate : function() {
      return [blendFuncSfactor.random(), blendFuncDfactor.random(),
              blendFuncSfactor.random(), blendFuncDfactor.random()];
    },
    checkArgValidity : function(s,d,as,ad) {
      return blendFuncSfactor.has(s) && blendFuncDfactor.has(d) &&
              blendFuncSfactor.has(as) && blendFuncDfactor.has(ad) ;
    },
    teardown : function() {
      GL.blendFuncSeparate(GL.ONE, GL.ZERO, GL.ONE, GL.ZERO);
    }
  }

};
