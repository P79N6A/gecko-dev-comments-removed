function forceLoadPluginElement(id) {
  var e = document.getElementById(id);
  var found = e.pluginFoundElement;
}

function forceLoadPlugin(ids, dontRemoveClassAttribute) {
  if (Array.isArray(ids)) {
    ids.forEach(function(element, index, array) {
      forceLoadPluginElement(element);
    });
  } else {
    forceLoadPluginElement(ids);
  }
  if (dontRemoveClassAttribute) {
    
    
    if (typeof dontRemoveClassAttribute === 'function') {
      dontRemoveClassAttribute();
    }
    return;
  }
  document.documentElement.removeAttribute("class");
}
