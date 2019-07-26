




#pragma once

#include <windows.ui.notifications.h>
#include <windows.data.xml.dom.h>
#include "mozwrlbase.h"

using namespace Microsoft::WRL;

class ToastNotificationHandler {
    typedef ABI::Windows::UI::Notifications::IToastNotification IToastNotification;
    typedef ABI::Windows::Data::Xml::Dom::IXmlNode IXmlNode;
    typedef ABI::Windows::Data::Xml::Dom::IXmlDocument IXmlDocument;

    void SetNodeValueString(HSTRING inputString, ComPtr<IXmlNode> node, ComPtr<IXmlDocument> xml);
  public:
    ToastNotificationHandler() {};
    ~ToastNotificationHandler() {};

    void DisplayNotification(HSTRING title, HSTRING msg, HSTRING imagePath);
    HRESULT OnActivate(IToastNotification *notification, IInspectable *inspectable);
};
