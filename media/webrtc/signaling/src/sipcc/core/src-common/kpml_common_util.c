



#include <errno.h>

#include "cpr_types.h"
#include "cpr_memory.h"
#include "cpr_timers.h"
#include "cpr_strings.h"
#include "phntask.h"
#include "ccsip_subsmanager.h"
#include "singly_link_list.h"
#include "ccapi.h"
#include "subapi.h"
#include "kpmlmap.h"
#include "dialplanint.h"
#include "uiapi.h"
#include "debug.h"
#include "phone_debug.h"
#include "kpml_common_util.h"

static kpml_status_type_t
add_char_to_bitmask (char character, unsigned long *bitmask)
{
    kpml_status_type_t rc = KPML_STATUS_OK;

    switch (character) {
    case 'x':
    case 'X':
        *bitmask |= REGEX_0_9;
        break;
    case '1':
        *bitmask |= REGEX_1;
        break;
    case '2':
        *bitmask |= REGEX_2;
        break;
    case '3':
        *bitmask |= REGEX_3;
        break;
    case '4':
        *bitmask |= REGEX_4;
        break;
    case '5':
        *bitmask |= REGEX_5;
        break;
    case '6':
        *bitmask |= REGEX_6;
        break;
    case '7':
        *bitmask |= REGEX_7;
        break;
    case '8':
        *bitmask |= REGEX_8;
        break;
    case '9':
        *bitmask |= REGEX_9;
        break;
    case '0':
        *bitmask |= REGEX_0;
        break;
    case '*':
        *bitmask |= REGEX_STAR;
        break;
    case '#':
        *bitmask |= REGEX_POUND;
        break;
    case 'A':
    case 'a':
        *bitmask |= REGEX_A;
        break;
    case 'B':
    case 'b':
        *bitmask |= REGEX_B;
        break;
    case 'C':
    case 'c':
        *bitmask |= REGEX_C;
        break;
    case 'D':
    case 'd':
        *bitmask |= REGEX_D;
        break;
    case '+':
        *bitmask |= REGEX_PLUS;
        break;

    default:

        rc = KPML_ERROR_INVALID_VALUE;
    }

    return (rc);
}















static kpml_status_type_t
handle_range_selector (char *str, unsigned long *bitmask)
{
    static const char *fname = "handle_range_selector";
    char *char_ptr;
    int first_digit = 0;
    int last_digit = 0;
    long first_shifted = 0;
    long last_shifted = 0;
    unsigned long temp_bitmask = 0;
    char digit[2];
    kpml_status_type_t rc = KPML_STATUS_OK;
    long strtol_result;
    char *strtol_end;

    digit[1] = NUL;
    if (!str || !bitmask) {
        KPML_ERROR(KPML_F_PREFIX"Invalid input params", fname);
        return (KPML_ERROR_INTERNAL);
    }

    char_ptr = str;

    

    
    char_ptr++;
    digit[0] = *char_ptr;

    errno = 0;
    strtol_result = strtol(digit, &strtol_end, 10);

    if (errno || digit == strtol_end) {
        KPML_ERROR(KPML_F_PREFIX"digit parse error: %s", __FUNCTION__, digit);
        return (KPML_ERROR_INTERNAL);
    }

    first_digit = (int) strtol_result;

    
    char_ptr++;
    if (*char_ptr == '-') {
        char_ptr++;
        digit[0] = *char_ptr;

        errno = 0;
        strtol_result = strtol(digit, &strtol_end, 10);

        if (errno || digit == strtol_end) {
            KPML_ERROR(KPML_F_PREFIX"digit parse error: %s", __FUNCTION__, digit);
            return (KPML_ERROR_INTERNAL);
        }

        last_digit = (int) strtol_result;

        
        char_ptr++;
        if (*char_ptr != ']') {
            KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not supported.\n",
                       DEB_F_PREFIX_ARGS(KPML_INFO, fname), str);
            rc = KPML_ERROR_INVALID_VALUE;

        } else if (first_digit > last_digit) {
            KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not "
                       "supported. First digit in the range must "
                       "be greater than the second.\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname), str);
            rc = KPML_ERROR_INVALID_VALUE;
        }
    } else {
        KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not supported.\n",
                   DEB_F_PREFIX_ARGS(KPML_INFO, fname), str);
        rc = KPML_ERROR_INVALID_VALUE;
    }

    if (rc == KPML_STATUS_OK) {

        









        first_shifted = REGEX_0_9;
        last_shifted = REGEX_0_9;
        first_shifted = (first_shifted << first_digit);
        last_shifted = (last_shifted >> (9 - last_digit));
        temp_bitmask = (first_shifted & last_shifted);
        *bitmask |= temp_bitmask;
    }

    KPML_DEBUG(DEB_F_PREFIX"1st/last digit=%d/%d, bitmask=%lu, "
               "return status = %d\n", DEB_F_PREFIX_ARGS(KPML_INFO, fname), first_digit,
               last_digit, *bitmask, rc);

    return (rc);
}
















static kpml_status_type_t
handle_character_selector (char *str, unsigned long *bitmask)
{
    static const char *fname = "handle_character_selector";
    char *char_ptr;
    boolean negative_selector = FALSE;
    unsigned long temp_bitmask = 0;
    unsigned long all_digits = REGEX_0_9;
    kpml_status_type_t rc = KPML_STATUS_OK;

    char_ptr = str;

    



    if (cpr_strcasecmp(str, KPML_DEFAULT_DTMF_REGEX) == 0) {
        *bitmask |= REGEX_0_9 | REGEX_STAR | REGEX_POUND |
                    REGEX_A | REGEX_B | REGEX_C | REGEX_D;

        return (rc);
    }

    
    char_ptr++;

    if (*char_ptr == '^') {
        
        char_ptr++;
        negative_selector = TRUE;
    }


    
    while ((rc == KPML_STATUS_OK) && (*char_ptr)) {

        if (*char_ptr != ']') {
            rc = add_char_to_bitmask(*char_ptr, &temp_bitmask);
            char_ptr++;
        } else {
            
            char_ptr++;
            if (*char_ptr) {
                KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not supported.\n",
                            DEB_F_PREFIX_ARGS(KPML_INFO, fname), str);
                rc = KPML_ERROR_INVALID_VALUE;
            }
        }
    }

    if (rc == KPML_STATUS_OK) {
        if (!negative_selector) {
            *bitmask |= temp_bitmask;
        } else {
            
            temp_bitmask = ~temp_bitmask;
            *bitmask |= (temp_bitmask & all_digits);
        }
    }

    KPML_DEBUG(DEB_F_PREFIX"bitmask=%lu, return status = %d\n",
                DEB_F_PREFIX_ARGS(KPML_INFO, fname), *bitmask, rc);

    return (rc);

}



















kpml_status_type_t
kpml_parse_regex_str (char *regex_str, kpml_regex_match_t *regex_match)
{
    static const char *fname = "kpml_parse_regex_str";
    int len;
    boolean single_char = FALSE;
    kpml_status_type_t rc = KPML_STATUS_OK;

    if (!regex_str || !regex_match) {
        KPML_DEBUG(DEB_F_PREFIX"Invalid input params. \n", DEB_F_PREFIX_ARGS(KPML_INFO, fname));
        return (KPML_ERROR_INTERNAL);
    }

    regex_match->num_digits = 1;
    regex_match->u.single_digit_bitmask = 0;

    













    len = strlen(regex_str);

    if (len == 1) {
        single_char = TRUE;
    } else {
        if (regex_str[1] == '{') {
            if ((strncmp(&regex_str[1], "{1}", 3) == 0) ||
                (strncmp(&regex_str[1], "{1,1}", 5) == 0)) {
                single_char = TRUE;
            } else {

                KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not supported.\n",
                           DEB_F_PREFIX_ARGS(KPML_INFO, fname), regex_str);
                return (KPML_ERROR_INVALID_VALUE);
            }
        }
    }

    if (single_char) {
        
        regex_match->num_digits = 1;
        rc = add_char_to_bitmask(regex_str[0],
                                 &(regex_match->u.single_digit_bitmask));

    } else if (regex_str[0] == '[') {

        if (strchr(regex_str, '-')) {
            
            rc = handle_range_selector(regex_str,
                                       &(regex_match->u.single_digit_bitmask));
        } else {

            
            rc = handle_character_selector(regex_str,
                                           &(regex_match->u.single_digit_bitmask));
        }


    } else {
        
        KPML_DEBUG(DEB_F_PREFIX"The Regex format %s is not supported.\n",
                DEB_F_PREFIX_ARGS(KPML_INFO, fname), regex_str);
        rc = KPML_ERROR_INVALID_VALUE;
    }

    return (rc);
}
