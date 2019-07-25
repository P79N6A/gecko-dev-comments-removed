




































#include "mozSpellChecker.h"
#include "nsIServiceManager.h"
#include "mozISpellI18NManager.h"
#include "nsIStringEnumerator.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"

#define DEFAULT_SPELL_CHECKER "@mozilla.org/spellchecker/engine;1"

NS_IMPL_CYCLE_COLLECTING_ADDREF(mozSpellChecker)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozSpellChecker)

NS_INTERFACE_MAP_BEGIN(mozSpellChecker)
  NS_INTERFACE_MAP_ENTRY(nsISpellChecker)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISpellChecker)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozSpellChecker)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_3(mozSpellChecker,
                           mConverter,
                           mTsDoc,
                           mPersonalDictionary)

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
  
  mSpellCheckingEngine = nsnull;

  return NS_OK;
} 

NS_IMETHODIMP 
mozSpellChecker::SetDocument(nsITextServicesDocument *aDoc, bool aFromStartofDoc)
{
  mTsDoc = aDoc;
  mFromStart = aFromStartofDoc;
  return NS_OK;
}


NS_IMETHODIMP 
mozSpellChecker::NextMisspelledWord(nsAString &aWord, nsTArray<nsString> *aSuggestions)
{
  if(!aSuggestions||!mConverter)
    return NS_ERROR_NULL_POINTER;

  PRInt32 selOffset;
  PRInt32 begin,end;
  nsresult result;
  result = SetupDoc(&selOffset);
  bool isMisspelled,done;
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
mozSpellChecker::CheckWord(const nsAString &aWord, bool *aIsMisspelled, nsTArray<nsString> *aSuggestions)
{
  nsresult result;
  bool correct;
  if(!mSpellCheckingEngine)
    return NS_ERROR_NULL_POINTER;

  *aIsMisspelled = false;
  result = mSpellCheckingEngine->Check(PromiseFlatString(aWord).get(), &correct);
  NS_ENSURE_SUCCESS(result, result);
  if(!correct){
    if(aSuggestions){
      PRUint32 count,i;
      PRUnichar **words;
      
      result = mSpellCheckingEngine->Suggest(PromiseFlatString(aWord).get(), &words, &count);
      NS_ENSURE_SUCCESS(result, result); 
      for(i=0;i<count;i++){
        aSuggestions->AppendElement(nsDependentString(words[i]));
      }
      
      if (count)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, words);
    }
    *aIsMisspelled = true;
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::Replace(const nsAString &aOldWord, const nsAString &aNewWord, bool aAllOccurrences)
{
  if(!mConverter)
    return NS_ERROR_NULL_POINTER;

  nsAutoString newWord(aNewWord); 

  if(aAllOccurrences){
    PRInt32 selOffset;
    PRInt32 startBlock,currentBlock,currOffset;
    PRInt32 begin,end;
    bool done;
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
              
              
              if((currentBlock == startBlock)&&(begin < selOffset)){
                selOffset +=
                  PRInt32(aNewWord.Length()) - PRInt32(aOldWord.Length());
                if(selOffset < begin) selOffset=begin;
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
mozSpellChecker::GetPersonalDictionary(nsTArray<nsString> *aWordList)
{
  if(!aWordList || !mPersonalDictionary)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIStringEnumerator> words;
  mPersonalDictionary->GetWordList(getter_AddRefs(words));
  
  bool hasMore;
  nsAutoString word;
  while (NS_SUCCEEDED(words->HasMore(&hasMore)) && hasMore) {
    words->GetNext(word);
    aWordList->AppendElement(word);
  }
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::GetDictionaryList(nsTArray<nsString> *aDictionaryList)
{
  nsresult rv;

  
  nsClassHashtable<nsStringHashKey, nsCString> dictionaries;
  dictionaries.Init();

  nsCOMArray<mozISpellCheckingEngine> spellCheckingEngines;
  rv = GetEngineList(&spellCheckingEngines);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < spellCheckingEngines.Count(); i++) {
    nsCOMPtr<mozISpellCheckingEngine> engine = spellCheckingEngines[i];

    PRUint32 count = 0;
    PRUnichar **words = NULL;
    engine->GetDictionaryList(&words, &count);
    for (PRUint32 k = 0; k < count; k++) {
      nsAutoString dictName;

      dictName.Assign(words[k]);

      
      
      if (dictionaries.Get(dictName, NULL))
        continue;

      dictionaries.Put(dictName, NULL);

      if (!aDictionaryList->AppendElement(dictName)) {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, words);
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, words);
  }

  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::GetCurrentDictionary(nsAString &aDictionary)
{
  if (!mSpellCheckingEngine) {
    aDictionary.AssignLiteral("");
    return NS_OK;
  }

  nsXPIDLString dictname;
  mSpellCheckingEngine->GetDictionary(getter_Copies(dictname));
  aDictionary = dictname;
  return NS_OK;
}

NS_IMETHODIMP 
mozSpellChecker::SetCurrentDictionary(const nsAString &aDictionary)
{
  mSpellCheckingEngine = nsnull;

  if (aDictionary.IsEmpty()) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMArray<mozISpellCheckingEngine> spellCheckingEngines;
  rv = GetEngineList(&spellCheckingEngines);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < spellCheckingEngines.Count(); i++) {
    
    
    
    mSpellCheckingEngine = spellCheckingEngines[i];

    rv = mSpellCheckingEngine->SetDictionary(PromiseFlatString(aDictionary).get());

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<mozIPersonalDictionary> personalDictionary = do_GetService("@mozilla.org/spellchecker/personaldictionary;1");
      mSpellCheckingEngine->SetPersonalDictionary(personalDictionary.get());

      nsXPIDLString language;
      nsCOMPtr<mozISpellI18NManager> serv(do_GetService("@mozilla.org/spellchecker/i18nmanager;1", &rv));
      NS_ENSURE_SUCCESS(rv, rv);
      return serv->GetUtil(language.get(),getter_AddRefs(mConverter));
    }
  }

  mSpellCheckingEngine = NULL;
  
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
mozSpellChecker::CheckCurrentDictionary()
{
  
  
  

  if (!mSpellCheckingEngine) {
    
    return NS_OK;
  }

  nsXPIDLString dictname;
  mSpellCheckingEngine->GetDictionary(getter_Copies(dictname));

  if (!dictname.IsEmpty()) {
    
    return NS_OK;
  }

  
  mSpellCheckingEngine = nsnull;
  return NS_OK;
}

nsresult
mozSpellChecker::SetupDoc(PRInt32 *outBlockOffset)
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
    mFromStart = false;
  }
  return rv;
}





nsresult
mozSpellChecker::GetCurrentBlockIndex(nsITextServicesDocument *aDoc, PRInt32 *outBlockIndex)
{
  PRInt32  blockIndex = 0;
  bool     isDone = false;
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

nsresult
mozSpellChecker::GetEngineList(nsCOMArray<mozISpellCheckingEngine>* aSpellCheckingEngines)
{
  nsresult rv;
  bool hasMoreEngines;

  nsCOMPtr<nsICategoryManager> catMgr = do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMgr)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsISimpleEnumerator> catEntries;

  
  
  rv = catMgr->EnumerateCategory("spell-check-engine", getter_AddRefs(catEntries));
  if (NS_FAILED(rv))
    return rv;

  while (catEntries->HasMoreElements(&hasMoreEngines), hasMoreEngines){
    nsCOMPtr<nsISupports> elem;
    rv = catEntries->GetNext(getter_AddRefs(elem));

    nsCOMPtr<nsISupportsCString> entry = do_QueryInterface(elem, &rv);
    if (NS_FAILED(rv))
      return rv;

    nsCString contractId;
    rv = entry->GetData(contractId);
    if (NS_FAILED(rv))
      return rv;

    
    
    nsCOMPtr<mozISpellCheckingEngine> engine =
      do_GetService(contractId.get(), &rv);
    if (NS_SUCCEEDED(rv)) {
      aSpellCheckingEngines->AppendObject(engine);
    }
  }

  
  nsCOMPtr<mozISpellCheckingEngine> engine =
    do_GetService(DEFAULT_SPELL_CHECKER, &rv);
  if (NS_FAILED(rv)) {
    
    
    return rv;
  }
  aSpellCheckingEngines->AppendObject(engine);

  return NS_OK;
}
