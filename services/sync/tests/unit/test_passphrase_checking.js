Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/crypto.js");

let __fakePrefs = {
  "log.logger.async" : "Debug",
  "username" : "foo",
  "serverURL" : "https://example.com/",
  "encryption" : "aes-256-cbc",
  "enabled" : true,
  "schedule" : 0
};

let __fakeDAVContents = {
  "meta/version" : "3",
  
  
  "private/privkey" : '{"version":1,"algorithm":"RSA","privkey":"3ytj94K6Wo0mBjAVsiIwjm5x2+ENvpKTDUqLCz19iXbESf8RT6O8PmY7Pqcndpn+adqaQdvmr0T1JQ5bfLEHev0WBfo8oWJb+OS4rKoCWxDNzGwrOlW5hCfxSekw0KrKjqZyDZ0hT1Qt9vn6thlV2v9YWfmyn0OIxNC9hUqGwU3Wb2F2ejM0Tw40+IIW4eLEvFxLGv0vnEXpZvesPt413proL6FGQJe6vyapBg+sdX1JMYGaKZY84PUGIiDPxTbQg7yIWTSe3WlDhJ001khFiyEoTZvPhiAGXfML9ycrCRZUWkHp/cfS7QiusJXs6co0tLjrIk/rTk8h4mHBnyPkFIxh4YrfC7Bwf9npwomhaZCEQ32VK+a8grTDsGYHPZexDm3TcD2+d+hZ/u4lUOHFscQKX4w83tq942yqFtElCD2yQoqEDr1Z9zge5XBnLcYiH9hL0ozfpxBlTtpR1kSH663JHqlYim0qhuk0zrGAPkHna07UMFufxvgQBSd/YUqWCimJFGi+5QeOOFO20Skj882Bh1QDYsmbxZ/JED5ocGNHWSqpaOL2ML1F9nD5rdtffI0BsTe+j9h+HV4GlvzUz0Jd6RRf9xN4RyxqfENb8iGH5Pwbry7Qyk16rfm0s6JgG8pNb/8quKD+87RAtQFybZtdQ9NfGg+gyRiU9pbb6FPuPnGp+KpktaHu/K3HnomrVUoyLQALfCSbPXg2D9ta6dRV0JRqOZz4w52hlHIa62iJO6QecbdBzPYGT0QfOy/vp6ndRDR+2xMD/BmlaQwm3+58cqhIw9SVV5h/Z5PVaXxAOqg5vpU1NjrbF4uIFo5rmR0PyA/6qtxZaBY6w3I4sUWdDkIer8QsyrFrO7MIEdxksvDoFIeIM5eN8BufLu3ymS5ZXBiFr/iRxlYcQVHK2hz0/7syWUYsrz5/l1mj+qbWGx+6daWOk3xt4SH+p0hUpMC4FbJ9m/xr4im+X5m5ZYiajaF1QPOXTTny2plE0vfqMVlwX1HFFTJrAP+E85sZI8LPHAYO80qhSi3tV/LHjxCnC1LHJXaRkG202pQFWF1yVT/o82HBt9OC1xY6TVcy4Uh+6piNIQ9FxXGWrzjz0AUkxwkSN3Foqlfiq+mqJmNwzIdEQTmNAcBBsN3vWngU4elHjYI5qFZBzxJIkH8tfvivOshrOZIZB9TD9GIRhQwIBWc6i4fnqE9GUK2Jle6werdFATiMU4msQg7ClURaMn/p3MOLoxTmsPd1iBYPQkqnJgEAdNfKj3KRqSc6M/x09hGDSzK2d9Y03pyDGPh2sopcMFdCQbMy8VOld2/hEGakMJv6BPoRfhKmJbgGVf5x4B9dWZNa8WCmlcxaZ7KG43UA0zLm1VgfTcDW5qazDFoxIcfhmO5KoRI3q8vNs+Wh+smLC6yFODdF9HzrPimEYSc6OWHWgUcuiIBRjKeo5gBTbExWmri2VG+cn005vQNxK+0s7JVyFB8TzZ96pV3nFjkYy9OUkaiJxGd6OVGcvhbbrcNsKkaZff7OsLqczf6x0bhwh+y8+bLjLkuusGYUdBvdeiuv12IfeRupvwD8Z3aZOgcD7d+8VTyTyd/KX9fu8P7tD5SojJ5joRPjcv4Q8/mhRgtwx1McMIL3YnKHG+U=","privkeyIV":"fZ7CB/KQAUjEhkmrEkns4Q==","privkeySalt":"JFs5h2RKX9m0Op9DlQIcCOSfOH1MuDrrrHxCx+CpCUU="}',
  "public/pubkey" : '{"version":1,"algorithm":"RSA","pubkey":"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxxwObnXIoYeQKMG9RbvLkgsu/idDo25qOX87jIEiXkgW1wLKp/1D/DBLUEW303tVszNGVt6bTyAXOIj6skpmYoDs9Z48kvU3+g7Vi4QXEw5moSS4fr+yFpKiYd2Kx1+jCFGvGZjBzAAvnjsWmWrSA+LHJSrFlKY6SM3kNg8KrE8dxUi3wztlZnhZgo1ZYe7/VeBOXUfThtoadIl1VdREw2e79eiMQpPa0XLv4grCaMd/wLRs0be1/nPt7li4NyT0fnYFWg75SU3ni/xSaq/zR4NmW/of5vB2EcKyUG+/mvNplQ0CX+v3hRBCdhpCyPmcbHKUluyKzj7Ms9pKyCkwxwIDAQAB"}'
};

let Service = loadInSandbox("resource://weave/service.js");

function TestService() {
  this.__superclassConstructor = Service.WeaveSvc;
  this.__superclassConstructor([]);
}

TestService.prototype = {
  _initLogs: function TS__initLogs() {
    this._log = Log4Moz.Service.getLogger("Service.Main");
  }
};
TestService.prototype.__proto__ = Service.WeaveSvc.prototype;

Cu.import("resource://weave/identity.js");
Function.prototype.async = Async.sugar;

function checkpassphrase_coroutine() {
  var self = yield;

  var idRSA = ID.get("WeaveCryptoID");

  
  

  var idTemp = new Identity("temp", "temp", null);
  
  Crypto.randomKeyGen.async(Crypto, self.cb, idTemp);
  yield;

  
  Crypto.wrapKey.async(Crypto, self.cb, idTemp.bulkKey, idRSA);
  let wrappedKey = yield;

  
  Crypto.unwrapKey.async(Crypto, self.cb, wrappedKey, idRSA);
  let unwrappedKey = yield;

  
  
  do_check_eq(unwrappedKey, idTemp.bulkKey);

  self.done(true);
}

function test_passphrase_checking() {
  var syncTesting = new SyncTestingInfrastructure();

  syncTesting.fakeDAVService.fakeContents = __fakeDAVContents;
  for (name in __fakePrefs)
    syncTesting.fakePrefService.fakeContents[name] = __fakePrefs[name];

  var testService = new TestService();

  function login(cb) {
    testService.login(cb);
  }

  syncTesting.runAsyncFunc("Logging in", login);

  function checkpassphrase(cb) {
    checkpassphrase_coroutine.async({}, cb);
  }

  syncTesting.runAsyncFunc("Checking passphrase", checkpassphrase);
}
