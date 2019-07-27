








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
                                    STATE_PENDING_SVC);
  let updates = getLocalUpdateString(patches, null, null, null, null, null,
                                     null, null, null, null, null, null,
                                     null, "true", channel);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeVersionFile(getAppVersion());
  writeStatusFile(STATE_PENDING_SVC);

  reloadUpdateManagerData();
  do_check_true(!!gUpdateManager.activeUpdate);

  setupAppFilesAsync();
}

function setupAppFilesFinished() {
  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    let applyToDir = getApplyDirFile();
    applyToDir.lastModifiedTime = yesterday;
  }

  stageUpdate();
}




function checkUpdateApplied() {
  gTimeoutRuns++;
  
  if (gUpdateManager.activeUpdate.state != STATE_APPLIED_SVC) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for update to equal: " +
               STATE_APPLIED_SVC +
               ", current state: " + gUpdateManager.activeUpdate.state);
    } else {
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    }
    return;
  }

  
  let state = readStatusState();
  if (state != STATE_APPLIED_SVC) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the update " +
               "status state to equal: " +
               STATE_APPLIED_SVC +
               ", current status state: " + state);
    } else {
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    }
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
    } else {
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateApplied);
    }
    return;
  }

  if (IS_WIN || IS_MACOSX) {
    let running = getPostUpdateFile(".running");
    debugDump("checking that the post update process running file doesn't " +
              "exist. Path: " + running.path);
    do_check_false(running.exists());
  }

  checkFilesAfterUpdateSuccess(getStageDirFile, true, false);

  log = getUpdatesPatchDir();
  log.append(FILE_UPDATE_LOG);
  debugDump("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  log = getUpdatesDir();
  log.append(FILE_LAST_LOG);
  if (IS_WIN || IS_MACOSX) {
    debugDump("testing " + log.path + " should exist");
    do_check_true(log.exists());
  } else {
    debugDump("testing " + log.path + " shouldn't exist");
    do_check_false(log.exists());
  }

  log = getUpdatesDir();
  log.append(FILE_BACKUP_LOG);
  debugDump("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  let updatesDir = getStageDirFile(DIR_UPDATES + "/0", true);
  debugDump("testing " + updatesDir.path + " shouldn't exist");
  do_check_false(updatesDir.exists());

  log = getStageDirFile(DIR_UPDATES + "/0/" + FILE_UPDATE_LOG, true);
  debugDump("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  log = getStageDirFile(DIR_UPDATES + "/" + FILE_LAST_LOG, true);
  if (IS_WIN || IS_MACOSX) {
    debugDump("testing " + log.path + " shouldn't exist");
    do_check_false(log.exists());
  } else {
    debugDump("testing " + log.path + " should exist");
    do_check_true(log.exists());
  }

  log = getStageDirFile(DIR_UPDATES + "/" + FILE_BACKUP_LOG, true);
  debugDump("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  
  
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
      do_throw("Exceeded MAX_TIMEOUT_RUNS while waiting for the update " +
               "status state to equal: " + STATE_SUCCEEDED +
               ", current status state: " + state);
    } else {
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    }
    return;
  }

  
  
  let updatedDir = getStageDirFile(null, true);
  if (updatedDir.exists()) {
    if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
      do_throw("Exceeded while waiting for updated dir to not exist. Path: " +
               updatedDir.path);
    } else {
      do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
    }
    return;
  }

  if (IS_WIN) {
    
    let updater = getUpdatesPatchDir();
    updater.append(FILE_UPDATER_BIN);
    if (updater.exists()) {
      if (gTimeoutRuns > MAX_TIMEOUT_RUNS) {
        do_throw("Exceeded while waiting for updater binary to no longer be " +
                 "in use");
      } else {
        try {
          updater.remove(false);
        } catch (e) {
          do_timeout(TEST_CHECK_TIMEOUT, checkUpdateFinished);
          return;
        }
      }
    }
  }

  if (IS_MACOSX) {
    debugDump("testing last modified time on the apply to directory has " +
              "changed after a successful update (bug 600098)");
    let now = Date.now();
    let applyToDir = getApplyDirFile();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  if (IS_WIN || IS_MACOSX) {
    let running = getPostUpdateFile(".running");
    debugDump("checking that the post update process running file exists. " +
              "Path: " + running.path);
    do_check_true(running.exists());
  }

  checkFilesAfterUpdateSuccess(getApplyDirFile, false, false);
  gSwitchApp = true;
  checkUpdateLogContents();
  gSwitchApp = false;
  checkCallbackAppLog();

  standardInit();

  let update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.state, STATE_SUCCEEDED);

  let updatesDir = getUpdatesPatchDir();
  debugDump("testing " + updatesDir.path + " should exist");
  do_check_true(updatesDir.exists());

  let log = getUpdatesPatchDir();
  log.append(FILE_UPDATE_LOG);
  debugDump("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  log = getUpdatesDir();
  log.append(FILE_LAST_LOG);
  debugDump("testing " + log.path + " should exist");
  do_check_true(log.exists());

  log = getUpdatesDir();
  log.append(FILE_BACKUP_LOG);
  if (IS_WIN || IS_MACOSX) {
    debugDump("testing " + log.path + " should exist");
    do_check_true(log.exists());
  } else {
    debugDump("testing " + log.path + " shouldn't exist");
    do_check_false(log.exists());
  }

  waitForFilesInUse();
}
