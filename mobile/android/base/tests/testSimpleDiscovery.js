



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SimpleServiceDiscovery.jsm");

function discovery_observer(subject, topic, data) {
  do_print("Observer: " + data);

  let service = SimpleServiceDiscovery.findServiceForID(data);
  if (!service)
    return;

  do_check_eq(service.friendlyName, "Pretend Device");
  do_check_eq(service.uuid, "uuid:5ec9ff92-e8b2-4a94-a72c-76b34e6dabb1");
  do_check_eq(service.manufacturer, "Copy Cat Inc.");
  do_check_eq(service.modelName, "Eureka Dongle");

  run_next_test();
};

var testDevice = {
  id: "test:dummy",
  target: "test:service",
  factory: function(service) {   },
  types: ["video/mp4"],
  extensions: ["mp4"]
};

add_test(function test_default() {
  do_register_cleanup(function cleanup() {
    SimpleServiceDiscovery.unregisterDevice(testDevice);
    Services.obs.removeObserver(discovery_observer, "ssdp-service-found");
  });

  Services.obs.addObserver(discovery_observer, "ssdp-service-found", false);

  
  SimpleServiceDiscovery.registerDevice(testDevice);

  
  let service = {
    location: "http://mochi.test:8888/tests/robocop/simpleservice.xml",
    target: "test:service"
  };

  do_print("Force a detailed ping from a pretend service");

  
  SimpleServiceDiscovery._processService(service);
});

run_next_test();
