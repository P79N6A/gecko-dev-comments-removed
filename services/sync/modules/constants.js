



































const EXPORTED_SYMBOLS = ['WEAVE_VERSION', 'MIN_SERVER_STORAGE_VERSION',
			  'PREFS_BRANCH',
			  'MODE_RDONLY', 'MODE_WRONLY',
			  'MODE_CREATE', 'MODE_APPEND', 'MODE_TRUNCATE',
			  'PERMS_FILE', 'PERMS_PASSFILE', 'PERMS_DIRECTORY',
			  'ONE_BYTE', 'ONE_KILOBYTE', 'ONE_MEGABYTE',
                          'CONNECTION_TIMEOUT', 'KEEP_DELTAS',
                          'WEAVE_STATUS_OK', 'WEAVE_STATUS_FAILED',
                          'WEAVE_STATUS_PARTIAL', 'SERVER_LOW_QUOTA',
                          'SERVER_DOWNTIME', 'SERVER_UNREACHABLE',
                          'LOGIN_FAILED_NO_USERNAME', 'LOGIN_FAILED_NO_PASSWORD',
                          'LOGIN_FAILED_REJECTED', 'METARECORD_DOWNLOAD_FAIL',
                          'VERSION_OUT_OF_DATE', 'DESKTOP_VERSION_OUT_OF_DATE',
                          'KEYS_DOWNLOAD_FAIL', 'NO_KEYS_NO_KEYGEN', 'KEYS_UPLOAD_FAIL',
                          'SETUP_FAILED_NO_PASSPHRASE', 'ABORT_SYNC_COMMAND',
                          'kSyncWeaveDisabled', 'kSyncNotLoggedIn',
                          'kSyncNetworkOffline', 'kSyncInPrivateBrowsing',
                          'kSyncNotScheduled',
                          'FIREFOX_ID', 'THUNDERBIRD_ID', 'FENNEC_ID', 'SEAMONKEY_ID'];

const WEAVE_VERSION = "@weave_version@";




const MIN_SERVER_STORAGE_VERSION = "@storage_version@";

const PREFS_BRANCH = "extensions.weave.";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_PASSFILE  = 0600;
const PERMS_DIRECTORY = 0755;

const ONE_BYTE = 1;
const ONE_KILOBYTE = 1024 * ONE_BYTE;
const ONE_MEGABYTE = 1024 * ONE_KILOBYTE;

const CONNECTION_TIMEOUT = 30000;

const KEEP_DELTAS = 25;


const WEAVE_STATUS_OK = "Sync succeeded.";
const WEAVE_STATUS_FAILED = "Sync failed.";
const WEAVE_STATUS_PARTIAL = "Sync partially succeeded, some data failed to sync.";


const SERVER_LOW_QUOTA = "Getting close to your Weave server storage quota.";
const SERVER_DOWNTIME = "Weave server is overloaded, try agian in 30 sec.";
const SERVER_UNREACHABLE = "Weave server is unreachable.";


const LOGIN_FAILED_NO_USERNAME = "No username set, login failed.";
const LOGIN_FAILED_NO_PASSWORD = "No password set, login failed.";
const LOGIN_FAILED_REJECTED = "Incorrect username or password.";
const METARECORD_DOWNLOAD_FAIL = "Can't download metadata record, HTTP error.";
const VERSION_OUT_OF_DATE = "This copy of Weave needs to be updated.";
const DESKTOP_VERSION_OUT_OF_DATE = "Weave needs updating on your desktop browser.";
const KEYS_DOWNLOAD_FAIL = "Can't download keys from server, HTTP error.";
const NO_KEYS_NO_KEYGEN = "Key generation disabled. Sync from the desktop first.";
const KEYS_UPLOAD_FAIL = "Could not upload keys.";
const SETUP_FAILED_NO_PASSPHRASE = "Could not get encryption passphrase.";


const kSyncWeaveDisabled = "Weave is disabled";
const kSyncNotLoggedIn = "User is not logged in";
const kSyncNetworkOffline = "Network is offline";
const kSyncInPrivateBrowsing = "Private browsing is enabled";
const kSyncNotScheduled = "Not scheduled to do sync";



const ABORT_SYNC_COMMAND = "aborting sync, process commands said so";


const FIREFOX_ID = "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}";
const THUNDERBIRD_ID = "{3550f703-e582-4d05-9a08-453d09bdfdc6}";
const FENNEC_ID = "{a23983c0-fd0e-11dc-95ff-0800200c9a66}";
const SEAMONKEY_ID = "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}";
