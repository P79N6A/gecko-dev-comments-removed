


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://services-common/preferences.js");
Cu.import("resource://gre/modules/services/healthreport/policy.jsm");
Cu.import("resource://testing-common/services/healthreport/mocks.jsm");


function getPolicy(name) {
  let prefs = new Preferences(name);
  let listener = new MockPolicyListener();

  return [new HealthReportPolicy(prefs, listener), prefs, listener];
}

function defineNow(policy, now) {
  print("Adjusting fake system clock to " + now);
  Object.defineProperty(policy, "now", {
    value: function customNow() {
      return now;
    },
    writable: true,
  });
}

function run_test() {
  run_next_test();
}

add_test(function test_constructor() {
  let prefs = new Preferences("foo.bar");
  let listener = {
    onRequestDataUpload: function() {},
    onRequestRemoteDelete: function() {},
    onNotifyDataPolicy: function() {},
  };

  let policy = new HealthReportPolicy(prefs, listener);
  do_check_true(Date.now() - policy.firstRunDate.getTime() < 1000);

  let tomorrow = Date.now() + 24 * 60 * 60 * 1000;
  do_check_true(tomorrow - policy.nextDataSubmissionDate.getTime() < 1000);

  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_UNNOTIFIED);

  run_next_test();
});

add_test(function test_prefs() {
  let [policy, prefs, listener] = getPolicy("prefs");

  let now = new Date();
  let nowT = now.getTime();

  policy.firstRunDate = now;
  do_check_eq(prefs.get("firstRunTime"), nowT);
  do_check_eq(policy.firstRunDate.getTime(), nowT);

  policy.dataSubmissionPolicyNotifiedDate= now;
  do_check_eq(prefs.get("dataSubmissionPolicyNotifiedTime"), nowT);
  do_check_eq(policy.dataSubmissionPolicyNotifiedDate.getTime(), nowT);

  policy.dataSubmissionPolicyResponseDate = now;
  do_check_eq(prefs.get("dataSubmissionPolicyResponseTime"), nowT);
  do_check_eq(policy.dataSubmissionPolicyResponseDate.getTime(), nowT);

  policy.dataSubmissionPolicyResponseType = "type-1";
  do_check_eq(prefs.get("dataSubmissionPolicyResponseType"), "type-1");
  do_check_eq(policy.dataSubmissionPolicyResponseType, "type-1");

  policy.dataSubmissionEnabled = false;
  do_check_false(prefs.get("dataSubmissionEnabled", true));
  do_check_false(policy.dataSubmissionEnabled);

  policy.dataSubmissionPolicyAccepted = false;
  do_check_false(prefs.get("dataSubmissionPolicyAccepted", true));
  do_check_false(policy.dataSubmissionPolicyAccepted);

  policy.lastDataSubmissionRequestedDate = now;
  do_check_eq(prefs.get("lastDataSubmissionRequestedTime"), nowT);
  do_check_eq(policy.lastDataSubmissionRequestedDate.getTime(), nowT);

  policy.lastDataSubmissionSuccessfulDate = now;
  do_check_eq(prefs.get("lastDataSubmissionSuccessfulTime"), nowT);
  do_check_eq(policy.lastDataSubmissionSuccessfulDate.getTime(), nowT);

  policy.lastDataSubmissionFailureDate = now;
  do_check_eq(prefs.get("lastDataSubmissionFailureTime"), nowT);
  do_check_eq(policy.lastDataSubmissionFailureDate.getTime(), nowT);

  policy.nextDataSubmissionDate = now;
  do_check_eq(prefs.get("nextDataSubmissionTime"), nowT);
  do_check_eq(policy.nextDataSubmissionDate.getTime(), nowT);

  policy.currentDaySubmissionFailureCount = 2;
  do_check_eq(prefs.get("currentDaySubmissionFailureCount", 0), 2);
  do_check_eq(policy.currentDaySubmissionFailureCount, 2);

  policy.pendingDeleteRemoteData = true;
  do_check_true(prefs.get("pendingDeleteRemoteData"));
  do_check_true(policy.pendingDeleteRemoteData);

  run_next_test();
});

add_test(function test_notify_state_prefs() {
  let [policy, prefs, listener] = getPolicy("notify_state_prefs");

  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_UNNOTIFIED);

  policy._dataSubmissionPolicyNotifiedDate = new Date();
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);

  policy.dataSubmissionPolicyResponseDate = new Date();
  policy._dataSubmissionPolicyNotifiedDate = null;
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_COMPLETE);

  run_next_test();
});

add_test(function test_initial_submission_notification() {
  let [policy, prefs, listener] = getPolicy("initial_submission_notification");

  do_check_eq(listener.notifyUserCount, 0);

  
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 0);

  
  defineNow(policy, new Date(policy.firstRunDate.getTime() +
                             policy.SUBMISSION_NOTIFY_INTERVAL_MSEC - 1));
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 0);
  do_check_null(policy._dataSubmissionPolicyNotifiedDate);
  do_check_eq(policy.dataSubmissionPolicyNotifiedDate.getTime(), 0);

  
  defineNow(policy, new Date(policy.firstRunDate.getTime() +
                             policy.SUBMISSION_NOTIFY_INTERVAL_MSEC));
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 1);
  listener.lastNotifyRequest.onUserNotifyComplete();
  do_check_true(policy._dataSubmissionPolicyNotifiedDate instanceof Date);
  do_check_true(policy.dataSubmissionPolicyNotifiedDate.getTime() > 0);
  do_check_eq(policy.dataSubmissionPolicyNotifiedDate.getTime(),
              policy._dataSubmissionPolicyNotifiedDate.getTime());
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);

  run_next_test();
});

add_test(function test_notification_implicit_acceptance() {
  let [policy, prefs, listener] = getPolicy("notification_implicit_acceptance");

  let now = new Date(policy.nextDataSubmissionDate.getTime() -
                     policy.SUBMISSION_NOTIFY_INTERVAL_MSEC + 1);
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 1);
  listener.lastNotifyRequest.onUserNotifyComplete();
  do_check_eq(policy.dataSubmissionPolicyResponseType, "none-recorded");

  do_check_true(5000 < policy.IMPLICIT_ACCEPTANCE_INTERVAL_MSEC);
  defineNow(policy, new Date(now.getTime() + 5000));
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 1);
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);
  do_check_eq(policy.dataSubmissionPolicyResponseDate.getTime(), 0);
  do_check_eq(policy.dataSubmissionPolicyResponseType, "none-recorded");

  defineNow(policy, new Date(now.getTime() + policy.IMPLICIT_ACCEPTANCE_INTERVAL_MSEC + 1));
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 1);
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_COMPLETE);
  do_check_eq(policy.dataSubmissionPolicyResponseDate.getTime(), policy.now().getTime());
  do_check_eq(policy.dataSubmissionPolicyResponseType, "accepted-implicit-time-elapsed");

  run_next_test();
});

add_test(function test_notification_rejected() {
  
  let [policy, prefs, listener] = getPolicy("notification_failed");

  let now = new Date(policy.nextDataSubmissionDate.getTime() -
                     policy.SUBMISSION_NOTIFY_INTERVAL_MSEC + 1);
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.notifyUserCount, 1);
  listener.lastNotifyRequest.onUserNotifyFailed(new Error("testing failed."));
  do_check_null(policy._dataSubmissionPolicyNotifiedDate);
  do_check_eq(policy.dataSubmissionPolicyNotifiedDate.getTime(), 0);
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_UNNOTIFIED);

  run_next_test();
});

add_test(function test_notification_accepted() {
  let [policy, prefs, listener] = getPolicy("notification_accepted");

  let now = new Date(policy.nextDataSubmissionDate.getTime() -
                     policy.SUBMISSION_NOTIFY_INTERVAL_MSEC + 1);
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  listener.lastNotifyRequest.onUserNotifyComplete();
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);
  do_check_false(policy.dataSubmissionPolicyAccepted);
  listener.lastNotifyRequest.onUserNotifyComplete();
  listener.lastNotifyRequest.onUserAccept("foo-bar");
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_COMPLETE);
  do_check_eq(policy.dataSubmissionPolicyResponseType, "accepted-foo-bar");
  do_check_true(policy.dataSubmissionPolicyAccepted);
  do_check_eq(policy.dataSubmissionPolicyResponseDate.getTime(), now.getTime());

  run_next_test();
});

add_test(function test_notification_rejected() {
  let [policy, prefs, listener] = getPolicy("notification_rejected");

  let now = new Date(policy.nextDataSubmissionDate.getTime() -
                     policy.SUBMISSION_NOTIFY_INTERVAL_MSEC + 1);
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  listener.lastNotifyRequest.onUserNotifyComplete();
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);
  do_check_false(policy.dataSubmissionPolicyAccepted);
  listener.lastNotifyRequest.onUserReject();
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_COMPLETE);
  do_check_eq(policy.dataSubmissionPolicyResponseType, "rejected-no-reason");
  do_check_false(policy.dataSubmissionPolicyAccepted);

  
  defineNow(policy, new Date(policy.nextDataSubmissionDate.getTime() + 10000));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);

  run_next_test();
});

add_test(function test_submission_kill_switch() {
  let [policy, prefs, listener] = getPolicy("submission_kill_switch");

  policy.firstRunDate = new Date(Date.now() - 3 * 24 * 60 * 60 * 1000);
  policy.nextDataSubmissionDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.recordUserAcceptance("accept-old-ack");
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  defineNow(policy,
    new Date(Date.now() + policy.SUBMISSION_REQUEST_EXPIRE_INTERVAL_MSEC + 100));
  policy.dataSubmissionEnabled = false;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_upload_kill_switch() {
  let [policy, prefs, listener] = getPolicy("upload_kill_switch");

  defineNow(policy, policy._futureDate(-24 * 60 * 60 * 1000));
  policy.recordUserAcceptance();
  defineNow(policy, policy.nextDataSubmissionDate);

  policy.dataUploadEnabled = false;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);
  policy.dataUploadEnabled = true;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_data_submission_no_data() {
  let [policy, prefs, listener] = getPolicy("data_submission_no_data");

  policy.dataSubmissionPolicyResponseDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.dataSubmissionPolicyAccepted = true;
  let now = new Date(policy.nextDataSubmissionDate.getTime() + 1);
  defineNow(policy, now);
  do_check_eq(listener.requestDataUploadCount, 0);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  listener.lastDataRequest.onNoDataAvailable();

  
  defineNow(policy, new Date(now.getTime() + 155 * 60 * 1000));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 2);

  run_next_test();
});

add_test(function test_data_submission_submit_failure_hard() {
  let [policy, prefs, listener] = getPolicy("data_submission_submit_failure_hard");

  policy.dataSubmissionPolicyResponseDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.dataSubmissionPolicyAccepted = true;
  let nextDataSubmissionDate = policy.nextDataSubmissionDate;
  let now = new Date(policy.nextDataSubmissionDate.getTime() + 1);
  defineNow(policy, now);

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  listener.lastDataRequest.onSubmissionFailureHard();
  do_check_eq(listener.lastDataRequest.state,
              listener.lastDataRequest.SUBMISSION_FAILURE_HARD);

  let expected = new Date(now.getTime() + 24 * 60 * 60 * 1000);
  do_check_eq(policy.nextDataSubmissionDate.getTime(), expected.getTime());

  defineNow(policy, new Date(now.getTime() + 10));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_data_submission_submit_try_again() {
  let [policy, prefs, listener] = getPolicy("data_submission_failure_soft");

  policy.recordUserAcceptance();
  let nextDataSubmissionDate = policy.nextDataSubmissionDate;
  let now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  listener.lastDataRequest.onSubmissionFailureSoft();
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
              nextDataSubmissionDate.getTime() + 15 * 60 * 1000);

  run_next_test();
});

add_test(function test_submission_daily_scheduling() {
  let [policy, prefs, listener] = getPolicy("submission_daily_scheduling");

  policy.dataSubmissionPolicyResponseDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.dataSubmissionPolicyAccepted = true;
  let nextDataSubmissionDate = policy.nextDataSubmissionDate;

  
  let now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  do_check_eq(policy.lastDataSubmissionRequestedDate.getTime(), now.getTime());

  let finishedDate = new Date(now.getTime() + 250);
  defineNow(policy, new Date(finishedDate.getTime() + 50));
  listener.lastDataRequest.onSubmissionSuccess(finishedDate);
  do_check_eq(policy.lastDataSubmissionSuccessfulDate.getTime(), finishedDate.getTime());

  
  

  let nextScheduled = new Date(finishedDate.getTime() + 24 * 60 * 60 * 1000);
  do_check_eq(policy.nextDataSubmissionDate.getTime(), nextScheduled.getTime());

  
  defineNow(policy, new Date(now.getTime() + 40000));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  defineNow(policy, nextScheduled);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 2);
  listener.lastDataRequest.onSubmissionSuccess(new Date(nextScheduled.getTime() + 200));
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
    new Date(nextScheduled.getTime() + 24 * 60 * 60 * 1000 + 200).getTime());

  run_next_test();
});

add_test(function test_submission_far_future_scheduling() {
  let [policy, prefs, listener] = getPolicy("submission_far_future_scheduling");

  let now = new Date(Date.now() - 24 * 60 * 60 * 1000);
  defineNow(policy, now);
  policy.recordUserAcceptance();
  now = new Date();
  defineNow(policy, now);

  let nextDate = policy._futureDate(3 * 24 * 60 * 60 * 1000 - 1);
  policy.nextDataSubmissionDate = nextDate;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);
  do_check_eq(policy.nextDataSubmissionDate.getTime(), nextDate.getTime());

  policy.nextDataSubmissionDate = new Date(nextDate.getTime() + 1);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
              policy._futureDate(24 * 60 * 60 * 1000).getTime());

  run_next_test();
});

add_test(function test_submission_backoff() {
  let [policy, prefs, listener] = getPolicy("submission_backoff");

  do_check_eq(policy.FAILURE_BACKOFF_INTERVALS.length, 2);

  policy.dataSubmissionPolicyResponseDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.dataSubmissionPolicyAccepted = true;

  let now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  do_check_eq(policy.currentDaySubmissionFailureCount, 0);

  now = new Date(now.getTime() + 5000);
  defineNow(policy, now);

  
  listener.lastDataRequest.onSubmissionFailureSoft();
  do_check_eq(policy.currentDaySubmissionFailureCount, 1);
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
              new Date(now.getTime() + policy.FAILURE_BACKOFF_INTERVALS[0]).getTime());
  do_check_eq(policy.lastDataSubmissionFailureDate.getTime(), now.getTime());

  
  now = new Date(policy.nextDataSubmissionDate.getTime() - 1);
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  
  now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 2);

  now = new Date(now.getTime() + 5000);
  defineNow(policy, now);

  
  listener.lastDataRequest.onSubmissionFailureSoft();
  do_check_eq(policy.currentDaySubmissionFailureCount, 2);
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
              new Date(now.getTime() + policy.FAILURE_BACKOFF_INTERVALS[1]).getTime());

  now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 3);

  now = new Date(now.getTime() + 5000);
  defineNow(policy, now);

  
  listener.lastDataRequest.onSubmissionFailureSoft();
  do_check_eq(policy.currentDaySubmissionFailureCount, 0);
  do_check_eq(policy.nextDataSubmissionDate.getTime(),
              new Date(now.getTime() + 24 * 60 * 60 * 1000).getTime());

  run_next_test();
});


add_test(function test_submission_expiring() {
  let [policy, prefs, listener] = getPolicy("submission_expiring");

  policy.dataSubmissionPolicyResponseDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.dataSubmissionPolicyAccepted = true;
  let nextDataSubmission = policy.nextDataSubmissionDate;
  let now = new Date(policy.nextDataSubmissionDate.getTime());
  defineNow(policy, now);
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  defineNow(policy, new Date(now.getTime() + 500));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  defineNow(policy, new Date(policy.now().getTime() +
                             policy.SUBMISSION_REQUEST_EXPIRE_INTERVAL_MSEC));

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 2);

  run_next_test();
});

add_test(function test_delete_remote_data() {
  let [policy, prefs, listener] = getPolicy("delete_remote_data");

  do_check_false(policy.pendingDeleteRemoteData);
  let nextSubmissionDate = policy.nextDataSubmissionDate;

  let now = new Date();
  defineNow(policy, now);

  policy.deleteRemoteData();
  do_check_true(policy.pendingDeleteRemoteData);
  do_check_neq(nextSubmissionDate.getTime(),
               policy.nextDataSubmissionDate.getTime());
  do_check_eq(now.getTime(), policy.nextDataSubmissionDate.getTime());

  do_check_eq(listener.requestRemoteDeleteCount, 1);
  do_check_true(listener.lastRemoteDeleteRequest.isDelete);
  defineNow(policy, policy._futureDate(1000));

  listener.lastRemoteDeleteRequest.onSubmissionSuccess(policy.now());
  do_check_false(policy.pendingDeleteRemoteData);

  run_next_test();
});


add_test(function test_delete_remote_data_priority() {
  let [policy, prefs, listener] = getPolicy("delete_remote_data_priority");

  let now = new Date();
  defineNow(policy, policy._futureDate(-24 * 60 * 60 * 1000));
  policy.recordUserAcceptance();
  defineNow(policy, new Date(now.getTime() + 3 * 24 * 60 * 60 * 1000));

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  policy._inProgressSubmissionRequest = null;

  policy.deleteRemoteData();
  policy.checkStateAndTrigger();

  do_check_eq(listener.requestRemoteDeleteCount, 1);
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_delete_remote_data_backoff() {
  let [policy, prefs, listener] = getPolicy("delete_remote_data_backoff");

  let now = new Date();
  defineNow(policy, policy._futureDate(-24 * 60 * 60 * 1000));
  policy.recordUserAcceptance();
  defineNow(policy, now);
  policy.nextDataSubmissionDate = now;
  policy.deleteRemoteData();

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestRemoteDeleteCount, 1);
  defineNow(policy, policy._futureDate(1000));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);
  do_check_eq(listener.requestRemoteDeleteCount, 1);

  defineNow(policy, policy._futureDate(500));
  listener.lastRemoteDeleteRequest.onSubmissionFailureSoft();
  defineNow(policy, policy._futureDate(50));

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestRemoteDeleteCount, 1);

  defineNow(policy, policy._futureDate(policy.FAILURE_BACKOFF_INTERVALS[0] - 50));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestRemoteDeleteCount, 2);

  run_next_test();
});



add_test(function test_delete_remote_data_in_progress_upload() {
  let [policy, prefs, listener] = getPolicy("delete_remote_data_in_progress_upload");

  let now = new Date();
  defineNow(policy, policy._futureDate(-24 * 60 * 60 * 1000));
  policy.recordUserAcceptance();
  defineNow(policy, policy.nextDataSubmissionDate);

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  defineNow(policy, policy._futureDate(50 * 1000));

  
  policy.deleteRemoteData();
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  do_check_eq(listener.requestRemoteDeleteCount, 0);

  
  defineNow(policy, policy._futureDate(10 * 1000));
  listener.lastDataRequest.onSubmissionSuccess(policy._futureDate(1000));
  defineNow(policy, policy._futureDate(5000));

  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);
  do_check_eq(listener.requestRemoteDeleteCount, 1);

  run_next_test();
});

add_test(function test_polling() {
  let [policy, prefs, listener] = getPolicy("polling");

  
  let now = new Date();
  Object.defineProperty(policy, "POLL_INTERVAL_MSEC", {
    value: 500,
  });
  let count = 0;

  Object.defineProperty(policy, "checkStateAndTrigger", {
    value: function fakeCheckStateAndTrigger() {
      let now2 = new Date();
      count++;

      do_check_true(now2.getTime() - now.getTime() >= 500);
      now = now2;
      HealthReportPolicy.prototype.checkStateAndTrigger.call(policy);

      if (count >= 2) {
        policy.stopPolling();

        do_check_eq(listener.notifyUserCount, 0);
        do_check_eq(listener.requestDataUploadCount, 0);

        run_next_test();
      }
    }
  });
  policy.startPolling();
});





add_test(function test_polling_implicit_acceptance() {
  let [policy, prefs, listener] = getPolicy("polling_implicit_acceptance");

  
  Object.defineProperty(policy, "POLL_INTERVAL_MSEC", {
    value: 250,
  });

  Object.defineProperty(policy, "IMPLICIT_ACCEPTANCE_INTERVAL_MSEC", {
    value: 750,
  });

  let count = 0;
  Object.defineProperty(policy, "checkStateAndTrigger", {
    value: function CheckStateAndTriggerProxy() {
      count++;
      print("checkStateAndTrigger count: " + count);

      
      HealthReportPolicy.prototype.checkStateAndTrigger.call(policy);

      
      
      
      
      
      
      

      do_check_eq(listener.notifyUserCount, 1);

      if (count == 1) {
        listener.lastNotifyRequest.onUserNotifyComplete();
      }

      if (count < 4) {
        do_check_false(policy.dataSubmissionPolicyAccepted);
        do_check_eq(listener.requestDataUploadCount, 0);
      } else {
        do_check_true(policy.dataSubmissionPolicyAccepted);
        do_check_eq(policy.dataSubmissionPolicyResponseType,
                    "accepted-implicit-time-elapsed");
        do_check_eq(listener.requestDataUploadCount, 1);
      }

      if (count > 4) {
        do_check_eq(listener.requestDataUploadCount, 1);
        policy.stopPolling();
        run_next_test();
      }
    }
  });

  policy.firstRunDate = new Date(Date.now() - 4 * 24 * 60 * 60 * 1000);
  policy.nextDataSubmissionDate = new Date(Date.now());
  policy.startPolling();
});

