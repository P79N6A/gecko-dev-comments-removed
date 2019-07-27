



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select the simple animated node");
  yield selectNode(".animated", inspector);

  let titleEl = panel.playerWidgets[0].el.querySelector(".animation-title");
  ok(titleEl,
    "The player widget has a title element, where meta-data should be displayed");

  let nameEl = titleEl.querySelector("strong");
  ok(nameEl, "The first <strong> tag was retrieved, it should contain the name");
  is(nameEl.textContent, "simple-animation", "The animation name is correct");

  let metaDataEl = titleEl.querySelector(".meta-data");
  ok(metaDataEl, "The meta-data element exists");

  let metaDataEls = metaDataEl.querySelectorAll("strong");
  is(metaDataEls.length, 2, "2 meta-data elements were found");
  is(metaDataEls[0].textContent, "2.00s",
    "The first meta-data is the duration, and is correct");

  info("Select the node with the delayed animation");
  yield selectNode(".delayed", inspector);

  titleEl = panel.playerWidgets[0].el.querySelector(".animation-title");
  nameEl = titleEl.querySelector("strong");
  is(nameEl.textContent, "simple-animation", "The animation name is correct");

  metaDataEls = titleEl.querySelectorAll(".meta-data strong");
  is(metaDataEls.length, 3,
    "3 meta-data elements were found for the delayed animation");
  is(metaDataEls[0].textContent, "3.00s",
    "The first meta-data is the duration, and is correct");
  is(metaDataEls[1].textContent, "60.00s",
    "The second meta-data is the delay, and is correct");
  is(metaDataEls[2].textContent, "10",
    "The third meta-data is the iteration count, and is correct");
});
