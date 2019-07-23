






































#include "EmbedPrompter.h"

#define ALLOC_NOT_CHECKED(newed) PR_BEGIN_MACRO               \
  /* This might not crash, but the code probably isn't really \
   * designed to handle it, perhaps the code should be fixed? \
   */                                                         \
  if (!newed) {                                               \
  }                                                           \
  PR_END_MACRO

enum {
  INCLUDE_USERNAME = 1 << 0,
  INCLUDE_PASSWORD = 1 << 1,
  INCLUDE_CHECKBOX = 1 << 2,
  INCLUDE_CANCEL   = 1 << 3
};

struct DialogDescription {
  int  flags;
  gchar* icon;
};




static const DialogDescription DialogTable[] = {
  { 0,                      GTK_STOCK_DIALOG_WARNING  },  
  { INCLUDE_CHECKBOX,       GTK_STOCK_DIALOG_WARNING  },  
  { INCLUDE_CANCEL,         GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CHECKBOX |
    INCLUDE_CANCEL,         GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CANCEL |
    INCLUDE_CHECKBOX,       GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CANCEL |
    INCLUDE_USERNAME |
    INCLUDE_PASSWORD |
    INCLUDE_CHECKBOX,       GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CANCEL |
    INCLUDE_PASSWORD |
    INCLUDE_CHECKBOX,       GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CANCEL,         GTK_STOCK_DIALOG_QUESTION },  
  { INCLUDE_CANCEL |
    INCLUDE_CHECKBOX,       GTK_STOCK_DIALOG_QUESTION }   
};

EmbedPrompter::EmbedPrompter(void)
  : mCheckValue(PR_FALSE),
    mItemList(nsnull),
    mItemCount(0),
    mButtonPressed(0),
    mConfirmResult(PR_FALSE),
    mSelectedItem(0),
    mWindow(NULL),
    mUserField(NULL),
    mPassField(NULL),
    mTextField(NULL),
    mOptionMenu(NULL),
    mCheckBox(NULL)
{
}

EmbedPrompter::~EmbedPrompter(void)
{
  if (mItemList)
    delete[] mItemList;
}

nsresult
EmbedPrompter::Create(PromptType aType, GtkWindow* aParentWindow)

{
  mWindow = gtk_dialog_new_with_buttons(
    mTitle.get(),
    aParentWindow,
    (GtkDialogFlags)0,
    NULL);
  
  
  if (aParentWindow && aParentWindow->group) {
    gtk_window_group_add_window(aParentWindow->group, GTK_WINDOW(mWindow));
  }

  
  gtk_window_set_default_size(GTK_WINDOW(mWindow), 100, 50);

  
  
  GtkWidget* dialogHBox = gtk_hbox_new(FALSE, 12);


  
  

  gtk_container_set_border_width(GTK_CONTAINER(mWindow), 6);
  gtk_dialog_set_has_separator(GTK_DIALOG(mWindow), FALSE);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(mWindow)->vbox), 12);
  gtk_container_set_border_width(GTK_CONTAINER(dialogHBox), 6);


  
  GtkWidget* contentsVBox = gtk_vbox_new(FALSE, 12);

  
  const gchar* iconDesc = DialogTable[aType].icon;
  GtkWidget* icon = gtk_image_new_from_stock(iconDesc, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment(GTK_MISC(icon), 0.5, 0.0);
  gtk_box_pack_start(GTK_BOX(dialogHBox), icon, FALSE, FALSE, 0);

  
  GtkWidget* label = gtk_label_new(mMessageText.get());
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_box_pack_start(GTK_BOX(contentsVBox), label, FALSE, FALSE, 0);

  int widgetFlags = DialogTable[aType].flags;

  if (widgetFlags & (INCLUDE_USERNAME | INCLUDE_PASSWORD)) {

    
    
    

    GtkWidget* userPassHBox = gtk_hbox_new(FALSE, 12);
    GtkWidget* userPassLabels = gtk_vbox_new(TRUE, 6);
    GtkWidget* userPassFields = gtk_vbox_new(TRUE, 6);

    if (widgetFlags & INCLUDE_USERNAME) {
      GtkWidget* userLabel = gtk_label_new("User Name:");
      gtk_box_pack_start(GTK_BOX(userPassLabels), userLabel, FALSE,
                 FALSE, 0);

      mUserField = gtk_entry_new();

      if (!mUser.IsEmpty())
        gtk_entry_set_text(GTK_ENTRY(mUserField), mUser.get());

      gtk_entry_set_activates_default(GTK_ENTRY(mUserField), TRUE);

      gtk_box_pack_start(
        GTK_BOX(userPassFields),
        mUserField,
        FALSE,
        FALSE,
        0);
    }
    if (widgetFlags & INCLUDE_PASSWORD) {
      GtkWidget* passLabel = gtk_label_new("Password:");
      gtk_box_pack_start(
        GTK_BOX(userPassLabels),
        passLabel,
        FALSE,
        FALSE,
        0);
      mPassField = gtk_entry_new();
      if (!mPass.IsEmpty())
        gtk_entry_set_text(GTK_ENTRY(mPassField), mPass.get());
      gtk_entry_set_visibility(GTK_ENTRY(mPassField), FALSE);
      gtk_entry_set_activates_default(GTK_ENTRY(mPassField), TRUE);
      gtk_box_pack_start(
        GTK_BOX(userPassFields),
        mPassField,
        FALSE,
        FALSE,
        0);
    }
    gtk_box_pack_start(
      GTK_BOX(userPassHBox),
      userPassLabels,
      FALSE,
      FALSE,
      0);
    gtk_box_pack_start(
      GTK_BOX(userPassHBox),
      userPassFields,
      FALSE,
      FALSE,
      0);
    gtk_box_pack_start(
      GTK_BOX(contentsVBox),
      userPassHBox,
      FALSE,
      FALSE,
      0);
  }
  if (aType == TYPE_PROMPT) {
    mTextField = gtk_entry_new();
    if (!mTextValue.IsEmpty())
      gtk_entry_set_text(GTK_ENTRY(mTextField), mTextValue.get());
    gtk_entry_set_activates_default(GTK_ENTRY(mTextField), TRUE);
    gtk_box_pack_start(GTK_BOX(contentsVBox), mTextField, FALSE, FALSE, 0);
  }
  
  if ((widgetFlags & INCLUDE_CHECKBOX) && !mCheckMessage.IsEmpty()) {
    mCheckBox = gtk_check_button_new_with_label(mCheckMessage.get());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mCheckBox),
                   mCheckValue);
    gtk_label_set_line_wrap(
      GTK_LABEL(gtk_bin_get_child(GTK_BIN(mCheckBox))),
      TRUE);
    gtk_box_pack_start(GTK_BOX(contentsVBox), mCheckBox, FALSE, FALSE, 0);
  }
  
  if (aType == TYPE_SELECT) {
    
    GtkWidget* menu = gtk_menu_new();
    for (PRUint32 i = 0; i < mItemCount; ++i) {
      GtkWidget* item = gtk_menu_item_new_with_label(mItemList[i].get());
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    
    mOptionMenu = gtk_option_menu_new();

    gtk_option_menu_set_menu(GTK_OPTION_MENU(mOptionMenu), menu);
    gtk_box_pack_start(GTK_BOX(contentsVBox), mOptionMenu, FALSE, FALSE, 0);
  }

  if (aType == TYPE_UNIVERSAL) {
    
    for (int i = EMBED_MAX_BUTTONS; i >= 0; --i) {
      if (!mButtonLabels[i].IsEmpty())
        gtk_dialog_add_button(GTK_DIALOG(mWindow),
                    mButtonLabels[i].get(), i);
    }
    gtk_dialog_set_default_response(GTK_DIALOG(mWindow), 0);
  } else {
    
    if (widgetFlags & INCLUDE_CANCEL)
      gtk_dialog_add_button(GTK_DIALOG(mWindow), GTK_STOCK_CANCEL,
                  GTK_RESPONSE_CANCEL);

    GtkWidget* okButton = gtk_dialog_add_button(GTK_DIALOG(mWindow),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_ACCEPT);
    gtk_widget_grab_default(okButton);
  }
  
  gtk_box_pack_start(GTK_BOX(dialogHBox), contentsVBox, FALSE, FALSE, 0);
  gtk_box_pack_start(
    GTK_BOX(GTK_DIALOG(mWindow)->vbox),
    dialogHBox,
    FALSE,
    FALSE,
    0);
  return NS_OK;
}

void
EmbedPrompter::SetTitle(const PRUnichar *aTitle)
{
  mTitle.Assign(NS_ConvertUTF16toUTF8(aTitle));
}

void
EmbedPrompter::SetTextValue(const PRUnichar *aTextValue)
{
  mTextValue.Assign(NS_ConvertUTF16toUTF8(aTextValue));
}

void
EmbedPrompter::SetCheckMessage(const PRUnichar *aMessage)
{
  mCheckMessage.Assign(NS_ConvertUTF16toUTF8(aMessage));
}

void
EmbedPrompter::SetMessageText(const PRUnichar *aMessageText)
{
  mMessageText.Assign(NS_ConvertUTF16toUTF8(aMessageText));
}

void
EmbedPrompter::SetUser(const PRUnichar *aUser)
{
  mUser.Assign(NS_ConvertUTF16toUTF8(aUser));
}

void
EmbedPrompter::SetPassword(const PRUnichar *aPass)
{
  mPass.Assign(NS_ConvertUTF16toUTF8(aPass));
}

void
EmbedPrompter::SetCheckValue(const PRBool aValue)
{
  mCheckValue = aValue;
}

void
EmbedPrompter::SetItems(const PRUnichar** aItemArray, PRUint32 aCount)
{
  if (mItemList)
    delete[] mItemList;

  mItemCount = aCount;
  mItemList = new nsCString[aCount];
  ALLOC_NOT_CHECKED(mItemList);
  for (PRUint32 i = 0; i < aCount; ++i)
    mItemList[i].Assign(NS_ConvertUTF16toUTF8(aItemArray[i]));
}

void
EmbedPrompter::SetButtons(const PRUnichar* aButton0Label,
              const PRUnichar* aButton1Label,
              const PRUnichar* aButton2Label)
{
  mButtonLabels[0].Assign(NS_ConvertUTF16toUTF8(aButton0Label));
  mButtonLabels[1].Assign(NS_ConvertUTF16toUTF8(aButton1Label));
  mButtonLabels[2].Assign(NS_ConvertUTF16toUTF8(aButton2Label));
}

void
EmbedPrompter::GetCheckValue(PRBool *aValue)
{
  *aValue = mCheckValue;
}

void
EmbedPrompter::GetConfirmValue(PRBool *aConfirmValue)
{
  *aConfirmValue = mConfirmResult;
}

void
EmbedPrompter::GetTextValue(PRUnichar **aTextValue)
{
  *aTextValue = ToNewUnicode(NS_ConvertUTF8toUTF16(mTextValue));
}

void
EmbedPrompter::GetUser(PRUnichar **aUser)
{
  *aUser = ToNewUnicode(NS_ConvertUTF8toUTF16(mUser));
}

void
EmbedPrompter::GetPassword(PRUnichar **aPass)
{
  *aPass = ToNewUnicode(NS_ConvertUTF8toUTF16(mPass));
}

void
EmbedPrompter::GetSelectedItem(PRInt32 *aIndex)
{
  *aIndex = mSelectedItem;
}

void
EmbedPrompter::GetButtonPressed(PRInt32 *aButton)
{
  *aButton = mButtonPressed;
}

void
EmbedPrompter::Run(void)
{
  gtk_widget_show_all(mWindow);
  gint response = gtk_dialog_run(GTK_DIALOG(mWindow));
  switch (response) {
  case GTK_RESPONSE_NONE:
  case GTK_RESPONSE_CANCEL:
  case GTK_RESPONSE_DELETE_EVENT:
    mConfirmResult = PR_FALSE;
    break;
  case GTK_RESPONSE_ACCEPT:
    mConfirmResult = PR_TRUE;
    SaveDialogValues();
    break;
  default:
    mButtonPressed = response;
    SaveDialogValues();
  }

  gtk_widget_destroy(mWindow);
}

void
EmbedPrompter::SaveDialogValues()
{
  if (mUserField)
    mUser.Assign(gtk_entry_get_text(GTK_ENTRY(mUserField)));

  if (mPassField)
    mPass.Assign(gtk_entry_get_text(GTK_ENTRY(mPassField)));

  if (mCheckBox)
    mCheckValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mCheckBox));

  if (mTextField)
    mTextValue.Assign(gtk_entry_get_text(GTK_ENTRY(mTextField)));

  if (mOptionMenu)
    mSelectedItem = gtk_option_menu_get_history(GTK_OPTION_MENU(mOptionMenu));
}
