


























package ch.boye.httpclientandroidlib.client.methods;

import ch.boye.httpclientandroidlib.concurrent.Cancellable;







public interface HttpExecutionAware {

    boolean isAborted();

    


    void setCancellable(Cancellable cancellable);

}

