






function spawnTest () {
  let [target, debuggee, front] = yield initBackend(SIMPLE_CONTEXT_URL);
  let [_, [destNode, oscNode, gainNode]] = yield Promise.all([
    front.setup({ reload: true }),
    get3(front, "create-node")
  ]);

  is((yield gainNode.isBypassed()), false, "Nodes start off unbypassed.");

  info("Calling node#bypass(true)");
  yield gainNode.bypass(true);

  is((yield gainNode.isBypassed()), true, "Node is now bypassed.");

  info("Calling node#bypass(false)");
  yield gainNode.bypass(false);

  is((yield gainNode.isBypassed()), false, "Node back to being unbypassed.");

  yield removeTab(target.tab);
  finish();
}
