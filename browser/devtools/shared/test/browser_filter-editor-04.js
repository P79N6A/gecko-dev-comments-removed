


"use strict";



const TEST_URI = "chrome://browser/content/devtools/filter-frame.xhtml";
const {CSSFilterEditorWidget} = require("devtools/shared/widgets/FilterWidget");
const LIST_ITEM_HEIGHT = 32;

add_task(function*() {
  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", TEST_URI);

  const container = doc.querySelector("#container");
  const initialValue = "blur(2px) contrast(200%) brightness(200%)";
  let widget = new CSSFilterEditorWidget(container, initialValue);

  const filters = widget.el.querySelector(".filters");
  function first() {
    return filters.children[0];
  }
  function mid() {
    return filters.children[1];
  }
  function last() {
    return filters.children[2];
  }

  info("Test re-ordering neighbour filters");
  widget._mouseDown({
    target: first().querySelector("i"),
    pageY: 0
  });
  widget._mouseMove({ pageY: LIST_ITEM_HEIGHT });

  
  is(mid().querySelector("label").textContent, "blur",
     "Should reorder elements correctly");

  widget._mouseUp();

  is(widget.getCssValue(), "contrast(200%) blur(2px) brightness(200%)",
     "Should reorder filters objects correctly");

  info("Test re-ordering first and last filters");
  widget._mouseDown({
    target: first().querySelector("i"),
    pageY: 0
  });
  widget._mouseMove({ pageY: LIST_ITEM_HEIGHT * 2 });

  
  is(last().querySelector("label").textContent, "contrast",
     "Should reorder elements correctly");
  widget._mouseUp();

  is(widget.getCssValue(), "brightness(200%) blur(2px) contrast(200%)",
     "Should reorder filters objects correctly");

  info("Test dragging first element out of list");
  const boundaries = filters.getBoundingClientRect();

  widget._mouseDown({
    target: first().querySelector("i"),
    pageY: 0
  });
  widget._mouseMove({ pageY: -LIST_ITEM_HEIGHT * 5 });
  ok(first().offsetTop >= boundaries.top,
     "First filter should not move outside filter list");

  widget._mouseUp();

  info("Test dragging last element out of list");
  widget._mouseDown({
    target: last().querySelector("i"),
    pageY: 0
  });
  widget._mouseMove({ pageY: -LIST_ITEM_HEIGHT * 5 });
  ok(last().offsetTop <= boundaries.bottom,
     "Last filter should not move outside filter list");

  widget._mouseUp();
});
