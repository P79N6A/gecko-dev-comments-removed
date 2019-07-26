




#ifndef TRANSFRMX_XML_EVENT_HANDLER_H
#define TRANSFRMX_XML_EVENT_HANDLER_H

#include "txCore.h"
#include "nsIAtom.h"

#define kTXNameSpaceURI "http://www.mozilla.org/TransforMiix"
#define kTXWrapper "transformiix:result"

class txOutputFormat;
class nsIDOMDocument;






class txAXMLEventHandler
{
public:
    virtual ~txAXMLEventHandler() {}

    








    virtual nsresult attribute(nsIAtom* aPrefix, nsIAtom* aLocalName,
                               nsIAtom* aLowercaseLocalName, int32_t aNsID,
                               const nsString& aValue) = 0;

    







    virtual nsresult attribute(nsIAtom* aPrefix,
                               const nsSubstring& aLocalName,
                               const int32_t aNsID,
                               const nsString& aValue) = 0;

    





    virtual nsresult characters(const nsSubstring& aData, bool aDOE) = 0;

    




    virtual nsresult comment(const nsString& aData) = 0;

    



    virtual nsresult endDocument(nsresult aResult) = 0;

    


    virtual nsresult endElement() = 0;

    





    virtual nsresult processingInstruction(const nsString& aTarget, 
                                           const nsString& aData) = 0;

    


    virtual nsresult startDocument() = 0;

    







    virtual nsresult startElement(nsIAtom* aPrefix,
                                  nsIAtom* aLocalName,
                                  nsIAtom* aLowercaseLocalName,
                                  int32_t aNsID) = 0;

    







    virtual nsresult startElement(nsIAtom* aPrefix,
                                  const nsSubstring& aLocalName,
                                  const int32_t aNsID) = 0;
};

#define TX_DECL_TXAXMLEVENTHANDLER                                           \
    virtual nsresult attribute(nsIAtom* aPrefix, nsIAtom* aLocalName,        \
                               nsIAtom* aLowercaseLocalName, int32_t aNsID,  \
                               const nsString& aValue);                      \
    virtual nsresult attribute(nsIAtom* aPrefix,                             \
                               const nsSubstring& aLocalName,                \
                               const int32_t aNsID,                          \
                               const nsString& aValue);                      \
    virtual nsresult characters(const nsSubstring& aData, bool aDOE);      \
    virtual nsresult comment(const nsString& aData);                         \
    virtual nsresult endDocument(nsresult aResult = NS_OK);                  \
    virtual nsresult endElement();                                           \
    virtual nsresult processingInstruction(const nsString& aTarget,          \
                                           const nsString& aData);           \
    virtual nsresult startDocument();                                        \
    virtual nsresult startElement(nsIAtom* aPrefix,                          \
                                  nsIAtom* aLocalName,                       \
                                  nsIAtom* aLowercaseLocalName,              \
                                  int32_t aNsID);                            \
    virtual nsresult startElement(nsIAtom* aPrefix,                          \
                                  const nsSubstring& aName,                  \
                                  const int32_t aNsID);


class txAOutputXMLEventHandler : public txAXMLEventHandler
{
public:
    




    virtual void getOutputDocument(nsIDOMDocument** aDocument) = 0;
};

#define TX_DECL_TXAOUTPUTXMLEVENTHANDLER                        \
    virtual void getOutputDocument(nsIDOMDocument** aDocument);




class txAOutputHandlerFactory
{
public:
    virtual ~txAOutputHandlerFactory() {}

    




    virtual nsresult
    createHandlerWith(txOutputFormat* aFormat,
                      txAXMLEventHandler** aHandler) = 0;

    







    virtual nsresult
    createHandlerWith(txOutputFormat* aFormat,
                      const nsSubstring& aName,
                      int32_t aNsID,
                      txAXMLEventHandler** aHandler) = 0;
};

#define TX_DECL_TXAOUTPUTHANDLERFACTORY                        \
    nsresult createHandlerWith(txOutputFormat* aFormat,        \
                               txAXMLEventHandler** aHandler); \
    nsresult createHandlerWith(txOutputFormat* aFormat,        \
                               const nsSubstring& aName,       \
                               int32_t aNsID,                  \
                               txAXMLEventHandler** aHandler);

#endif
