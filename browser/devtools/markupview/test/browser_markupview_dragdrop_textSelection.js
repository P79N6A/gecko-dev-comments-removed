



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_dragdrop.html";
const GRAB_DELAY = 400;

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);
  let markup = inspector.markup;

  info("Expanding span#before");
  let spanFront = yield getNodeFront("#before", inspector);
  let spanContainer = yield getContainerForNodeFront(spanFront, inspector);
  let span = yield getNode("#before");

  yield inspector.markup.expandNode(spanFront);
  yield waitForMultipleChildrenUpdates(inspector);

  spanContainer.elt.scrollIntoView(true);

  info("Selecting #before's text content");

  let textContent = spanContainer.elt.children[1].firstChild.container;

  let selectRange = markup.doc.createRange();
  selectRange.selectNode(textContent.editor.elt.querySelector('[tabindex]'));
  markup.doc.getSelection().addRange(selectRange);

  info("Simulating mouseDown on #before");

  spanContainer._onMouseDown({
    pageX: 0,
    pageY: 0,
    target: spanContainer.tagLine,
    stopPropagation: function() {},
    preventDefault: function() {}
  });

  yield wait(GRAB_DELAY + 1);

  is(spanContainer.isDragging, false, "isDragging should be false if there is a text selected");
});