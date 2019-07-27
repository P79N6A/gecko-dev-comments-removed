



"use strict";




const {findOptimalTimeInterval} = require("devtools/animationinspector/utils");
const {TimeScale} = require("devtools/animationinspector/components");

const TIME_GRADUATION_MIN_SPACING = 40;

add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {panel} = yield openAnimationInspectorNewUI();

  let timeline = panel.animationsTimelineComponent;
  let headerEl = timeline.timeHeaderEl;

  info("Find out how many time graduations should there be");
  let width = headerEl.offsetWidth;
  let scale = width / (TimeScale.maxEndTime - TimeScale.minStartTime);
  
  
  let interval = findOptimalTimeInterval(scale, TIME_GRADUATION_MIN_SPACING);
  let nb = Math.ceil(width / interval);

  is(headerEl.querySelectorAll(".time-tick").length, nb,
     "The expected number of time ticks were found");

  info("Make sure graduations are evenly distributed and show the right times");
  [...headerEl.querySelectorAll(".time-tick")].forEach((tick, i) => {
    let left = parseFloat(tick.style.left);
    is(Math.round(left), Math.round(i * interval),
      "Graduation " + i + " is positioned correctly");

    
    
    
    let formattedTime = TimeScale.formatTime(
      TimeScale.distanceToRelativeTime(i * interval, width));
    is(tick.textContent, formattedTime,
      "Graduation " + i + " has the right text content");
  });
});
