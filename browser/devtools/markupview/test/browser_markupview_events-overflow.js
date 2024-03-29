


"use strict";

const TEST_URL = TEST_URL_ROOT + "doc_markup_events-overflow.html";
const TEST_DATA = [
  {
    desc: "editor overflows container",
    initialScrollTop: -1, 
    headerToClick: 49, 
    alignBottom: true,
    alignTop: false,
  },
  {
    desc: "header overflows the container",
    initialScrollTop: 2,
    headerToClick: 0,
    alignBottom: false,
    alignTop: true,
  },
  {
    desc: "neither header nor editor overflows the container",
    initialScrollTop: 2,
    headerToClick: 5,
    alignBottom: false,
    alignTop: false,
  },
];

add_task(function*() {
  let { inspector } = yield addTab(TEST_URL).then(openInspector);

  let markupContainer = yield getContainerForSelector("#events", inspector);
  let evHolder = markupContainer.elt.querySelector(".markupview-events");
  let tooltip = inspector.markup.tooltip;

  info("Clicking to open event tooltip.");
  EventUtils.synthesizeMouseAtCenter(evHolder, {}, inspector.markup.doc.defaultView);
  yield tooltip.once("shown");
  info("EventTooltip visible.");

  let container = tooltip.content;
  let containerRect = container.getBoundingClientRect();
  let headers = container.querySelectorAll(".event-header");

  for (let data of TEST_DATA) {
    info("Testing scrolling when " + data.desc);

    if (data.initialScrollTop < 0) {
      info("Scrolling container to the bottom.");
      let newScrollTop = container.scrollHeight - container.clientHeight;
      data.initialScrollTop = container.scrollTop = newScrollTop;
    } else {
      info("Scrolling container by " + data.initialScrollTop + "px");
      container.scrollTop = data.initialScrollTop;
    }

    is(container.scrollTop, data.initialScrollTop, "Container scrolled.");

    info("Clicking on header #" + data.headerToClick);
    let header = headers[data.headerToClick];

    let ready = tooltip.once("event-tooltip-ready");
    EventUtils.synthesizeMouseAtCenter(header, {}, header.ownerGlobal);
    yield ready;

    info("Event handler expanded.");

    
    yield promiseNextTick();

    if (data.alignTop) {
      let headerRect = header.getBoundingClientRect();

      is(Math.round(headerRect.top), Math.round(containerRect.top),
        "Clicked header is aligned with the container top.");

    } else if (data.alignBottom) {
      let editorRect = header.nextElementSibling.getBoundingClientRect();

      is(Math.round(editorRect.bottom), Math.round(containerRect.bottom),
        "Clicked event handler code is aligned with the container bottom.");

    } else {
      is(container.scrollTop, data.initialScrollTop,
        "Container did not scroll, as expected.");
    }
  }
});
