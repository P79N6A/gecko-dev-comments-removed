








const START_STATE = STATE_PENDING_SVC;
const END_STATE = STATE_APPLIED_SVC;

function run_test() {
  if (MOZ_APP_NAME == "xulrunner") {
    logTestInfo("Unable to run this test on xulrunner");
    return;
  }

  if (!shouldRunServiceTest()) {
    return;
  }

  setupTestCommon();
  gTestFiles = gTestFilesCompleteSuccess;
  gTestDirs = gTestDirsCompleteSuccess;
  setupUpdaterTest(FILE_COMPLETE_MAR);

  createUpdaterINI(false);

  if (IS_WIN) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED, true);
  }

  let channel = Services.prefs.getCharPref(PREF_APP_UPDATE_CHANNEL);
  let patches = getLocalPatchString(null, null, null, null, null, "true",
                                    START_STATE);
  let updates = getLocalUpdateString(patches, null, null, null, null, null,
                                     null, null, null, null, null, null,
                                     null, "true", channel);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeVersionFile(getAppVersion());
  writeStatusFile(START_STATE);

  reloadUpdateManagerData();
  Assert.ok(!!gUpdateManager.activeUpdate,
            "the active update should be defined");

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  setAppBundleModTime();
  stageUpdate();
}




function checkUpdateApplied() {
  gTimeoutRuns++;
  
  if (gUpdateManager.activeUpdate.state != END_STATE) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the " +
               "active update status state to equal: " +
               END_STATE +
               ", current state: " + gUpdateManager.activeUpdate.state);
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    return;
  }

  
  let state = readStatusState();
  if (state != END_STATE) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the update " +
               "status state to equal: " +
               END_STATE +
               ", current status state: " + state);
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    return;
  }

  
  let log;
  if (IS_WIN || IS_MACOSX) {
    log = getUpdatesDir();
  } else {
    log = getStageDirFile(null, true);
    log.append(DIR_UPDATES);
  }
  log.append(FILE_LAST_LOG);
  if (!log.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the update log " +
               "to be created. Path: " + log.path);
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    return;
  }

  checkPostUpdateRunningFile(false);
  checkFilesAfterUpdateSuccess(getStageDirFile, true, false);

  log = getUpdatesPatchDir();
  log.append(FILE_UPDATE_LOG);
  Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);

  log = getUpdatesDir();
  log.append(FILE_LAST_LOG);
  if (IS_WIN || IS_MACOSX) {
    Assert.ok(log.exists(), MSG_SHOULD_EXIST);
  } else {
    Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);
  }

  log = getUpdatesDir();
  log.append(FILE_BACKUP_LOG);
  Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);

  let updatesDir = getStageDirFile(DIR_UPDATES + "/" + DIR_PATCH, true);
  Assert.ok(!updatesDir.exists(), MSG_SHOULD_NOT_EXIST);

  log = getStageDirFile(DIR_UPDATES + "/" + FILE_LAST_LOG, true);
  if (IS_WIN || IS_MACOSX) {
    Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);
  } else {
    Assert.ok(log.exists(), MSG_SHOULD_EXIST);
  }

  log = getStageDirFile(DIR_UPDATES + "/" + FILE_BACKUP_LOG, true);
  Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);

  
  
  do_timeout(TEST_CHECK_TIMEOUT, launchAppToApplyUpdate);
}





function checkUpdateFinished() {
  if (IS_WIN || IS_MACOSX) {
    gCheckFunc = finishCheckUpdateApplied;
    checkPostUpdateAppLog();
  } else {
    finishCheckUpdateApplied();
  }
}





function finishCheckUpdateApplied() {
  gTimeoutRuns++;
  
  let state = readStatusState();
  if (state != STATE_SUCCEEDED) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the " +
               "update status file state to equal: " +
               STATE_SUCCEEDED +
               ", current status state: " + state);
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    return;
  }

  
  
  let updatedDir = getStageDirFile(null, true);
  if (updatedDir.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the updated " +
               "directory to not exist. Path: " + updatedDir.path);
    }
    do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    return;
  }

  if (IS_WIN) {
    
    let updater = getUpdatesPatchDir();
    updater.append(FILE_UPDATER_BIN);
    if (updater.exists()) {
      if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
        do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the " +
                 "updater binary to no longer be in use");
      }
      try {
        updater.remove(false);
      } catch (e) {
        do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
        return;
      }
    }
  }

  checkPostUpdateRunningFile(true);
  checkAppBundleModTime();
  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  gSwitchApp = true;
  checkUpdateLogContents();
  gSwitchApp = false;
  checkCallbackAppLog();

  standardInit();

  let update = gUpdateManager.getUpdateAt(0);
  Assert.equal(update.state, STATE_SUCCEEDED,
               "the update state" + MSG_SHOULD_EQUAL);

  let updatesDir = getUpdatesPatchDir();
  Assert.ok(updatesDir.exists(), MSG_SHOULD_EXIST);

  let log = getUpdatesPatchDir();
  log.append(FILE_UPDATE_LOG);
  Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);

  log = getUpdatesDir();
  log.append(FILE_LAST_LOG);
  Assert.ok(log.exists(), MSG_SHOULD_EXIST);

  log = getUpdatesDir();
  log.append(FILE_BACKUP_LOG);
  if (IS_WIN || IS_MACOSX) {
    Assert.ok(log.exists(), MSG_SHOULD_EXIST);
  } else {
    Assert.ok(!log.exists(), MSG_SHOULD_NOT_EXIST);
  }

  waitForFilesInUse();
}
