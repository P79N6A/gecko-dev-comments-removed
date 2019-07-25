





































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;



var gVerbose = (location.href.split(/[\?,]/).indexOf("verbose") !== -1);

var gAddedObserver = false;

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  var tmrTable = {};
  var e = mgr.enumerateReporters();
  while (e.hasMoreElements()) {
    var mr = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
    var process;
    var tmr = {};
    var i = mr.path.indexOf(':');
    if (i === -1) {
      process = "Main";
      tmr._tpath = mr.path;
    } else {
      process = mr.path.slice(0, i);
      tmr._tpath = mr.path.slice(i + 1);
    }
    tmr._description = mr.description;
    tmr._memoryUsed  = mr.memoryUsed;

    if (!tmrTable[process]) {
      tmrTable[process] = {};
    }
    var tmrs = tmrTable[process];
    if (tmrs[tmr._tpath]) {
      
      
      tmrs[tmr._tpath]._memoryUsed += tmr._memoryUsed;
    } else {
      tmrs[tmr._tpath] = tmr;
    }
  }

  
  
  var text = genProcessText("Main", tmrTable["Main"]);
  for (var process in tmrTable) {
    if (process !== "Main") {
      text += genProcessText(process, tmrTable[process]);
    }
  }

  
  const GCDesc = "Do a global garbage collection.";
  
  const CCDesc = "Do a global garbage collection followed by a cycle " +
                 "collection. (It currently is not possible to do a cycle " +
                 "collection on its own, see bug 625302.)";
  const MPDesc = "Send three \"heap-minimize\" notifications in a row.  Each " +
                 "notification triggers a global garbage collection followed " +
                 "by a cycle collection, and causes the process to reduce " +
                 "memory usage in other ways, e.g. by flushing various caches.";

  text += "<div>" +
    "<button title='" + GCDesc + "' onclick='doGlobalGC()'>GC</button>" +
    "<button title='" + CCDesc + "' onclick='doGlobalGCandCC()'>GC + CC</button>" +
    "<button title='" + MPDesc + "' onclick='sendHeapMinNotifications()'>" + "Minimize memory usage</button>" +
    "</div>";

  
  text += gVerbose
        ? "<span class='option'><a href='about:memory'>Less verbose</a></span>"
        : "<span class='option'><a href='about:memory?verbose'>More verbose</a></span>";

  var div = document.createElement("div");
  div.innerHTML = text;
  content.appendChild(div);
}










function genProcessText(aProcess, aTmrs)
{
  
  
  var mappedHeapUsedTmr = aTmrs["mapped/heap/used"];
  aTmrs["heap-used"] = {
    _tpath:       "heap-used",
    _description: mappedHeapUsedTmr._description,
    _memoryUsed:  mappedHeapUsedTmr._memoryUsed
  };

  

















  function buildTree(aTreeName, aOmitThresholdPerc)
  {
    function findKid(aName, aKids)
    {
      for (var i = 0; i < aKids.length; i++) {
        if (aKids[i]._name === aName) {
          return aKids[i];
        }
      }
      return undefined;
    }

    
    
    
    var t = { _name: "falseRoot", _kids: [] };
    for (var tpath in aTmrs) {
      var tmr = aTmrs[tpath];
      if (tmr._tpath.slice(0, aTreeName.length) === aTreeName) {
        var names = tmr._tpath.split('/');
        var u = t;
        for (var i = 0; i < names.length; i++) {
          var name = names[i];
          var uMatch = findKid(name, u._kids);
          if (uMatch) {
            u = uMatch;
          } else {
            var v = { _name: name, _kids: [] };
            u._kids.push(v);
            u = v;
          }
        }
        u._hasReporter = true;
      }
    }
    
    
    t = t._kids[0];

    
    
    
    function fillInTree(aT, aPretpath)
    {
      var tpath = aPretpath ? aPretpath + '/' + aT._name : aT._name;
      if (aT._kids.length === 0) {
        
        aT._memoryUsed = getBytes(aTmrs, tpath);
        aT._description = getDescription(aTmrs, tpath);
      } else {
        
        var childrenBytes = 0;
        for (var i = 0; i < aT._kids.length; i++) {
          
          var b = fillInTree(aT._kids[i], tpath);
          childrenBytes += (b === -1 ? 0 : b);
        }
        if (aT._hasReporter === true) {
          
          
          aT._memoryUsed = getBytes(aTmrs, tpath);
          aT._description = getDescription(aTmrs, tpath);
          if (aT._memoryUsed !== -1) {
            var other = {
              _name: "other",
              _description: "All unclassified " + aT._name + " memory.",
              _memoryUsed: aT._memoryUsed - childrenBytes,
              _kids: []
            };
            aT._kids.push(other);
          }
        } else {
          
          
          aT._memoryUsed = childrenBytes;
          aT._description = "The sum of all entries below " + aT._name + ".";
        }
      }
      return aT._memoryUsed;
    }
    fillInTree(t, "");

    function shouldOmit(aBytes)
    {
      return !gVerbose &&
             t._memoryUsed !== -1 &&
             (100 * aBytes / t._memoryUsed) < aOmitThresholdPerc;
    }

    






    function filterTree(aT)
    {
      var cmpTmrs = function(a, b) { return b._memoryUsed - a._memoryUsed };
      aT._kids.sort(cmpTmrs);

      for (var i = 0; i < aT._kids.length; i++) {
        if (shouldOmit(aT._kids[i]._memoryUsed)) {
          
          
          
          var i0 = i;
          var aggBytes = 0;
          var aggNames = [];
          for ( ; i < aT._kids.length; i++) {
            aggBytes += aT._kids[i]._memoryUsed;
            aggNames.push(aT._kids[i]._name);
          }
          aT._kids.splice(i0);
          var n = i - i0;
          var tmrSub = {
            _name: "(" + n + " omitted)",
            _description: "Omitted sub-trees: " + aggNames.join(", ") + ".",
            _memoryUsed: aggBytes,
            _kids: []
          };
          aT._kids[i0] = tmrSub;
          break;
        }
        filterTree(aT._kids[i]);
      }
    }
    filterTree(t);

    return t;
  }

  
  
  
  var mappedTree   = buildTree("mapped",    0.01);
  var heapUsedTree = buildTree("heap-used", 0.1);

  
  var text = "";
  text += "<h1>" + aProcess + " Process</h1>\n\n";
  text += genTreeText(mappedTree, "Mapped Memory");
  text += genTreeText(heapUsedTree, "Used Heap Memory");
  text += genOtherText(aTmrs);
  text += "<hr></hr>";
  return text;
}








function formatBytes(aBytes)
{
  var unit = gVerbose ? "B" : "MB";

  if (aBytes === -1) {
    return "??? " + unit;
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










function getBytes(aTmrs, aTpath)
{
  var tmr = aTmrs[aTpath];
  if (tmr) {
    var bytes = tmr._memoryUsed;
    tmr.done = true;
    return bytes;
  }
  
  
  
  
  return -2 * 1024 * 1024;
}










function getDescription(aTmrs, aTpath)
{
  var r = aTmrs[aTpath];
  return r ? r._description : "???";
}

function genMrValueText(aValue)
{
  return "<span class='mrValue'>" + aValue + "</span>";
}

function genMrNameText(aDesc, aName)
{
  return "-- <span class='mrName' title=\"" + aDesc + "\">" +
         aName + "</span>\n";
}










function genTreeText(aT, aTreeName)
{
  var treeBytes = aT._memoryUsed;
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

    
    
    var tMemoryUsedStr = formatBytes(aT._memoryUsed);
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
    if (treeBytes !== -1) {
      if (aT._memoryUsed === -1) {
        perc = "??.??";
      } else if (aT._memoryUsed === treeBytes) {
        perc = "100.0";
      } else {
        perc = (100 * aT._memoryUsed / treeBytes).toFixed(2);
        perc = pad(perc, 5, '0');
      }
      perc = "<span class='mrPerc'>(" + perc + "%)</span> ";
    }

    var text = indent + genMrValueText(tMemoryUsedStr) + " " + perc +
               genMrNameText(aT._description, aT._name);

    for (var i = 0; i < aT._kids.length; i++) {
      
      aIndentGuide.push({ _isLastKid: (i === aT._kids.length - 1), _depth: 3 });
      text += genTreeText2(aT._kids[i], aIndentGuide, tBytesLength);
      aIndentGuide.pop();
    }
    return text;
  }

  var text = genTreeText2(aT, [], treeBytesLength);
  
  return "<h2>" + aTreeName + "</h2>\n<pre>" + text + "</pre>\n";
}








function genOtherText(aTmrs)
{
  
  
  
  var maxBytes = 0;
  for (var tpath in aTmrs) {
    var tmr = aTmrs[tpath];
    if (!tmr.done && tmr._memoryUsed > maxBytes) {
      maxBytes = tmr._memoryUsed;
    }
  }

  
  var maxBytesLength = formatBytes(maxBytes).length;
  var text = "";
  for (var tpath in aTmrs) {
    var tmr = aTmrs[tpath];
    if (!tmr.done) {
      text += genMrValueText(
                pad(formatBytes(tmr._memoryUsed), maxBytesLength, ' ')) + " ";
      text += genMrNameText(tmr._description, tmr._tpath);
    }
  }

  
  return "<h2>Other Measurements</h2>\n<pre>" + text + "</pre>\n";
}

