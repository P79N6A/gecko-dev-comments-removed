



 "use strict";

const dbginfo = new WeakMap();







function addMarker(cm, line, type) {
  let info = cm.lineInfo(line);

  if (info.gutterMarkers)
    return void info.gutterMarkers.breakpoints.classList.add(type);

  let mark = cm.getWrapperElement().ownerDocument.createElement("div");
  mark.className = type;
  mark.innerHTML = "";

  cm.setGutterMarker(info.line, "breakpoints", mark);
}





function removeMarker(cm, line, type) {
  let info = cm.lineInfo(line);

  if (!info || !info.gutterMarkers)
    return;

  info.gutterMarkers.breakpoints.classList.remove(type);
}






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






function doSearch(cm, rev, query) {
  let state = getSearchState(cm);

  if (state.query)
    return searchNext(cm, rev);

  cm.operation(function () {
    if (state.query) return;

    state.query = query;
    state.posFrom = state.posTo = { line: 0, ch: 0 };
    searchNext(cm, rev);
  });
}




function searchNext(cm, rev) {
  cm.operation(function () {
    let state = getSearchState(cm)
    let cursor = getSearchCursor(cm, state.query, rev ? state.posFrom : state.posTo);

    if (!cursor.find(rev)) {
      cursor = getSearchCursor(cm, state.query, rev ?
        { line: cm.lastLine(), ch: null } : { line: cm.firstLine(), ch: 0 });
      if (!cursor.find(rev))
        return;
    }

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
  let markers = cm.lineInfo(line).gutterMarkers;

  return markers != null &&
    markers.breakpoints.classList.contains("breakpoint");
}








function addBreakpoint(ctx, line, cond) {
  if (hasBreakpoint(ctx, line))
    return;

  let { ed, cm } = ctx;
  let meta = dbginfo.get(ed);
  let info = cm.lineInfo(line);

  addMarker(cm, line, "breakpoint");
  meta.breakpoints[line] = { condition: cond };

  info.handle.on("delete", function onDelete() {
    info.handle.off("delete", onDelete);
    meta.breakpoints[info.line] = null;
  });

  ed.emit("breakpointAdded", line);
}





function removeBreakpoint(ctx, line) {
  if (!hasBreakpoint(ctx, line))
    return;

  let { ed, cm } = ctx;
  let meta = dbginfo.get(ed);
  let info = cm.lineInfo(line);

  meta.breakpoints[info.line] = null;
  removeMarker(cm, info.line, "breakpoint");
  ed.emit("breakpointRemoved", line);
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
  let { ed, cm } = ctx;
  let meta = dbginfo.get(ed);

  meta.debugLocation = line;
  addMarker(cm, line, "debugLocation");
}





function getDebugLocation(ctx) {
  let { ed } = ctx;
  let meta = dbginfo.get(ed);

  return meta.debugLocation;
}





function clearDebugLocation(ctx) {
  let { ed, cm } = ctx;
  let meta = dbginfo.get(ed);

  if (meta.debugLocation != null) {
    removeMarker(cm, meta.debugLocation, "debugLocation");
    meta.debugLocation = null;
  }
}




function find(ctx, query) {
  let { cm } = ctx;
  clearSearch(cm);
  doSearch(cm, false, query);
}




function findNext(ctx, query) {
  let { cm } = ctx;
  doSearch(cm, false, query);
}




function findPrev(ctx, query) {
  let { cm } = ctx;
  doSearch(cm, true, query);
}




[
  initialize, hasBreakpoint, addBreakpoint, removeBreakpoint,
  getBreakpoints, setDebugLocation, getDebugLocation,
  clearDebugLocation, find, findNext, findPrev
].forEach(function (func) { module.exports[func.name] = func; });