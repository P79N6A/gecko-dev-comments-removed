







function getID(manifest) {
  manifest = manifest || {};

  if (manifest.id) {

    if (typeof manifest.id !== "string") {
      return null;
    }

    
    if (isValidAOMName(manifest.id)) {
      return manifest.id;
    }
    
    return null;
  }

  
  
  
  if (manifest.name) {

    if (typeof manifest.name !== "string") {
      return null;
    }

    var modifiedName = "@" + manifest.name;
    return isValidAOMName(modifiedName) ? modifiedName : null;
  }

  
  
  return null;
}

module.exports = getID;






function isValidAOMName (s) {
  return /^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i.test(s || "");
}
