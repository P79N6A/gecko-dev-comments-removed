







function testPingPong(test, bot) {
  test.assert(typeof bot.ping === 'function', 'Bot does not exposes ping.');

  bot.ping(gotAnswer);

  function gotAnswer(answer) {
    test.log('bot > ' + answer);
    test.done();
  }
}

registerBotTest('testPingPong/chrome', testPingPong, ['chrome']);
