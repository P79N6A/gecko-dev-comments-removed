


























package ch.boye.httpclientandroidlib.protocol;

import java.util.List;

import ch.boye.httpclientandroidlib.HttpRequestInterceptor;








public interface HttpRequestInterceptorList {

    




    void addRequestInterceptor(HttpRequestInterceptor interceptor);

    





    void addRequestInterceptor(HttpRequestInterceptor interceptor, int index);

    




    int getRequestInterceptorCount();

    








    HttpRequestInterceptor getRequestInterceptor(int index);

    


    void clearRequestInterceptors();

    




    void removeRequestInterceptorByClass(Class clazz);

    








    void setInterceptors(List list);

}

