


"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let notificationBox = document.getElementById("high-priority-global-notificationbox");

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  UITourTest();
}









function simulateVote(aId, aScore) {
  
  let notification = notificationBox.getNotificationWithValue("heartbeat-" + aId);

  let ratingContainer = notification.childNodes[0];
  ok(ratingContainer, "The notification has a valid rating container.");

  let ratingElement = ratingContainer.getElementsByAttribute("data-score", aScore);
  ok(ratingElement[0], "The rating container contains the requested rating element.");

  ratingElement[0].click();
}







function cleanUpNotification(aId) {
  let notification = notificationBox.getNotificationWithValue("heartbeat-" + aId);
  notificationBox.removeNotification(notification);
}

let tests = [
  


  function test_heartbeat_stars_show(done) {
    let flowId = "ui-ratefirefox-" + Math.random();
    let engagementURL = "http://example.com";

    gContentAPI.observe(function (aEventName, aData) {
      switch (aEventName) {
        case "Heartbeat:NotificationOffered": {
          info("'Heartbeat:Offered' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          cleanUpNotification(flowId);
          break;
        }
        case "Heartbeat:NotificationClosed": {
          info("'Heartbeat:NotificationClosed' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          done();
          break;
        }
        default:
          
          ok(false, "Unexpected notification received: " + aEventName);
      }
    });

    gContentAPI.showHeartbeat("How would you rate Firefox?", "Thank you!", flowId, engagementURL);
  },

  


  function test_heartbeat_null_engagementURL(done) {
    let flowId = "ui-ratefirefox-" + Math.random();
    let originalTabCount = gBrowser.tabs.length;

    gContentAPI.observe(function (aEventName, aData) {
      switch (aEventName) {
        case "Heartbeat:NotificationOffered": {
          info("'Heartbeat:Offered' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          
          simulateVote(flowId, 2);
          break;
        }
        case "Heartbeat:Voted": {
          info("'Heartbeat:Voted' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          break;
        }
        case "Heartbeat:NotificationClosed": {
          info("'Heartbeat:NotificationClosed' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          is(gBrowser.tabs.length, originalTabCount, "No engagement tab should be opened.");
          done();
          break;
        }
        default:
          
          ok(false, "Unexpected notification received: " + aEventName);
      }
    });

    gContentAPI.showHeartbeat("How would you rate Firefox?", "Thank you!", flowId, null);
  },

   


  function test_heartbeat_invalid_engagement_URL(done) {
    let flowId = "ui-ratefirefox-" + Math.random();
    let originalTabCount = gBrowser.tabs.length;
    let invalidEngagementURL = "invalidEngagement";

    gContentAPI.observe(function (aEventName, aData) {
      switch (aEventName) {
        case "Heartbeat:NotificationOffered": {
          info("'Heartbeat:Offered' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          
          simulateVote(flowId, 2);
          break;
        }
        case "Heartbeat:Voted": {
          info("'Heartbeat:Voted' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          break;
        }
        case "Heartbeat:NotificationClosed": {
          info("'Heartbeat:NotificationClosed' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          is(gBrowser.tabs.length, originalTabCount, "No engagement tab should be opened.");
          done();
          break;
        }
        default:
          
          ok(false, "Unexpected notification received: " + aEventName);
      }
    });

    gContentAPI.showHeartbeat("How would you rate Firefox?", "Thank you!", flowId, invalidEngagementURL);
  },

  


  function test_heartbeat_stars_vote(done) {
    const expectedScore = 4;
    let flowId = "ui-ratefirefox-" + Math.random();

    gContentAPI.observe(function (aEventName, aData) {
      switch (aEventName) {
        case "Heartbeat:NotificationOffered": {
          info("'Heartbeat:Offered' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          
          simulateVote(flowId, expectedScore);
          break;
        }
        case "Heartbeat:Voted": {
          info("'Heartbeat:Voted' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          is(aData.score, expectedScore, "Should report a score of " + expectedScore);
          break;
        }
        case "Heartbeat:NotificationClosed": {
          info("'Heartbeat:NotificationClosed' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          done();
          break;
        }
        default:
          
          ok(false, "Unexpected notification received: " + aEventName);
      }
    });

    gContentAPI.showHeartbeat("How would you rate Firefox?", "Thank you!", flowId, null);
  },

  


  function test_heartbeat_engagement_tab(done) {
    let engagementURL = "http://example.com";
    let flowId = "ui-ratefirefox-" + Math.random();
    let originalTabCount = gBrowser.tabs.length;
    const expectedTabCount = originalTabCount + 1;
    let heartbeatVoteSeen = false;

    gContentAPI.observe(function (aEventName, aData) {
      switch (aEventName) {
        case "Heartbeat:NotificationOffered": {
          info("'Heartbeat:Offered' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          
          simulateVote(flowId, 1);
          break;
        }
        case "Heartbeat:Voted": {
          info("'Heartbeat:Voted' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          heartbeatVoteSeen = true;
          break;
        }
        case "Heartbeat:NotificationClosed": {
          ok(heartbeatVoteSeen, "Heartbeat vote should have been received");
          info("'Heartbeat:NotificationClosed' notification received (timestamp " + aData.timestamp.toString() + ").");
          ok(Number.isFinite(aData.timestamp), "Timestamp must be a number.");
          is(gBrowser.tabs.length, expectedTabCount, "Engagement URL should open in a new tab.");
          gBrowser.removeCurrentTab();
          done();
          break;
        }
        default:
          
          ok(false, "Unexpected notification received: " + aEventName);
      }
    });

    gContentAPI.showHeartbeat("How would you rate Firefox?", "Thank you!", flowId, engagementURL);
  }
];
