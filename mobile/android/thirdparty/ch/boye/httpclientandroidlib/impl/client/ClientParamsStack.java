


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.params.AbstractHttpParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;






































@NotThreadSafe
@Deprecated
public class ClientParamsStack extends AbstractHttpParams {

    
    protected final HttpParams applicationParams;

    
    protected final HttpParams clientParams;

    
    protected final HttpParams requestParams;

    
    protected final HttpParams overrideParams;


    









    public ClientParamsStack(final HttpParams aparams, final HttpParams cparams,
                             final HttpParams rparams, final HttpParams oparams) {
        applicationParams = aparams;
        clientParams      = cparams;
        requestParams     = rparams;
        overrideParams    = oparams;
    }


    






    public ClientParamsStack(final ClientParamsStack stack) {
        this(stack.getApplicationParams(),
             stack.getClientParams(),
             stack.getRequestParams(),
             stack.getOverrideParams());
    }


    












    public ClientParamsStack(final ClientParamsStack stack,
                             final HttpParams aparams, final HttpParams cparams,
                             final HttpParams rparams, final HttpParams oparams) {
        this((aparams != null) ? aparams : stack.getApplicationParams(),
             (cparams != null) ? cparams : stack.getClientParams(),
             (rparams != null) ? rparams : stack.getRequestParams(),
             (oparams != null) ? oparams : stack.getOverrideParams());
    }


    




    public final HttpParams getApplicationParams() {
        return applicationParams;
    }

    




    public final HttpParams getClientParams() {
        return clientParams;
    }

    




    public final HttpParams getRequestParams() {
        return requestParams;
    }

    




    public final HttpParams getOverrideParams() {
        return overrideParams;
    }


    








    public Object getParameter(final String name) {
        Args.notNull(name, "Parameter name");

        Object result = null;

        if (overrideParams != null) {
            result = overrideParams.getParameter(name);
        }
        if ((result == null) && (requestParams != null)) {
            result = requestParams.getParameter(name);
        }
        if ((result == null) && (clientParams != null)) {
            result = clientParams.getParameter(name);
        }
        if ((result == null) && (applicationParams != null)) {
            result = applicationParams.getParameter(name);
        }
        return result;
    }

    












    public HttpParams setParameter(final String name, final Object value)
        throws UnsupportedOperationException {

        throw new UnsupportedOperationException
            ("Setting parameters in a stack is not supported.");
    }


    











    public boolean removeParameter(final String name) {
        throw new UnsupportedOperationException
        ("Removing parameters in a stack is not supported.");
    }


    












    public HttpParams copy() {
        return this;
    }


}
