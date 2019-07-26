


































ArgGenerators = {





  stencilMaskSeparate : {
    generate : function() { return [cullFace.random(), randomInt(0xffffffff)]; },
    checkArgValidity : function(face, mask) {
      return cullFace.has(face);
    },
    teardown : function() { GL.stencilMask(0xffffffff); }
  },
  stencilOp : {
    generate : function() {
      return [stencilOp.random(), stencilOp.random(), stencilOp.random()];
    },
    checkArgValidity : function(sfail, dpfail, dppass) {
      return stencilOp.has(sfail) && stencilOp.has(dpfail) && stencilOp.has(dppass);
    },
    teardown : function() { GL.stencilOp(GL.KEEP, GL.KEEP, GL.KEEP); }
  },
  stencilOpSeparate : {
    generate : function() {
      return [cullFace.random(), stencilOp.random(), stencilOp.random(), stencilOp.random()];
    },
    checkArgValidity : function(face, sfail, dpfail, dppass) {
      return cullFace.has(face) && stencilOp.has(sfail) &&
              stencilOp.has(dpfail) && stencilOp.has(dppass);
    },
    teardown : function() { GL.stencilOp(GL.KEEP, GL.KEEP, GL.KEEP); }
  },


  texImage2D : {
    noAlreadyTriedCheck : true, 
    setup : function() {
      var tex = GL.createTexture();
      var tex2 = GL.createTexture();
      GL.bindTexture(GL.TEXTURE_2D, tex);
      GL.bindTexture(GL.TEXTURE_CUBE_MAP, tex2);
      return [tex, tex2];
    },
    generate : function() {
      var format = texImageFormat.random();
      if (Math.random() < 0.5) {
        var img = randomImage(16,16);
        var a = [ texImageTarget.random(), 0, format, format, GL.UNSIGNED_BYTE, img ];
        return a;
      } else {
        var pix = null;
        if (Math.random > 0.5) {
          pix = new Uint8Array(16*16*4);
        }
        return [
          texImageTarget.random(), 0,
          format, 16, 16, 0,
          format, GL.UNSIGNED_BYTE, pix
        ];
      }
    },
    checkArgValidity : function(target, level, internalformat, width, height, border, format, type, data) {
               
      if (!texImageTarget.has(target) || castToInt(level) < 0)
        return false;
      if (arguments.length <= 6) {
        var xformat = width;
        var xtype = height;
        var ximage = border;
        if ((ximage instanceof HTMLImageElement ||
             ximage instanceof HTMLVideoElement ||
             ximage instanceof HTMLCanvasElement ||
             ximage instanceof ImageData) &&
            texImageInternalFormat.has(internalformat) &&
            texImageFormat.has(xformat) &&
            texImageType.has(xtype) &&
            internalformat == xformat)
          return true;
        return false;
      }
      var w = castToInt(width), h = castToInt(height), b = castToInt(border);
      return texImageInternalFormat.has(internalformat) && w >= 0 && h >= 0 &&
            b == 0 && (data == null || data.byteLength == w*h*4) &&
            texImageFormat.has(format) && texImageType.has(type)
            && internalformat == format;
    },
    teardown : function(tex, tex2) {
      GL.bindTexture(GL.TEXTURE_2D, null);
      GL.bindTexture(GL.TEXTURE_CUBE_MAP, null);
      GL.deleteTexture(tex);
      GL.deleteTexture(tex2);
    }
  },
  texParameterf : {
    generate : function() {
      var pname = texParameterPname.random();
      var param = texParameterParam[pname].random();
      return [bindTextureTarget.random(), pname, param];
    },
    checkArgValidity : function(target, pname, param) {
      if (!bindTextureTarget.has(target))
        return false;
      if (!texParameterPname.has(pname))
        return false;
      return texParameterParam[pname].has(param);
    }
  },
  texParameteri : {
    generate : function() {
      var pname = texParameterPname.random();
      var param = texParameterParam[pname].random();
      return [bindTextureTarget.random(), pname, param];
    },
    checkArgValidity : function(target, pname, param) {
      if (!bindTextureTarget.has(target))
        return false;
      if (!texParameterPname.has(pname))
        return false;
      return texParameterParam[pname].has(param);
    }
  },
  texSubImage2D : {}, 



  uniform1f : {}, 
  uniform1fv : {}, 
  uniform1i : {}, 
  uniform1iv : {}, 
  uniform2f : {}, 
  uniform2fv : {}, 
  uniform2i : {}, 
  uniform2iv : {}, 
  uniform3f : {}, 
  uniform3fv : {}, 
  uniform3i : {}, 
  uniform3iv : {}, 
  uniform4f : {}, 
  uniform4fv : {}, 
  uniform4i : {}, 
  uniform4iv : {}, 
  uniformMatrix2fv : {}, 
  uniformMatrix3fv : {}, 
  uniformMatrix4fv : {}, 
  useProgram : {}, 



  validateProgram : {}, 
  vertexAttrib1f : {}, 
  vertexAttrib1fv : {}, 
  vertexAttrib2f : {}, 
  vertexAttrib2fv : {}, 
  vertexAttrib3f : {}, 
  vertexAttrib3fv : {}, 
  vertexAttrib4f : {}, 
  vertexAttrib4fv : {}, 
  vertexAttribPointer : {}, 
  viewport : {
    generate : function() {
      return [randomInt(3000)-1500, randomInt(3000)-1500, randomIntFromRange(0,3000), randomIntFromRange(0,3000)];
    },
    checkArgValidity : function(x,y,w,h) {
      return castToInt(w) >= 0 && castToInt(h) >= 0;
    },
    teardown : function() {
      GL.viewport(0,0,GL.canvas.width, GL.canvas.height);
    }
  }

};
