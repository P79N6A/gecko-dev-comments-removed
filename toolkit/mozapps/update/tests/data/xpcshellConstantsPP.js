





const INSTALL_LOCALE = "@AB_CD@";
const MOZ_APP_NAME = "@MOZ_APP_NAME@";
const BIN_SUFFIX = "@BIN_SUFFIX@";


#ifdef MOZ_APP_VENDOR
const MOZ_APP_VENDOR = "@MOZ_APP_VENDOR@";
#else
const MOZ_APP_VENDOR = "";
#endif


const MOZ_APP_BASENAME = "@MOZ_APP_BASENAME@";
const APP_BIN_SUFFIX = "@BIN_SUFFIX@";

const APP_INFO_NAME = "XPCShell";
const APP_INFO_VENDOR = "Mozilla";

#ifdef XP_WIN
const IS_WIN = true;
#else
const IS_WIN = false;
#endif

#ifdef XP_MACOSX
const IS_MACOSX = true;
#else
const IS_MACOSX = false;
#endif

#ifdef XP_UNIX
const IS_UNIX = true;
#else
const IS_UNIX = false;
#endif

#ifdef ANDROID
const IS_ANDROID = true;
#else
const IS_ANDROID = false;
#endif

#ifdef MOZ_WIDGET_GONK
const IS_TOOLKIT_GONK = true;
#else
const IS_TOOLKIT_GONK = false;
#endif

#ifdef MOZ_VERIFY_MAR_SIGNATURE
const IS_MAR_CHECKS_ENABLED = true;
#else
const IS_MAR_CHECKS_ENABLED = false;
#endif

#ifdef DISABLE_UPDATER_AUTHENTICODE_CHECK
 const IS_AUTHENTICODE_CHECK_ENABLED = false;
#else
 const IS_AUTHENTICODE_CHECK_ENABLED = true;
#endif
