


"use strict";





const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);

function connect_client(callback)
{
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  client.connect(() => {
    client.listTabs(response => {
      callback(client, response.profilerActor);
    });
  });
}

function run_test()
{
  DebuggerServer.init(() => true);
  DebuggerServer.addBrowserActors();

  connect_client((client, actor) => {
    activate_profiler(client, actor, () => {
      test_events(client, actor, () => {
        client.close(do_test_finished);
      });
    });
  })

  do_test_pending();
}

function activate_profiler(client, actor, callback)
{
  client.request({ to: actor, type: "startProfiler" }, response => {
    do_check_true(response.started);
    client.request({ to: actor, type: "isActive" }, response => {
      do_check_true(response.isActive);
      callback();
    });
  });
}

function register_events(client, actor, events, callback)
{
  client.request({
    to: actor,
    type: "registerEventNotifications",
    events: events
  }, callback);
}

function unregister_events(client, actor, events, callback)
{
  client.request({
    to: actor,
    type: "unregisterEventNotifications",
    events: events
  }, callback);
}

function emit_and_wait_for_event(client, subject, topic, data, callback)
{
  client.addListener("eventNotification", (type, response) => {
    do_check_eq(type, "eventNotification");
    do_check_eq(response.topic, topic);
    do_check_eq(typeof response.subject, "object");

    delete subject.wrappedJSObject;
    do_check_eq(JSON.stringify(response.subject), JSON.stringify(subject));

    do_check_eq(response.data, data);
    callback();
  });

  
  
  subject.wrappedJSObject = subject;
  Services.obs.notifyObservers(subject, topic, data);
}

function test_events(client, actor, callback)
{
  register_events(client, actor, ["foo", "bar"], response => {
    do_check_eq(typeof response.registered, "object");
    do_check_eq(response.registered.length, 2);
    do_check_eq(response.registered[0], "foo");
    do_check_eq(response.registered[1], "bar");

    register_events(client, actor, ["foo"], response => {
      do_check_eq(typeof response.registered, "object");
      do_check_eq(response.registered.length, 0);

      emit_and_wait_for_event(client, { hello: "world" }, "foo", "bar", () => {

        unregister_events(client, actor, ["foo", "bar", "baz"], response => {
          do_check_eq(typeof response.unregistered, "object");
          do_check_eq(response.unregistered.length, 2);
          do_check_eq(response.unregistered[0], "foo");
          do_check_eq(response.unregistered[1], "bar");

          
          
          
          Services.obs.notifyObservers(null, "foo", null);
          Services.obs.notifyObservers(null, "bar", null);

          callback();
        });
      });
    });
  });
}
