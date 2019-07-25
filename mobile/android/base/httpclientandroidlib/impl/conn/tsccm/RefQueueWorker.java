

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;












@Deprecated
public class RefQueueWorker implements Runnable {

    
    protected final ReferenceQueue<?> refQueue;

    
    protected final RefQueueHandler refHandler;


    



    protected volatile Thread workerThread;


    





    public RefQueueWorker(ReferenceQueue<?> queue, RefQueueHandler handler) {
        if (queue == null) {
            throw new IllegalArgumentException("Queue must not be null.");
        }
        if (handler == null) {
            throw new IllegalArgumentException("Handler must not be null.");
        }

        refQueue   = queue;
        refHandler = handler;
    }


    





    public void run() {

        if (this.workerThread == null) {
            this.workerThread = Thread.currentThread();
        }

        while (this.workerThread == Thread.currentThread()) {
            try {
                
                Reference<?> ref = refQueue.remove();
                refHandler.handleReference(ref);
            } catch (InterruptedException ignore) {
            }
        }
    }


    



    public void shutdown() {
        Thread wt = this.workerThread;
        if (wt != null) {
            this.workerThread = null; 
            wt.interrupt();
        }
    }


    




    @Override
    public String toString() {
        return "RefQueueWorker::" + this.workerThread;
    }

} 

