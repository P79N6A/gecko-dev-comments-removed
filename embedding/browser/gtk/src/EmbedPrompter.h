







































#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#include "nsReadableUtils.h"
#else
#include "nsStringAPI.h"
#endif
#include <gtk/gtk.h>

#include <stdlib.h>
#define EMBED_MAX_BUTTONS 3

class EmbedPrompter {

public:

    EmbedPrompter();
    ~EmbedPrompter();

    enum PromptType {
        TYPE_ALERT,
        TYPE_ALERT_CHECK,
        TYPE_CONFIRM,
        TYPE_CONFIRM_CHECK,
        TYPE_PROMPT,
        TYPE_PROMPT_USER_PASS,
        TYPE_PROMPT_PASS,
        TYPE_SELECT,
        TYPE_UNIVERSAL
    };

    nsresult Create(PromptType aType, GtkWindow* aParentWindow);
    void     SetTitle(const PRUnichar *aTitle);
    void     SetTextValue(const PRUnichar *aTextValue);
    void     SetCheckMessage(const PRUnichar *aCheckMessage);
    void     SetCheckValue(const PRBool aValue);
    void     SetMessageText(const PRUnichar *aMessageText);
    void     SetUser(const PRUnichar *aUser);
    void     SetPassword(const PRUnichar *aPass);
    void     SetButtons(const PRUnichar* aButton0Label,
                        const PRUnichar* aButton1Label,
                        const PRUnichar* aButton2Label);
    void     SetItems(const PRUnichar **aItemArray, PRUint32 aCount);

    void     GetCheckValue(PRBool *aValue);
    void     GetConfirmValue(PRBool *aConfirmValue);
    void     GetTextValue(PRUnichar **aTextValue);
    void     GetUser(PRUnichar **aUser);
    void     GetPassword(PRUnichar **aPass);
    void     GetButtonPressed(PRInt32 *aButton);
    void     GetSelectedItem(PRInt32 *aIndex);

    void     Run(void);

private:

    void     SaveDialogValues();

    nsCString    mTitle;
    nsCString    mMessageText;
    nsCString    mTextValue;
    nsCString    mCheckMessage;
    PRBool       mCheckValue;
    nsCString    mUser;
    nsCString    mPass;
    nsCString    mButtonLabels[EMBED_MAX_BUTTONS];
    nsCString   *mItemList;
    PRUint32     mItemCount;

    PRInt32      mButtonPressed;
    PRBool       mConfirmResult;
    PRInt32      mSelectedItem;

    GtkWidget   *mWindow;
    GtkWidget   *mUserField;
    GtkWidget   *mPassField;
    GtkWidget   *mTextField;
    GtkWidget   *mOptionMenu;
    GtkWidget   *mCheckBox;
};
