





































#include "mozSpellChecker.h"
#include "nsIServiceManager.h"
#include "mozISpellI18NManager.h"
#include "nsIStringEnumerator.h"

#define UNREASONABLE_WORD_LENGTH 64

NS_IMPL_ISUPPORTS1(mozSpellChecker, nsISpellChecker)

mozSpellChecker::mozSpellChecker()
{
}

mozSpellChecker::~mozSpellChecker()
{
  if(mPersonalDictionary){
    
    mPersonalDictionary->EndSession();
  }
  mSpellCheckingEngine = nsnull;
  mPersonalDictionary = nsnull;
}

nsresult 
mozSpellChecker::Init()
{
  mPersonalDictionary = do_GetService("@mozilla.org/spellchecker/personaldictionary;1");
  
  nsresult rv;
  mSpellCheckingEngine = do_GetService("@mozilla.org/spellchecker/myspell;1",&rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mSpellCheckingEngine->SetPersonalDictionary(mPersonalDictionary);
  return NS_OK;
} 

NS_IMETHODIMP 
mozSpellChecker::SetDocument(nsITextServicesDocument *aDoc, PRBool aFromStartofDoc)
{
  mTsDoc = aDoc;
  mFromStart = aFromStartofDoc;
  return NS_OK;
}


NS_IMETHODIMP 
mozSpellChecker::NextMisspelledWord(nsAString &aWord, nsStringArray *aSuggestions)
{
  if(!aSuggestions||!mConverter)
    return NS_ERROR_NULL_POINTER;

  PRUint32 selOffset;
  PRInt32 begin,end;
  nsresult result;
  result = SetupDoc(&selOffset);
  PRBool isMisspelled,done;
  if (NS_FAILED(result))
    return result;

  while( NS_SUCCEEDED(mTsDoc->IsDone(&done)) && !done )
    {
      nsString str;
      result = mTsDoc->GetCurrentTextBlock(&str);
  
      if (NS_FAILED(result))
        return result;
      do{
        result = mConverter->FindNextWord(str.get(),str.Length(),selOffset,&begin,&end);
        if(NS_SUCCEEDED(result)&&(begin != -1)){
          const nsAString &currWord = Substring(str, begin, end - begin);
          result = CheckWord(currWord, &isMisspelled, aSuggestions);
          if(isMisspelled){
            aWord = currWord;
            mTsDoc->SetSelection(begin, end-begin);
            mTsDoc->ScrollSelectionIntoView();
            return NS_OK;
          }
        }
        selOffset = end;
      }while(end != -1);
      mTsDoc->NextBlock();
      selOffset=0;
    }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::CheckWord(const nsAString &aWord, PRBool *aIsMisspelled, nsStringArray *aSuggestions)
{
  nsresult result;
  PRBool correct;
  if(!mSpellCheckingEngine)
    return NS_ERROR_NULL_POINTER;

  
  
  if (aWord.Length() > UNREASONABLE_WORD_LENGTH) {
    *aIsMisspelled = PR_TRUE;
    return NS_OK;
  }

  *aIsMisspelled = PR_FALSE;
  result = mSpellCheckingEngine->Check(PromiseFlatString(aWord).get(), &correct);
  NS_ENSURE_SUCCESS(result, result);
  if(!correct){
    if(aSuggestions){
      PRUint32 count,i;
      PRUnichar **words;
      
      result = mSpellCheckingEngine->Suggest(PromiseFlatString(aWord).get(), &words, &count);
      NS_ENSURE_SUCCESS(result, result); 
      for(i=0;i<count;i++){
        aSuggestions->AppendString(nsDependentString(words[i]));
      }
      
      if (count)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, words);
    }
    if(aIsMisspelled){
      *aIsMisspelled = PR_TRUE;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::Replace(const nsAString &aOldWord, const nsAString &aNewWord, PRBool aAllOccurrences)
{
  if(!mConverter)
    return NS_ERROR_NULL_POINTER;

  nsAutoString newWord(aNewWord); 

  if(aAllOccurrences){
    PRUint32 selOffset;
    PRInt32 startBlock,currentBlock,currOffset;
    PRInt32 begin,end;
    PRBool done;
    nsresult result;
    nsAutoString str;

    
    result = SetupDoc(&selOffset);
    if(NS_FAILED(result))
      return result;
    result = GetCurrentBlockIndex(mTsDoc,&startBlock);
    if(NS_FAILED(result))
      return result;

    
    result = mTsDoc->FirstBlock();
    currOffset=0;
    currentBlock = 0;
    while( NS_SUCCEEDED(mTsDoc->IsDone(&done)) && !done )
      {
        result = mTsDoc->GetCurrentTextBlock(&str);
        do{
          result = mConverter->FindNextWord(str.get(),str.Length(),currOffset,&begin,&end);
          if(NS_SUCCEEDED(result)&&(begin != -1)){
            if (aOldWord.Equals(Substring(str, begin, end-begin))) {
              
              
              if((currentBlock == startBlock)&&(begin < (PRInt32) selOffset)){
                selOffset += (aNewWord.Length() - aOldWord.Length());
                if(selOffset < 0) selOffset=0;
              }
              mTsDoc->SetSelection(begin, end-begin);
              mTsDoc->InsertText(&newWord);
              mTsDoc->GetCurrentTextBlock(&str);
              end += (aNewWord.Length() - aOldWord.Length());  
            }
          }
          currOffset = end;
        }while(currOffset != -1);
        mTsDoc->NextBlock();
        currentBlock++;
        currOffset=0;          
      }

    
    result = mTsDoc->FirstBlock();
    currentBlock = 0;
    while(( NS_SUCCEEDED(mTsDoc->IsDone(&done)) && !done ) &&(currentBlock < startBlock)){
      mTsDoc->NextBlock();
    }








    if( NS_SUCCEEDED(mTsDoc->IsDone(&done)) && !done ){
      nsString str;                                
      result = mTsDoc->GetCurrentTextBlock(&str);  
      result = mConverter->FindNextWord(str.get(),str.Length(),selOffset,&begin,&end);
            if(end == -1)
             {
                mTsDoc->NextBlock();
                selOffset=0;
                result = mTsDoc->GetCurrentTextBlock(&str); 
                result = mConverter->FindNextWord(str.get(),str.Length(),selOffset,&begin,&end);
                mTsDoc->SetSelection(begin, 0);
             }
         else
                mTsDoc->SetSelection(begin, 0);
    }
 }
  else{
    mTsDoc->InsertText(&newWord);
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::IgnoreAll(const nsAString &aWord)
{
  if(mPersonalDictionary){
    mPersonalDictionary->IgnoreWord(PromiseFlatString(aWord).get());
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::AddWordToPersonalDictionary(const nsAString &aWord)
{
  nsresult res;
  PRUnichar empty=0;
  if (!mPersonalDictionary)
    return NS_ERROR_NULL_POINTER;
  res = mPersonalDictionary->AddWord(PromiseFlatString(aWord).get(),&empty);
  return res;
}

NS_IMETHODIMP 
mozSpellChecker::RemoveWordFromPersonalDictionary(const nsAString &aWord)
{
  nsresult res;
  PRUnichar empty=0;
  if (!mPersonalDictionary)
    return NS_ERROR_NULL_POINTER;
  res = mPersonalDictionary->RemoveWord(PromiseFlatString(aWord).get(),&empty);
  return res;
}

NS_IMETHODIMP 
mozSpellChecker::GetPersonalDictionary(nsStringArray *aWordList)
{
  if(!aWordList || !mPersonalDictionary)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIStringEnumerator> words;
  mPersonalDictionary->GetWordList(getter_AddRefs(words));
  
  PRBool hasMore;
  nsAutoString word;
  while (NS_SUCCEEDED(words->HasMore(&hasMore)) && hasMore) {
    words->GetNext(word);
    aWordList->AppendString(word);
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::GetDictionaryList(nsStringArray *aDictionaryList)
{
  nsAutoString temp;
  PRUint32 count,i;
  PRUnichar **words;
  
  if(!aDictionaryList || !mSpellCheckingEngine)
    return NS_ERROR_NULL_POINTER;
  mSpellCheckingEngine->GetDictionaryList(&words,&count);
  for(i=0;i<count;i++){
    temp.Assign(words[i]);
    aDictionaryList->AppendString(temp);
  }
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, words);

  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::GetCurrentDictionary(nsAString &aDictionary)
{
  nsXPIDLString dictname;
  mSpellCheckingEngine->GetDictionary(getter_Copies(dictname));
  aDictionary = dictname;
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::SetCurrentDictionary(const nsAString &aDictionary)
{
  if(!mSpellCheckingEngine)
    return NS_ERROR_NULL_POINTER;
 
  nsresult res;
  res = mSpellCheckingEngine->SetDictionary(PromiseFlatString(aDictionary).get());
  if(NS_FAILED(res)){
    NS_WARNING("Dictionary load failed");
    return res;
  }
  nsXPIDLString language;
  
  nsCOMPtr<mozISpellI18NManager> serv(do_GetService("@mozilla.org/spellchecker/i18nmanager;1", &res));
  if(serv && NS_SUCCEEDED(res)){
    res = serv->GetUtil(language.get(),getter_AddRefs(mConverter));
  }
  return res;
}

nsresult
mozSpellChecker::SetupDoc(PRUint32 *outBlockOffset)
{
  nsresult  rv;

  nsITextServicesDocument::TSDBlockSelectionStatus blockStatus;
  PRInt32 selOffset;
  PRInt32 selLength;
  *outBlockOffset = 0;

  if (!mFromStart) 
  {
    rv = mTsDoc->LastSelectedBlock(&blockStatus, &selOffset, &selLength);
    if (NS_SUCCEEDED(rv) && (blockStatus != nsITextServicesDocument::eBlockNotFound))
    {
      switch (blockStatus)
      {
        case nsITextServicesDocument::eBlockOutside:  
        case nsITextServicesDocument::eBlockPartial:  
          
          *outBlockOffset = selOffset + selLength;
          break;
                    
        case nsITextServicesDocument::eBlockInside:  
          
          rv = mTsDoc->NextBlock();
          *outBlockOffset = 0;
          break;
                
        case nsITextServicesDocument::eBlockContains: 
          *outBlockOffset = selOffset + selLength;
          break;
        
        case nsITextServicesDocument::eBlockNotFound: 
        default:
          NS_NOTREACHED("Shouldn't ever get this status");
      }
    }
    else  
    {
      rv = mTsDoc->FirstBlock();
      *outBlockOffset = 0;
    }
  
  }
  else 
  {
    rv = mTsDoc->FirstBlock();
    mFromStart = PR_FALSE;
  }
  return rv;
}





nsresult
mozSpellChecker::GetCurrentBlockIndex(nsITextServicesDocument *aDoc, PRInt32 *outBlockIndex)
{
  PRInt32  blockIndex = 0;
  PRBool   isDone = PR_FALSE;
  nsresult result = NS_OK;

  do
  {
    aDoc->PrevBlock();

    result = aDoc->IsDone(&isDone);

    if (!isDone)
      blockIndex ++;

  } while (NS_SUCCEEDED(result) && !isDone);
  
  *outBlockIndex = blockIndex;

  return result;
}
