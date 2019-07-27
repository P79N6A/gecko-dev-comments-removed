



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_dragdrop_autoscroll.html";
const GRAB_DELAY = 400;

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let markup = inspector.markup;

  let container = yield getContainerForSelector("#first", inspector);
  let rect = container.elt.getBoundingClientRect();

  info("Simulating mouseDown on #first");
  container._onMouseDown({
    target: container.tagLine,
    pageX: 10,
    pageY: rect.top,
    stopPropagation: function() {},
    preventDefault: function() {}
  });

  yield wait(GRAB_DELAY + 1);

  let clientHeight = markup.doc.documentElement.clientHeight;
  info("Simulating mouseMove on #first with pageY: " + clientHeight);

  let ev = {
    target: container.tagLine,
    pageX: 10,
    pageY: clientHeight,
    preventDefault: function() {}
  };

  info("Listening on scroll event");
  let scroll = onScroll(markup.win);

  markup._onMouseMove(ev);

  yield scroll;

  container._onMouseUp(ev);
  markup._onMouseUp(ev);

  ok("Scroll event fired");
});

function onScroll(win) {
  return new Promise((resolve, reject) => {
    win.onscroll = function(e) {
      resolve(e);
    }
  });
};