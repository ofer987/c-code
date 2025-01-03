#include <stdio.h>
#include <stdlib.h>

struct list_item {
  int value;
  struct list_item *next;
};
typedef struct list_item list_item;

struct list {
  struct list_item *head;
};
typedef struct list list;

list* create_list(size_t size);
void remove_cs101(list *l, list_item *target);
void remove_elegant(list *l, list_item *target);
size_t list_size(list *l);

int main(int argc, [[maybe_unused]] char *argv[argc]) {
  list* my_list = create_list(10);

  size_t size = list_size(my_list);
  printf("Original list has %zu items\n", size);

  list_item *selected_item = my_list->head->next->next;
  /* list_item *selected_item = my_list->head; */
  printf("Selected Item has value %d\n", selected_item->value);
  /* remove_cs101(my_list, selected_item); */
  remove_elegant(my_list, selected_item);

  size = list_size(my_list);
  printf("Modified list has %zu items\n", size);

  return EXIT_SUCCESS;
}

list* create_list(size_t size) {
  list_item *last_item = NULL;

  for (int i = 0; i < size; i += 1) {
    list_item *item;

    item = malloc(sizeof(list_item));

    item->value = size - i;
    item->next = last_item;

    last_item = item;
  }

  list *result;
  result = malloc(sizeof(list));
  result->head = last_item;

  return result;
}

size_t list_size(list* l) {
  list_item *item = l->head;

  size_t size = 0;
  while (item != NULL) {
    printf("Value is %d\n", item->value);
    item = item->next;

    size += 1;
  }

  return size;
}


/* The textbook version */
void remove_cs101(list *l, list_item *target) {
  list_item *item = l->head;

  if (item == target) {
    l->head = item->next;
    free(target);

    return;
  }

  while (item->next != NULL) {
    if (item->next == target) {
      printf("Current item has value %d\n", item->value);
      printf("Found the item with value %d\n", item->next->value);
      printf("Next item will now be an item with value %d\n",
          item->next->next->value);

      /* (*item).next = item->next->next; */
      item->next = item->next->next;

      free(target);

      return;
    }

    item = item->next;
  }
}

/* A more elegant solution */
void remove_elegant(list *l, list_item *target) {
  list_item **current_item = &l->head;
  list_item *item_pointer = l->head;

  /*
   * If target is the first item in the list, then
   * current_item is the reference to the head of the list
   * And the item_pointer is the first item
   *
   * If the target is the second item in the list, then
   * current_item is the reference to the first item
   * item_pointer is the first item
   *
   *
   */

  while (item_pointer != NULL) {
    if (item_pointer == target) {
      *current_item = item_pointer->next;
      free(target);

      return;
    }

    current_item = &(*current_item)->next;
    item_pointer = item_pointer->next;
  }
}
