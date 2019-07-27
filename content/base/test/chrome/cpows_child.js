dump('loaded child cpow test\n');

content.document.title = "Hello, Kitty";
const Cu = Components.utils;

var done_count = 0;
var is_remote;

(function start() {
    [is_remote] = sendSyncMessage("cpows:is_remote");
    parent_test();
    dom_test();
    xray_test();
    compartment_test();
    sync_test();
    async_test();
    rpc_test();
    nested_sync_test();
    
    
    
    
    lifetime_test(function() {
      done_count++;
      if (done_count == 2)
        sendSyncMessage("cpows:done", {});
    });
  }
)();

function ok(condition, message) {
  dump('condition: ' + condition  + ', ' + message + '\n');
  if (!condition) {
    sendAsyncMessage("cpows:fail", { message: message });
    throw 'failed check: ' + message;
  }
}

var sync_obj;
var async_obj;

function make_object()
{
  let o = { };
  o.i = 5;
  o.b = true;
  o.s = "hello";
  o.x = { i: 10 };
  o.f = function () { return 99; };
  o.ctor = function() { this.a = 3; }

  
  var throwing = new Proxy({}, new Proxy({}, {
      get: function (trap) { throw trap; }
    }));

  let array = [1, 2, 3];

  let for_json = { "n": 3, "a": array, "s": "hello", o: { "x": 10 } };

  return { "data": o,
           "throwing": throwing,
           "document": content.document,
           "array": array,
           "for_json": for_json
         };
}

function make_json()
{
  return { check: "ok" };
}

function parent_test()
{
  function f(check_func) {
    let result = check_func(10);
    ok(result == 20, "calling function in parent worked");
    return result;
  }

  addMessageListener("cpows:from_parent", (msg) => {
    let obj = msg.objects.obj;
    ok(obj.a == 1, "correct value from parent");
    done_count++;
    if (done_count == 2)
      sendSyncMessage("cpows:done", {});
  });
  sendSyncMessage("cpows:parent_test", {}, {func: f});
}

function dom_test()
{
  let element = content.document.createElement("div");
  element.id = "it_works";
  content.document.body.appendChild(element);

  sendAsyncMessage("cpows:dom_test", {}, {element: element});
  Components.utils.schedulePreciseGC(function() {
    sendSyncMessage("cpows:dom_test_after_gc");
  });
}

function xray_test()
{
  let element = content.document.createElement("div");
  element.wrappedJSObject.foo = "hello";

  sendSyncMessage("cpows:xray_test", {}, {element: element});
}



function compartment_test()
{
  
  
  if (!is_remote) {
    return;
  }

  let sb = Cu.Sandbox('http://www.example.com', { wantGlobalProperties: ['XMLHttpRequest'] });
  sb.eval('function getUnprivilegedObject() { var xhr = new XMLHttpRequest(); xhr.expando = 42; return xhr; }');
  function testParentObject(obj) {
    let results = [];
    function is(a, b, msg) { results.push({ result: a === b ? "PASS" : "FAIL", message: msg }) };
    function ok(x, msg) { results.push({ result: x ? "PASS" : "FAIL", message: msg }) };

    let cpowLocation = Cu.getCompartmentLocation(obj);
    ok(/Privileged Junk/.test(cpowLocation),
       "child->parent CPOWs should live in the privileged junk scope: " + cpowLocation);
    is(obj(), 42, "child->parent CPOW is invokable");
    is(obj.expando, undefined, "child->parent CPOW cannot access properties");

    return results;
  }
  sendSyncMessage("cpows:compartment_test", {}, { getUnprivilegedObject: sb.getUnprivilegedObject,
                                                  testParentObject: testParentObject });
}

function sync_test()
{
  dump('beginning cpow sync test\n');
  sync_obj = make_object();
  sendSyncMessage("cpows:sync",
    make_json(),
    make_object());
}

function async_test()
{
  dump('beginning cpow async test\n');
  async_obj = make_object();
  sendAsyncMessage("cpows:async",
    make_json(),
    async_obj);
}

var rpc_obj;

function rpc_test()
{
  dump('beginning cpow rpc test\n');
  rpc_obj = make_object();
  rpc_obj.data.reenter = function  () {
    sendRpcMessage("cpows:reenter", { }, { data: { valid: true } });
    return "ok";
  }
  sendRpcMessage("cpows:rpc",
    make_json(),
    rpc_obj);
}

function nested_sync_test()
{
  dump('beginning cpow nested sync test\n');
  sync_obj = make_object();
  sync_obj.data.reenter = function () {
    let caught = false;
    try {
       sendSyncMessage("cpows:reenter_sync", { }, { });
    } catch (e) {
      caught = true;
    }
    if (!ok(caught, "should not allow nested sync"))
      return "fail";
    return "ok";
  }
  sendSyncMessage("cpows:nested_sync",
    make_json(),
    rpc_obj);
}

function lifetime_test(finish)
{
  if (!is_remote) {
    
    
    
    finish();
    return;
  }

  dump("beginning lifetime test\n");
  var obj = {"will_die": {"f": 1}};
  let [result] = sendSyncMessage("cpows:lifetime_test_1", {}, {obj: obj});
  ok(result == 10, "got sync result");
  ok(obj.wont_die.f == 2, "got reverse CPOW");
  obj.will_die = null;
  Components.utils.schedulePreciseGC(function() {
    addMessageListener("cpows:lifetime_test_3", (msg) => {
      ok(obj.wont_die.f == 2, "reverse CPOW still works");
      finish();
    });
    sendSyncMessage("cpows:lifetime_test_2");
  });
}
