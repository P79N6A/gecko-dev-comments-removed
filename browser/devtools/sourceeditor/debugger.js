



 "use strict";

const {Cu} = require("chrome");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const dbginfo = new WeakMap();






function SearchState() {
  this.posFrom = this.posTo = this.query = null;
}

function getSearchState(cm) {
  return cm.state.search || (cm.state.search = new SearchState());
}

function getSearchCursor(cm, query, pos) {
  
  return cm.getSearchCursor(query, pos,
    typeof query == "string" && query == query.toLowerCase());
}






function doSearch(ctx, rev, query) {
  let { cm } = ctx;
  let state = getSearchState(cm);

  if (state.query) {
    searchNext(ctx, rev);
    return;
  }

  cm.operation(function () {
    if (state.query) return;

    state.query = query;
    state.posFrom = state.posTo = { line: 0, ch: 0 };
    searchNext(ctx, rev);
  });
}




function searchNext(ctx, rev) {
  let { cm, ed } = ctx;
  cm.operation(function () {
    let state = getSearchState(cm)
    let cursor = getSearchCursor(cm, state.query, rev ? state.posFrom : state.posTo);

    if (!cursor.find(rev)) {
      cursor = getSearchCursor(cm, state.query, rev ?
        { line: cm.lastLine(), ch: null } : { line: cm.firstLine(), ch: 0 });
      if (!cursor.find(rev))
        return;
    }

    ed.alignLine(cursor.from().line, "center");
    cm.setSelection(cursor.from(), cursor.to());
    state.posFrom = cursor.from();
    state.posTo = cursor.to();
  });
}




function clearSearch(cm) {
  let state = getSearchState(cm);

  if (!state.query)
    return;

  state.query = null;
}







function initialize(ctx) {
  let { ed } = ctx;

  dbginfo.set(ed, {
    breakpoints:   {},
    debugLocation: null
  });
}





function hasBreakpoint(ctx, line) {
  let { cm } = ctx;
  
  
  if (cm.lineInfo(line) === null) {
    return null;
  }
  let markers = cm.lineInfo(line).gutterMarkers;

  return markers != null &&
    markers.breakpoints &&
    markers.breakpoints.classList.contains("breakpoint");
}








function addBreakpoint(ctx, line, cond) {
  function _addBreakpoint(ctx, line, cond) {
    let { ed, cm } = ctx;
    let meta = dbginfo.get(ed);
    let info = cm.lineInfo(line);

    ed.addMarker(line, "breakpoints", "breakpoint");
    meta.breakpoints[line] = { condition: cond };

    info.handle.on("delete", function onDelete() {
      info.handle.off("delete", onDelete);
      meta.breakpoints[info.line] = null;
    });

    ed.emit("breakpointAdded", line);
    deferred.resolve();
  }

  if (hasBreakpoint(ctx, line))
    return;

  let deferred = promise.defer();
  
  
  if (ctx.cm.lineInfo(line) === null) {
    DevToolsUtils.executeSoon(() => _addBreakpoint(ctx, line, cond));
  } else {
    _addBreakpoint(ctx, line, cond);
  }
  return deferred.promise;
}





function removeBreakpoint(ctx, line) {
  if (!hasBreakpoint(ctx, line))
    return;

  let { ed, cm } = ctx;
  let meta = dbginfo.get(ed);
  let info = cm.lineInfo(line);

  meta.breakpoints[info.line] = null;
  ed.removeMarker(info.line, "breakpoints", "breakpoint");
  ed.emit("breakpointRemoved", line);
}

function moveBreakpoint(ctx, fromLine, toLine) {
  let { ed, cm } = ctx;
  let info = cm.lineInfo(fromLine);

  var fromTop = cm.cursorCoords({ line: fromLine }).top;
  var toTop = cm.cursorCoords({ line: toLine }).top;

  var marker = ed.getMarker(info.line, "breakpoints", "breakpoint");
  if (marker) {
    marker.setAttribute("adding", "");
    marker.style.transform = "translateY(" + (toTop - fromTop) + "px)";
    marker.addEventListener('transitionend', function(e) {
      ed.removeBreakpoint(info.line);
      ed.addBreakpoint(toLine);

      
      
      e.target.removeAttribute("adding");
      e.target.style.transform = "none";
    });
  }
}




function getBreakpoints(ctx) {
  let { ed } = ctx;
  let meta = dbginfo.get(ed);

  return Object.keys(meta.breakpoints).reduce((acc, line) => {
    if (meta.breakpoints[line] != null)
      acc.push({ line: line, condition: meta.breakpoints[line].condition });
    return acc;
  }, []);
}






function setDebugLocation(ctx, line) {
  let { ed } = ctx;
  let meta = dbginfo.get(ed);

  clearDebugLocation(ctx);

  meta.debugLocation = line;
  ed.addMarker(line, "breakpoints", "debugLocation");
  ed.addLineClass(line, "debug-line");
}





function getDebugLocation(ctx) {
  let { ed } = ctx;
  let meta = dbginfo.get(ed);

  return meta.debugLocation;
}





function clearDebugLocation(ctx) {
  let { ed } = ctx;
  let meta = dbginfo.get(ed);

  if (meta.debugLocation != null) {
    ed.removeMarker(meta.debugLocation, "breakpoints", "debugLocation");
    ed.removeLineClass(meta.debugLocation, "debug-line");
    meta.debugLocation = null;
  }
}




function find(ctx, query) {
  clearSearch(ctx.cm);
  doSearch(ctx, false, query);
}




function findNext(ctx, query) {
  doSearch(ctx, false, query);
}




function findPrev(ctx, query) {
  doSearch(ctx, true, query);
}




[
  initialize, hasBreakpoint, addBreakpoint, removeBreakpoint,
  moveBreakpoint, getBreakpoints, setDebugLocation, getDebugLocation,
  clearDebugLocation, find, findNext, findPrev
].forEach(function (func) { module.exports[func.name] = func; });
