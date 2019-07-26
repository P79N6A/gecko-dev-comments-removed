






var l10n = exports;

Cu.import("resource://gre/modules/Services.jsm");










function getEntity(aDTDs, aEntityId) {
  
  aDTDs.push("resource:///res/dtd/xhtml11.dtd");

  
  var references = "";
  for (i = 0; i < aDTDs.length; i++) {
    var id = 'dtd' + i;
    references += '<!ENTITY % ' + id + ' SYSTEM "' + aDTDs[i] + '">%' + id + ';';
  }

  var header = '<?xml version="1.0"?><!DOCTYPE elem [' + references + ']>';
  var element = '<elem id="entity">&' + aEntityId + ';</elem>';
  var content = header + element;

  var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
               createInstance(Ci.nsIDOMParser);
  var doc = parser.parseFromString(content, 'text/xml');
  var node = doc.querySelector('elem[id="entity"]');

  if (!node) {
    throw new Error("Unkown entity '" + aEntityId + "'");
  }

  return node.textContent;
}











function getProperty(aURL, aProperty) {
  var bundle = Services.strings.createBundle(aURL);

  try {
    return bundle.GetStringFromName(aProperty);
  } catch (ex) {
    throw new Error("Unkown property '" + aProperty + "'");
  }
}



l10n.getEntity = getEntity;
l10n.getProperty = getProperty;
