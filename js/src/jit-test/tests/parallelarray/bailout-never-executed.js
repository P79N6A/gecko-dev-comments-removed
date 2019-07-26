



load(libdir + "parallelarray-helpers.js");

function makeObject(e, i, c) {
  var v = {element: e, index: i, collection: c};

  if (e == 512) 
    delete v.i;

  return v;
}

if (getBuildConfiguration().parallelJS)
  compareAgainstArray(range(0, 512), "map", makeObject);
