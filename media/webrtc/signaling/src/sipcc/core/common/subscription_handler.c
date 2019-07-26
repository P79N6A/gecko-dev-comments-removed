






































#include "cpr_types.h"
#include "cc_types.h"
#include "phone_platform_constants.h"
#include "cc_constants.h"
#include "phone_debug.h"
#include "prot_configmgr.h"
#include "cc_blf.h"
#include "ccapi_snapshot.h"

#define SPEEDDIAL_START_BUTTON_NUMBER 2

static unsigned char transactionIds[MAX_REG_LINES];
static boolean displayBLFState = TRUE;
static cc_blf_state_t blfStates[MAX_REG_LINES];
static boolean isBLFHandlerRunning = FALSE;
static boolean isAvailable = FALSE;

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

static void ccBLFHandlerInitialized();











boolean sub_hndlr_isAlertingBLFState(int inst) 
{
    static const char fname[] = "sub_hndlr_isAlertingBLFState";

    if ((displayBLFState == TRUE) && (blfStates[inst - 1] == CC_SIP_BLF_ALERTING)) {
        CCAPP_DEBUG(DEB_F_PREFIX"inst=%d, isAlerting=TRUE\n",
                    DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                    inst);

        return TRUE;
    }
    CCAPP_DEBUG(DEB_F_PREFIX"inst=%d, isAlerting=FALSE\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                inst);
    return FALSE;
}
    










boolean sub_hndlr_isInUseBLFState(int inst) 
{
    static const char fname[] = "sub_hndlr_isInUseBLFState";

    if ((displayBLFState == TRUE) && (blfStates[inst - 1] == CC_SIP_BLF_INUSE)) {
    CCAPP_DEBUG(DEB_F_PREFIX"inst=%d, isInUse=TRUE\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                inst);
        return TRUE;
    }
    CCAPP_DEBUG(DEB_F_PREFIX"inst=%d, isInUse=FALSE\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                inst);
    return FALSE;
}
    









boolean sub_hndlr_isAvailable() 
{
    static const char fname[] = "sub_hndlr_isAvailable";

    CCAPP_DEBUG(DEB_F_PREFIX"isAvailable=%d\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                isAvailable);
    return isAvailable;
}

static unsigned short get_new_trans_id()
{
    static unsigned short curr_trans_id = 0;

    if (++curr_trans_id == 0) {
        curr_trans_id = 1;
    }

    return curr_trans_id;
}
    









void sub_hndlr_start() 
{
    static const char fname[] = "sub_hndlr_start";
    int i;
    cc_uint32_t lineFeature = 0;
    cc_uint32_t featureOptionMask = 0;
    char speedDialNumber[MAX_LINE_NAME_SIZE] = {0};
    char primaryLine[MAX_LINE_NAME_SIZE] = {0};
    int transId;

    CCAPP_DEBUG(DEB_F_PREFIX"entering\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
    
    isAvailable = TRUE;

    
    config_get_line_string(CFGID_LINE_NAME, primaryLine, 1, sizeof(primaryLine));
        
    


    for (i = SPEEDDIAL_START_BUTTON_NUMBER; i <= MAX_REG_LINES; i++) {
        
        config_get_line_value(CFGID_LINE_FEATURE, &lineFeature, sizeof(lineFeature), i);

            
        CCAPP_DEBUG(DEB_F_PREFIX"inst=%d, lineFeature=%d\n",
                    DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                    i, lineFeature);
        switch (lineFeature) {
        case cfgLineFeatureSpeedDialBLF:
            config_get_line_string(CFGID_LINE_SPEEDDIAL_NUMBER, speedDialNumber, i, sizeof(speedDialNumber));
            if (speedDialNumber[0] == 0) {
                break;
            }
            config_get_line_value(CFGID_LINE_FEATURE, &featureOptionMask, sizeof(featureOptionMask), i);
                    
            transId = get_new_trans_id();
            transactionIds[i - 1] = transId;
            CC_BLF_subscribe(transId,
                             INT_MAX,
                             primaryLine,
                             speedDialNumber,
                             i,
                             featureOptionMask );
            break;
        default:
            break;
        }
        
        
        ccBLFHandlerInitialized();
    }
}
    
static void ccBLFHandlerInitialized() 
{
    if (!isBLFHandlerRunning) {
        CC_BLF_init();
        isBLFHandlerRunning = TRUE;
    }
}
    









void sub_hndlr_stop() 
{
    static const char fname[] = "sub_hndlr_stop";
    int i;

    CCAPP_DEBUG(DEB_F_PREFIX"entering\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
    isAvailable = FALSE;
    isBLFHandlerRunning = FALSE;

    
    for (i = SPEEDDIAL_START_BUTTON_NUMBER; i <= MAX_REG_LINES; i++) {
        
        transactionIds[i - 1] = 0;
        
        blfStates[i - 1] = CC_SIP_BLF_UNKNOWN;
    }
    CC_BLF_unsubscribe_All();
}

    









static void hideBLFButtonsDisplay() 
{
    static const char fname[] = "hideBLFButtonsDisplay";
    int i;
    cc_uint32_t lineFeature = 0;

    CCAPP_DEBUG(DEB_F_PREFIX"entering\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
    displayBLFState = FALSE;
    for (i = SPEEDDIAL_START_BUTTON_NUMBER; i <= MAX_REG_LINES; i++) {
        
        config_get_line_value(CFGID_LINE_FEATURE, &lineFeature, sizeof(lineFeature), i);
            
        switch (lineFeature) {
        case cfgLineFeatureSpeedDialBLF:
            ccsnap_gen_blfFeatureEvent(CC_SIP_BLF_UNKNOWN, i);
            break;
        default:
            break;
        }
    }
}
    









static void unhideBLFButtonsDisplay()
{
    static const char fname[] = "unhideBLFButtonsDisplay";
    int i;
    cc_uint32_t lineFeature = 0;
    char speedDialNumber[MAX_LINE_NAME_SIZE] = {0};

    CCAPP_DEBUG(DEB_F_PREFIX"entering\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

    displayBLFState = TRUE;

    for (i = SPEEDDIAL_START_BUTTON_NUMBER; i <= MAX_REG_LINES; i++) {
        
        config_get_line_value(CFGID_LINE_FEATURE, &lineFeature, sizeof(lineFeature), i);
        config_get_line_string(CFGID_LINE_SPEEDDIAL_NUMBER, speedDialNumber, i, sizeof(speedDialNumber));

        switch (lineFeature) {
        case cfgLineFeatureSpeedDialBLF:
            ccsnap_gen_blfFeatureEvent(blfStates[i - 1], i);
            break;
        default:
            break;
        }
    }
}
    









void sub_hndlr_controlBLFButtons(boolean state)
{
    static const char fname[] = "sub_hndlr_controlBLFButtons";

    if (state == TRUE) {
        CCAPP_DEBUG(DEB_F_PREFIX"going to hide\n",
                    DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
        hideBLFButtonsDisplay();
    } else {
        CCAPP_DEBUG(DEB_F_PREFIX"going to unhide\n",
                    DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
        unhideBLFButtonsDisplay();
    }
}
    












void sub_hndlr_NotifyBLFStatus(int requestId, cc_blf_state_t status, int appId) 
{
    static const char fname[] = "sub_hndlr_NotifyBLFStatus";
    cc_uint32_t lineFeature = 0;
    char speedDialNumber[MAX_LINE_NAME_SIZE] = {0};


    CCAPP_DEBUG(DEB_F_PREFIX"requestId=%d, status=%d, appId=%d\n",
                DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),
                requestId, status, appId);
    if (appId == 0) {
        
    } else {
        config_get_line_value(CFGID_LINE_FEATURE, &lineFeature, sizeof(lineFeature), appId);
        config_get_line_string(CFGID_LINE_SPEEDDIAL_NUMBER, speedDialNumber, appId, sizeof(speedDialNumber));
                    
        blfStates[appId - 1] = status;
        if (displayBLFState == FALSE) {
            return; 
        }
        if ((lineFeature == cfgLineFeatureSpeedDialBLF)) {
            ccsnap_gen_blfFeatureEvent(status, appId);
        }
    }
}

