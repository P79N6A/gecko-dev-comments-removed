

























package ch.boye.httpclientandroidlib.client.methods;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.client.utils.CloneUtils;
import ch.boye.httpclientandroidlib.concurrent.Cancellable;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionReleaseTrigger;
import ch.boye.httpclientandroidlib.message.AbstractHttpMessage;

@SuppressWarnings("deprecation")
public abstract class AbstractExecutionAwareRequest extends AbstractHttpMessage implements
        HttpExecutionAware, AbortableHttpRequest, Cloneable, HttpRequest {

    private final AtomicBoolean aborted;
    private final AtomicReference<Cancellable> cancellableRef;

    protected AbstractExecutionAwareRequest() {
        super();
        this.aborted = new AtomicBoolean(false);
        this.cancellableRef = new AtomicReference<Cancellable>(null);
    }

    @Deprecated
    public void setConnectionRequest(final ClientConnectionRequest connRequest) {
        setCancellable(new Cancellable() {

            public boolean cancel() {
                connRequest.abortRequest();
                return true;
            }

        });
    }

    @Deprecated
    public void setReleaseTrigger(final ConnectionReleaseTrigger releaseTrigger) {
        setCancellable(new Cancellable() {

            public boolean cancel() {
                try {
                    releaseTrigger.abortConnection();
                    return true;
                } catch (final IOException ex) {
                    return false;
                }
            }

        });
    }

    public void abort() {
        if (this.aborted.compareAndSet(false, true)) {
            final Cancellable cancellable = this.cancellableRef.getAndSet(null);
            if (cancellable != null) {
                cancellable.cancel();
            }
        }
    }

    public boolean isAborted() {
        return this.aborted.get();
    }

    


    public void setCancellable(final Cancellable cancellable) {
        if (!this.aborted.get()) {
            this.cancellableRef.set(cancellable);
        }
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        final AbstractExecutionAwareRequest clone = (AbstractExecutionAwareRequest) super.clone();
        clone.headergroup = CloneUtils.cloneObject(this.headergroup);
        clone.params = CloneUtils.cloneObject(this.params);
        return clone;
    }

    


    public void completed() {
        this.cancellableRef.set(null);
    }

    




    public void reset() {
        final Cancellable cancellable = this.cancellableRef.getAndSet(null);
        if (cancellable != null) {
            cancellable.cancel();
        }
        this.aborted.set(false);
    }

}
