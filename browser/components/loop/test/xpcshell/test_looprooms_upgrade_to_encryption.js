



"use strict";

Cu.import("resource://services-common/utils.js");

const loopCrypto = Cu.import("resource:///modules/loop/crypto.js", {}).LoopCrypto;
const { LOOP_ROOMS_CACHE_FILENAME } = Cu.import("resource:///modules/loop/LoopRoomsCache.jsm", {});

let gTimerArgs = [];

timerHandlers.startTimer = function(callback, delay) {
  gTimerArgs.push({callback, delay});
  return gTimerArgs.length;
};

let gRoomPatches = [];

const kContextEnabledPref = "loop.contextInConverations.enabled";

const kFxAKey = "uGIs-kGbYt1hBBwjyW7MLQ";


const kRoomsResponses = new Map([
  ["_nxD4V4FflQ", {
    roomToken: "_nxD4V4FflQ",
    roomName: "First Room Name",
    roomUrl: "http://localhost:3000/rooms/_nxD4V4FflQ"
  }],
  ["QzBbvGmIZWU", {
    roomToken: "QzBbvGmIZWU",
    roomName: "Loopy Discussion",
    roomUrl: "http://localhost:3000/rooms/QzBbvGmIZWU"
  }]
]);


add_task(function* setup_server() {
  loopServer.registerPathHandler("/registration", (req, res) => {
    res.setStatusLine(null, 200, "OK");
    res.processAsync();
    res.finish();
  });

  loopServer.registerPathHandler("/rooms", (req, res) => {
    res.setStatusLine(null, 200, "OK");

    res.write(JSON.stringify([...kRoomsResponses.values()]));

    res.processAsync();
    res.finish();
  });

  function returnRoomDetails(res, roomName) {
    roomDetail.roomName = roomName;
    res.setStatusLine(null, 200, "OK");
    res.write(JSON.stringify(roomDetail));
    res.processAsync();
    res.finish();
  }

  function getJSONData(body) {
    return JSON.parse(CommonUtils.readBytesFromInputStream(body));
  }

  
  [...kRoomsResponses.values()].forEach(function(room) {
    loopServer.registerPathHandler("/rooms/" + encodeURIComponent(room.roomToken), (req, res) => {
      let roomDetail = extend({}, room);
      if (req.method == "PATCH") {
        let data = getJSONData(req.bodyInputStream);
        Assert.ok("context" in data, "should have encrypted context");
        gRoomPatches.push(data);
        delete roomDetail.roomName;
        roomDetail.context = data.context;
        res.setStatusLine(null, 200, "OK");
        res.write(JSON.stringify(roomDetail));
        res.processAsync();
        res.finish();
      } else {
        res.setStatusLine(null, 200, "OK");
        res.write(JSON.stringify(room));
        res.processAsync();
        res.finish();
      }
    });
  });

  mockPushHandler.registrationPushURL = kEndPointUrl;

  yield MozLoopService.promiseRegisteredWithServers();
});


add_task(function* test_get_rooms_upgrades_to_encryption() {
  let rooms = yield LoopRooms.promise("getAll");

  
  Assert.equal(LoopRoomsInternal.encryptionQueue.queue.length, 2, "Should have two rooms queued");
  Assert.equal(gTimerArgs.length, 1, "Should have started a timer");

  
  yield gTimerArgs[0].callback();

  Assert.equal(gRoomPatches.length, 1, "Should have patched one room");
  Assert.equal(gTimerArgs.length, 2, "Should have started a second timer");

  yield gTimerArgs[1].callback();

  Assert.equal(gRoomPatches.length, 2, "Should have patches a second room");
  Assert.equal(gTimerArgs.length, 2, "Should not have queued another timer");

  
  rooms = yield LoopRooms.promise("getAll");

  Assert.equal(rooms.length, 2, "Should have two rooms");

  
  for (let room of rooms) {
    let roomData = yield loopCrypto.decryptBytes(room.roomKey, room.context.value);

    Assert.deepEqual(JSON.parse(roomData),
      { roomName: kRoomsResponses.get(room.roomToken).roomName },
      "Should have encrypted the data correctly");
  }
});

function run_test() {
  setupFakeLoopServer();

  Services.prefs.setCharPref("loop.key.fxa", kFxAKey);
  Services.prefs.setBoolPref(kContextEnabledPref, true);

  
  MozLoopServiceInternal.fxAOAuthTokenData = { token_type: "bearer" };
  MozLoopServiceInternal.fxAOAuthProfile = { email: "fake@invalid.com" };

  do_register_cleanup(function () {
    Services.prefs.clearUserPref(kContextEnabledPref);
    Services.prefs.clearUserPref("loop.key.fxa");

    MozLoopServiceInternal.fxAOAuthTokenData = null;
    MozLoopServiceInternal.fxAOAuthProfile = null;
  });

  run_next_test();
}
