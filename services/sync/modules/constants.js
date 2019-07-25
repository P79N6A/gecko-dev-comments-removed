



































<<<<<<< local
const EXPORTED_SYMBOLS = ["WEAVE_VERSION", "COMPATIBLE_VERSION",
			  "PREFS_BRANCH", "PWDMGR_HOST",
			  'MODE_RDONLY', 'MODE_WRONLY',
			  'MODE_CREATE', 'MODE_APPEND', 'MODE_TRUNCATE',
			  'PERMS_FILE', 'PERMS_PASSFILE', 'PERMS_DIRECTORY',
			  'ONE_BYTE', 'ONE_KILOBYTE', 'ONE_MEGABYTE',
                          'CONNECTION_TIMEOUT', 'MAX_UPLOAD_RECORDS',
                          'WEAVE_STATUS_OK', 'WEAVE_STATUS_FAILED',
                          'WEAVE_STATUS_PARTIAL', 'SERVER_LOW_QUOTA',
                          'SERVER_DOWNTIME', 'SERVER_UNREACHABLE',
                          'LOGIN_FAILED_NO_USERNAME', 'LOGIN_FAILED_NO_PASSWORD',
                          'LOGIN_FAILED_NETWORK_ERROR','LOGIN_FAILED_INVALID_PASSPHRASE',
                          'LOGIN_FAILED_LOGIN_REJECTED', 'METARECORD_DOWNLOAD_FAIL',
                          'VERSION_OUT_OF_DATE', 'DESKTOP_VERSION_OUT_OF_DATE',
                          'KEYS_DOWNLOAD_FAIL', 'NO_KEYS_NO_KEYGEN', 'KEYS_UPLOAD_FAIL',
                          'SETUP_FAILED_NO_PASSPHRASE', 'ABORT_SYNC_COMMAND',
                          'kSyncWeaveDisabled', 'kSyncNotLoggedIn',
                          'kSyncNetworkOffline', 'kSyncInPrivateBrowsing',
                          'kSyncNotScheduled',
                          'FIREFOX_ID', 'THUNDERBIRD_ID', 'FENNEC_ID', 'SEAMONKEY_ID',
                          'UI_DATA_TYPES_PER_ROW'];
=======

let EXPORTED_SYMBOLS = [((this[key] = val), key) for ([key, val] in Iterator({
>>>>>>> other

WEAVE_VERSION:                         "@weave_version@",

// Last client version this client can read. If the server contains an older
// version, this client will wipe the data on the server first.
COMPATIBLE_VERSION:                    "@compatible_version@",

PREFS_BRANCH:                          "extensions.weave.",

// Host "key" to access Weave Identity in the password manager
PWDMGR_HOST:                           "chrome://weave",

// File IO Flags
MODE_RDONLY:                           0x01,
MODE_WRONLY:                           0x02,
MODE_CREATE:                           0x08,
MODE_APPEND:                           0x10,
MODE_TRUNCATE:                         0x20,

// File Permission flags
PERMS_FILE:                            0644,
PERMS_PASSFILE:                        0600,
PERMS_DIRECTORY:                       0755,

// Number of records to upload in a single POST (multiple POSTS if exceeded)
// Record size limit is currently 10K, so 100 is a bit over 1MB
MAX_UPLOAD_RECORDS:                    100,

// Top-level statuses:
STATUS_OK:                             "success.status_ok",
SYNC_FAILED:                           "error.sync.failed",
LOGIN_FAILED:                          "error.login.failed",
SYNC_FAILED_PARTIAL:                   "error.sync.failed_partial",
STATUS_DISABLED:                       "service.disabled",

// success states
LOGIN_SUCCEEDED:                       "success.login",
SYNC_SUCCEEDED:                        "success.sync",
ENGINE_SUCCEEDED:                      "success.engine",

// login failure status codes:
LOGIN_FAILED_NO_USERNAME:              "error.login.reason.no_username",
LOGIN_FAILED_NO_PASSWORD:              "error.login.reason.no_password",
LOGIN_FAILED_NETWORK_ERROR:            "error.login.reason.network",
LOGIN_FAILED_INVALID_PASSPHRASE:       "error.login.reason.passphrase.",
LOGIN_FAILED_LOGIN_REJECTED:           "error.login.reason.password",

// sync failure status codes
METARECORD_DOWNLOAD_FAIL:              "error.sync.reason.metarecord_download_fail",
VERSION_OUT_OF_DATE:                   "error.sync.reason.version_out_of_date",
DESKTOP_VERSION_OUT_OF_DATE:           "error.sync.reason.desktop_version_out_of_date",
KEYS_DOWNLOAD_FAIL:                    "error.sync.reason.keys_download_fail",
NO_KEYS_NO_KEYGEN:                     "error.sync.reason.no_keys_no_keygen",
KEYS_UPLOAD_FAIL:                      "error.sync.reason.keys_upload_fail",
SETUP_FAILED_NO_PASSPHRASE:            "error.sync.reason.setup_failed_no_passphrase",
ABORT_SYNC_COMMAND:                    "aborting sync, process commands said so",

// engine failure status codes
ENGINE_UPLOAD_FAIL:                    "error.engine.reason.record_upload_fail",
ENGINE_DOWNLOAD_FAIL:                  "error.engine.reason.record_download_fail",
ENGINE_UNKNOWN_FAIL:                   "error.engine.reason.unknown_fail",
ENGINE_METARECORD_UPLOAD_FAIL:         "error.engine.reason.metarecord_upload_fail",

// Ways that a sync can be disabled (messages only to be printed in debug log)
kSyncWeaveDisabled:                    "Weave is disabled",
kSyncNotLoggedIn:                      "User is not logged in",
kSyncNetworkOffline:                   "Network is offline",
kSyncInPrivateBrowsing:                "Private browsing is enabled",
kSyncNotScheduled:                     "Not scheduled to do sync",
kSyncBackoffNotMet:                    "Trying to sync before the server said it's okay",

// Application IDs
<<<<<<< local
const FIREFOX_ID = "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}";
const THUNDERBIRD_ID = "{3550f703-e582-4d05-9a08-453d09bdfdc6}";
const FENNEC_ID = "{a23983c0-fd0e-11dc-95ff-0800200c9a66}";
const SEAMONKEY_ID = "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}";
=======
FIREFOX_ID:                            "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
THUNDERBIRD_ID:                        "{3550f703-e582-4d05-9a08-453d09bdfdc6}",
FENNEC_ID:                             "{a23983c0-fd0e-11dc-95ff-0800200c9a66}",
SEAMONKEY_ID:                          "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}",
>>>>>>> other

<<<<<<< local



const UI_DATA_TYPES_PER_ROW = 3;=======
}))];
>>>>>>> other
