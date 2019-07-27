


























package ch.boye.httpclientandroidlib.protocol;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;






@ThreadSafe
public class HttpDateGenerator {

    
    public static final
        String PATTERN_RFC1123 = "EEE, dd MMM yyyy HH:mm:ss zzz";

    
    public static final TimeZone GMT = TimeZone.getTimeZone("GMT");

    @GuardedBy("this")
    private final DateFormat dateformat;
    @GuardedBy("this")
    private long dateAsLong = 0L;
    @GuardedBy("this")
    private String dateAsText = null;

    public HttpDateGenerator() {
        super();
        this.dateformat = new SimpleDateFormat(PATTERN_RFC1123, Locale.US);
        this.dateformat.setTimeZone(GMT);
    }

    public synchronized String getCurrentDate() {
        final long now = System.currentTimeMillis();
        if (now - this.dateAsLong > 1000) {
            
            this.dateAsText = this.dateformat.format(new Date(now));
            this.dateAsLong = now;
        }
        return this.dateAsText;
    }

}
