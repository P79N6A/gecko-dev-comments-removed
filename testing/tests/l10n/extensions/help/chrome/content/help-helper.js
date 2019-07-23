






































var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

var RDF = Cc['@mozilla.org/rdf/rdf-service;1'].getService(Ci.nsIRDFService);
var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var AI = Cc['@mozilla.org/xre/app-info;1'].getService(Ci.nsIXULAppInfo);
var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
prefs = prefs.getBranch("l10n.tests.help.");
var prefName = AI.name + '.' + AI.version.substr(0,3); 
var gData = eval('(' + prefs.getCharPref(prefName) + ')');


var toc = ios.newURI(gData.datasources[0].url, null, null);

function getDS(data) {
  try {
    var ds = RDF.GetDataSourceBlocking(data.url);
  } catch (e) {
    Components.utils.reportError('failed to load RDF for ' + data.url);
  }
  ds.QueryInterface(Ci.rdfIDataSource);
  return ds;
}
gData.rdfds = gData.datasources.map(getDS);

var nc = "http://home.netscape.com/NC-rdf#";
var lnk = RDF.GetResource(nc + 'link');

var toCheck = {};
var checkList = [];
var isGood;
var current = null;
var frm, gb = document.documentElement;

function triplehandler(aBaseURI) {
  return function(aSubj, aPred, aObj, aTruth) {
    if (aPred.EqualsNode(lnk)) {
      aObj.QueryInterface(Ci.nsIRDFLiteral);
      var target = ios.newURI(aObj.Value, null, aBaseURI);
      target.QueryInterface(Ci.nsIURL);
      var ref = target.ref;
      var base = target.prePath + target.filePath;
      if (! (base in toCheck)) {
        toCheck[base] = {};
      }
      toCheck[base][ref] = [aSubj, target];
    }
  }
}

function onLoad(event) {
  
  if (event.target != document) {
    return;
  }
  document.documentElement.removeAttribute('onload');
  frm = document.getElementById('frm');
  frm.addEventListener('load', frmLoad, true);
  gData.current = -1;
  runDS();
}

function runDS() {
  gData.current++;
  if (gData.datasources.length == gData.current) {
    postChecks();
    return;
  }
  toCheck = {};
  checkList = [];
  isGood = true;
  var data = gData.datasources[gData.current];
  Components.utils.reportError('testing ' + data.title);
  createBox(data.title);
  var uri = ios.newURI(data.url, null, null);
  gData.rdfds[gData.current].visitAllTriples(triplehandler(uri));
  for (key in toCheck) {
    checkList.push([key, toCheck[key]]);
  }
  current = checkList.pop();
  frm.setAttribute('src', current[0]);
}

function frmLoad(event) {
  event.stopPropagation();
  event.preventDefault();
  
  checkIDs(current[1], current[0]);
  if (checkList.length == 0) {
    if (isGood) {
      logMessage('PASS');
    }
    runDS();
    return;
  }
  current = checkList.pop();
  frm.setAttribute('src', current[0]);
}

function checkIDs(refs, url) {
  if (!frm.contentDocument) {
    Components.utils.reportError(url + ' not found\n')
    return;
  }
  for (var ref in refs) {
    if (ref && !frm.contentDocument.getElementById(ref)) {
      isGood = false;
      var msg = 'ID "' + ref + '" in ' + url + ' not found';
      Components.utils.reportError(msg);
      var d = document.createElement('description');
      d.setAttribute('value', msg);
      gb.appendChild(d);
    }
  }
}

function postChecks() {
  if (gData.entrypoints) {
    createBox('Entry points');
    Components.utils.reportError('testing entry points');
    var good = 0; bad = 0;
    for each (idref in gData.entrypoints) {
      var r=RDF.GetResource(toc.spec + '#' + idref);
      if (gData.rdfds[0].GetTarget(r,lnk,true)) {
        good++;
      }
      else {
        bad++;
        logMessage('Entrypoint "' + idref + '" not resolved.');
      }
    }
    if (bad == 0) {
      logMessage('PASS');
    }
  }
  Components.utils.reportError('TESTING:DONE');
}



function createBox(aTitle) {
  gb = document.createElement('groupbox');
  gb.appendChild(document.createElement('caption'));
  gb.firstChild.setAttribute('label', aTitle);
  document.documentElement.appendChild(gb);
}

function logMessage(aMsg) {
  Components.utils.reportError(aMsg);
  var d = document.createElement('description');
  d.setAttribute('value', aMsg);
  gb.appendChild(d);
}
