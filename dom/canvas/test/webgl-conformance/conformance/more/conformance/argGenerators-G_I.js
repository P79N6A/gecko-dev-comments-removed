


































ArgGenerators = {





  getAttribLocation : {
    generate : function() {
      var program = GL.createProgram();
      var name = randomName();
      GL.bindAttribLocation(program, randomVertexAttribute(), name);
      return [program, name];
    },
    checkArgValidity : function(program, name) {
      return GL.isProgram(program) && isValidName(name);
    },
    cleanup : function(program, name) {
      try { GL.deleteProgram(program); } catch(e) {}
    }
  },






























































};
