








const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/Geometry.jsm');






function getSelectionHandler() {
  return (!this._selectionHandler) ?
    this._selectionHandler = Services.wm.getMostRecentWindow("navigator:browser").SelectionHandler :
    this._selectionHandler;
}

function todo(result, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    todo: result,
    msg: msg
  });
}

function ok(result, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: result,
    msg: msg
  });
}

function is(one, two, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: one === two,
    msg: msg + " : " + one + " === " + two
  });
}

function isNot(one, two, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: one !== two,
    msg: msg + " : " + one + " !== " + two
  });
}

function lessThan(n1, n2, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: n1 < n2,
    msg: msg + " : " + n1 + " < " + n2
  });
}

function greaterThan(n1, n2, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: n1 > n2,
    msg: msg + " : " + n1 + " > " + n2
  });
}


function truncPoint(point) {
  return new Point(Math.trunc(point.x), Math.trunc(point.y));
}

function pointEquals(p1, p2, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: truncPoint(p1).equals(truncPoint(p2)),
    msg: msg + " : " + p1.toString() + " == " + p2.toString()
  });
}

function pointNotEquals(p1, p2, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: !truncPoint(p1).equals(truncPoint(p2)),
    msg: msg + " : " + p1.toString() + " == " + p2.toString()
  });
}

function selectionExists(selection, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: !truncPoint(selection.anchorPt).equals(truncPoint(selection.focusPt)),
    msg: msg + " : anchor:" + selection.anchorPt.toString() +
      " focus:" + selection.focusPt.toString()
  });
}

function selectionEquals(s1, s2, msg) {
  return Messaging.sendRequestForResult({
    type: TYPE_NAME,
    result: truncPoint(s1.anchorPt).equals(truncPoint(s2.anchorPt)) &&
      truncPoint(s1.focusPt).equals(truncPoint(s2.focusPt)),
    msg: msg
  });
}






function finishTests() {
  Messaging.sendRequest({
    type: TYPE_NAME,
    result: true,
    msg: "Done!",
    done: true
  });
}

