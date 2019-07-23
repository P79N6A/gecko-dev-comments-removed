








































#ifndef __NS_INSTALL_H__
#define __NS_INSTALL_H__

class nsInstall
{
    public:

        enum
        {
            BAD_PACKAGE_NAME            = -200,
            UNEXPECTED_ERROR            = -201,
            ACCESS_DENIED               = -202,
            EXECUTION_ERROR             = -203,
            NO_INSTALL_SCRIPT           = -204,
            NO_CERTIFICATE              = -205,
            NO_MATCHING_CERTIFICATE     = -206,
            CANT_READ_ARCHIVE           = -207,
            INVALID_ARGUMENTS           = -208,
            ILLEGAL_RELATIVE_PATH       = -209,
            USER_CANCELLED              = -210,
            INSTALL_NOT_STARTED         = -211,
            SILENT_MODE_DENIED          = -212,
            NO_SUCH_COMPONENT           = -213,
            DOES_NOT_EXIST              = -214,
            READ_ONLY                   = -215,
            IS_DIRECTORY                = -216,
            NETWORK_FILE_IS_IN_USE      = -217,
            APPLE_SINGLE_ERR            = -218,
            INVALID_PATH_ERR            = -219,
            PATCH_BAD_DIFF              = -220,
            PATCH_BAD_CHECKSUM_TARGET   = -221,
            PATCH_BAD_CHECKSUM_RESULT   = -222,
            UNINSTALL_FAILED            = -223,
            PACKAGE_FOLDER_NOT_SET      = -224,
            EXTRACTION_FAILED           = -225,
            FILENAME_ALREADY_USED       = -226,
            INSTALL_CANCELLED           = -227,
            DOWNLOAD_ERROR              = -228,
            SCRIPT_ERROR                = -229,

            ALREADY_EXISTS              = -230,
            IS_FILE                     = -231,
            SOURCE_DOES_NOT_EXIST       = -232,
            SOURCE_IS_DIRECTORY         = -233,
            SOURCE_IS_FILE              = -234,
            INSUFFICIENT_DISK_SPACE     = -235,
            FILENAME_TOO_LONG           = -236,

            UNABLE_TO_LOCATE_LIB_FUNCTION = -237,
            UNABLE_TO_LOAD_LIBRARY        = -238,

            CHROME_REGISTRY_ERROR       = -239,

            MALFORMED_INSTALL           = -240,

            KEY_ACCESS_DENIED           = -241,
            KEY_DOES_NOT_EXIST          = -242,
            VALUE_DOES_NOT_EXIST        = -243,

            UNSUPPORTED_TYPE            = -244,

            INVALID_SIGNATURE           = -260,
            INVALID_HASH                = -261,
            INVALID_HASH_TYPE           = -262,

            OUT_OF_MEMORY               = -299,

            GESTALT_UNKNOWN_ERR         = -5550,
            GESTALT_INVALID_ARGUMENT    = -5551,

            SUCCESS                     = 0,
            REBOOT_NEEDED               = 999
        };
};

#endif
