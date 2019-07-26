


MARIONETTE_TIMEOUT = 10000;

const SELF = "5554";

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}

let sms = window.navigator.mozSms;
ok(sms instanceof MozSmsManager);

function randomString16() {
  return Math.random().toString(36).substr(2, 16);
}

function times(str, n) {
  return (new Array(n + 1)).join(str);
}

function repeat(func, array, oncomplete) {
  (function do_call(index) {
    let next = index < (array.length - 1) ? do_call.bind(null, index + 1) : oncomplete;
    func.apply(null, [array[index], next]);
  })(0);
}

function doTest(body, callback) {
  sms.addEventListener("received", function onReceived(event) {
    event.target.removeEventListener(event.type, arguments.callee);

    let message = event.message;
    is(message.body, body, "message.body");

    window.setTimeout(callback, 0);
  });

  let request = sms.send(SELF, body);
  request.onerror = function onerror() {
    ok(false, "failed to send message '" + body + "' to '" + SELF + "'");
  };
}

repeat(doTest, [
  
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
], cleanUp);
