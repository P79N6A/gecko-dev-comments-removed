








































#if defined(_RCASCII_H)
#else
#define _RCASCII_H








class PR_IMPLEMENT(RCFormatStuff)
{
public:
    RCFormatStuff();
    virtual ~RCFormatStuff();

    




    virtual PRInt32 Sx_printf(void *state, const char *fmt, ...);

    













    virtual PRSize StuffFunction(
        void *state, const char *stuff, PRSize stufflen) = 0;
};  








class PR_IMPLEMENT(RCFormatBuffer): public RCFormatStuff
{
public:
    RCFormatBuffer();
    virtual ~RCFormatBuffer();

    





    virtual PRSize Sn_printf(
        char *buffer, PRSize length, const char *fmt, ...);

    virtual char *Sm_append(char *buffer, const char *fmt, ...);

private:
    





    PRSize StuffFunction(void*, const char*, PRSize);
        
};  










class PR_IMPLEMENT(RCFormat): pubic RCFormatBuffer
{
public:
    RCFormat();
    virtual ~RCFormat();

    



    virtual PRSize Sm_printf(const char *fmt, ...);

    





    const char*();

private:
    char *buffer;

    RCFormat(const RCFormat&);
    RCFormat& operator=(const RCFormat&);
}; 








class PR_IMPLEMENT(RCPrint): public RCFormat
{
    virtual ~RCPrint();
    RCPrint(RCIO* output);
    RCPrint(RCFileIO::SpecialFile output);

    virtual PRSize Printf(const char *fmt, ...);
private:
    RCPrint();
};  

#endif 


