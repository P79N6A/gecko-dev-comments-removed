


"use strict";




const { NodeListActor } = devtools.require("devtools/server/actors/inspector");

function run_test() {
  check_actor_for_list(null);
  check_actor_for_list([]);
  check_actor_for_list(["fakenode"]);
}

function check_actor_for_list(nodelist) {
  do_print("Checking NodeListActor with nodelist '" + nodelist + "' works.");
  let actor = new NodeListActor({}, nodelist);
  let form = actor.form();

  
  ok(true, "No exceptions occured.");
  equal(form.length, nodelist ? nodelist.length : 0,
    "NodeListActor reported correct length.");
}
