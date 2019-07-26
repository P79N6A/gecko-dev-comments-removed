


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const SELF = "5554";

function randomString16() {
  return Math.random().toString(36).substr(2, 16);
}

function times(str, n) {
  return (new Array(n + 1)).join(str);
}

function test(aBody) {
  let promises = [];

  promises.push(waitForManagerEvent('received')
    .then(function(aEvent) {
      let message = aEvent.message;
      is(message.body, aBody, "message.body");
    }));

  promises.push(sendSmsWithSuccess(SELF, aBody));

  return Promise.all(promises);
}

const TEST_DATA = [
  
  randomString16(),
  
  times(randomString16(), 100),

  
  "\u95dc\u95dc\u96ce\u9ce9\uff0c\u5728\u6cb3\u4e4b\u6d32\u3002",
  
  times("\u95dc\u95dc\u96ce\u9ce9\uff0c\u5728\u6cb3\u4e4b\u6d32\u3002", 100),

  
  "zertuuuppzuzyeueieieyeieoeiejeheejrueufjfjfjjfjfkxifjfjfufjjfjfufujdjduxxjdu"
  + "djdjdjdudhdjdhdjdbddhbfjfjxbuwjdjdudjddjdhdhdvdyudusjdudhdjjfjdvdudbddjdbd"
  + "usjfbjdfudjdhdjbzuuzyzehdjjdjwybwudjvwywuxjdbfudsbwuwbwjdjdbwywhdbddudbdjd"
  + "uejdhdudbdduwjdbjddudjdjdjdudjdbdjdhdhdjjdjbxudjdbxufjudbdjhdjdisjsjzusbzh"
  + "xbdudksksuqjgdjdb jeudi jeudis duhebevzcevevsvs DVD suscite eh du d des jv"
  + " y b Dj. Du  wh. Hu Deb wh. Du web h w show d DVD h w v.  Th\u00e9 \u00e9c"
  + "hec d whdvdj. Wh d'h\u00f4tel DVD. IMAX eusjw ii ce",

  
  
  "\u0147",
  
  
  "\u20ac****",
];

startTestBase(function testCaseMain() {
  return ensureMobileMessage()
    .then(function() {
      let promise = Promise.resolve();

      for (let i = 0; i < TEST_DATA.length; i++) {
        let text = TEST_DATA[i];
        promise = promise.then(() => test(text));
      }

      return promise;
    });
});
