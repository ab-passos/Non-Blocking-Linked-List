#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
/*
bool __sync_bool_compare_and_swap__sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
These builtins perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
The ìboolî version returns true if the comparison is successful and newval was written. The ìvalî version returns the contents of *ptr before the operation

*/

volatile unsigned long counter = 0;

/*static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;*/

static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

int CAS( volatile unsigned long* value, unsigned long comp_val, unsigned long new_val )
{
    return __sync_bool_compare_and_swap( value, comp_val, new_val );
}

int cas( uintptr_t* value, uintptr_t comp_val, uintptr_t new_val )
{
    return __sync_bool_compare_and_swap( value, comp_val, new_val );
}


int randomNumber(){
	return((1 + (int)( 20.0 * rand() / ( RAND_MAX + 1.0 ) )));
}


/*Sequential List*/
typedef struct Elem 
{
        int data;
        struct Elem *next;
}elem;

void insert(elem *pointer, int data)
{
        /* Iterate through the list till we encounter the last elem.*/
        while(pointer->next!=NULL)
        {
                pointer = pointer -> next;
        }
        /* Allocate memory for the new elem and put data in it.*/
        pointer->next = (elem *)malloc(sizeof(elem));
        pointer = pointer->next;
        pointer->data = data;
        pointer->next = NULL;
}

int finding(elem *pointer, int key)
{
        pointer =  pointer -> next; 
        /* Iterate through the entire linked list and search for the key. */
		 pointer = pointer -> next; 
        while(pointer!=NULL)
        {
                if(pointer->data == key) 
                {
                        return 1;
                }
                pointer = pointer -> next;
        }
        /*Key is not found */
        return 0;
}

void delete(elem *pointer, int data)
{
		printf("Delete da data = %d\n", data);
        /* Go to the elem for which the elem next to it has to be deleted */
        while(pointer->next!=NULL && (pointer->next)->data != data)
        {
                pointer = pointer -> next;
        }
        if(pointer->next==NULL)
        {
                printf("Element %d is not present in the list\n",data);
                return;
        }
        /* Now pointer points to a elem and the elem next to it has to be removed */
        elem *temp;
        temp = pointer -> next;
        /*temp points to the elem which has to be removed*/
        pointer->next = temp->next;
        /*We removed the elem which is next to the pointer (which is also temp) */
        free(temp);
        /* Beacuse we deleted the elem, we no longer require the memory used for it . 
           free() will deallocate the memory.
         */
        return;
}

void print(elem *pointer)
{
	pointer = pointer->next;
	printf("Print Sequential\n");
		while(pointer != NULL){
			printf("%d ",pointer->data);
			pointer = pointer->next;
		}
	printf("\n");	
	return;
}

void create(elem **head)
{
	(*head) = malloc(sizeof(elem));
	(*head)->next = NULL;
}

/*Linked list types*/
struct node {
   int key;
   struct node * next;
};

typedef struct node Node;


static const bool is_marked_reference(const uintptr_t p)
{
    return p & 0x1;
}

static const uintptr_t get_unmarked_reference(const uintptr_t p)
{	
    return p & 0xFFFFFFFE;
}

static const uintptr_t get_marked_reference(const uintptr_t p)
{
    return p | 0x1;
}

/*
	Create a linked list
*/
void Create_Linked_list(Node **head, Node **tail)
{
	printf("Create Function\n");
    (*head) = malloc(sizeof(Node));
    (**head).key = 0;
    (*tail) = malloc(sizeof(Node));
    (**tail).key = 100000;
    (*head)->next = (*tail);
}

/*
	Search
	- it takes a search key and returns references to two nodes called left node and right node for that key 
*/
static Node *search(Node *head, Node *tail, Node **left_node, int key){
	
	Node *left_node_next = NULL, *right_node = NULL;
	
	/*printf("Search Function\n");*/
	
	search_again:
	do{
		Node *t = head;

		Node *t_next = head->next;

		
		/* 1: Find left_node and right_node*/
		do{
			if(!is_marked_reference((uintptr_t) t_next)){
				(*left_node) = t;
				left_node_next = t_next;
			}
			
			t = (Node*) get_unmarked_reference((uintptr_t) t->next);
			
			if(t == tail){break;}
			t_next = t->next;

		}while(is_marked_reference((uintptr_t) t_next) || (t->key < key));
				
		right_node = t;

		/* 2: Check nodes are adjacent */
		if(left_node_next == right_node){
			if((right_node != tail) && is_marked_reference((uintptr_t) right_node->next)){
					goto search_again; /*G1*/
				}
			else{
					return right_node;
				}
		}
		
		/* 3: Remove one or more marked nodes */
		if(cas((uintptr_t*) (*left_node)->next, *(uintptr_t*) left_node_next, *(uintptr_t*) right_node)){
			if((right_node != tail) && is_marked_reference((uintptr_t) right_node->next))
				goto search_again; /*G2*/
			else{
				return right_node;
			}
		}
	}while(true); /*B2*/
	
	return NULL;
}

/*
	Insert
*/
bool Insert_Element_With_Index(Node *head, Node *tail, int new_key){
	Node *new_node = malloc(sizeof(Node));
	Node *right_node, *left_node;
	
	new_node->key = new_key;
	
	/*printf("Insert Function\n");*/
	
	do{
		
		right_node = search(head, tail, &left_node, new_key);
		
		if((right_node != tail) && (right_node->key == new_key)){
			return false;
		}
			
		new_node->next = right_node;

		if(cas(((uintptr_t*) &left_node->next), (uintptr_t) right_node, (uintptr_t) new_node )){
			return true;
		}	
	}while(true); /*B3*/
	
	return true;
}

bool Remove_Element_With_Index(Node *head, Node *tail, int search_key){
	
	Node *right_node, *right_node_next, *left_node;
	
	/*printf("Delete Function\n");*/
	
	do{
		
		right_node = search(head, tail, &left_node, search_key);
		
		if((right_node == tail) && (right_node->key != search_key))
			return false;
		right_node_next = right_node->next;
		
		if(!is_marked_reference((uintptr_t) right_node_next)){
			if(cas((uintptr_t*) &right_node->next, (uintptr_t) right_node_next, get_marked_reference(*(uintptr_t*) right_node_next))){
				printf("**** %d*******************\n",search_key);
				break;
			}
				
		}
	}while(true); /*B4*/
	
	if(!(cas((uintptr_t*) &left_node->next, (uintptr_t) right_node, (uintptr_t) right_node_next))){ /*C4*/
		right_node = search(head, tail, &left_node, search_key); 
		printf("**** %d*******************\n",search_key);
	}
	return true;
}

bool find(Node *head, Node *tail, int search_key){
	Node *right_node, *left_node;
	
	right_node = search(head, tail, &left_node, search_key);
	
	if((right_node == tail) || right_node->key != search_key)
		return false;
	else
		return true;
}

void print_list(Node *head, Node *tail)
{	
	Node *current_node;
	
	printf("Print Function\n");
    printf("[");
	
    for (current_node = head->next; current_node != tail; current_node = current_node->next) {
     
		if(is_marked_reference((uintptr_t) current_node))
			continue;				
        printf(" %d ", current_node->key);
    }
    printf("]\n");
}

bool nextBool()
{
	
	int val;
	val = (1 + (int)( 20.0 * rand() / ( RAND_MAX + 1.0 )));
	if(val % 2 == 0)
		return true;
	else
		return false;
}

static Node *list_head;
static Node *list_tail;
static elem *start;

void *worker(void *arg)
{
	int i = 0;
	int aux = 0;
	for (i = 0; i < *((int *)arg); ++i){
		
		/*If true generates insert, otherwise generates remove*/
		if(nextBool()){
			aux = randomNumber();
			if(Insert_Element_With_Index(list_head,list_tail,aux))
			{
				pthread_mutex_lock(&mutex1);
				insert(start,aux);	
				pthread_mutex_unlock(&mutex1);
			}	
		}
		else{
			aux = randomNumber();
			if(find(list_head,list_tail,aux))
			{
				if(Remove_Element_With_Index(list_head,list_tail,aux))
				{
					pthread_mutex_lock(&mutex2);
					delete(start,aux);	
					pthread_mutex_unlock(&mutex2);
				}
				/*
				printf("Deleted = %d\n",aux);
				printf("The ID of this thread is: %u\n", (unsigned int)pthread_self());
				*/
				
			}
		}
		pthread_mutex_lock(&mutex1);
		print_list(list_head, list_tail);
		print(start);
		pthread_mutex_unlock(&mutex1);
	}	
	return NULL;
}

int main(int argc, char **argv) {
	int i, count;
	pthread_t *t;
	
	/*
	Create_Linked_list(&list_head,&list_tail);
	Insert_Element_With_Index(list_head, list_tail, 10);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 20);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 30);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 40);
	print_list(list_head, list_tail);
	*/
	srand(time(NULL));
	
    create(&start);
	Create_Linked_list(&list_head,&list_tail);
	
	/*
	Insert_Element_With_Index(list_head, list_tail, 25);
	insert(start,25);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 30);
	insert(start,30);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 20);
	insert(start,20);
	printf("- %s - \n",Insert_Element_With_Index(list_head, list_tail, 20)?"true":"false");
	printf("- %s - \n",Remove_Element_With_Index(list_head, list_tail, 20)?"true":"false");
	printf("- %s - \n",Remove_Element_With_Index(list_head, list_tail, 50)?"true":"false");
	*/
		
	/*	
	Create_Linked_list(&list_head,&list_tail);
	Insert_Element_With_Index(list_head, list_tail, 25);
	insert(start,25);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 30);
	insert(start,30);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 20);
	insert(start,20);
	print_list(list_head, list_tail);
	print(start);
	printf("- %s - \n",Insert_Element_With_Index(list_head, list_tail, 20)?"true":"false");
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 20);
	print_list(list_head, list_tail);
	printf("BOOL - %s - \n",nextBool()?"true":"false");
	printf("BOOL - %s - \n",nextBool()?"true":"false");
	printf("BOOL - %s - \n",nextBool()?"true":"false");
	printf("BOOL - %s - \n",nextBool()?"true":"false");
	printf("BOOL - %s - \n",nextBool()?"true":"false");
	*/
	/*
	Nao funciona
	*/
	/*
	Create_Linked_list(&list_head,&list_tail);
	Insert_Element_With_Index(list_head, list_tail, 1);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 3);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 5);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 2);
	print_list(list_head, list_tail);
	Insert_Element_With_Index(list_head, list_tail, 50);
	print_list(list_head, list_tail);
	Remove_Element_With_Index(list_head, list_tail, 3);
	print_list(list_head, list_tail);
	Remove_Element_With_Index(list_head, list_tail, 50);
	print_list(list_head, list_tail);
	Remove_Element_With_Index(list_head, list_tail, 1);
	print_list(list_head, list_tail);
	*/
/*	printf("%d\n",list_head->key);
	printf("%d\n",(list_head->next)->key);
	printf("%d\n",((list_head->next)->next)->key);
	printf("%d\n",list_tail->key);
	printf("%d\n",list_head->key);
	print_list(list_head, list_tail);
	*/
	/*
	printf("---------\n");
	printf("%d\n",list_head->key);
	elem = list_head->next;
	printf("%d\n",elem->key);
	printf("---------\n");
	
	ele = 10;
    printf("- %s - \n",insert(list_head, list_tail, ele)?"true":"false");
	
	printf("%d\n",list_head->key);
	
	
	elem = list_head->next;
	printf("%d\n",elem->key);
	elem2 = elem->next;
	printf("%d\n",elem->key);
	
	print_list(list_head, list_tail);
	
	
	ele = 20;
    insert(list_head, list_tail, ele);
	
	ele = 10;
	*/
	/*delete(list_head, list_tail, ele);*/
	/*
	printf("Elem 10: %s\n",(find(list_head, list_tail, ele))?"true":"false");
	
	printf("Elem 20: %s\n",(find(list_head, list_tail, 20))?"true":"false");
	
	print_list(list_head, list_tail);
	*/
	if (argc != 3) {
		fprintf(stderr, "%s <#-threads> <#-iterations>\n", argv[0]);
		return 1;
	}
	count = atoi(argv[2]);
	/* create # threads */
	t = (pthread_t *)calloc(atoi(argv[1]), sizeof(pthread_t));
	for (i = 0; i < atoi(argv[1]); ++i)
		assert(!pthread_create(&(t[i]), NULL, worker, (void *)&count));
	/* wait for the completion of all the threads */
	for (i = 0; i < atoi(argv[1]); ++i)
		assert(!pthread_join(t[i], NULL));
	/* print counter value */
	printf("all thread done -> counter=%lu\n", counter);
	
	print_list(list_head, list_tail);
	print(start);
	
	
	return 0;
}