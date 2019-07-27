


"use strict";

Cu.import("resource://services-common/utils.js");
Cu.import("resource:///modules/loop/LoopRooms.jsm");
Cu.import("resource:///modules/Chat.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

const kGuestKey = "uGIs-kGbYt1hBBwjyW7MLQ";
const kChannelGuest = MozLoopService.channelIDs.roomsGuest;

const kRoomsResponses = new Map([
  ["_nxD4V4FflQ", {
    roomToken: "_nxD4V4FflQ",
    
    
    context: {
      wrappedKey: "F3V27oPB+FgjFbVPML2PupONYqoIZ53XRU4BqG46Lr3eyIGumgCEqgjSe/MXAXiQ//8=",
      value: "df7B4SNxhOI44eJjQavCevADyCCxz6/DEZbkOkRUMVUxzS42FbzN6C2PqmCKDYUGyCJTwJ0jln8TLw==",
      alg: "AES-GCM"
    },
    roomUrl: "http://localhost:3000/rooms/_nxD4V4FflQ",
    maxSize: 2,
    ctime: 1405517546,
    participants: [{
      displayName: "Alexis",
      account: "alexis@example.com",
      roomConnectionId: "2a1787a6-4a73-43b5-ae3e-906ec1e763cb"
    }, {
      displayName: "Adam",
      roomConnectionId: "781f012b-f1ea-4ce1-9105-7cfc36fb4ec7"
    }]
  }],
  ["QzBbvGmIZWU", {
    roomToken: "QzBbvGmIZWU",
    roomName: "Second Room Name",
    roomUrl: "http://localhost:3000/rooms/QzBbvGmIZWU",
    maxSize: 2,
    ctime: 140551741
  }]
]);

let gRoomsAdded = [];
let gRoomsUpdated = [];

const onRoomAdded = function(e, room) {
  gRoomsAdded.push(room);
};

const onRoomUpdated = function(e, room) {
  gRoomsUpdated.push(room);
};

let gQueryString = null;

add_task(function* setup_server() {
  loopServer.registerPathHandler("/registration", (req, res) => {
    res.setStatusLine(null, 200, "OK");
    res.processAsync();
    res.finish();
  });

  loopServer.registerPathHandler("/rooms", (req, res) => {
    gQueryString = req.queryString;

    res.setStatusLine(null, 200, "OK");
    
    
    res.write(JSON.stringify([...kRoomsResponses.values()]));
    res.processAsync();
    res.finish();
  });

  mockPushHandler.registrationPushURL = kEndPointUrl;

  yield MozLoopService.promiseRegisteredWithServers();
});


add_task(function* test_notification_firstTime() {
  roomsPushNotification("1", kChannelGuest);

  
  
  yield waitForCondition(() => gRoomsAdded.length === 2);

  let rooms = yield LoopRooms.promise("getAll");
  Assert.equal(rooms.length, 2);
  Assert.equal(gQueryString, "", "Query string should not be set.");
});


add_task(function* test_notification_firstTime() {
  roomsPushNotification("2", kChannelGuest);
  yield waitForCondition(() => gRoomsUpdated.length === 2);

  let rooms = yield LoopRooms.promise("getAll");
  Assert.equal(rooms.length, 2);
  Assert.equal(gQueryString, "version=2", "Query string should be set.");
});

function run_test() {
  setupFakeLoopServer();

  Services.prefs.setCharPref("loop.key", kGuestKey);
  LoopRooms.on("add", onRoomAdded);
  LoopRooms.on("update", onRoomUpdated);

  do_register_cleanup(function () {
    Services.prefs.clearUserPref("loop.key");

    LoopRooms.off("add", onRoomAdded);
    LoopRooms.off("update", onRoomUpdated);
  });

  run_next_test();
}
