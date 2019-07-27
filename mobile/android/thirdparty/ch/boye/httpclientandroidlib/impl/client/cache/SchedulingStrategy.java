

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.Closeable;






public interface SchedulingStrategy extends Closeable
{
    





    void schedule(AsynchronousValidationRequest revalidationRequest);
}
