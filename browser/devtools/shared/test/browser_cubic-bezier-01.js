



"use strict";



const TEST_URI = "chrome://browser/content/devtools/cubic-bezier-frame.xhtml";
const {CubicBezierWidget} =
  devtools.require("devtools/shared/widgets/CubicBezierWidget");

add_task(function*() {
  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", TEST_URI);

  info("Checking that the graph markup is created in the parent");
  let container = doc.querySelector("#container");
  let w = new CubicBezierWidget(container);

  ok(container.querySelector(".display-wrap"),
    "The display has been added");

  ok(container.querySelector(".coordinate-plane"),
    "The coordinate plane has been added");
  let buttons = container.querySelectorAll("button");
  is(buttons.length, 2,
    "The 2 control points have been added");
  is(buttons[0].className, "control-point");
  is(buttons[0].id, "P1");
  is(buttons[1].className, "control-point");
  is(buttons[1].id, "P2");
  ok(container.querySelector("canvas"), "The curve canvas has been added");

  info("Destroying the widget");
  w.destroy();
  is(container.children.length, 0, "All nodes have been removed");

  host.destroy();
  gBrowser.removeCurrentTab();
});
