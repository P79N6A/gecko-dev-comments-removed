



"use strict";




exports.dirname = path => {
  return Services.io.newURI(
    ".", null, Services.io.newURI(path, null, null)).spec;
}





exports.joinURI = (initialPath, ...paths) => {
  let uri;

  try {
    uri = Services.io.newURI(initialPath, null, null);
  }
  catch(e) {
    return;
  }

  for(let path of paths) {
    if(path) {
      uri = Services.io.newURI(path, null, uri);
    }
  }

  return uri.spec;
}
