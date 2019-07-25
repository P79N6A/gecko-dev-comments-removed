#filter substitution






































let EXPORTED_SYMBOLS = [((this[key] = val), key) for ([key, val] in Iterator({

WEAVE_CHANNEL:                         "@weave_channel@",
WEAVE_VERSION:                         "@weave_version@",
WEAVE_ID:                              "@weave_id@",


SYNC_API_VERSION:                      "1.1",
USER_API_VERSION:                      "1.0",
MISC_API_VERSION:                      "1.0",




STORAGE_VERSION:                       5,

UPDATED_DEV_URL:                       "https://services.mozilla.com/sync/updated/?version=@weave_version@&channel=@weave_channel@",
UPDATED_REL_URL:                       "http://www.mozilla.com/firefox/sync/updated.html",

PREFS_BRANCH:                          "services.sync.",


PWDMGR_HOST:                           "chrome://weave",
PWDMGR_PASSWORD_REALM:                 "Mozilla Services Password",
PWDMGR_PASSPHRASE_REALM:               "Mozilla Services Encryption Passphrase",
PWDMGR_KEYBUNDLE_REALM:                "Mozilla Services Key Bundles",


DEFAULT_KEYBUNDLE_NAME:                "[default]",



HMAC_INPUT:                            "Sync-AES_256_CBC-HMAC256",


SYNC_KEY_ENCODED_LENGTH:               26,
SYNC_KEY_DECODED_LENGTH:               16,
SYNC_KEY_HYPHENATED_LENGTH:            31,    


SINGLE_USER_SYNC:                      24 * 60 * 60 * 1000, 
MULTI_DESKTOP_SYNC:                    60 * 60 * 1000, 
MULTI_MOBILE_SYNC:                     5 * 60 * 1000, 
PARTIAL_DATA_SYNC:                     60 * 1000, 

MAX_ERROR_COUNT_BEFORE_BACKOFF:        3,
MAX_IGNORE_ERROR_COUNT:                5,




HMAC_EVENT_INTERVAL:                   600000,


MASTER_PASSWORD_LOCKED_RETRY_INTERVAL: 15 * 60 * 1000,   


MOBILE_BATCH_SIZE:                     50,





DEFAULT_GUID_FETCH_BATCH_SIZE:         50,
DEFAULT_MOBILE_GUID_FETCH_BATCH_SIZE:  50,


DEFAULT_STORE_BATCH_SIZE:              1,
HISTORY_STORE_BATCH_SIZE:              50, 
FORMS_STORE_BATCH_SIZE:                50, 
PASSWORDS_STORE_BATCH_SIZE:            50, 


SINGLE_USER_THRESHOLD:                 1000,
MULTI_DEVICE_THRESHOLD:                300,


SCORE_INCREMENT_SMALL:                 1,
SCORE_INCREMENT_MEDIUM:                10,


SCORE_INCREMENT_XLARGE:                300 + 1, 


MODE_RDONLY:                           0x01,
MODE_WRONLY:                           0x02,
MODE_CREATE:                           0x08,
MODE_APPEND:                           0x10,
MODE_TRUNCATE:                         0x20,


PERMS_FILE:                            0644,
PERMS_PASSFILE:                        0600,
PERMS_DIRECTORY:                       0755,




MAX_UPLOAD_RECORDS:                    100,
MAX_HISTORY_UPLOAD:                    5000,
MAX_HISTORY_DOWNLOAD:                  5000,


STATUS_OK:                             "success.status_ok",
SYNC_FAILED:                           "error.sync.failed",
LOGIN_FAILED:                          "error.login.failed",
SYNC_FAILED_PARTIAL:                   "error.sync.failed_partial",
CLIENT_NOT_CONFIGURED:                 "service.client_not_configured",
STATUS_DISABLED:                       "service.disabled",
MASTER_PASSWORD_LOCKED:                "service.master_password_locked",


LOGIN_SUCCEEDED:                       "success.login",
SYNC_SUCCEEDED:                        "success.sync",
ENGINE_SUCCEEDED:                      "success.engine",


LOGIN_FAILED_NO_USERNAME:              "error.login.reason.no_username",
LOGIN_FAILED_NO_PASSWORD:              "error.login.reason.no_password2",
LOGIN_FAILED_NO_PASSPHRASE:            "error.login.reason.no_synckey",
LOGIN_FAILED_NETWORK_ERROR:            "error.login.reason.network",
LOGIN_FAILED_SERVER_ERROR:             "error.login.reason.server",
LOGIN_FAILED_INVALID_PASSPHRASE:       "error.login.reason.synckey",
LOGIN_FAILED_LOGIN_REJECTED:           "error.login.reason.account",


METARECORD_DOWNLOAD_FAIL:              "error.sync.reason.metarecord_download_fail",
VERSION_OUT_OF_DATE:                   "error.sync.reason.version_out_of_date",
DESKTOP_VERSION_OUT_OF_DATE:           "error.sync.reason.desktop_version_out_of_date",
SETUP_FAILED_NO_PASSPHRASE:            "error.sync.reason.setup_failed_no_passphrase",
CREDENTIALS_CHANGED:                   "error.sync.reason.credentials_changed",
ABORT_SYNC_COMMAND:                    "aborting sync, process commands said so",
NO_SYNC_NODE_FOUND:                    "error.sync.reason.no_node_found",
OVER_QUOTA:                            "error.sync.reason.over_quota",

RESPONSE_OVER_QUOTA:                   "14",


ENGINE_UPLOAD_FAIL:                    "error.engine.reason.record_upload_fail",
ENGINE_DOWNLOAD_FAIL:                  "error.engine.reason.record_download_fail",
ENGINE_UNKNOWN_FAIL:                   "error.engine.reason.unknown_fail",
ENGINE_APPLY_FAIL:                     "error.engine.reason.apply_fail",
ENGINE_METARECORD_DOWNLOAD_FAIL:       "error.engine.reason.metarecord_download_fail",
ENGINE_METARECORD_UPLOAD_FAIL:         "error.engine.reason.metarecord_upload_fail",

JPAKE_ERROR_CHANNEL:                   "jpake.error.channel",
JPAKE_ERROR_NETWORK:                   "jpake.error.network",
JPAKE_ERROR_SERVER:                    "jpake.error.server",
JPAKE_ERROR_TIMEOUT:                   "jpake.error.timeout",
JPAKE_ERROR_INTERNAL:                  "jpake.error.internal",
JPAKE_ERROR_INVALID:                   "jpake.error.invalid",
JPAKE_ERROR_NODATA:                    "jpake.error.nodata",
JPAKE_ERROR_KEYMISMATCH:               "jpake.error.keymismatch",
JPAKE_ERROR_WRONGMESSAGE:              "jpake.error.wrongmessage",
JPAKE_ERROR_USERABORT:                 "jpake.error.userabort",


kSyncMasterPasswordLocked:             "User elected to leave Master Password locked",
kSyncWeaveDisabled:                    "Weave is disabled",
kSyncNotLoggedIn:                      "User is not logged in",
kSyncNetworkOffline:                   "Network is offline",
kSyncBackoffNotMet:                    "Trying to sync before the server said it's okay",
kFirstSyncChoiceNotMade:               "User has not selected an action for first sync",


FIREFOX_ID:                            "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
FENNEC_ID:                             "{a23983c0-fd0e-11dc-95ff-0800200c9a66}",
SEAMONKEY_ID:                          "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}",
TEST_HARNESS_ID:                       "xuth@mozilla.org",

MIN_PP_LENGTH:                         12,
MIN_PASS_LENGTH:                       8

}))];
