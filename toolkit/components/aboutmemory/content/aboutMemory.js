





































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;



var gVerbose = (location.href.split(/[\?,]/).indexOf("verbose") !== -1);

var gAddedObserver = false;

const KIND_NONHEAP = Ci.nsIMemoryReporter.KIND_NONHEAP;
const KIND_HEAP    = Ci.nsIMemoryReporter.KIND_HEAP;
const KIND_OTHER   = Ci.nsIMemoryReporter.KIND_OTHER;
const UNITS_BYTES  = Ci.nsIMemoryReporter.UNITS_BYTES;
const UNITS_COUNT  = Ci.nsIMemoryReporter.UNITS_COUNT;
const UNITS_COUNT_CUMULATIVE = Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE;
const UNITS_PERCENTAGE = Ci.nsIMemoryReporter.UNITS_PERCENTAGE;

const kUnknown = -1;    

const kTreeDescriptions = {
  'explicit' :
    "This tree covers explicit memory allocations by the application, " +
    "both at the operating system level (via calls to functions such as " +
    "VirtualAlloc, vm_allocate, and mmap), and at the heap allocation level " +
    "(via functions such as malloc, calloc, realloc, memalign, operator " +
    "new, and operator new[]).  It excludes memory that is mapped implicitly " +
    "such as code and data segments, and thread stacks.  It also excludes " +
    "heap memory that has been freed by the application but is still being " +
    "held onto by the heap allocator.  It is not guaranteed to cover every " +
    "explicit allocation, but it does cover most (including the entire " +
    "heap), and therefore it is the single best number to focus on when " +
    "trying to reduce memory usage.",

  'resident':
    "This tree shows how much space in physical memory each of the " +
    "process's mappings is currently using (the mapping's 'resident set size', " +
    "or 'RSS'). This is a good measure of the 'cost' of the mapping, although " +
    "it does not take into account the fact that shared libraries may be mapped " +
    "by multiple processes but appear only once in physical memory. " +
    "Note that the 'resident' value here might not equal the value for " +
    "'resident' under 'Other Measurements' because the two measurements are not " +
    "taken at exactly the same time.",

  'pss':
    "This tree shows how much space in physical memory can be 'blamed' on this " +
    "process.  For each mapping, its 'proportional set size' (PSS) is the " +
    "mapping's resident size divided by the number of processes which use the " +
    "mapping.  So if a mapping is private to this process, its PSS should equal " +
    "its RSS.  But if a mapping is shared between three processes, its PSS in " +
    "each of the processes would be 1/3 its RSS.",

  'vsize':
    "This tree shows how much virtual addres space each of the process's " +
    "mappings takes up (the mapping's 'vsize').  A mapping may have a large " +
    "vsize but use only a small amount of physical memory; the resident set size " +
    "of a mapping is a better measure of the mapping's 'cost'. Note that the " +
    "'vsize' value here might not equal the value for 'vsize' under 'Other " +
    "Measurements' because the two measurements are not taken at exactly the " +
    "same time.",

  'swap':
    "This tree shows how much space in the swap file each of the process's " +
    "mappings is currently using. Mappings which are not in the swap file " +
    "(i.e., nodes which would have a value of 0 in this tree) are omitted."
};

const kTreeNames = {
  'explicit': 'Explicit Allocations',
  'resident': 'Resident Set Size (RSS) Breakdown',
  'pss':      'Proportional Set Size (PSS) Breakdown',
  'vsize':    'Virtual Size Breakdown',
  'swap':     'Swap Usage Breakdown',
  'other':    'Other Measurements'
};

const kMapTreePaths = ['map/resident', 'map/pss', 'map/vsize', 'map/swap'];

function onLoad()
{
  var os = Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-memory-reporter-request", null);

  os.addObserver(ChildMemoryListener, "child-memory-reporter-update", false);
  gAddedObserver = true;

  update();
}

function onUnload()
{
  
  
  
  if (gAddedObserver) {
    var os = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
    os.removeObserver(ChildMemoryListener, "child-memory-reporter-update");
  }
}

function ChildMemoryListener(aSubject, aTopic, aData)
{
  update();
}

function $(n)
{
  return document.getElementById(n);
}

function doGlobalGC()
{
  Cu.forceGC();
  var os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-gc-request", null);
  update();
}

function doCC()
{
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .cycleCollect();
  var os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-cc-request", null);
  update();
}




function sendHeapMinNotifications()
{
  function runSoon(f)
  {
    var tm = Cc["@mozilla.org/thread-manager;1"]
              .getService(Ci.nsIThreadManager);

    tm.mainThread.dispatch({ run: f }, Ci.nsIThread.DISPATCH_NORMAL);
  }

  function sendHeapMinNotificationsInner()
  {
    var os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);
    os.notifyObservers(null, "memory-pressure", "heap-minimize");

    if (++j < 3)
      runSoon(sendHeapMinNotificationsInner);
    else
      runSoon(update);
  }

  var j = 0;
  sendHeapMinNotificationsInner();
}

function toggleTreeVisibility(aEvent)
{
  var headerElem = aEvent.target;

  
  
  var treeElem = $(headerElem.id.replace(/^header-/, 'pre-'));

  treeElem.classList.toggle('collapsed');
}

function Reporter(aPath, aKind, aUnits, aAmount, aDescription)
{
  this._path        = aPath;
  this._kind        = aKind;
  this._units       = aUnits;
  this._amount      = aAmount;
  this._description = aDescription;
  
  
}

Reporter.prototype = {
  
  
  
  
  merge: function(r) {
    if (this._amount !== kUnknown && r._amount !== kUnknown) {
      this._amount += r._amount;
    } else if (this._amount === kUnknown && r._amount !== kUnknown) {
      this._amount = r._amount;
    }
    this._nMerged = this._nMerged ? this._nMerged + 1 : 2;
  },

  treeNameMatches: function(aTreeName) {
    return this._path.slice(0, aTreeName.length) === aTreeName;
  }
};

function getReportersByProcess()
{
  var mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
      getService(Ci.nsIMemoryReporterManager);

  
  
  
  
  
  
  
  
  
  
  var reportersByProcess = {};

  function addReporter(aProcess, aPath, aKind, aUnits, aAmount, aDescription)
  {
    var process = aProcess === "" ? "Main" : aProcess;
    var r = new Reporter(aPath, aKind, aUnits, aAmount, aDescription);
    if (!reportersByProcess[process]) {
      reportersByProcess[process] = {};
    }
    var reporters = reportersByProcess[process];
    var reporter = reporters[r._path];
    if (reporter) {
      
      
      reporter.merge(r);
    } else {
      reporters[r._path] = r;
    }
  }

  
  var e = mgr.enumerateReporters();
  while (e.hasMoreElements()) {
    var rOrig = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
    try {
      addReporter(rOrig.process, rOrig.path, rOrig.kind, rOrig.units,
                  rOrig.amount, rOrig.description);
    }
    catch(e) {
      debug("An error occurred when collecting results from the memory reporter " +
            rOrig.path + ": " + e);
    }
  }
  var e = mgr.enumerateMultiReporters();
  while (e.hasMoreElements()) {
    var mrOrig = e.getNext().QueryInterface(Ci.nsIMemoryMultiReporter);
    try {
      mrOrig.collectReports(addReporter, null);
    }
    catch(e) {
      debug("An error occurred when collecting a multi-reporter's results: " + e);
    }
  }

  return reportersByProcess;
}




function update()
{
  
  
  var content = $("content");
  content.parentNode.replaceChild(content.cloneNode(false), content);
  content = $("content");

  if (gVerbose)
    content.parentNode.classList.add('verbose');
  else
    content.parentNode.classList.add('non-verbose');

  
  
  var reportersByProcess = getReportersByProcess();
  var text = genProcessText("Main", reportersByProcess["Main"]);
  for (var process in reportersByProcess) {
    if (process !== "Main") {
      text += genProcessText(process, reportersByProcess[process]);
    }
  }

  
  const GCDesc = "Do a global garbage collection.";
  const CCDesc = "Do a cycle collection.";
  const MPDesc = "Send three \"heap-minimize\" notifications in a " +
                 "row.  Each notification triggers a global garbage " +
                 "collection followed by a cycle collection, and causes the " +
                 "process to reduce memory usage in other ways, e.g. by " +
                 "flushing various caches.";

  text += "<div>" +
    "<button title='" + GCDesc + "' onclick='doGlobalGC()'>GC</button>" +
    "<button title='" + CCDesc + "' onclick='doCC()'>CC</button>" +
    "<button title='" + MPDesc + "' onclick='sendHeapMinNotifications()'>" + "Minimize memory usage</button>" +
    "</div>";

  
  text += "<div>";
  text += gVerbose
        ? "<span class='option'><a href='about:memory'>Less verbose</a></span>"
        : "<span class='option'><a href='about:memory?verbose'>More verbose</a></span>";
  text += "</div>";

  text += "<div>" +
          "<span class='option'><a href='about:support'>Troubleshooting information</a></span>" +
          "</div>";

  text += "<div>" +
          "<span class='legend'>Hover the pointer over the name of a memory " +
          "reporter to see a detailed description of what it measures. Click a " +
          "heading to expand or collapse its tree.</span>" +
          "</div>";

  var div = document.createElement("div");
  div.innerHTML = text;
  content.appendChild(div);
}




function TreeNode(aName)
{
  
  this._name = aName;
  this._kids = [];
  
  
  
  
  
  
  
  
}

TreeNode.prototype = {
  findKid: function(aName) {
    for (var i = 0; i < this._kids.length; i++) {
      if (this._kids[i]._name === aName) {
        return this._kids[i];
      }
    }
    return undefined;
  },

  toString: function() {
    return formatBytes(this._amount);
  }
};

TreeNode.compare = function(a, b) {
  return b._amount - a._amount;
};











function buildTree(aReporters, aTreeName)
{
  
  
  

  
  
  var foundReporter = false;
  for (var path in aReporters) {
    if (aReporters[path].treeNameMatches(aTreeName)) {
      foundReporter = true;
      break;
    }
  }

  if (!foundReporter) {
    
    return null;
  }

  var t = new TreeNode("falseRoot");
  for (var path in aReporters) {
    
    var r = aReporters[path];
    if (r.treeNameMatches(aTreeName)) {
      assert(r._kind === KIND_HEAP || r._kind === KIND_NONHEAP,
             "reporters in the tree must have KIND_HEAP or KIND_NONHEAP");
      assert(r._units === UNITS_BYTES, "r._units === UNITS_BYTES");
      var names = r._path.split('/');
      var u = t;
      for (var i = 0; i < names.length; i++) {
        var name = names[i];
        var uMatch = u.findKid(name);
        if (uMatch) {
          u = uMatch;
        } else {
          var v = new TreeNode(name);
          u._kids.push(v);
          u = v;
        }
      }
      
      u._kind = r._kind;
      if (r._nMerged) {
        u._nMerged = r._nMerged;
      }
    }
  }

  
  
  t = t._kids[0];

  
  
  function fillInTree(aT, aPrepath)
  {
    var path = aPrepath ? aPrepath + '/' + aT._name : aT._name;
    if (aT._kids.length === 0) {
      
      assert(aT._kind !== undefined, "aT._kind !== undefined");
      aT._description = getDescription(aReporters, path);
      var amount = getBytes(aReporters, path);
      if (amount !== kUnknown) {
        aT._amount = amount;
      } else {
        aT._amount = 0;
        aT._hasProblem = true;
      }
    } else {
      
      var childrenBytes = 0;
      for (var i = 0; i < aT._kids.length; i++) {
        
        childrenBytes += fillInTree(aT._kids[i], path);
      }
      if (aT._kind !== undefined) {
        aT._description = getDescription(aReporters, path);
        var amount = getBytes(aReporters, path);
        if (amount !== kUnknown) {
          
          
          aT._amount = amount;
          var other = new TreeNode("other");
          other._description = "All unclassified " + aT._name + " memory.",
          other._amount = aT._amount - childrenBytes,
          aT._kids.push(other);
        } else {
          
          
          aT._amount = childrenBytes;
          aT._hasProblem = true;
        }
      } else {
        
        
        aT._amount = childrenBytes;
        aT._description = "The sum of all entries below '" + aT._name + "'.";
      }
    }
    assert(aT._amount !== kUnknown, "aT._amount !== kUnknown");
    return aT._amount;
  }

  fillInTree(t, "");

  
  
  var slashCount = 0;
  for (var i = 0; i < aTreeName.length; i++) {
    if (aTreeName[i] == '/') {
      assert(t._kids.length == 1, "Not expecting multiple kids here.");
      t = t._kids[0];
    }
  }

  
  t._description = kTreeDescriptions[t._name];

  return t;
}




function fixUpExplicitTree(aT, aReporters) {
  
  
  
  var s = "";
  function getKnownHeapUsedBytes(aT)
  {
    if (aT._kind === KIND_HEAP) {
      return aT._amount;
    } else {
      var n = 0;
      for (var i = 0; i < aT._kids.length; i++) {
        n += getKnownHeapUsedBytes(aT._kids[i]);
      }
      return n;
    }
  }

  
  
  
  var heapUsedBytes = getBytes(aReporters, "heap-allocated", true);
  var unknownHeapUsedBytes = 0;
  var hasProblem = true;
  if (heapUsedBytes !== kUnknown) {
    unknownHeapUsedBytes = heapUsedBytes - getKnownHeapUsedBytes(aT);
    hasProblem = false;
  }
  var heapUnclassified = new TreeNode("heap-unclassified");
  
  
  
  heapUnclassified._description =
      kindToString(KIND_HEAP) +
      "Memory not classified by a more specific reporter. This includes " +
      "waste due to internal fragmentation in the heap allocator (caused " +
      "when the allocator rounds up request sizes).";
  heapUnclassified._amount = unknownHeapUsedBytes;
  if (hasProblem) {
    heapUnclassified._hasProblem = true;
  }

  aT._kids.push(heapUnclassified);
  aT._amount += unknownHeapUsedBytes;
}










function filterTree(aTotalBytes, aT)
{
  const omitThresholdPerc = 0.5; 

  function shouldOmit(aBytes)
  {
    return !gVerbose &&
           aTotalBytes !== kUnknown &&
           (100 * aBytes / aTotalBytes) < omitThresholdPerc;
  }

  aT._kids.sort(TreeNode.compare);

  for (var i = 0; i < aT._kids.length; i++) {
    if (shouldOmit(aT._kids[i]._amount)) {
      
      
      
      var i0 = i;
      var aggBytes = 0;
      for ( ; i < aT._kids.length; i++) {
        aggBytes += aT._kids[i]._amount;
      }
      aT._kids.splice(i0, aT._kids.length);
      var n = i - i0;
      var rSub = new TreeNode("(" + n + " omitted)");
      rSub._amount = aggBytes;
      rSub._description =
        n + " sub-trees that were below the " + omitThresholdPerc +
        "% significance threshold.  Click 'More verbose' at the bottom of " +
        "this page to see them.";

      
      
      
      aT._kids[i0] = rSub;
      aT._kids.sort(TreeNode.compare);
      break;
    }
    filterTree(aTotalBytes, aT._kids[i]);
  }
}










function genProcessText(aProcess, aReporters)
{
  var explicitTree = buildTree(aReporters, 'explicit');
  fixUpExplicitTree(explicitTree, aReporters);
  filterTree(explicitTree._amount, explicitTree);
  var explicitText = genTreeText(explicitTree, aProcess);

  var mapTreeText = '';
  kMapTreePaths.forEach(function(t) {
    var tree = buildTree(aReporters, t);

    
    if (tree) {
      filterTree(tree._amount, tree);
      mapTreeText += genTreeText(tree, aProcess);
    }
  });

  
  
  var otherText = genOtherText(aReporters, aProcess);

  
  return "<h1>" + aProcess + " Process</h1>\n\n" +
         explicitText + mapTreeText + otherText +
         "<hr></hr>";
}








function formatInt(aN)
{
  var neg = false;
  if (aN < 0) {
    neg = true;
    aN = -aN;
  }
  var s = "";
  while (true) {
    var k = aN % 1000;
    aN = Math.floor(aN / 1000);
    if (aN > 0) {
      if (k < 10) {
        s = ",00" + k + s;
      } else if (k < 100) {
        s = ",0" + k + s;
      } else {
        s = "," + k + s;
      }
    } else {
      s = k + s;
      break;
    }
  }
  return neg ? "-" + s : s;
}








function formatBytes(aBytes)
{
  var unit = gVerbose ? "B" : "MB";

  var s;
  if (gVerbose) {
    s = formatInt(aBytes) + " " + unit;
  } else {
    var mbytes = (aBytes / (1024 * 1024)).toFixed(2);
    var a = String(mbytes).split(".");
    s = formatInt(a[0]) + "." + a[1] + " " + unit;
  }
  return s;
}








function formatPercentage(aPerc100x)
{
  return (aPerc100x / 100).toFixed(2) + "%";
}












function pad(aS, aN, aC)
{
  var padding = "";
  var n2 = aN - aS.length;
  for (var i = 0; i < n2; i++) {
    padding += aC;
  }
  return padding + aS;
}













function getBytes(aReporters, aPath, aDoNotMark)
{
  var r = aReporters[aPath];
  assert(r, "getBytes: no such Reporter: " + aPath);
  if (!aDoNotMark) {
    r._done = true;
  }
  return r._amount;
}










function getDescription(aReporters, aPath)
{
  var r = aReporters[aPath];
  assert(r, "getDescription: no such Reporter: " + aPath);
  return r._description;
}

function genMrValueText(aValue)
{
  return "<span class='mrValue'>" + aValue + "</span>";
}

function kindToString(aKind)
{
  switch (aKind) {
   case KIND_NONHEAP: return "(Non-heap) ";
   case KIND_HEAP:    return "(Heap) ";
   case KIND_OTHER:
   case undefined:    return "";
   default:           assert(false, "bad kind in kindToString");
  }
}

function escapeQuotes(aStr)
{
  return aStr.replace(/\&/g, '&amp;').replace(/'/g, '&#39;');
}


function escapeAll(aStr)
{
  return aStr.replace(/\&/g, '&amp;').replace(/'/g, '&#39;').
              replace(/\</g, '&lt;').replace(/>/g, '&gt;').
              replace(/\"/g, '&quot;');
}




function flipBackslashes(aStr)
{
  return aStr.replace(/\\/g, '/');
}

function prepName(aStr)
{
  return escapeAll(flipBackslashes(aStr));
}

function prepDesc(aStr)
{
  return escapeQuotes(flipBackslashes(aStr));
}

function genMrNameText(aKind, aDesc, aName, aHasProblem, aNMerged)
{
  var text = "-- <span class='mrName hasDesc' title='" +
             kindToString(aKind) + prepDesc(aDesc) +
             "'>" + prepName(aName) + "</span>";
  if (aHasProblem) {
    const problemDesc =
      "Warning: this memory reporter was unable to compute a useful value. " +
      "The reported value is the sum of all entries below '" + aName + "', " +
      "which is probably less than the true value.";
    text += " <span class='mrStar' title=\"" + problemDesc + "\">[*]</span>";
  }
  if (aNMerged) {
    const dupDesc = "This value is the sum of " + aNMerged +
                    " memory reporters that all have the same path.";
    text += " <span class='mrStar' title=\"" + dupDesc + "\">[" + 
            aNMerged + "]</span>";
  }
  return text + '\n';
}










function genTreeText(aT, aProcess)
{
  var treeBytes = aT._amount;
  var rootStringLength = aT.toString().length;
  var isExplicitTree = aT._name == 'explicit';

  













  function genTreeText2(aT, aIndentGuide, aParentStringLength)
  {
    function repeatStr(aC, aN)
    {
      var s = "";
      for (var i = 0; i < aN; i++) {
        s += aC;
      }
      return s;
    }

    
    
    
    
    
    const kHorizontal       = "\u2500",
          kVertical         = "\u2502",
          kUpAndRight       = "\u2514",
          kVerticalAndRight = "\u251c";
    var indent = "<span class='treeLine'>";
    if (aIndentGuide.length > 0) {
      for (var i = 0; i < aIndentGuide.length - 1; i++) {
        indent += aIndentGuide[i]._isLastKid ? " " : kVertical;
        indent += repeatStr(" ", aIndentGuide[i]._depth - 1);
      }
      indent += aIndentGuide[i]._isLastKid ? kUpAndRight : kVerticalAndRight;
      indent += repeatStr(kHorizontal, aIndentGuide[i]._depth - 1);
    }

    
    
    var tString = aT.toString();
    var extraIndentLength = Math.max(aParentStringLength - tString.length, 0);
    if (extraIndentLength > 0) {
      for (var i = 0; i < extraIndentLength; i++) {
        indent += kHorizontal;
      }
      aIndentGuide[aIndentGuide.length - 1]._depth += extraIndentLength;
    }
    indent += "</span>";

    
    var perc = "";
    if (aT._amount === treeBytes) {
      perc = "100.0";
    } else {
      perc = (100 * aT._amount / treeBytes).toFixed(2);
      perc = pad(perc, 5, '0');
    }
    perc = "<span class='mrPerc'>(" + perc + "%)</span> ";

    
    
    var kind = isExplicitTree ? aT._kind : undefined;
    var text = indent + genMrValueText(tString) + " " + perc +
               genMrNameText(kind, aT._description, aT._name,
                             aT._hasProblem, aT._nMerged);

    for (var i = 0; i < aT._kids.length; i++) {
      
      aIndentGuide.push({ _isLastKid: (i === aT._kids.length - 1), _depth: 3 });
      text += genTreeText2(aT._kids[i], aIndentGuide, tString.length);
      aIndentGuide.pop();
    }
    return text;
  }

  var text = genTreeText2(aT, [], rootStringLength);

  
  
  return genSectionMarkup(aProcess, aT._name, text, !isExplicitTree);
}

function OtherReporter(aPath, aUnits, aAmount, aDescription, 
                       aNMerged)
{
  
  this._path        = aPath;
  this._units       = aUnits;
  if (aAmount === kUnknown) {
    this._amount     = 0;
    this._hasProblem = true;
  } else {
    this._amount = aAmount;
  }
  this._description = aDescription;
  this.asString = this.toString();
}

OtherReporter.prototype = {
  toString: function() {
    switch(this._units) {
      case UNITS_BYTES:            return formatBytes(this._amount);
      case UNITS_COUNT:
      case UNITS_COUNT_CUMULATIVE: return formatInt(this._amount);
      case UNITS_PERCENTAGE:       return formatPercentage(this._amount);
      default:
        assert(false, "bad units in OtherReporter.toString");
    }
  }
};

OtherReporter.compare = function(a, b) {
  return a._path < b._path ? -1 :
         a._path > b._path ?  1 :
         0;
};










function genOtherText(aReportersByProcess, aProcess)
{
  
  
  
  var maxStringLength = 0;
  var otherReporters = [];
  for (var path in aReportersByProcess) {
    var r = aReportersByProcess[path];
    if (!r._done) {
      assert(r._kind === KIND_OTHER, "_kind !== KIND_OTHER for " + r._path);
      assert(r.nMerged === undefined);  
      var hasProblem = false;
      if (r._amount === kUnknown) {
        hasProblem = true;
      }
      var o = new OtherReporter(r._path, r._units, r._amount, r._description);
      otherReporters.push(o);
      if (o.asString.length > maxStringLength) {
        maxStringLength = o.asString.length;
      }
    }
  }
  otherReporters.sort(OtherReporter.compare);

  
  var text = "";
  for (var i = 0; i < otherReporters.length; i++) {
    var o = otherReporters[i];
    text += genMrValueText(pad(o.asString, maxStringLength, ' ')) + " ";
    text += genMrNameText(KIND_OTHER, o._description, o._path, o._hasProblem);
  }

  
  const desc = "This list contains other memory measurements that cross-cut " +
               "the requested memory measurements above."

  return genSectionMarkup(aProcess, 'other', text, false);
}

function genSectionMarkup(aProcess, aName, aText, aCollapsed)
{
  var headerId = 'header-' + aProcess + '-' + aName;
  var preId = 'pre-' + aProcess + '-' + aName;
  var elemClass = (aCollapsed ? 'collapsed' : '') + ' tree';

  
  return '<h2 id="' + headerId + '" class="' + elemClass + '" ' +
         'onclick="toggleTreeVisibility(event)">' +
           kTreeNames[aName] +
         '</h2>\n' +
         '<pre id="' + preId + '" class="' + elemClass + '">' + aText + '</pre>\n';
}

function assert(aCond, aMsg)
{
  if (!aCond) {
    throw("assertion failed: " + aMsg);
  }
}

function debug(x)
{
  var content = $("content");
  var div = document.createElement("div");
  div.innerHTML = JSON.stringify(x);
  content.appendChild(div);
}
