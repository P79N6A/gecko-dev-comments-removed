




































let EXPORTED_SYMBOLS = [((this[key] = val), key) for ([key, val] in Iterator({

WEAVE_VERSION:                         "@weave_version@",
WEAVE_ID:                              "@weave_id@",




STORAGE_VERSION:                       @storage_version@,

DEFAULT_SERVER:                        "@server_url@",

PREFS_BRANCH:                          "extensions.weave.",


PWDMGR_HOST:                           "chrome:


SINGLE_USER_SYNC:                      24 * 60 * 60 * 1000, 
MULTI_DESKTOP_SYNC:                    60 * 60 * 1000, 
MULTI_MOBILE_SYNC:                     5 * 60 * 1000, 
PARTIAL_DATA_SYNC:                     60 * 1000, 


SINGLE_USER_THRESHOLD:                 1000,
MULTI_DESKTOP_THRESHOLD:               500,
MULTI_MOBILE_THRESHOLD:                100,


MODE_RDONLY:                           0x01,
MODE_WRONLY:                           0x02,
MODE_CREATE:                           0x08,
MODE_APPEND:                           0x10,
MODE_TRUNCATE:                         0x20,


PERMS_FILE:                            0644,
PERMS_PASSFILE:                        0600,
PERMS_DIRECTORY:                       0755,



MAX_UPLOAD_RECORDS:                    100,


STATUS_OK:                             "success.status_ok",
SYNC_FAILED:                           "error.sync.failed",
LOGIN_FAILED:                          "error.login.failed",
SYNC_FAILED_PARTIAL:                   "error.sync.failed_partial",
STATUS_DISABLED:                       "service.disabled",


LOGIN_SUCCEEDED:                       "success.login",
SYNC_SUCCEEDED:                        "success.sync",
ENGINE_SUCCEEDED:                      "success.engine",


LOGIN_FAILED_NO_USERNAME:              "error.login.reason.no_username",
LOGIN_FAILED_NO_PASSWORD:              "error.login.reason.no_password",
LOGIN_FAILED_NETWORK_ERROR:            "error.login.reason.network",
LOGIN_FAILED_SERVER_ERROR:             "error.login.reason.server",
LOGIN_FAILED_INVALID_PASSPHRASE:       "error.login.reason.passphrase",
LOGIN_FAILED_LOGIN_REJECTED:           "error.login.reason.password",


METARECORD_DOWNLOAD_FAIL:              "error.sync.reason.metarecord_download_fail",
VERSION_OUT_OF_DATE:                   "error.sync.reason.version_out_of_date",
DESKTOP_VERSION_OUT_OF_DATE:           "error.sync.reason.desktop_version_out_of_date",
KEYS_DOWNLOAD_FAIL:                    "error.sync.reason.keys_download_fail",
NO_KEYS_NO_KEYGEN:                     "error.sync.reason.no_keys_no_keygen",
KEYS_UPLOAD_FAIL:                      "error.sync.reason.keys_upload_fail",
SETUP_FAILED_NO_PASSPHRASE:            "error.sync.reason.setup_failed_no_passphrase",
CREDENTIALS_CHANGED:                   "error.sync.reason.credentials_changed",
ABORT_SYNC_COMMAND:                    "aborting sync, process commands said so",
NO_SYNC_NODE_FOUND:                    "error.sync.reason.no_node_found",


ENGINE_UPLOAD_FAIL:                    "error.engine.reason.record_upload_fail",
ENGINE_DOWNLOAD_FAIL:                  "error.engine.reason.record_download_fail",
ENGINE_UNKNOWN_FAIL:                   "error.engine.reason.unknown_fail",
ENGINE_METARECORD_UPLOAD_FAIL:         "error.engine.reason.metarecord_upload_fail",


kSyncWeaveDisabled:                    "Weave is disabled",
kSyncNotLoggedIn:                      "User is not logged in",
kSyncNetworkOffline:                   "Network is offline",
kSyncInPrivateBrowsing:                "Private browsing is enabled",
kSyncBackoffNotMet:                    "Trying to sync before the server said it's okay",


FIREFOX_ID:                            "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
FENNEC_ID:                             "{a23983c0-fd0e-11dc-95ff-0800200c9a66}",
SEAMONKEY_ID:                          "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}",

MIN_PP_LENGTH:                         12,
MIN_PASS_LENGTH:                       8

}))];
