

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.HttpResponse;











public interface ConnectionBackoffStrategy {

    







    boolean shouldBackoff(Throwable t);

    









    boolean shouldBackoff(HttpResponse resp);
}
