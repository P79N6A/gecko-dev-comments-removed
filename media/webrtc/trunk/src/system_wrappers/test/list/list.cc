









#include <stdio.h>
#include <stdlib.h>

#include "list_wrapper.h"

const int kNumberOfElements = 10;

void FailTest(bool failed)
{
    if (failed)
    {
        printf("Test failed!\n");
        printf("Press enter to continue:");
        getchar();
        exit(0);
    }
}

int GetStoredIntegerValue(ListItem* list_item)
{
    void* list_item_pointer = list_item->GetItem();
    if (list_item_pointer != NULL)
    {
        return *(reinterpret_cast<int*>(list_item_pointer));
    }
    return static_cast<int>(list_item->GetUnsignedItem());
}

void PrintList(ListWrapper& list)
{
    ListItem* list_item = list.First();
    printf("List: ");
    while (list_item != NULL)
    {
        int item_value = GetStoredIntegerValue(list_item);
        FailTest(item_value < 0);
        printf(" %d",item_value);
        list_item = list.Next(list_item);
    }
    printf("\n");
}


void ListSanity(ListWrapper& list)
{
    if(list.Empty())
    {
      return;
    }
    ListItem* item_iter = list.First();
    
    int previous_value = GetStoredIntegerValue(item_iter) - 1;
    while (item_iter != NULL)
    {
        const int value = GetStoredIntegerValue(item_iter);
        FailTest(value != previous_value + 1);
        previous_value = value;
        item_iter = list.Next(item_iter);
    }
}

int main(int , char* [])
{
    printf("List Test:\n");
    int element_array[kNumberOfElements];
    for (int i = 0; i < kNumberOfElements; i++)
    {
        element_array[i] = i;
    }
    
    ListWrapper test_list;
    for (int i = 2; i < kNumberOfElements - 2; i++)
    {
        FailTest(test_list.PushBack((void*)&element_array[i]) != 0);
    }
    
    FailTest(test_list.PushBack(element_array[kNumberOfElements - 2]) != 0);
    FailTest(test_list.PushBack(element_array[kNumberOfElements - 1]) != 0);
    
    FailTest(test_list.PushFront(element_array[1]) != 0);
    
    FailTest(test_list.PushFront((void*)&element_array[0]) != 0);
    
    FailTest(test_list.GetSize() != kNumberOfElements);
    PrintList(test_list);
    
    FailTest(test_list.PopFront() != 0);
    
    FailTest(test_list.PopBack() != 0);
    
    FailTest(test_list.GetSize() != kNumberOfElements - 2);
    
    FailTest(test_list.Empty());
    
    ListItem* first_item = test_list.First();
    FailTest(first_item == NULL);
    
    ListItem* last_item = test_list.Last();
    FailTest(last_item == NULL);
    
    ListItem* second_item = test_list.Next(first_item);
    FailTest(second_item == NULL);
    FailTest(test_list.Next(last_item) != NULL);
    FailTest(test_list.Next(NULL) != NULL);
    
    ListItem* second_to_last_item = test_list.Previous(last_item);
    FailTest(second_to_last_item == NULL);
    FailTest(test_list.Previous(first_item) != NULL);
    FailTest(test_list.Previous(NULL) != NULL);
    
    FailTest(last_item->GetUnsignedItem() !=
             kNumberOfElements - 2);
    FailTest(last_item->GetItem() !=
             NULL);
    
    FailTest(GetStoredIntegerValue(second_to_last_item) !=
             kNumberOfElements - 3);
    FailTest(second_to_last_item->GetUnsignedItem() != 0);
    
    FailTest(test_list.PopFront() != 0);
    FailTest(test_list.PopBack() != 0);
    
    
    ListItem* insert_item_last = new ListItem(reinterpret_cast<void*>(&element_array[kNumberOfElements - 2]));
    FailTest(test_list.Insert(test_list.Last(),insert_item_last) != 0);
    FailTest(test_list.Insert(NULL,insert_item_last) == 0);
    ListItem* insert_item_last2 = new ListItem(reinterpret_cast<void*>(&element_array[kNumberOfElements - 2]));
    FailTest(test_list.Insert(insert_item_last2,NULL) == 0);
    
    ListItem* insert_item_first = new ListItem(reinterpret_cast<void*>(&element_array[1]));
    FailTest(test_list.InsertBefore(test_list.First(),insert_item_first) != 0);
    FailTest(test_list.InsertBefore(NULL,insert_item_first) == 0);
    ListItem* insert_item_first2 = new ListItem(reinterpret_cast<void*>(&element_array[1]));
    FailTest(test_list.InsertBefore(insert_item_first2,NULL) == 0);
    PrintList(test_list);
    ListSanity(test_list);
    
    int counter = 0;
    while (test_list.PopFront() == 0)
    {
        FailTest(counter++ > kNumberOfElements);
    }
    PrintList(test_list);
    
    FailTest(test_list.GetSize() != 0);
    FailTest(test_list.PopFront() != -1);
    FailTest(test_list.PopBack() != -1);
    FailTest(!test_list.Empty());
    FailTest(test_list.First() != NULL);
    FailTest(test_list.Last() != NULL);
    FailTest(test_list.Next(NULL) != NULL);
    FailTest(test_list.Previous(NULL) != NULL);
    FailTest(test_list.Erase(NULL) != -1);
    
    ListItem* new_item = new ListItem(reinterpret_cast<void*>(&element_array[0]));
    FailTest(test_list.Insert(NULL,new_item) != 0);
    FailTest(test_list.Empty());
    FailTest(test_list.PopFront() != 0);
    ListItem* new_item2 = new ListItem(reinterpret_cast<void*>(&element_array[0]));
    FailTest(test_list.InsertBefore(NULL,new_item2) != 0);
    FailTest(test_list.Empty());

    printf("Tests passed successfully!\n");
}
