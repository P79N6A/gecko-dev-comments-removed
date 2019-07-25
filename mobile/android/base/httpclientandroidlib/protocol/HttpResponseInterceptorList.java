


























package ch.boye.httpclientandroidlib.protocol;

import java.util.List;

import ch.boye.httpclientandroidlib.HttpResponseInterceptor;








public interface HttpResponseInterceptorList {

    




    void addResponseInterceptor(HttpResponseInterceptor interceptor);

    





    void addResponseInterceptor(HttpResponseInterceptor interceptor, int index);

    




    int getResponseInterceptorCount();

    








    HttpResponseInterceptor getResponseInterceptor(int index);

    


    void clearResponseInterceptors();

    




    void removeResponseInterceptorByClass(Class clazz);

    








    void setInterceptors(List list);

}

