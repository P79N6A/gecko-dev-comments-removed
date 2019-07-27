


























package ch.boye.httpclientandroidlib.protocol;

import java.util.List;

import ch.boye.httpclientandroidlib.HttpResponseInterceptor;










@Deprecated
public interface HttpResponseInterceptorList {

    




    void addResponseInterceptor(HttpResponseInterceptor interceptor);

    





    void addResponseInterceptor(HttpResponseInterceptor interceptor, int index);

    




    int getResponseInterceptorCount();

    








    HttpResponseInterceptor getResponseInterceptor(int index);

    


    void clearResponseInterceptors();

    




    void removeResponseInterceptorByClass(Class<? extends HttpResponseInterceptor> clazz);

    








    void setInterceptors(List<?> list);

}

