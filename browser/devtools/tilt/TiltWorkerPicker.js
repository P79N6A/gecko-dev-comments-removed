







































"use strict";







self.onmessage = function TWP_onMessage(event)
{
  let data = event.data;
  let thickness = data.thickness;
  let vertices = data.vertices;
  let ray = data.ray;

  let intersection = null;
  let hit = [];

  
  function dsq(p1, p2) {
    let xd = p2[0] - p1[0];
    let yd = p2[1] - p1[1];
    let zd = p2[2] - p1[2];

    return xd * xd + yd * yd + zd * zd;
  }

  
  
  for (let i = 0, len = vertices.length; i < len; i += 36) {

    
    let v0f = [vertices[i],     vertices[i + 1],  vertices[i + 2]];
    let v1f = [vertices[i + 3], vertices[i + 4],  vertices[i + 5]];
    let v2f = [vertices[i + 6], vertices[i + 7],  vertices[i + 8]];
    let v3f = [vertices[i + 9], vertices[i + 10], vertices[i + 11]];

    
    let v0b = [v0f[0], v0f[1], v0f[2] - thickness];
    let v1b = [v1f[0], v1f[1], v1f[2] - thickness];
    let v2b = [v2f[0], v2f[1], v2f[2] - thickness];
    let v3b = [v3f[0], v3f[1], v3f[2] - thickness];

    
    if (!v0f[0] && !v1f[0] && !v2f[0] && !v3f[0]) {
      continue;
    }

    
    if (self.intersect(v0f, v1f, v2f, ray, hit) || 
        self.intersect(v0f, v2f, v3f, ray, hit) || 
        self.intersect(v0b, v1b, v1f, ray, hit) || 
        self.intersect(v0b, v1f, v0f, ray, hit) || 
        self.intersect(v3f, v2b, v3b, ray, hit) || 
        self.intersect(v3f, v2f, v2b, ray, hit) || 
        self.intersect(v0b, v0f, v3f, ray, hit) || 
        self.intersect(v0b, v3f, v3b, ray, hit) || 
        self.intersect(v1f, v1b, v2b, ray, hit) || 
        self.intersect(v1f, v2b, v2f, ray, hit)) { 

      
      let d = dsq(hit, ray.origin);

      
      if (intersection === null || d < intersection.distance) {
        intersection = {
          
          
          index: i / 36,
          distance: d
        };
      }
    }
  }

  self.postMessage(intersection);
  close();
};




self.intersect = (function() {

  
  function create() {
    return new Float32Array(3);
  }

  
  function add(aVec, aVec2, aDest) {
    aDest[0] = aVec[0] + aVec2[0];
    aDest[1] = aVec[1] + aVec2[1];
    aDest[2] = aVec[2] + aVec2[2];
    return aDest;
  }

  
  function subtract(aVec, aVec2, aDest) {
    aDest[0] = aVec[0] - aVec2[0];
    aDest[1] = aVec[1] - aVec2[1];
    aDest[2] = aVec[2] - aVec2[2];
    return aDest;
  }

  
  function scale(aVec, aVal, aDest) {
    aDest[0] = aVec[0] * aVal;
    aDest[1] = aVec[1] * aVal;
    aDest[2] = aVec[2] * aVal;
    return aDest;
  }

  
  function cross(aVec, aVec2, aDest) {
    let x = aVec[0];
    let y = aVec[1];
    let z = aVec[2];
    let x2 = aVec2[0];
    let y2 = aVec2[1];
    let z2 = aVec2[2];

    aDest[0] = y * z2 - z * y2;
    aDest[1] = z * x2 - x * z2;
    aDest[2] = x * y2 - y * x2;
    return aDest;
  }

  
  function dot(aVec, aVec2) {
    return aVec[0] * aVec2[0] + aVec[1] * aVec2[1] + aVec[2] * aVec2[2];
  }

  let edge1 = create();
  let edge2 = create();
  let pvec = create();
  let tvec = create();
  let qvec = create();
  let lvec = create();

  
  
  return function intersect(aVert0, aVert1, aVert2, aRay, aDest) {
    let dir = aRay.direction;
    let orig = aRay.origin;

    
    subtract(aVert1, aVert0, edge1);
    subtract(aVert2, aVert0, edge2);

    
    cross(dir, edge2, pvec);

    
    let inv_det = 1 / dot(edge1, pvec);

    
    subtract(orig, aVert0, tvec);

    
    let u = dot(tvec, pvec) * inv_det;
    if (u < 0 || u > 1) {
      return false;
    }

    
    cross(tvec, edge1, qvec);

    
    let v = dot(dir, qvec) * inv_det;
    if (v < 0 || u + v > 1) {
      return false;
    }

    
    let t = dot(edge2, qvec) * inv_det;

    scale(dir, t, lvec);
    add(orig, lvec, aDest);
    return true;
  };
}());
