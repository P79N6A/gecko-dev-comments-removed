



"use strict";



const TEST_URI = "chrome://browser/content/devtools/cubic-bezier-frame.xhtml";
const {CubicBezierWidget} =
  devtools.require("devtools/shared/widgets/CubicBezierWidget");
const {PREDEFINED} = require("devtools/shared/widgets/CubicBezierPresets");

add_task(function*() {
  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", TEST_URI);

  
  
  
  doc.body.setAttribute("style", "position: fixed");

  let container = doc.querySelector("#container");
  let w = new CubicBezierWidget(container, PREDEFINED.linear);

  let rect = w.curve.getBoundingClientRect();
  rect.graphTop = rect.height * w.bezierCanvas.padding[0];
  rect.graphBottom = rect.height - rect.graphTop;
  rect.graphHeight = rect.graphBottom - rect.graphTop;

  yield pointsCanBeDragged(w, win, doc, rect);
  yield curveCanBeClicked(w, win, doc, rect);
  yield pointsCanBeMovedWithKeyboard(w, win, doc, rect);

  w.destroy();
  host.destroy();
  gBrowser.removeCurrentTab();
});

function* pointsCanBeDragged(widget, win, doc, offsets) {
  info("Checking that the control points can be dragged with the mouse");

  info("Listening for the update event");
  let onUpdated = widget.once("updated");

  info("Generating a mousedown/move/up on P1");
  widget._onPointMouseDown({target: widget.p1});
  doc.onmousemove({pageX: offsets.left, pageY: offsets.graphTop});
  doc.onmouseup();

  let bezier = yield onUpdated;
  ok(true, "The widget fired the updated event");
  ok(bezier, "The updated event contains a bezier argument");
  is(bezier.P1[0], 0, "The new P1 time coordinate is correct");
  is(bezier.P1[1], 1, "The new P1 progress coordinate is correct");

  info("Listening for the update event");
  onUpdated = widget.once("updated");

  info("Generating a mousedown/move/up on P2");
  widget._onPointMouseDown({target: widget.p2});
  doc.onmousemove({pageX: offsets.right, pageY: offsets.graphBottom});
  doc.onmouseup();

  bezier = yield onUpdated;
  is(bezier.P2[0], 1, "The new P2 time coordinate is correct");
  is(bezier.P2[1], 0, "The new P2 progress coordinate is correct");
}

function* curveCanBeClicked(widget, win, doc, offsets) {
  info("Checking that clicking on the curve moves the closest control point");

  info("Listening for the update event");
  let onUpdated = widget.once("updated");

  info("Click close to P1");
  let x = offsets.left + (offsets.width / 4.0);
  let y = offsets.graphTop + (offsets.graphHeight / 4.0);
  widget._onCurveClick({pageX: x, pageY: y});

  let bezier = yield onUpdated;
  ok(true, "The widget fired the updated event");
  is(bezier.P1[0], 0.25, "The new P1 time coordinate is correct");
  is(bezier.P1[1], 0.75, "The new P1 progress coordinate is correct");
  is(bezier.P2[0], 1, "P2 time coordinate remained unchanged");
  is(bezier.P2[1], 0, "P2 progress coordinate remained unchanged");

  info("Listening for the update event");
  onUpdated = widget.once("updated");

  info("Click close to P2");
  x = offsets.right - (offsets.width / 4);
  y = offsets.graphBottom - (offsets.graphHeight / 4);
  widget._onCurveClick({pageX: x, pageY: y});

  bezier = yield onUpdated;
  is(bezier.P2[0], 0.75, "The new P2 time coordinate is correct");
  is(bezier.P2[1], 0.25, "The new P2 progress coordinate is correct");
  is(bezier.P1[0], 0.25, "P1 time coordinate remained unchanged");
  is(bezier.P1[1], 0.75, "P1 progress coordinate remained unchanged");
}

function* pointsCanBeMovedWithKeyboard(widget, win, doc, offsets) {
  info("Checking that points respond to keyboard events");

  let singleStep = 3;
  let shiftStep = 30;

  info("Moving P1 to the left");
  let newOffset = parseInt(widget.p1.style.left) - singleStep;
  let x = widget.bezierCanvas.
          offsetsToCoordinates({style: {left: newOffset}})[0];

  let onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 37));
  let bezier = yield onUpdated;

  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], 0.75, "The new P1 progress coordinate is correct");

  info("Moving P1 to the left, fast");
  newOffset = parseInt(widget.p1.style.left) - shiftStep;
  x = widget.bezierCanvas.
      offsetsToCoordinates({style: {left: newOffset}})[0];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 37, true));
  bezier = yield onUpdated;
  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], 0.75, "The new P1 progress coordinate is correct");

  info("Moving P1 to the right, fast");
  newOffset = parseInt(widget.p1.style.left) + shiftStep;
  x = widget.bezierCanvas.
    offsetsToCoordinates({style: {left: newOffset}})[0];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 39, true));
  bezier = yield onUpdated;
  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], 0.75, "The new P1 progress coordinate is correct");

  info("Moving P1 to the bottom");
  newOffset = parseInt(widget.p1.style.top) + singleStep;
  let y = widget.bezierCanvas.
    offsetsToCoordinates({style: {top: newOffset}})[1];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 40));
  bezier = yield onUpdated;
  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], y, "The new P1 progress coordinate is correct");

  info("Moving P1 to the bottom, fast");
  newOffset = parseInt(widget.p1.style.top) + shiftStep;
  y = widget.bezierCanvas.
    offsetsToCoordinates({style: {top: newOffset}})[1];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 40, true));
  bezier = yield onUpdated;
  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], y, "The new P1 progress coordinate is correct");

  info("Moving P1 to the top, fast");
  newOffset = parseInt(widget.p1.style.top) - shiftStep;
  y = widget.bezierCanvas.
    offsetsToCoordinates({style: {top: newOffset}})[1];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p1, 38, true));
  bezier = yield onUpdated;
  is(bezier.P1[0], x, "The new P1 time coordinate is correct");
  is(bezier.P1[1], y, "The new P1 progress coordinate is correct");

  info("Checking that keyboard events also work with P2");
  info("Moving P2 to the left");
  newOffset = parseInt(widget.p2.style.left) - singleStep;
  x = widget.bezierCanvas.
    offsetsToCoordinates({style: {left: newOffset}})[0];

  onUpdated = widget.once("updated");
  widget._onPointKeyDown(getKeyEvent(widget.p2, 37));
  bezier = yield onUpdated;
  is(bezier.P2[0], x, "The new P2 time coordinate is correct");
  is(bezier.P2[1], 0.25, "The new P2 progress coordinate is correct");
}

function getKeyEvent(target, keyCode, shift=false) {
  return {
    target: target,
    keyCode: keyCode,
    shiftKey: shift,
    preventDefault: () => {}
  };
}
