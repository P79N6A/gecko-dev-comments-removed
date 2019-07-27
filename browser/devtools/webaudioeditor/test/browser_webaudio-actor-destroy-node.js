






function spawnTest () {
  let { target, front } = yield initBackend(DESTROY_NODES_URL);

  let waitUntilDestroyed = getN(front, "destroy-node", 10);
  let [, , created] = yield Promise.all([
    front.setup({ reload: true }),
    once(front, "start-context"),
    
    getN(front, "create-node", 13)
  ]);

  
  forceCC();

  let destroyed = yield waitUntilDestroyed;

  let destroyedTypes = yield Promise.all(destroyed.map(actor => actor.getType()));
  destroyedTypes.forEach((type, i) => {
    ok(type, "AudioBufferSourceNode", "Only buffer nodes are destroyed");
    ok(actorIsInList(created, destroyed[i]),
      "`destroy-node` called only on AudioNodes in current document.");
  });

  yield removeTab(target.tab);
  finish();
}

function actorIsInList (list, actor) {
  for (let i = 0; i < list.length; i++) {
    if (list[i].actorID === actor.actorID)
      return list[i];
  }
  return null;
}
