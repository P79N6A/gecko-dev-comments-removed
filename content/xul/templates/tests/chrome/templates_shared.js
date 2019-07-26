
















































const ZOO_NS = "http://www.some-fictitious-zoo.com/";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const debug = false;

var expectedConsoleMessages = [];
var expectLoggedMessages = null;

try {
  const RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].
                getService(Components.interfaces.nsIRDFService);
  const ContainerUtils = Components.classes["@mozilla.org/rdf/container-utils;1"].
                           getService(Components.interfaces.nsIRDFContainerUtils);
} catch(ex) { }

var xmlDoc;

function test_template()
{
  var root = document.getElementById("root");

  var ds;
  if (queryType == "rdf" && RDF) {
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].
                      getService(Components.interfaces.nsIIOService);

    var src = window.location.href.replace(/test_tmpl.*xul/, "animals.rdf");
    ds = RDF.GetDataSourceBlocking(src);

    if (expectLoggedMessages) {
      Components.classes["@mozilla.org/consoleservice;1"].
                 getService(Components.interfaces.nsIConsoleService).reset();
    }

    if (root.getAttribute("datasources") == "rdf:null")
      root.setAttribute("datasources", "animals.rdf");
  }
  else if (queryType == "xml") {
    var src = window.location.href.replace(/test_tmpl.*xul/, "animals.xml");
    xmlDoc = new XMLHttpRequest();
    xmlDoc.open("get", src, false);
    xmlDoc.send(null);
  }

  
  if (needsOpen)
    root.open = true;

  if (expectLoggedMessages)
    expectLoggedMessages();

  checkResults(root, 0);

  if (changes.length) {
    var usedds = ds;
    
    
    
    if (queryType == "rdf")
      usedds = copyRDFDataSource(root, ds);
    if (needsOpen)
      root.open = true;
    setTimeout(iterateChanged, 0, root, usedds);
  }
  else {
    if (needsOpen)
      root.open = false;
    if (expectedConsoleMessages.length)
      compareConsoleMessages();
    SimpleTest.finish();
  }
}

function iterateChanged(root, ds)
{
  Components.classes["@mozilla.org/consoleservice;1"].
             getService(Components.interfaces.nsIConsoleService).reset();

  for (var c = 0; c < changes.length; c++) {
    changes[c](ds, root);
    checkResults(root, c + 1);
  }

  if (needsOpen)
    root.open = false;
  if (expectedConsoleMessages.length)
    compareConsoleMessages();
  SimpleTest.finish();
}

function checkResults(root, step)
{
  var output = expectedOutput.copy();
  setForCurrentStep(output, step);

  var error;
  var actualoutput = root;
  if (isTreeBuilder) {
    
    
    actualoutput = treeViewToDOM(root);
    error = compareOutput(actualoutput, output.treechildren, false);
  }
  else {
    error = compareOutput(actualoutput, output, true);
  }

  var adjtestid = testid;
  if (step > 0)
    adjtestid += " dynamic step " + step;

  var stilltodo = ((step == 0 && notWorkingYet) || (step > 0 && notWorkingYetDynamic));
  if (stilltodo)
    todo(false, adjtestid);
  else
    ok(!error, adjtestid);

  if ((!stilltodo && error) || debug) {
    
    var serializedXML = "";
    var rootNodes = actualoutput.childNodes;
    for (var n = 0; n < rootNodes.length; n++) {
      var node = rootNodes[n];
      if (node.localName != "template")
        serializedXML += ((new XMLSerializer()).serializeToString(node));
    }

    
    const nsrepl = new RegExp("xmlns=\"" + XUL_NS + "\" ", "g");
    serializedXML = serializedXML.replace(nsrepl, "");
    if (debug)
      dump("-------- " + adjtestid + "  " + error + ":\n" + serializedXML + "\n");
    if (error)
      is(serializedXML, "Same", "Error is: " + error);
  }
}




function setForCurrentStep(content, currentStep)
{
  var todelete = [];
  for each (var child in content) {
    var stepstr = child.@step.toString();
    var stepsarr = stepstr.split(",");
    for (var s = 0; s < stepsarr.length; s++) {
      var step = parseInt(stepsarr[s]);
      if ((step > 0 && step > currentStep) ||
          (step < 0 && -step <= currentStep)) {
        todelete.push(child);
      }
    }
  }

  
  for (var d = 0; d < todelete.length; d++)
    delete content.*[todelete[d].childIndex()];
  
  for each (var child in content) {
    delete child.@step;
    setForCurrentStep(child, currentStep);
  }
}








function compareOutput(actual, expected, isroot)
{
  
  
  
  if (expected.localName() != "output" && isroot) {
    
    
    
    if (actual.childNodes.length != 2)
      return "incorrect child node count of root " +
             (actual.childNodes.length - 1) + " expected 1";
    return compareOutput(actual.lastChild, expected, false);
  }

  var t;

  
  if (expected.nodeKind() == "text") {
    if (actual.nodeValue != expected.toString())
      return "Text " + actual.nodeValue + " doesn't match " + expected.toString();
    return "";
  }

  if (!isroot) {
    var anyid = false;
    
    if (actual.localName != expected.localName())
      return "Tag name " + expected.localName() + " not found";

    
    

    var expectedAttrs = expected.attributes();
    for (var a = 0; a < expectedAttrs.length(); a++) {
      var attr = expectedAttrs[a];
      expectedAttrs.length(); 
      var expectval = "" + attr;
      
      
      if (attr.name() == "anyid" && expectval == "true") {
        anyid = true;
        if (!actual.hasAttribute("id"))
          return "expected id attribute";
      }
      else if (actual.getAttribute(attr.name()) != expectval) {
        return "attribute " + attr.name() + " is '" +
               actual.getAttribute(attr.name()) + "' instead of  '" + expectval + "'";
      }
    }

    
    
    var length = actual.attributes.length;
    for (t = 0; t < length; t++) {
      var aattr = actual.attributes[t];
      var expectval = "" + expected.@[aattr.name];
      
      if (expectval != actual.getAttribute(aattr.name) &&
          aattr.name != "staticHint" && aattr.name != "xmlns" &&
          (aattr.name != "id" || !anyid))
        return "extra attribute " + aattr.name;
    }
  }

  
  
  length = actual.childNodes.length - (isroot ? 1 : 0);
  if (length != expected.children().length())
    return "incorrect child node count of " + actual.localName + " " + length +
           " expected " + expected.children().length();

  
  var unordered = (expected.localName() == "output" && expected.@unordered == "true");

  
  var adj = 0;
  for (t = 0; t < actual.childNodes.length; t++) {
    var actualnode = actual.childNodes[t];
    
    
    if (isroot && actualnode.localName == "template") {
      adj++;
    }
    else {
      var output = "unexpected";
      if (unordered) {
        var expectedChildren = expected.children();
        for (var e = 0; e < expectedChildren.length(); e++) {
          output = compareOutput(actualnode, expectedChildren[e], false);
          if (!output)
            break;
        }
      }
      else {
        output = compareOutput(actualnode, expected.children()[t - adj], false);
      }

      
      if (output)
        return output;
    }
  }

  return "";
}




function copyRDFDataSource(root, sourceds)
{
  var sourceds;
  var dsourcesArr = [];
  var composite = root.database;
  var dsources = composite.GetDataSources();
  while (dsources.hasMoreElements()) {
    sourceds = dsources.getNext().QueryInterface(Components.interfaces.nsIRDFDataSource);
    dsourcesArr.push(sourceds);
  }

  for (var d = 0; d < dsourcesArr.length; d++)
    composite.RemoveDataSource(dsourcesArr[d]);

  var newds = Components.classes["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
                createInstance(Components.interfaces.nsIRDFDataSource);

  var sourcelist = sourceds.GetAllResources();
  while (sourcelist.hasMoreElements()) {
    var source = sourcelist.getNext();
    var props = sourceds.ArcLabelsOut(source);
    while (props.hasMoreElements()) {
      var prop = props.getNext();
      if (prop instanceof Components.interfaces.nsIRDFResource) {
        var targets = sourceds.GetTargets(source, prop, true);
        while (targets.hasMoreElements())
          newds.Assert(source, prop, targets.getNext(), true);
      }
    }
  }

  composite.AddDataSource(newds);
  root.builder.rebuild();

  return newds;
}





function treeViewToDOM(tree)
{
  var treechildren = document.createElement("treechildren");

  if (tree.view)
    treeViewToDOMInner(tree.columns, treechildren, tree.view, tree.builder, 0, 0);

  return treechildren;
}

function treeViewToDOMInner(columns, treechildren, view, builder, start, level)
{
  var end = view.rowCount;

  for (var i = start; i < end; i++) {
    if (view.getLevel(i) < level)
      return i - 1;

    var id = builder ? builder.getResourceAtIndex(i).Value : "id" + i;
    var item = document.createElement("treeitem");
    item.setAttribute("id", id);
    treechildren.appendChild(item);

    var row = document.createElement("treerow");
    item.appendChild(row);

    for (var c = 0; c < columns.length; c++) {
      var cell = document.createElement("treecell");
      var label = view.getCellText(i, columns[c]);
      if (label)
        cell.setAttribute("label", label);
      row.appendChild(cell);
    }

    if (view.isContainer(i)) {
      item.setAttribute("container", "true");
      item.setAttribute("empty", view.isContainerEmpty(i) ? "true" : "false");

      if (!view.isContainerEmpty(i) && view.isContainerOpen(i)) {
        item.setAttribute("open", "true");

        var innertreechildren = document.createElement("treechildren");
        item.appendChild(innertreechildren);

        i = treeViewToDOMInner(columns, innertreechildren, view, builder, i + 1, level + 1);
      }
    }
  }

  return i;
}

function expectConsoleMessage(ref, id, isNew, isActive, extra)
{
  var message = "In template with id root" +
                (ref ? " using ref " + ref : "") + "\n    " +
                (isNew ? "New " : "Removed ") + (isActive ? "active" : "inactive") +
                " result for query " + extra + ": " + id;
  expectedConsoleMessages.push(message);
}

function compareConsoleMessages()
{
  var consoleService = Components.classes["@mozilla.org/consoleservice;1"].
                         getService(Components.interfaces.nsIConsoleService);
  var out = {};
  consoleService.getMessageArray(out, {});
  var messages = (out.value || []).map(function (m) m.message);
  
  var expect = expectedConsoleMessages.concat();
  for (var m = 0; m < messages.length; m++) {
    if (messages[m] == expect[0]) {
      ok(true, "found message " + expect.shift());
    }
  }
  if (expect.length != 0) {
    ok(false, "failed to find expected console messages: " + expect);
  }
}

function copyToProfile(filename)
{
  if (Cc === undefined) {
    var Cc = Components.classes;
    var Ci = Components.interfaces;
  }

  var loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                         .getService(Ci.mozIJSSubScriptLoader);
  loader.loadSubScript("chrome://mochikit/content/chrome-harness.js");

  var file = Cc["@mozilla.org/file/directory_service;1"]
                       .getService(Ci.nsIProperties)
                       .get("ProfD", Ci.nsIFile);
  file.append(filename);

  var parentURI = getResolvedURI(getRootDirectory(window.location.href));
  if (parentURI.JARFile) {
    parentURI = extractJarToTmp(parentURI);
  } else {
    var fileHandler = Cc["@mozilla.org/network/protocol;1?name=file"].
                      getService(Ci.nsIFileProtocolHandler);
    parentURI = fileHandler.getFileFromURLSpec(parentURI.spec);
  }

  parentURI = parentURI.QueryInterface(Ci.nsILocalFile);
  parentURI.append(filename);
  try {
    var retVal = parentURI.copyToFollowingLinks(file.parent, filename);
  } catch (ex) {
    
  }
}
