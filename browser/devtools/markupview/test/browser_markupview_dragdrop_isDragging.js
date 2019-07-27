



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_dragdrop.html";
const GRAB_DELAY = 400;

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let el = yield getContainerForSelector("#test", inspector);
  let rect = el.tagLine.getBoundingClientRect();

  info("Simulating mouseDown on #test");
  el._onMouseDown({
    target: el.tagLine,
    pageX: rect.x,
    pageY: rect.y,
    stopPropagation: function() {},
    preventDefault: function() {}
  });

  is(el.isDragging, false, "isDragging should not be set to true immedietly");

  info("Waiting " + (GRAB_DELAY + 1) + "ms");
  yield wait(GRAB_DELAY + 1);
  ok(el.isDragging, "isDragging true after GRAB_DELAY has passed");

  info("Simulating mouseUp on #test");
  el._onMouseUp({
    target: el.tagLine,
    pageX: rect.x,
    pageY: rect.y
  });

  is(el.isDragging, false, "isDragging false after mouseUp");
});
