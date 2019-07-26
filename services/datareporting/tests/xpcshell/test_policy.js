


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://services-common/preferences.js");
Cu.import("resource://gre/modules/services/datareporting/policy.jsm");
Cu.import("resource://testing-common/services/datareporting/mocks.jsm");


function getPolicy(name) {
  let branch = "testing.datareporting." + name;
  let policyPrefs = new Preferences(branch + ".policy.");
  let healthReportPrefs = new Preferences(branch + ".healthreport.");

  let listener = new MockPolicyListener();
  let policy = new DataReportingPolicy(policyPrefs, healthReportPrefs, listener);

  return [policy, policyPrefs, healthReportPrefs, listener];
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
  let policyPrefs = new Preferences("foo.bar.policy.");
  let hrPrefs = new Preferences("foo.bar.healthreport.");
  let listener = {
    onRequestDataUpload: function() {},
    onRequestRemoteDelete: function() {},
    onNotifyDataPolicy: function() {},
  };

  let policy = new DataReportingPolicy(policyPrefs, hrPrefs, listener);
  do_check_true(Date.now() - policy.firstRunDate.getTime() < 1000);

  let tomorrow = Date.now() + 24 * 60 * 60 * 1000;
  do_check_true(tomorrow - policy.nextDataSubmissionDate.getTime() < 1000);

  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_UNNOTIFIED);

  run_next_test();
});

add_test(function test_prefs() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("prefs");

  let now = new Date();
  let nowT = now.getTime();

  policy.firstRunDate = now;
  do_check_eq(policyPrefs.get("firstRunTime"), nowT);
  do_check_eq(policy.firstRunDate.getTime(), nowT);

  policy.dataSubmissionPolicyNotifiedDate= now;
  do_check_eq(policyPrefs.get("dataSubmissionPolicyNotifiedTime"), nowT);
  do_check_eq(policy.dataSubmissionPolicyNotifiedDate.getTime(), nowT);

  policy.dataSubmissionPolicyResponseDate = now;
  do_check_eq(policyPrefs.get("dataSubmissionPolicyResponseTime"), nowT);
  do_check_eq(policy.dataSubmissionPolicyResponseDate.getTime(), nowT);

  policy.dataSubmissionPolicyResponseType = "type-1";
  do_check_eq(policyPrefs.get("dataSubmissionPolicyResponseType"), "type-1");
  do_check_eq(policy.dataSubmissionPolicyResponseType, "type-1");

  policy.dataSubmissionEnabled = false;
  do_check_false(policyPrefs.get("dataSubmissionEnabled", true));
  do_check_false(policy.dataSubmissionEnabled);

  policy.dataSubmissionPolicyAccepted = false;
  do_check_false(policyPrefs.get("dataSubmissionPolicyAccepted", true));
  do_check_false(policy.dataSubmissionPolicyAccepted);

  policy.dataSubmissionPolicyAcceptedVersion = 2;
  do_check_eq(policyPrefs.get("dataSubmissionPolicyAcceptedVersion"), 2);

  do_check_false(policy.dataSubmissionPolicyBypassAcceptance);
  policyPrefs.set("dataSubmissionPolicyBypassAcceptance", true);
  do_check_true(policy.dataSubmissionPolicyBypassAcceptance);

  policy.lastDataSubmissionRequestedDate = now;
  do_check_eq(hrPrefs.get("lastDataSubmissionRequestedTime"), nowT);
  do_check_eq(policy.lastDataSubmissionRequestedDate.getTime(), nowT);

  policy.lastDataSubmissionSuccessfulDate = now;
  do_check_eq(hrPrefs.get("lastDataSubmissionSuccessfulTime"), nowT);
  do_check_eq(policy.lastDataSubmissionSuccessfulDate.getTime(), nowT);

  policy.lastDataSubmissionFailureDate = now;
  do_check_eq(hrPrefs.get("lastDataSubmissionFailureTime"), nowT);
  do_check_eq(policy.lastDataSubmissionFailureDate.getTime(), nowT);

  policy.nextDataSubmissionDate = now;
  do_check_eq(hrPrefs.get("nextDataSubmissionTime"), nowT);
  do_check_eq(policy.nextDataSubmissionDate.getTime(), nowT);

  policy.currentDaySubmissionFailureCount = 2;
  do_check_eq(hrPrefs.get("currentDaySubmissionFailureCount", 0), 2);
  do_check_eq(policy.currentDaySubmissionFailureCount, 2);

  policy.pendingDeleteRemoteData = true;
  do_check_true(hrPrefs.get("pendingDeleteRemoteData"));
  do_check_true(policy.pendingDeleteRemoteData);

  policy.healthReportUploadEnabled = false;
  do_check_false(hrPrefs.get("uploadEnabled"));
  do_check_false(policy.healthReportUploadEnabled);

  run_next_test();
});

add_test(function test_notify_state_prefs() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("notify_state_prefs");

  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_UNNOTIFIED);

  policy._dataSubmissionPolicyNotifiedDate = new Date();
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_WAIT);

  policy.dataSubmissionPolicyResponseDate = new Date();
  policy._dataSubmissionPolicyNotifiedDate = null;
  do_check_eq(policy.notifyState, policy.STATE_NOTIFY_COMPLETE);

  run_next_test();
});

add_test(function test_initial_submission_notification() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("initial_submission_notification");

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

add_test(function test_bypass_acceptance() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("bypass_acceptance");

  policyPrefs.set("dataSubmissionPolicyBypassAcceptance", true);
  do_check_false(policy.dataSubmissionPolicyAccepted);
  do_check_true(policy.dataSubmissionPolicyBypassAcceptance);
  defineNow(policy, new Date(policy.nextDataSubmissionDate.getTime()));
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_notification_implicit_acceptance() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("notification_implicit_acceptance");

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
  
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("notification_failed");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("notification_accepted");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("notification_rejected");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("submission_kill_switch");

  policy.firstRunDate = new Date(Date.now() - 3 * 24 * 60 * 60 * 1000);
  policy.nextDataSubmissionDate = new Date(Date.now() - 24 * 60 * 60 * 1000);
  policy.recordUserAcceptance("accept-old-ack");
  do_check_eq(policyPrefs.get("dataSubmissionPolicyAcceptedVersion"), 1);
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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("upload_kill_switch");

  defineNow(policy, policy._futureDate(-24 * 60 * 60 * 1000));
  policy.recordUserAcceptance();
  defineNow(policy, policy.nextDataSubmissionDate);

  
  hrPrefs.ignore("uploadEnabled", policy.uploadEnabledObserver);

  policy.healthReportUploadEnabled = false;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 0);
  policy.healthReportUploadEnabled = true;
  policy.checkStateAndTrigger();
  do_check_eq(listener.requestDataUploadCount, 1);

  run_next_test();
});

add_test(function test_data_submission_no_data() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("data_submission_no_data");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("data_submission_submit_failure_hard");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("data_submission_failure_soft");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("submission_daily_scheduling");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("submission_far_future_scheduling");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("submission_backoff");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("submission_expiring");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("delete_remote_data");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("delete_remote_data_priority");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("delete_remote_data_backoff");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("delete_remote_data_in_progress_upload");

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
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("polling");
  let intended = 500;

  
  let then = Date.now();
  print("Starting run: " + then);
  Object.defineProperty(policy, "POLL_INTERVAL_MSEC", {
    value: intended,
  });
  let count = 0;

  Object.defineProperty(policy, "checkStateAndTrigger", {
    value: function fakeCheckStateAndTrigger() {
      let now = Date.now();
      let after = now - then;
      count++;

      print("Polled at " + now + " after " + after + "ms, intended " + intended);
      do_check_true(after >= intended);
      DataReportingPolicy.prototype.checkStateAndTrigger.call(policy);

      if (count >= 2) {
        policy.stopPolling();

        do_check_eq(listener.notifyUserCount, 0);
        do_check_eq(listener.requestDataUploadCount, 0);

        run_next_test();
      }

      
      
      
      
      
      
      then = Date.now();
    }
  });
  policy.startPolling();
});





add_test(function test_polling_implicit_acceptance() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("polling_implicit_acceptance");

  
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

      
      DataReportingPolicy.prototype.checkStateAndTrigger.call(policy);

      
      
      
      
      
      
      

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

add_test(function test_record_health_report_upload_enabled() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("record_health_report_upload_enabled");

  
  do_check_false(policy.pendingDeleteRemoteData);
  do_check_true(policy.healthReportUploadEnabled);
  do_check_eq(listener.requestRemoteDeleteCount, 0);

  
  
  policy.recordHealthReportUploadEnabled(false, "testing 1 2 3");
  do_check_false(policy.healthReportUploadEnabled);
  do_check_true(policy.pendingDeleteRemoteData);
  do_check_eq(listener.requestRemoteDeleteCount, 1);

  
  listener.lastRemoteDeleteRequest.onNoDataAvailable();
  do_check_false(policy.pendingDeleteRemoteData);

  
  policy.recordHealthReportUploadEnabled(true, "testing 1 2 3");
  do_check_false(policy.pendingDeleteRemoteData);
  do_check_true(policy.healthReportUploadEnabled);

  run_next_test();
});

add_test(function test_pref_change_initiates_deletion() {
  let [policy, policyPrefs, hrPrefs, listener] = getPolicy("record_health_report_upload_enabled");

  
  do_check_false(policy.pendingDeleteRemoteData);
  do_check_true(policy.healthReportUploadEnabled);
  do_check_eq(listener.requestRemoteDeleteCount, 0);

  
  
  
  Object.defineProperty(policy, "deleteRemoteData", {
    value: function deleteRemoteDataProxy() {
      do_check_false(policy.healthReportUploadEnabled);
      do_check_false(policy.pendingDeleteRemoteData);     

      run_next_test();
    },
  });

  hrPrefs.set("uploadEnabled", false);
});
 
