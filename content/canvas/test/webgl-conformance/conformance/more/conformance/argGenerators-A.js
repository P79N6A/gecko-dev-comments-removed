


































ArgGenerators = {





  activeTexture : {
    generate : function() { return [textureUnit.random()]; },
    checkArgValidity : function(t) { return textureUnit.has(t); },
    teardown : function() { GL.activeTexture(GL.TEXTURE0); }
  },
  attachShader : {
    generate : function() {
      var p = GL.createProgram();
      var sh = GL.createShader(shaderType.random());
      return [p, sh];
    },
    checkArgValidity : function(p, sh) {
      return GL.isProgram(p) && GL.isShader(sh) && !GL.getAttachedShaders(p).has(sh);
    },
    cleanup : function(p, sh) {
      try {GL.detachShader(p,sh);} catch(e) {}
      try {GL.deleteProgram(p);} catch(e) {}
      try {GL.deleteShader(sh);} catch(e) {}
    }
  }

};
