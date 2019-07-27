


































ArgGenerators = {





  bufferData : {
    setup : function() {
      var buf = GL.createBuffer();
      var ebuf = GL.createBuffer();
      GL.bindBuffer(GL.ARRAY_BUFFER, buf);
      GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, ebuf);
      return [buf, ebuf];
    },
    generate : function(buf, ebuf) {
      return [bufferTarget.random(), randomBufferData(), bufferMode.random()];
    },
    checkArgValidity : function(target, bufData, mode) {
      return bufferTarget.has(target) && isBufferData(bufData) && bufferMode.has(mode);
    },
    teardown : function(buf, ebuf) {
      GL.deleteBuffer(buf);
      GL.deleteBuffer(ebuf);
    },
  }

};
