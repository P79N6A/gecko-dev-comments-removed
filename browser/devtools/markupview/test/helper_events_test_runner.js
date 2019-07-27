







function* runEventPopupTests() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  yield inspector.markup.expandAll();

  for (let {selector, expected} of TEST_DATA) {
    yield checkEventsForNode(selector, expected, inspector);
  }

  gBrowser.removeCurrentTab();

  
  
  
  yield promiseNextTick();
}








function* checkEventsForNode(selector, expected, inspector) {
  let container = yield getContainerForSelector(selector, inspector);
  let evHolder = container.elt.querySelector(".markupview-events");
  let tooltip = inspector.markup.tooltip;

  yield selectNode(selector, inspector);

  
  info("Clicking evHolder");
  EventUtils.synthesizeMouseAtCenter(evHolder, {}, inspector.markup.doc.defaultView);
  yield tooltip.once("shown");
  info("tooltip shown");

  
  let content = tooltip.content;
  let headers = content.querySelectorAll(".event-header");
  let nodeFront = container.node;
  let cssSelector = nodeFront.nodeName + "#" + nodeFront.id;

  for (let i = 0; i < headers.length; i++) {
    info("Processing header[" + i + "] for " + cssSelector);

    let header = headers[i];
    let type = header.querySelector(".event-tooltip-event-type");
    let filename = header.querySelector(".event-tooltip-filename");
    let attributes = header.querySelectorAll(".event-tooltip-attributes");
    let contentBox = header.nextElementSibling;

    is(type.getAttribute("value"), expected[i].type,
       "type matches for " + cssSelector);
    is(filename.getAttribute("value"), expected[i].filename,
       "filename matches for " + cssSelector);

    is(attributes.length, expected[i].attributes.length,
       "we have the correct number of attributes");

    for (let j = 0; j < expected[i].attributes.length; j++) {
      is(attributes[j].getAttribute("value"), expected[i].attributes[j],
         "attribute[" + j + "] matches for " + cssSelector);
    }

    EventUtils.synthesizeMouseAtCenter(header, {}, type.ownerGlobal);
    yield tooltip.once("event-tooltip-ready");

    let editor = tooltip.eventEditors.get(contentBox).editor;
    is(editor.getText(), expected[i].handler,
       "handler matches for " + cssSelector);
  }

  tooltip.hide();
}
