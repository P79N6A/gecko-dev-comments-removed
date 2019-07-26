






































#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "xml_defs.h"
#include "logger.h"
#include "logmsg.h"
#include "util_parse.h"
#include "debug.h"
#include "phone_types.h"
#include "regmgrapi.h"


#include "upgrade.h"
#include "dialplan.h"

extern char DirectoryBuffer[DIALPLAN_MAX_SIZE];







const char *tone_names[] = {
    "Bellcore-Inside",
    "Bellcore-Outside",
    "Bellcore-Busy",
    "Bellcore-Alerting",
    "Bellcore-BusyVerify",
    "Bellcore-Stutter",
    "Bellcore-MsgWaiting",
    "Bellcore-Reorder",
    "Bellcore-CallWaiting",
    "Bellcore-Cw2",
    "Bellcore-Cw3",
    "Bellcore-Cw4",
    "Bellcore-Hold",
    "Bellcore-Confirmation",
    "Bellcore-Permanent",
    "Bellcore-Reminder",
    "Bellcore-None",
    "Cisco-ZipZip",
    "Cisco-Zip",
    "Cisco-BeepBonk"
};

static struct DialTemplate *basetemplate;
char DialTemplateFile[MAX_TEMPLATE_LENGTH];
char g_dp_version_stamp[MAX_DP_VERSION_STAMP_LEN];










static void
addbytes (char **output, int *outlen, const char *input, int inlen)
{
    char *target = *output;

    if (inlen == -1) {
        inlen = strlen(input);
    }
    if (inlen >= *outlen) {
        inlen = *outlen - 1;
    }
    memcpy(target, input, inlen);
    target += inlen;
    *outlen += inlen;
    *output = target;
    


    *target = '\0';
}











static boolean
poundDialingEnabled (void)
{
    if (sip_regmgr_get_cc_mode(1) == REG_MODE_NON_CCM) {
        


        return (TRUE);
    } else {
        return (FALSE);
    }
}












boolean
isDialedDigit (char input)
{
    boolean result = FALSE;

    if (!isdigit(input)) {
        if ((input == '*') || (input == '+') || ((input == '#') && (!poundDialingEnabled()))) {
            result = TRUE;
        }
    } else {
        result = TRUE;
    }

    return (result);
}














static boolean
MatchLineNumber (const line_t templateLine, const line_t line)
{
    
    return (boolean) ((templateLine == line) || (templateLine == 0));
}




















DialMatchAction
MatchDialTemplate (const char *pattern,
                   const line_t line,
                   int *timeout,
                   char *rewrite,
                   int rewritelen,
                   RouteMode *pRouteMode,
                   vcm_tones_t *pTone)
{
    DialMatchAction result = DIAL_NOMATCH;
    struct DialTemplate *ptempl = basetemplate;
    struct DialTemplate *pbestmatch = NULL;
    boolean bestmatch_dialnow = FALSE;
    int best_comma_count = 0;
    DialMatchAction partialmatch_type = DIAL_NOMATCH;
    boolean partialmatch = FALSE;

    int matchlen = 0;
    int partialmatchlen = 0;
    int givedialtone = 0;
    int comma_counter = 0;

    




    if (rewrite != NULL) {
        char *output = rewrite;
        int room = rewritelen;

        addbytes(&output, &room, pattern, -1);
    }

    





    if (ptempl == NULL) {
        if (strchr(pattern, '#') && (poundDialingEnabled())) {
            return DIAL_IMMEDIATELY;
        } else {
            return DIAL_NOMATCH;
        }
    }

    



    while (ptempl != NULL) {
        if (MatchLineNumber(ptempl->line, line)) {
            char *pinput = (char *) pattern;
            char *pmatch = ptempl->pattern;
            int thismatchlen = 0;
            DialMatchAction thismatch = DIAL_FULLMATCH;
            char *subs[MAX_SUBTITUTIONS];
            int subslen[MAX_SUBTITUTIONS];
            int subscount = -1;
            boolean dialnow = FALSE;

            while (*pinput) {
                int idx;

                




                if (pmatch[0] == ',') {
                    comma_counter++;
                }

                


                while (pmatch[0] == ',') {
                    pmatch++;
                }
                



                if (((pmatch[0] == '.') && isDialedDigit(pinput[0])) ||
                     (pmatch[0] == '*')) {
                    















                    if (subscount < (MAX_SUBTITUTIONS - 1)) {
                        subscount++;
                        subs[subscount] = pinput;
                        subslen[subscount] = 1;
                    }
                    if (pmatch[0] == '.') {
                        thismatch = DIAL_FULLPATTERN;
                        



                        while (isdigit(pinput[1]) && (pmatch[1] == '.')) {
                            pinput++;
                            pmatch++;
                            subslen[subscount]++;
                        }
                    } else {
                        thismatch = DIAL_WILDPATTERN;
                        






                        if (pmatch[1] == DIAL_ESCAPE) {
                            idx = 2;
                        } else {
                            idx = 1;
                        }

                        



                        if ((pinput[0] == '#') && (poundDialingEnabled())) {
                            dialnow = TRUE;
                        } else {
                            while ((pinput[1] != '\0') &&
                                   (pinput[1] != pmatch[idx])) {
                                


                                if ((pinput[1] == '#') &&
                                    (poundDialingEnabled())) {
                                    break;
                                }
                                pinput++;
                                subslen[subscount]++;
                            }
                        }
                    }
                    


                } else {
                    





                    if ((pmatch[0] == DIAL_ESCAPE) && (pmatch[1] != '\0')) {
                        pmatch++;
                    }

                    if (pmatch[0] != pinput[0]) {
                        




                        if ((pinput[0] == '#') && (poundDialingEnabled())) {
                            dialnow = TRUE;
                            break;
                        }
                        


                        thismatchlen = -1;
                        thismatch = DIAL_NOMATCH;
                        break;

                    } else {
                        


                        thismatchlen++;
                    }
                }
                pmatch++;
                pinput++;
            }

            








            if ((*pinput == NUL) || (dialnow)) {
                if ((thismatchlen > partialmatchlen) ||
                    ((thismatchlen == partialmatchlen) &&
                     (thismatch > partialmatch_type))) {
                    partialmatch_type = thismatch;
                    partialmatchlen = thismatchlen;
                    pbestmatch = ptempl;
                    partialmatch = TRUE;
                    bestmatch_dialnow = dialnow;
                    best_comma_count = comma_counter;
                    result = DIAL_NOMATCH;
                }
            }

            













            if (pmatch[0] == '\0') {
                


                if ((thismatchlen > matchlen) ||
                    ((thismatchlen == matchlen) && (thismatch > result)) ||
                    ((thismatch == DIAL_WILDPATTERN) &&
                     ((result == DIAL_NOMATCH) && (partialmatch == FALSE)))) {
                    


                    pbestmatch = ptempl;
                    bestmatch_dialnow = dialnow;
                    matchlen = thismatchlen;
                    result = thismatch;
                    











                    if (rewrite != NULL) {
                        int dotindex = -1;
                        int dotsleft = 0;
                        char *output = rewrite;
                        int room = rewritelen;
                        char *prewrite = pbestmatch->rewrite;

                        if ((prewrite == NULL) || (prewrite[0] == '\0')) {
                            


                            addbytes(&output, &room, pattern, -1);
                        } else {
                            while (prewrite[0] != '\0') {
                                if (prewrite[0] == '.') {
                                    


                                    while ((dotsleft == 0) &&
                                           (dotindex < subscount)) {
                                        dotindex++;
                                        dotsleft = subslen[dotindex];
                                    }
                                    if (dotsleft > 0) {
                                        addbytes(&output, &room,
                                                 subs[dotindex] +
                                                 subslen[dotindex] - dotsleft,
                                                 1);
                                        dotsleft--;
                                    }
                                } else if (prewrite[0] == '%') {
                                    int idx = prewrite[1] - '1';

                                    prewrite++; 
                                    if ((prewrite[0] == 's') ||
                                        (prewrite[0] == '0')) {
                                        


                                        addbytes(&output, &room, pattern, -1);
                                    } else if ((idx >= 0) &&
                                               (idx <= subscount)) {
                                        



                                        addbytes(&output, &room, subs[idx],
                                                 subslen[idx]);
                                    } else if (prewrite[0]) {
                                        



                                        addbytes(&output, &room, prewrite, 1);
                                    }
                                } else {
                                    


                                    addbytes(&output, &room, prewrite, 1);
                                }
                                


                                if (prewrite[0]) {
                                    prewrite++;
                                }
                            }
                        }
                        


                        switch (pbestmatch->userMode) {
                        case UserPhone:
                            if (*(output - 1) == '>') {
                                --room;
                                --output;
                                addbytes(&output, &room, ";user=phone>", -1);
                            } else {
                                addbytes(&output, &room, ";user=phone", -1);
                            }
                            break;
                        case UserIP:
                            if (*(output - 1) == '>') {
                                --room;
                                --output;
                                addbytes(&output, &room, ";user=ip>", -1);
                            } else {
                                addbytes(&output, &room, ";user=ip", -1);
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
            } else if (pmatch[0] == ',') {
                givedialtone = 1;
            }

            


            if (thismatchlen > matchlen) {
                matchlen = thismatchlen;
            }
        }
        


        ptempl = ptempl->next;
        comma_counter = 0;
    }
    


    switch (result) {
    case DIAL_FULLPATTERN:
    case DIAL_FULLMATCH:
        givedialtone = 0;
        
    case DIAL_WILDPATTERN:
        if (timeout != NULL) {
            *timeout = pbestmatch->timeout;
        }
        if (pRouteMode != NULL) {
            *pRouteMode = pbestmatch->routeMode;
        }
        break;

    default:
        




        if (partialmatch) {
            if ((timeout != NULL) && (*timeout == 0)) {
                *timeout = DIAL_TIMEOUT;
            }
        } else if ((*(pattern + strlen(pattern) - 1) == '#') &&
                   (poundDialingEnabled())) {
            






            result = DIAL_IMMEDIATELY;
        }
        break;
    }

    


    if (bestmatch_dialnow) {
        




        if (!((poundDialingEnabled()) && (strchr(pattern, '#')) &&
              partialmatch)) {
            result = DIAL_IMMEDIATELY;
            if (timeout != NULL) {
                *timeout = 0;
            }
        }
    }

    if (givedialtone) {
        




        if (pTone != NULL) {
            *pTone = VCM_DEFAULT_TONE;
            if (pbestmatch != NULL) {
                if (best_comma_count < pbestmatch->tones_defined) {
                    *pTone = pbestmatch->tone[best_comma_count];
                }
            }
        }
        result = DIAL_GIVETONE;
    }

    return result;   
}













void
InitDialPlan (boolean wipe)
{
}












void
FreeDialTemplates (void)
{
    struct DialTemplate *pnext;

    while (basetemplate != NULL) {
        pnext = basetemplate->next;
        cpr_free(basetemplate);
        basetemplate = pnext;
    }
}


















static void
AddDialTemplate (const char *pattern, const line_t line,
                 int timeout, UserMode userMode,
                 const char *rewrite, RouteMode routeMode,
                 vcm_tones_t tone[MAX_TONES], int tones_defined)
{
    struct DialTemplate *pnewtemplate;
    int patternlen = strlen(pattern);
    int counter;

    pnewtemplate = (struct DialTemplate *)
        cpr_malloc(sizeof(struct DialTemplate) + patternlen + strlen(rewrite) +
                   2);
    if (pnewtemplate != NULL) {
        pnewtemplate->next = NULL;
        pnewtemplate->pattern = (char *) (pnewtemplate + 1);
        strcpy(pnewtemplate->pattern, (char *) pattern);
        pnewtemplate->rewrite = pnewtemplate->pattern + patternlen + 1;
        strcpy(pnewtemplate->rewrite, (char *) rewrite);
        pnewtemplate->line = line;
        pnewtemplate->timeout = timeout;
        pnewtemplate->userMode = userMode;
        pnewtemplate->routeMode = routeMode;
        pnewtemplate->tones_defined = tones_defined;
        for (counter = 0; counter < MAX_TONES; counter++) {
            pnewtemplate->tone[counter] = tone[counter];
        }

        


        if (basetemplate == NULL) {
            basetemplate = pnewtemplate;
        } else {
            struct DialTemplate *base = basetemplate;

            while (base->next != NULL) {
                base = base->next;
            }
            base->next = pnewtemplate;
        }
    }
}











int32_t
show_dialplan_cmd (int32_t argc, const char *argv[])
{
    struct DialTemplate *pTemp;
    char umode[32], rmode[32];
    char line_str[32];
    uint32_t idx = 1;
    int32_t counter = 0;

    debugif_printf("Dialplan is....\n");
    debugif_printf("Dialplan version: %s\n", g_dp_version_stamp);

    pTemp = basetemplate;
    if (basetemplate == NULL) {
        debugif_printf("EMPTY\n");
        return 0;
    }
    while (pTemp != NULL) {
        switch (pTemp->routeMode) {
        case RouteEmergency:
            strcpy(rmode, "Emergency");
            break;
        case RouteFQDN:
            strcpy(rmode, "FQDN");
            break;
        default:
            strcpy(rmode, "Default");
            break;
        }

        switch (pTemp->userMode) {
        case UserPhone:
            strcpy(umode, "Phone");
            break;
        case UserIP:
            strcpy(umode, "IP");
            break;
        default:
            strcpy(umode, "Unspecified");
            break;
        }

        if (pTemp->line == 0) {
            sprintf(line_str, "All");
        } else {
            sprintf(line_str, "%d", pTemp->line);
        }
        debugif_printf("%02d. Pattern: %s  Rewrite: %s Line: %s\n"
                       "    Timeout: %04d   UserMode: %s  RouteMode: %s\n",
                       idx, pTemp->pattern, pTemp->rewrite, line_str,
                       pTemp->timeout, umode, rmode);
        for (counter = 0; counter < pTemp->tones_defined; counter++) {
            debugif_printf("    Tone %d: %s\n", counter + 1,
                           tone_names[(int) (pTemp->tone[counter])]);
        }
        pTemp = pTemp->next;
        idx++;

    }
    return (0);
}






















static int
ParseDialEntry (char **parseptr)
{
    char dialtemplate[MAX_TEMPLATE_LENGTH];
    char rewrite[MAX_TEMPLATE_LENGTH];
    int timeout = DIAL_TIMEOUT; 
    unsigned char line = 0;     
    int counter = 0;
    int tone_counter = 0;
    UserMode usermode = UserUnspec;
    RouteMode routeMode = RouteDefault;
    vcm_tones_t tone[MAX_TONES];
    ParseDialState state = STATE_ANY;

    dialtemplate[0] = '\0';
    rewrite[0] = '\0';
    
    for (counter = 0; counter < MAX_TONES; counter++) {
        tone[counter] = VCM_DEFAULT_TONE;
    }

    for (;;) {
        char buffer[64];
        XMLToken tok;

        tok = parse_xml_tokens(parseptr, buffer, sizeof(buffer));
        switch (tok) {
        case TOK_KEYWORD:
            if (state != STATE_ANY) {
                if ((cpr_strcasecmp(buffer, "TEMPLATE") == 0) &&
                    (state == STATE_END_TAG_STARTED)) {
                    state = STATE_END_TAG_FOUND;
                } else {
                    return 1;
                }
            } else if (cpr_strcasecmp(buffer, "MATCH") == 0) {
                state = STATE_GOT_MATCH;
            } else if (cpr_strcasecmp(buffer, "LINE") == 0) {
                state = STATE_GOT_LINE;
            } else if (cpr_strcasecmp(buffer, "TIMEOUT") == 0) {
                state = STATE_GOT_TIMEOUT;
            } else if (cpr_strcasecmp(buffer, "USER") == 0) {
                state = STATE_GOT_USER;
            } else if (cpr_strcasecmp(buffer, "REWRITE") == 0) {
                state = STATE_GOT_REWRITE;
            } else if (cpr_strcasecmp(buffer, "ROUTE") == 0) {
                state = STATE_GOT_ROUTE;
            } else if (cpr_strcasecmp(buffer, "TONE") == 0) {
                state = STATE_GOT_TONE;
            } else {
                return 1;
            }
            break;

        case TOK_EQ:
            switch (state) {
            case STATE_GOT_MATCH:
                state = STATE_GOT_MATCH_EQ;
                break;
            case STATE_GOT_LINE:
                state = STATE_GOT_LINE_EQ;
                break;
            case STATE_GOT_TIMEOUT:
                state = STATE_GOT_TIMEOUT_EQ;
                break;
            case STATE_GOT_USER:
                state = STATE_GOT_USER_EQ;
                break;
            case STATE_GOT_REWRITE:
                state = STATE_GOT_REWRITE_EQ;
                break;
            case STATE_GOT_ROUTE:
                state = STATE_GOT_ROUTE_EQ;
                break;
            case STATE_GOT_TONE:
                state = STATE_GOT_TONE_EQ;
                break;
                
            default:
                return 1;
            }
            break;

        case TOK_STR:
            switch (state) {
            case STATE_GOT_MATCH_EQ:
                sstrncpy(dialtemplate, buffer, sizeof(dialtemplate));
                break;

            case STATE_GOT_LINE_EQ:
                line = (unsigned char) atoi(buffer);
                break;

            case STATE_GOT_TIMEOUT_EQ:
                timeout = atoi(buffer);

                if (timeout < 0) {
                    return 1;
                }
                break;

            case STATE_GOT_USER_EQ:
                if (cpr_strcasecmp(buffer, "PHONE") == 0) {
                    usermode = UserPhone;
                } else if (cpr_strcasecmp(buffer, "IP") == 0) {
                    usermode = UserIP;
                } else {
                    return 1;
                }
                break;

            case STATE_GOT_ROUTE_EQ:
                if (cpr_strcasecmp(buffer, "DEFAULT") == 0) {
                    routeMode = RouteDefault;
                } else if (cpr_strcasecmp(buffer, "EMERGENCY") == 0) {
                    routeMode = RouteEmergency;
                } else if (cpr_strcasecmp(buffer, "FQDN") == 0) {
                    routeMode = RouteFQDN;
                } else {
                    return 1;
                }
                break;

            case STATE_GOT_TONE_EQ:
                if (tone_counter < MAX_TONES) {
                    
                    if (*buffer == '\000') {
                        tone[tone_counter] = VCM_DEFAULT_TONE;
                    } else {
                        for (counter = 0; counter < VCM_MAX_DIALTONE; counter++) {
                            if (cpr_strcasecmp(buffer, tone_names[counter]) ==
                                0) {
                                tone[tone_counter] = (vcm_tones_t) counter;
                                break;
                            }
                        }
                        
                        if (counter == VCM_MAX_DIALTONE) {
                            return 1;
                        }
                    }
                    tone_counter++;
                    
                } else {
                    return 1;
                }
                break;

            case STATE_GOT_REWRITE_EQ:
                sstrncpy(rewrite, buffer, sizeof(rewrite));
                break;

            default:
                return 1;
            }
            state = STATE_ANY;
            break;

        case TOK_EMPTYBRACKET: 
            AddDialTemplate(dialtemplate, line, timeout, usermode, rewrite,
                            routeMode, tone, tone_counter);
            if (state == STATE_ANY) {
                return 0;
            }
            


            return 1;

        case TOK_RBRACKET:     
            if (state == STATE_ANY) {
                state = STATE_START_TAG_COMPLETED;
            } else if (state == STATE_END_TAG_FOUND) {
                AddDialTemplate(dialtemplate, line, timeout, usermode, rewrite,
                                routeMode, tone, tone_counter);
                return 0;
            } else {
                return 1;
            }
            break;

        case TOK_ENDLBRACKET:  
            if (state == STATE_START_TAG_COMPLETED) {
                state = STATE_END_TAG_STARTED;
            } else {
                return 1;
            }
            break;


        default:
            


            return 1;
        }
    }
}













static int
ParseDialVersion (char **parseptr)
{
    ParseDialState state = STATE_ANY;
    char version_stamp[MAX_DP_VERSION_STAMP_LEN] = { 0 };
    int len;

    for (;;) {
        char buffer[MAX_DP_VERSION_STAMP_LEN];
        XMLToken tok;

        tok = parse_xml_tokens(parseptr, buffer, sizeof(buffer));
        switch (tok) {
        case TOK_KEYWORD:
            switch (state) {
            case STATE_START_TAG_COMPLETED:
                


                memcpy(version_stamp, buffer, MAX_DP_VERSION_STAMP_LEN);
                break;
            case STATE_END_TAG_STARTED:
                if (cpr_strcasecmp(buffer, "versionStamp") != 0) {
                    return 1;
                }
                state = STATE_END_TAG_FOUND;
                break;
                
            default:
                break;
            }
            break;

        case TOK_RBRACKET:     
            if (state == STATE_ANY) {
                state = STATE_START_TAG_COMPLETED;
            } else if (state == STATE_END_TAG_FOUND) {
                
                len = strlen(version_stamp);
                if (len <= 2)
                {
                    CCAPP_ERROR("ParseDialVersion(): Version length [%d] is way too small", len);
                    return (1);                                
                }
                
                memset(g_dp_version_stamp, 0, MAX_DP_VERSION_STAMP_LEN);
                if ((version_stamp[0] == '{')
                    && (version_stamp[len - 1] == '}')) {
                    memcpy(g_dp_version_stamp, (version_stamp + 1), (len - 2));
                } else {
                    memcpy(g_dp_version_stamp, version_stamp, len);
                }
                return 0;
            } else {
                return 1;
            }
            break;

        case TOK_ENDLBRACKET:  
            if (state == STATE_START_TAG_COMPLETED) {
                state = STATE_END_TAG_STARTED;
            } else {
                return 1;
            }
            break;

        default:
            


            return 1;
        }
    }
}













boolean
ParseDialTemplate (char *parseptr)
{
    char buffer[MAX_TEMPLATE_LENGTH];
    XMLToken tok;
    int LookKey;
    int LookEndKey;
    int errors = 0;
    int insideDialPlan = 0;

    LookKey = 0;
    LookEndKey = 0;
    FreeDialTemplates();
    



    g_dp_version_stamp[0] = 0;


    if (parseptr == NULL) {
        debugif_printf("ParseDialTempate(): parseptr=NULL Returning.\n");
        return (FALSE);
    }

    while ((tok =
            parse_xml_tokens(&parseptr, buffer, sizeof(buffer))) != TOK_EOF) {
        if (LookEndKey) {
            if (tok == TOK_RBRACKET) {
                LookEndKey = 0;
            } else if ((tok == TOK_KEYWORD)
                       && !cpr_strcasecmp(buffer, "DIALTEMPLATE")) {
                insideDialPlan = 0;
            }
        } else if (tok == TOK_LBRACKET) {
            LookKey = 1;
        } else if ((LookKey != 0) && (tok == TOK_KEYWORD)
                   && !cpr_strcasecmp(buffer, "DIALTEMPLATE")) {
            insideDialPlan = 1;
        } else if ((LookKey != 0) && (tok == TOK_KEYWORD)
                   && !strcmp(buffer, "TEMPLATE")) {
            if (insideDialPlan) {
                errors += ParseDialEntry(&parseptr);
            } else {
                errors++;
            }
        } else if ((LookKey != 0) && (tok == TOK_KEYWORD)
                   && !cpr_strcasecmp(buffer, "versionStamp")) {
            if (insideDialPlan) {
                
                errors += ParseDialVersion(&parseptr);
            } else {
                errors++;
            }
        } else if (tok == TOK_ENDLBRACKET) {
            LookEndKey = 1;
        } else {
            LookKey = 0;
        }
    }
    


    log_clear(LOG_CFG_PARSE_DIAL);
    if (errors != 0) {
        log_msg(LOG_CFG_PARSE_DIAL, errors, DialTemplateFile);
        return (FALSE);
    }

    return (TRUE);
}

