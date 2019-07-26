


MARIONETTE_TIMEOUT = 20000;



const GSM_SMS_STRICT_7BIT_CHARMAP = {
  "\u0024": "\u0024", 
  "\u00a5": "\u00a5", 
  "\u00c0": "\u0041", 
  "\u00c1": "\u0041", 
  "\u00c2": "\u0041", 
  "\u00c4": "\u00c4", 
  "\u00c5": "\u00c5", 
  "\u00c6": "\u00c6", 
  "\u00c7": "\u00c7", 
  "\u00c8": "\u0045", 
  "\u00c9": "\u00c9", 
  "\u00ca": "\u0045", 
  "\u00cb": "\u0045", 
  "\u00cc": "\u0049", 
  "\u00cd": "\u0049", 
  "\u00ce": "\u0049", 
  "\u00cf": "\u0049", 
  "\u00d1": "\u00d1", 
  "\u00d2": "\u004f", 
  "\u00d3": "\u004f", 
  "\u00d4": "\u004f", 
  "\u00d6": "\u00d6", 
  "\u00d9": "\u0055", 
  "\u00da": "\u0055", 
  "\u00db": "\u0055", 
  "\u00dc": "\u00dc", 
  "\u00df": "\u00df", 
  "\u00e0": "\u00e0", 
  "\u00e1": "\u0061", 
  "\u00e2": "\u0061", 
  "\u00e4": "\u00e4", 
  "\u00e5": "\u00e5", 
  "\u00e6": "\u00e6", 
  "\u00e7": "\u00c7", 
  "\u00e8": "\u00e8", 
  "\u00e9": "\u00e9", 
  "\u00ea": "\u0065", 
  "\u00eb": "\u0065", 
  "\u00ec": "\u00ec", 
  "\u00ed": "\u0069", 
  "\u00ee": "\u0069", 
  "\u00ef": "\u0069", 
  "\u00f1": "\u00f1", 
  "\u00f2": "\u00f2", 
  "\u00f3": "\u006f", 
  "\u00f4": "\u006f", 
  "\u00f6": "\u00f6", 
  "\u00f8": "\u00f8", 
  "\u00f9": "\u00f9", 
  "\u00fa": "\u0075", 
  "\u00fb": "\u0075", 
  "\u00fc": "\u00fc", 
  "\u00fe": "\u0074", 
  "\u0100": "\u0041", 
  "\u0101": "\u0061", 
  "\u0106": "\u0043", 
  "\u0107": "\u0063", 
  "\u010c": "\u0043", 
  "\u010d": "\u0063", 
  "\u010f": "\u0064", 
  "\u0110": "\u0044", 
  "\u0111": "\u0064", 
  "\u0112": "\u0045", 
  "\u0113": "\u0065", 
  "\u0118": "\u0045", 
  "\u0119": "\u0065", 
  "\u012a": "\u0049", 
  "\u012b": "\u0069", 
  "\u012e": "\u0049", 
  "\u012f": "\u0069", 
  "\u0141": "\u004c", 
  "\u0142": "\u006c", 
  "\u0143": "\u004e", 
  "\u0144": "\u006e", 
  "\u0147": "\u004e", 
  "\u0148": "\u006e", 
  "\u014c": "\u004f", 
  "\u014d": "\u006f", 
  "\u0152": "\u004f", 
  "\u0153": "\u006f", 
  "\u0158": "\u0052", 
  "\u0159": "\u0072", 
  "\u0160": "\u0053", 
  "\u0161": "\u0073", 
  "\u0165": "\u0074", 
  "\u016a": "\u0055", 
  "\u016b": "\u0075", 
  "\u0178": "\u0059", 
  "\u0179": "\u005a", 
  "\u017a": "\u007a", 
  "\u017b": "\u005a", 
  "\u017c": "\u007a", 
  "\u017d": "\u005a", 
  "\u017e": "\u007a", 
  "\u025b": "\u0045", 
  "\u0398": "\u0398", 
  "\u20a4": "\u00a3", 
  "\u20ac": "\u20ac", 
};



const SELF = "5554";

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

let sms = window.navigator.mozSms;
ok(sms instanceof MozSmsManager);

function repeat(func, array, oncomplete) {
  (function do_call(index) {
    let next = index < (array.length - 1) ? do_call.bind(null, index + 1) : oncomplete;
    array[index].push(next);
    func.apply(null, array[index]);
  })(0);
}

function testStrict7BitEncodingHelper(sent, received, next) {
  
  
  
  
  

  let count = 0;
  function done(step) {
    count += step;
    if (count >= 2) {
      window.setTimeout(next, 0);
    }
  }

  sms.addEventListener("received", function onReceived(event) {
    event.target.removeEventListener("received", onReceived);

    let message = event.message;
    is(message.body, received, "received message.body");

    done(1);
  });

  let request = sms.send(SELF, sent);
  request.addEventListener("success", function onRequestSuccess(event) {
    let message = event.target.result;
    is(message.body, sent, "sent message.body");

    done(1);
  });
  request.addEventListener("error", function onRequestError(event) {
    ok(false, "Can't send message out!!!");
    done(2);
  });
}

function test_enabled() {
  log("Testing with dom.sms.strict7BitEncoding enabled");

  SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding", true);

  let cases = [];

  
  let sent = "", received = "";
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    sent += c;
    received += GSM_SMS_STRICT_7BIT_CHARMAP[c];
  }
  cases.push([sent, received]);

  
  
  cases.push(["\u65b0\u5e74\u5feb\u6a02", "****"]); 

  repeat(testStrict7BitEncodingHelper, cases, test_disabled);
}

function test_disabled() {
  log("Testing with dom.sms.strict7BitEncoding disabled");

  SpecialPowers.setBoolPref("dom.sms.strict7BitEncoding", false);

  let cases = [];

  
  let sent = "";
  for (let c in GSM_SMS_STRICT_7BIT_CHARMAP) {
    sent += c;
  }
  cases.push([sent, sent]);

  cases.push(["\u65b0\u5e74\u5feb\u6a02", "\u65b0\u5e74\u5feb\u6a02"]);

  repeat(testStrict7BitEncodingHelper, cases, cleanUp);
}

function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  SpecialPowers.clearUserPref("dom.sms.strict7BitEncoding");

  finish();
}

test_enabled();
