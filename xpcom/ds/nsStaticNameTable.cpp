/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* Class to manage lookup of static names in a table. */

#include "nscore.h"
#include "nsString.h"
#include "nsReadableUtils.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "nsStaticNameTable.h"

struct nameTableEntry : public PLDHashEntryHdr
{
  // no ownership here!
  const char *mKey;
  PRInt32 mIndex;
};

PR_STATIC_CALLBACK(PRBool)
matchNameKeys(PLDHashTable*, const PLDHashEntryHdr* aHdr,
              const void* key)
{
  const nameTableEntry* entry =
    NS_STATIC_CAST(const nameTableEntry *, aHdr);
  const char *keyValue = NS_STATIC_CAST(const char*, key);

  return (strcmp(entry->mKey, keyValue)==0);

}

PR_STATIC_CALLBACK(const void*)
getNameKey(PLDHashTable*, PLDHashEntryHdr* aHdr)
{
  nameTableEntry* entry =
    NS_STATIC_CAST(nameTableEntry*, aHdr);

  return entry->mKey;
}

struct PLDHashTableOps nametable_HashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    getNameKey,
    PL_DHashStringKey,
    matchNameKeys,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nsnull,
};


nsStaticCaseInsensitiveNameTable::nsStaticCaseInsensitiveNameTable()
  : mNameArray(nsnull), mCount(0), mNullStr("")
{
    MOZ_COUNT_CTOR(nsStaticCaseInsensitiveNameTable);
    mNameTable.ops = nsnull;
}  

nsStaticCaseInsensitiveNameTable::~nsStaticCaseInsensitiveNameTable()
{
    // manually call the destructor on placement-new'ed objects
    for (PRInt32 index = 0; index < mCount; index++) {
      mNameArray[index].~nsDependentCString();
    }
    nsMemory::Free((void*)mNameArray);
    PL_DHashTableFinish(&mNameTable);
    MOZ_COUNT_DTOR(nsStaticCaseInsensitiveNameTable);
}  

PRBool 
nsStaticCaseInsensitiveNameTable::Init(const char* Names[], PRInt32 Count)
{
    NS_ASSERTION(!mNameArray, "double Init");  
    NS_ASSERTION(!mNameTable.ops, "double Init");  
    NS_ASSERTION(Names, "null name table");
    NS_ASSERTION(Count, "0 count");

    mCount = Count;
    mNameArray = (nsDependentCString*)nsMemory::Alloc(Count * sizeof(nsDependentCString));
    PL_DHashTableInit(&mNameTable, &nametable_HashTableOps, nsnull,
                      sizeof(nameTableEntry), Count);
    if (!mNameArray || !mNameTable.ops) {
        return PR_FALSE;
    }

    for (PRInt32 index = 0; index < Count; ++index) {
        char*    raw = (char*) Names[index];
        PRUint32 len = strlen(raw);
#ifdef DEBUG
       {
       // verify invarients of contents
       nsCAutoString temp1(raw);
       nsDependentCString temp2(raw);
       ToLowerCase(temp1);
       NS_ASSERTION(temp1.Equals(temp2), "upper case char in table");
       }
#endif
        // use placement-new to initialize the string object
        new (&mNameArray[index]) nsDependentCString(raw);

        nameTableEntry *entry =
          NS_STATIC_CAST(nameTableEntry*,
                         PL_DHashTableOperate(&mNameTable, raw, PL_DHASH_ADD));
        
        NS_ASSERTION(entry->mKey == 0, "Entry already exists!");

        entry->mKey = raw;      // not owned!
        entry->mIndex = index;
    }
    return PR_TRUE;
}  

inline PRInt32
LookupLowercasedKeyword(const nsACString& aLowercasedKeyword, 
                        PLDHashTable& aTable)
{
    const nsPromiseFlatCString& flatString = PromiseFlatCString(aLowercasedKeyword);   
    nameTableEntry *entry =
      NS_STATIC_CAST(nameTableEntry*,
                     PL_DHashTableOperate(&aTable, flatString.get(), PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_FREE(entry))
      return nsStaticCaseInsensitiveNameTable::NOT_FOUND;
    
    return entry->mIndex;
}  

PRInt32
nsStaticCaseInsensitiveNameTable::Lookup(const nsACString& aName)
{
    NS_ASSERTION(mNameArray, "not inited");  
    NS_ASSERTION(mNameTable.ops, "not inited");  
    NS_ASSERTION(mCount,     "not inited");

    nsCAutoString strLower(aName);
    ToLowerCase(strLower);
    return LookupLowercasedKeyword(strLower, mNameTable);
}  

PRInt32
nsStaticCaseInsensitiveNameTable::Lookup(const nsAString& aName)
{
    NS_ASSERTION(mNameArray, "not inited");  
    NS_ASSERTION(mNameTable.ops, "not inited");  
    NS_ASSERTION(mCount,     "not inited");
   
    nsCAutoString strLower;
    strLower.AssignWithConversion(aName);
    ToLowerCase(strLower);
    return LookupLowercasedKeyword(strLower, mNameTable);
}  

const nsAFlatCString& 
nsStaticCaseInsensitiveNameTable::GetStringValue(PRInt32 index)
{
    NS_ASSERTION(mNameArray, "not inited");  
    NS_ASSERTION(mNameTable.ops, "not inited");  
    NS_ASSERTION(mCount,     "not inited");
    
    if ((NOT_FOUND < index) && (index < mCount)) {
        return mNameArray[index];
    } else {
        return mNullStr;
    }
}  

