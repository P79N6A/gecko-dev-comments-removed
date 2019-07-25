





































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;



var gVerbose = (location.href.split(/[\?,]/).indexOf("verbose") !== -1);

var gAddedObserver = false;

const KIND_MAPPED = Ci.nsIMemoryReporter.KIND_MAPPED;
const KIND_HEAP   = Ci.nsIMemoryReporter.KIND_HEAP;
const KIND_OTHER  = Ci.nsIMemoryReporter.KIND_OTHER;
const UNITS_BYTES = Ci.nsIMemoryReporter.UNITS_BYTES;
const UNITS_COUNT = Ci.nsIMemoryReporter.UNITS_COUNT;

const kUnknown = -1;    

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
  update();
}

function doGlobalGCandCC()
{
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .garbageCollect();
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




function update()
{
  
  
  var content = $("content");
  content.parentNode.replaceChild(content.cloneNode(false), content);
  content = $("content");

  var mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
      getService(Ci.nsIMemoryReporterManager);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  var reportersByProcess = {};
  var e = mgr.enumerateReporters();
  while (e.hasMoreElements()) {
    var rOrig = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
    var process = rOrig.process === "" ? "Main" : rOrig.process;
    var r = {
      _path:        rOrig.path,
      _kind:        rOrig.kind,
      _units:       rOrig.units,
      _amount:      rOrig.amount,
      _description: rOrig.description
    };
    if (!reportersByProcess[process]) {
      reportersByProcess[process] = {};
    }
    var reporters = reportersByProcess[process];
    if (reporters[r._path]) {
      
      
      reporters[r._path]._amount += r._amount;
    } else {
      reporters[r._path] = r;
    }
  }

  
  
  var text = genProcessText("Main", reportersByProcess["Main"]);
  for (var process in reportersByProcess) {
    if (process !== "Main") {
      text += genProcessText(process, reportersByProcess[process]);
    }
  }

  
  const GCDesc = "Do a global garbage collection.";
  
  const CCDesc = "Do a global garbage collection followed by a cycle " +
                 "collection. (It currently is not possible to do a cycle " +
                 "collection on its own, see bug 625302.)";
  const MPDesc = "Send three \"heap-minimize\" notifications in a " +
                 "row.  Each notification triggers a global garbage " +
                 "collection followed by a cycle collection, and causes the " +
                 "process to reduce memory usage in other ways, e.g. by " +
                 "flushing various caches.";

  text += "<div>" +
    "<button title='" + GCDesc + "' onclick='doGlobalGC()'>GC</button>" +
    "<button title='" + CCDesc + "' onclick='doGlobalGCandCC()'>GC + CC</button>" +
    "<button title='" + MPDesc + "' onclick='sendHeapMinNotifications()'>" + "Minimize memory usage</button>" +
    "</div>";

  
  text += "<div>";
  text += gVerbose
        ? "<span class='option'><a href='about:memory'>Less verbose</a></span>"
        : "<span class='option'><a href='about:memory?verbose'>More verbose</a></span>";
  text += "</div>";

  text += "<div>" +
          "<span class='legend'>Hover the pointer over the name of a memory " +
          "reporter to see a detailed description of what it measures.</span>"
          "</div>";


  var div = document.createElement("div");
  div.innerHTML = text;
  content.appendChild(div);
}




function cmp_amount(a, b)
{
  if (a._units != b._units)
    return b._units - a._units;
  else
    return b._amount - a._amount;
};










function genProcessText(aProcess, aReporters)
{
  














  function buildTree()
  {
    const treeName = "explicit";
    const omitThresholdPerc = 0.5; 

    function findKid(aName, aKids)
    {
      for (var i = 0; i < aKids.length; i++) {
        if (aKids[i]._name === aName) {
          return aKids[i];
        }
      }
      return undefined;
    }

    
    
    
    var t = {
      _name: "falseRoot",
      _kind: KIND_OTHER,
      _kids: []
    };
    for (var path in aReporters) {
      var r = aReporters[path];
      if (r._path.slice(0, treeName.length) === treeName) {
        var names = r._path.split('/');
        var u = t;
        for (var i = 0; i < names.length; i++) {
          var name = names[i];
          var uMatch = findKid(name, u._kids);
          if (uMatch) {
            u = uMatch;
          } else {
            var v = {
              _name: name,
              _kind: KIND_OTHER,
              _kids: []
            };
            u._kids.push(v);
            u = v;
          }
        }
        u._kind = r._kind;
        u._hasReporter = true;
      }
    }
    
    
    t = t._kids[0];

    
    
    
    function fillInTree(aT, aPrepath)
    {
      var path = aPrepath ? aPrepath + '/' + aT._name : aT._name;
      if (aT._kids.length === 0) {
        
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
          
          var b = fillInTree(aT._kids[i], path);
          childrenBytes += (b === kUnknown ? 0 : b);
        }
        if (aT._hasReporter === true) {
          aT._description = getDescription(aReporters, path);
          var amount = getBytes(aReporters, path);
          if (amount !== kUnknown) {
            
            
            aT._amount = amount;
            var other = {
              _name: "other",
              _kind: KIND_OTHER,
              _description: "All unclassified " + aT._name + " memory.",
              _amount: aT._amount - childrenBytes,
              _kids: []
            };
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
      return aT._amount;
    }
    fillInTree(t, "");

    
    
    
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

    
    
    
    var heapUsedBytes = getBytes(aReporters, "heap-used", true);
    var unknownHeapUsedBytes = 0;
    var hasProblem = true;
    if (heapUsedBytes !== kUnknown) {
      unknownHeapUsedBytes = heapUsedBytes - getKnownHeapUsedBytes(t);
      hasProblem = false;
    }
    var heapUnclassified = {
      _name: "heap-unclassified",
      _kind: KIND_HEAP,
      _description:
        "Memory not classified by a more specific reporter. This includes " +
        "memory allocated by the heap allocator in excess of that requested " +
        "by the application; this can happen when the heap allocator rounds " +
        "up request sizes.",
      _amount: unknownHeapUsedBytes,
      _hasProblem: hasProblem,
      _kids: []
    }
    t._kids.push(heapUnclassified);
    t._amount += unknownHeapUsedBytes;

    function shouldOmit(aBytes)
    {
      return !gVerbose &&
             t._amount !== kUnknown &&
             (100 * aBytes / t._amount) < omitThresholdPerc;
    }

    






    function filterTree(aT)
    {
      aT._kids.sort(cmp_amount);

      for (var i = 0; i < aT._kids.length; i++) {
        if (shouldOmit(aT._kids[i]._amount)) {
          
          
          
          var i0 = i;
          var aggBytes = 0;
          var aggNames = [];
          for ( ; i < aT._kids.length; i++) {
            aggBytes += aT._kids[i]._amount;
            aggNames.push(aT._kids[i]._name);
          }
          aT._kids.splice(i0);
          var n = i - i0;
          var rSub = {
            _name: "(" + n + " omitted)",
            _kind: KIND_OTHER,
            _description: "Omitted sub-trees: " + aggNames.join(", ") + ".",
            _amount: aggBytes,
            _kids: []
          };
          aT._kids[i0] = rSub;
          break;
        }
        filterTree(aT._kids[i]);
      }
    }
    filterTree(t);

    return t;
  }

  
  var text = "";
  text += "<h1>" + aProcess + " Process</h1>\n\n";
  text += genTreeText(buildTree());
  text += genOtherText(aReporters);
  text += "<hr></hr>";
  return text;
}









function formatReporterAmount(aReporter)
{
  switch(aReporter._units) {
    case UNITS_BYTES: return formatBytes(aReporter._amount);
    case UNITS_COUNT: return formatInt(aReporter._amount);
    default:          return "(???)"
  }
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
  if (r) {
    var bytes = r._amount;
    if (!aDoNotMark) {
      r._done = true;
    }
    return bytes;
  }
  
  
  
  return -2 * 1024 * 1024;
}










function getDescription(aReporters, aPath)
{
  var r = aReporters[aPath];
  return r ? r._description : "???";
}

function genMrValueText(aValue)
{
  return "<span class='mrValue'>" + aValue + "</span>";
}

function kindToString(aKind)
{
  switch (aKind) {
   case KIND_MAPPED: return "(Mapped) ";
   case KIND_HEAP:   return "(Heap) ";
   case KIND_OTHER:  return "";
   default:          return "(???) ";
  }
}

function escapeQuotes(aStr)
{
  return aStr.replace(/'/g, '&#39;');
}

function genMrNameText(aKind, aDesc, aName, aHasProblem)
{
  const problemDesc =
    "Warning: this memory reporter was unable to compute a useful value. " +
    "The reported value is the sum of all entries below '" + aName + "', " +
    "which is probably less than the true value.";
  var text = "-- <span class='mrName hasDesc' title='" +
             kindToString(aKind) + escapeQuotes(aDesc) +
             "'>" + aName + "</span>";
  text += aHasProblem
        ? " <span class='mrStar' title=\"" + problemDesc + "\">[*]</span>\n"
        : "\n";
  return text;
}








function genTreeText(aT)
{
  var treeBytes = aT._amount;
  var treeBytesLength = formatBytes(treeBytes).length;

  













  function genTreeText2(aT, aIndentGuide, aParentBytesLength)
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

    
    
    var tMemoryUsedStr = formatBytes(aT._amount);
    var tBytesLength = tMemoryUsedStr.length;
    var extraIndentLength = Math.max(aParentBytesLength - tBytesLength, 0);
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

    var text = indent + genMrValueText(tMemoryUsedStr) + " " + perc +
               genMrNameText(aT._kind, aT._description, aT._name,
                             aT._hasProblem);

    for (var i = 0; i < aT._kids.length; i++) {
      
      aIndentGuide.push({ _isLastKid: (i === aT._kids.length - 1), _depth: 3 });
      text += genTreeText2(aT._kids[i], aIndentGuide, tBytesLength);
      aIndentGuide.pop();
    }
    return text;
  }

  var text = genTreeText2(aT, [], treeBytesLength);
  
  const desc =
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
    "trying to reduce memory usage.";
               
  return "<h2 class='hasDesc' title='" + escapeQuotes(desc) +
         "'>Explicit Allocations</h2>\n" + "<pre>" + text + "</pre>\n";
}








function genOtherText(aReporters)
{
  
  
  
  var maxAmountLength = 0;
  var rArray = [];
  for (var path in aReporters) {
    var r = aReporters[path];
    if (!r._done) {
      var hasProblem = false;
      if (r._amount === kUnknown) {
        hasProblem = true;
      }
      var elem = {
        _path:        r._path,
        _kind:        r._kind,
        _units:       r._units,
        _amount:  hasProblem ? 0 : r._amount,
        _description: r._description,
        _hasProblem:  hasProblem
      };
      rArray.push(elem);
      var thisAmountLength = formatReporterAmount(elem).length;
      if (thisAmountLength > maxAmountLength) {
        maxAmountLength = thisAmountLength;
      }
    }
  }
  rArray.sort(cmp_amount);

  
  var text = "";
  for (var i = 0; i < rArray.length; i++) {
    var elem = rArray[i];
    text += genMrValueText(
              pad(formatReporterAmount(elem), maxAmountLength, ' ')) + " ";
    text += genMrNameText(elem._kind, elem._description, elem._path,
                          elem._hasProblem);
  }

  
  const desc = "This list contains other memory measurements that cross-cut " +
               "the requested memory measurements above."
  return "<h2 class='hasDesc' title='" + desc + "'>Other Measurements</h2>\n" +
         "<pre>" + text + "</pre>\n";
}

function debug(x)
{
  var content = $("content");
  var div = document.createElement("div");
  div.innerHTML = JSON.stringify(x);
  content.appendChild(div);
}

