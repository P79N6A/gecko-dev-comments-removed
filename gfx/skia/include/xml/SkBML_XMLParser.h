








#ifndef SkBML_XMLParser_DEFINED
#define SkBML_XMLParser_DEFINED

class SkStream;
class SkWStream;
class SkXMLParser;
class SkXMLWriter;

class BML_XMLParser {
public:
    

    static void Read(SkStream& s, SkXMLWriter& writer);
    

    static void Read(SkStream& s, SkWStream& output);
    

    static void Read(SkStream& s, SkXMLParser& output);
};

#endif 

