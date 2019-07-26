




"use strict";









self.onmessage = function TWC_onMessage(event)
{
  let data = event.data;
  let maxGroupNodes = parseInt(data.maxGroupNodes);
  let style = data.style;
  let texWidth = data.texWidth;
  let texHeight = data.texHeight;
  let nodesInfo = data.nodesInfo;

  let mesh = {
    allVertices: [],
    groups: [],
    width: 0,
    height: 0
  };

  let vertices;
  let texCoord;
  let color;
  let stacksIndices;
  let wireframeIndices;
  let index;

  
  
  self.random.seed(0);

  
  for (let n = 0, len = nodesInfo.length; n < len; n++) {

    
    if (n % maxGroupNodes === 0) {
      vertices = []; 
      texCoord = [];
      color = [];
      stacksIndices = [];
      wireframeIndices = [];
      index = 0;
    }

    let info = nodesInfo[n];
    let coord = info.coord;

    
    let z = coord.depth + coord.thickness;
    let y = coord.top;
    let x = coord.left;
    let w = coord.width;
    let h = coord.height;

    
    if (x + w > texWidth) {
      w = texWidth - x;
    }
    if (y + h > texHeight) {
      h = texHeight - y;
    }

    x += self.random.next();
    y += self.random.next();
    w -= self.random.next() * 0.1;
    h -= self.random.next() * 0.1;

    let xpw = x + w;
    let yph = y + h;
    let zmt = coord.depth;

    let xotw = x / texWidth;
    let yoth = y / texHeight;
    let xpwotw = xpw / texWidth;
    let yphoth = yph / texHeight;

    
    let fill = style[info.name] || style.highlight.defaultFill;

    let r = fill[0];
    let g = fill[1];
    let b = fill[2];
    let g10 = r * 1.1;
    let g11 = g * 1.1;
    let g12 = b * 1.1;
    let g20 = r * 0.6;
    let g21 = g * 0.6;
    let g22 = b * 0.6;

    
    vertices.push(x,   y,   z,                                 
                  x,   yph, z,                                            
                  xpw, yph, z,                                            
                  xpw, y,   z,                                            
    
    
    
    
                  x,   y,   z,                                 
                  x,   yph, z,                                            
                  xpw, yph, z,                                            
                  xpw, y,   z,                                            
                  x,   y,   zmt,                                
                  x,   yph, zmt,                                          
                  xpw, yph, zmt,                                          
                  xpw, y,   zmt);                                         

    
    texCoord.push(xotw,   yoth,
                  xotw,   yphoth,
                  xpwotw, yphoth,
                  xpwotw, yoth,
                  -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0);

    
    color.push(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
               g10, g11, g12,
               g10, g11, g12,
               g10, g11, g12,
               g10, g11, g12,
               g20, g21, g22,
               g20, g21, g22,
               g20, g21, g22,
               g20, g21, g22);

    let i = index; 
    let ip1 = i + 1;
    let ip2 = ip1 + 1;
    let ip3 = ip2 + 1;
    let ip4 = ip3 + 1;
    let ip5 = ip4 + 1;
    let ip6 = ip5 + 1;
    let ip7 = ip6 + 1;
    let ip8 = ip7 + 1;
    let ip9 = ip8 + 1;
    let ip10 = ip9 + 1;
    let ip11 = ip10 + 1;

    
    stacksIndices.unshift(i,    ip1,  ip2,  i,    ip2,  ip3,
                          ip8,  ip9,  ip5,  ip8,  ip5,  ip4,
                          ip7,  ip6,  ip10, ip7,  ip10, ip11,
                          ip8,  ip4,  ip7,  ip8,  ip7,  ip11,
                          ip5,  ip9,  ip10, ip5,  ip10, ip6);

    
    if (coord.thickness !== 0) {
      wireframeIndices.unshift(i,    ip1, ip1,  ip2,
                               ip2,  ip3, ip3,  i,
                               ip8,  i,   ip9,  ip1,
                               ip11, ip3, ip10, ip2);
    }

    
    index += 12;

    
    mesh.width = Math.max(w, mesh.width);
    mesh.height = Math.max(h, mesh.height);

    
    
    if (((n + 1) % maxGroupNodes === 0) || (n === len - 1)) {
      mesh.groups.push({
        vertices: vertices,
        texCoord: texCoord,
        color: color,
        stacksIndices: stacksIndices,
        wireframeIndices: wireframeIndices
      });
      mesh.allVertices = mesh.allVertices.concat(vertices);
    }
  }

  self.postMessage(mesh);
  close();
};




self.random = {

  


  _generator: null,

  


  next: function RNG_next()
  {
    return this._generator();
  },

  





  seed: function RNG_seed()
  {
    let s0 = 0;
    let s1 = 0;
    let s2 = 0;
    let c = 1;

    if (arguments.length === 0) {
      return this.seed(+new Date());
    } else {
      s0 = this.mash(" ");
      s1 = this.mash(" ");
      s2 = this.mash(" ");

      for (let i = 0, len = arguments.length; i < len; i++) {
        s0 -= this.mash(arguments[i]);
        if (s0 < 0) {
          s0 += 1;
        }
        s1 -= this.mash(arguments[i]);
        if (s1 < 0) {
          s1 += 1;
        }
        s2 -= this.mash(arguments[i]);
        if (s2 < 0) {
          s2 += 1;
        }
      }

      let random = function() {
        let t = 2091639 * s0 + c * 2.3283064365386963e-10; 
        s0 = s1;
        s1 = s2;
        return (s2 = t - (c = t | 0));
      };
      random.uint32 = function() {
        return random() * 0x100000000; 
      };
      random.fract53 = function() {
        return random() +
              (random() * 0x200000 | 0) * 1.1102230246251565e-16; 
      };
      return (this._generator = random);
    }
  },

  



  mash: function RNG_mash(data)
  {
    let h, n = 0xefc8249d;

    for (let i = 0, data = data.toString(), len = data.length; i < len; i++) {
      n += data.charCodeAt(i);
      h = 0.02519603282416938 * n;
      n = h >>> 0;
      h -= n;
      h *= n;
      n = h >>> 0;
      h -= n;
      n += h * 0x100000000; 
    }
    return (n >>> 0) * 2.3283064365386963e-10; 
  }
};
