


































ArgGenerators = {





  bufferSubData : {
    setup : function() {
      var buf = GL.createBuffer();
      var ebuf = GL.createBuffer();
      GL.bindBuffer(GL.ARRAY_BUFFER, buf);
      GL.bufferData(GL.ARRAY_BUFFER, 256, GL.STATIC_DRAW);
      GL.bindBuffer(GL.ELEMENT_ARRAY_BUFFER, ebuf);
      GL.bufferData(GL.ELEMENT_ARRAY_BUFFER, 256, GL.STATIC_DRAW);
      return [buf, ebuf];
    },
    generate : function(buf, ebuf) {
      var d = randomBufferSubData(256);
      return [bufferTarget.random(), d.offset, d.data];
    },
    checkArgValidity : function(target, offset, data) {
      return bufferTarget.has(target) && offset >= 0 && data.byteLength >= 0 && offset + data.byteLength <= 256;
    },
    teardown : function(buf, ebuf) {
      GL.deleteBuffer(buf);
      GL.deleteBuffer(ebuf);
    },
  }

};
