





#ifndef _MMAP_INCLUDED_
#define _MMAP_INCLUDED_





class TMMap {
public:
    TMMap(const char* fileName) : 
        fSize(-1), 
        fp(NULL),
        fBuff(0)   
    {
        if ((fp = fopen(fileName, "r")) == NULL)
            return;
        char c = getc(fp);
        fSize = 0;
        while (c != EOF) {
            fSize++;
            c = getc(fp);
        }
        if (c == EOF)
            fSize++;
        rewind(fp);
        fBuff = (char*)malloc(sizeof(char) * fSize);
        int count = 0;
        c = getc(fp);
        while (c != EOF) {
            fBuff[count++] = c;
            c = getc(fp);
        }
        fBuff[count++] = c;
    }

    char* getData() { return fBuff; }
    int   getSize() { return fSize; }

    ~TMMap() {
        if (fp != NULL)
            fclose(fp);
    }
    
private:
    int             fSize;      
    FILE *fp;
    char*           fBuff;      
};

#endif 
