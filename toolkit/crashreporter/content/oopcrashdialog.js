


const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/CrashSubmit.jsm");

var id;

function collectData() {
  let directoryService = Cc["@mozilla.org/file/directory_service;1"].
    getService(Ci.nsIProperties);

  let dumpFile = window.arguments[0].QueryInterface(Ci.nsIFile);
  id = dumpFile.leafName.replace(/.dmp$/, "");
}

function submitDone()
{
  
  window.close();
}

function onSubmit()
{
  document.documentElement.getButton('accept').disabled = true;
  document.documentElement.getButton('accept').label = 'Sending';
  document.getElementById('throbber').src = 'chrome://global/skin/icons/loading_16.png';
  CrashSubmit.submit(id, document.getElementById('iframe-holder'),
                     submitDone, submitDone);
  return false;
}
