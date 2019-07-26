








#include "SkXMLParser.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "tinyxml.h"

static void walk_elem(SkXMLParser* parser, const TiXmlElement* elem)
{
    

    parser->startElement(elem->Value());

    const TiXmlAttribute* attr = elem->FirstAttribute();
    while (attr)
    {
        

        parser->addAttribute(attr->Name(), attr->Value());
        attr = attr->Next();
    }
    

    const TiXmlNode* node = elem->FirstChild();
    while (node)
    {
        if (node->ToElement())
            walk_elem(parser, node->ToElement());
        else if (node->ToText())
            parser->text(node->Value(), strlen(node->Value()));
        node = node->NextSibling();
    }

    parser->endElement(elem->Value());
}

static bool load_buf(SkXMLParser* parser, const char buf[])
{
    TiXmlDocument                   doc;

    (void)doc.Parse(buf);
    if (doc.Error())
    {
        printf("tinyxml error: <%s> row[%d] col[%d]\n", doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
        return false;
    }

    walk_elem(parser, doc.RootElement());
    return true;
}

bool SkXMLParser::parse(SkStream& stream)
{
    size_t size = stream.getLength();

    SkAutoMalloc    buffer(size + 1);
    char*           buf = (char*)buffer.get();

    stream.read(buf, size);
    buf[size] = 0;

    return load_buf(this, buf);
}

bool SkXMLParser::parse(const char doc[], size_t len)
{
    SkAutoMalloc    buffer(len + 1);
    char*           buf = (char*)buffer.get();

    memcpy(buf, doc, len);
    buf[len] = 0;

    return load_buf(this, buf);
}

void SkXMLParser::GetNativeErrorString(int error, SkString* str)
{
    if (str)
        str->set("GetNativeErrorString not implemented for TinyXml");
}
