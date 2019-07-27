


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Date;
import java.util.TimeZone;

import ch.boye.httpclientandroidlib.annotation.Immutable;











@Deprecated
@Immutable
public final class DateUtils {

    


    public static final String PATTERN_RFC1123 = ch.boye.httpclientandroidlib.client.utils.DateUtils.PATTERN_RFC1123;

    


    public static final String PATTERN_RFC1036 = ch.boye.httpclientandroidlib.client.utils.DateUtils.PATTERN_RFC1036;

    



    public static final String PATTERN_ASCTIME = ch.boye.httpclientandroidlib.client.utils.DateUtils.PATTERN_ASCTIME;

    public static final TimeZone GMT = TimeZone.getTimeZone("GMT");

    










    public static Date parseDate(final String dateValue) throws DateParseException {
        return parseDate(dateValue, null, null);
    }

    









    public static Date parseDate(final String dateValue, final String[] dateFormats)
        throws DateParseException {
        return parseDate(dateValue, dateFormats, null);
    }

    













    public static Date parseDate(
        final String dateValue,
        final String[] dateFormats,
        final Date startDate
    ) throws DateParseException {
        final Date d = ch.boye.httpclientandroidlib.client.utils.DateUtils.parseDate(dateValue, dateFormats, startDate);
        if (d == null) {
            throw new DateParseException("Unable to parse the date " + dateValue);
        }
        return d;
    }

    







    public static String formatDate(final Date date) {
        return ch.boye.httpclientandroidlib.client.utils.DateUtils.formatDate(date);
    }

    












    public static String formatDate(final Date date, final String pattern) {
        return ch.boye.httpclientandroidlib.client.utils.DateUtils.formatDate(date, pattern);
    }

    
    private DateUtils() {
    }

}
