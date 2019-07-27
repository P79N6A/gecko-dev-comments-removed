



"use strict";





add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");

  let ui = yield openAnimationInspector();
  yield testEventsOrder(ui);

  ui = yield closeAnimationInspectorAndRestartWithNewUI();
  yield testEventsOrder(ui);
});

function* testEventsOrder({inspector, panel, controller}) {
  info("Listen for the players-updated, ui-updated and inspector-updated events");
  let receivedEvents = [];
  controller.once(controller.PLAYERS_UPDATED_EVENT, () => {
    receivedEvents.push(controller.PLAYERS_UPDATED_EVENT);
  });
  panel.once(panel.UI_UPDATED_EVENT, () => {
    receivedEvents.push(panel.UI_UPDATED_EVENT);
  });
  inspector.once("inspector-updated", () => {
    receivedEvents.push("inspector-updated");
  });

  info("Selecting an animated node");
  let node = yield getNodeFront(".animated", inspector);
  yield selectNode(node, inspector);

  info("Check that all events were received, and in the right order");
  is(receivedEvents.length, 3, "3 events were received");
  is(receivedEvents[0], controller.PLAYERS_UPDATED_EVENT,
    "The first event received was the players-updated event");
  is(receivedEvents[1], panel.UI_UPDATED_EVENT,
    "The second event received was the ui-updated event");
  is(receivedEvents[2], "inspector-updated",
    "The third event received was the inspector-updated event");
}
