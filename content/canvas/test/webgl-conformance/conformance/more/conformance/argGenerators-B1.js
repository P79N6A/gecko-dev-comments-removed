


































ArgGenerators = {





  bindAttribLocation : {
    generate : function() {
      var program = GL.createProgram();
      return [program, randomVertexAttribute(), randomName()];
    },
    checkArgValidity : function(program, index, name) {
      return GL.isProgram(program) && isVertexAttribute(index) && isValidName(name);
    },
    cleanup : function(program, index, name) {
      try { GL.deleteProgram(program); } catch(e) {}
    }
  }

};
