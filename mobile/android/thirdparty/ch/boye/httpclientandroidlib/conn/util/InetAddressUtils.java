


























package ch.boye.httpclientandroidlib.conn.util;

import java.util.regex.Pattern;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class InetAddressUtils {

    private InetAddressUtils() {
    }

    private static final String IPV4_BASIC_PATTERN_STRING =
            "(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}" + 
             "([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])"; 

    private static final Pattern IPV4_PATTERN =
        Pattern.compile("^" + IPV4_BASIC_PATTERN_STRING + "$");

    private static final Pattern IPV4_MAPPED_IPV6_PATTERN = 
            Pattern.compile("^::[fF]{4}:" + IPV4_BASIC_PATTERN_STRING + "$");

    private static final Pattern IPV6_STD_PATTERN =
        Pattern.compile(
                "^[0-9a-fA-F]{1,4}(:[0-9a-fA-F]{1,4}){7}$");

    private static final Pattern IPV6_HEX_COMPRESSED_PATTERN =
        Pattern.compile(
                "^(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)" + 
                 "::" +
                 "(([0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4}){0,5})?)$"); 

    


    private static final char COLON_CHAR = ':';

    
    private static final int MAX_COLON_COUNT = 7;

    





    public static boolean isIPv4Address(final String input) {
        return IPV4_PATTERN.matcher(input).matches();
    }

    public static boolean isIPv4MappedIPv64Address(final String input) {
        return IPV4_MAPPED_IPV6_PATTERN.matcher(input).matches();
    }

    





    public static boolean isIPv6StdAddress(final String input) {
        return IPV6_STD_PATTERN.matcher(input).matches();
    }

    





    public static boolean isIPv6HexCompressedAddress(final String input) {
        int colonCount = 0;
        for(int i = 0; i < input.length(); i++) {
            if (input.charAt(i) == COLON_CHAR) {
                colonCount++;
            }
        }
        return  colonCount <= MAX_COLON_COUNT && IPV6_HEX_COMPRESSED_PATTERN.matcher(input).matches();
    }

    





    public static boolean isIPv6Address(final String input) {
        return isIPv6StdAddress(input) || isIPv6HexCompressedAddress(input);
    }

}
