#include "hashmap_project.h"

//This program takes strings as key and value.
//Multithreaded-uses mutexes to ensure no race condition
//Ensure hashmap can support concurrent insertions and removals

/** 
Compare if the keys are the same. 
*/
int compare(void* obj1, void* obj2) {
	//Cast back to char ptr
	char* a=(char*)obj1;
	char* b=(char*)obj2;
	
	if(strcmp(a,b) == 0) { //match!
		return 1;
	}
	return 0; 
}

/**
A very simple hash function for the input string key.
Generate a hash value.
*/
size_t hash(void* input) {
	size_t result=0;

	int length=strlen(((char*)input));
	for(int i = 0; i < length; i++) {
		result += ((char*)input)[i];
		//compress to 10 buckets
		result = (result* ((char*)input)[i]) % NUMBER_BUCKETS; 
	}
	return result;
}

/**
Free key in heap memory 
*/
void key_destroy(void* thing) {
	free((char*)thing);
}

/**
Free vaue in heap memory 
*/
void value_destroy(void* thing) {
	free((char*)thing );
}

/**
Creates a new hashmap and initialise 
*/
struct hash_map* hash_map_new(size_t (*hash)(void*), int (*cmp)(void*, void*), 
	void (*key_destruct)(void*), void (*value_destruct)(void*)) {

	if(hash == NULL || cmp == NULL || key_destruct == NULL || 
		value_destruct == NULL) {
		return NULL;
	}
	
	struct hash_map* hashmap=malloc(sizeof(struct hash_map));
	struct node** h=malloc(NUMBER_BUCKETS* sizeof(struct node*)); //assign space
	hashmap->array=h;

	
	//initialise all 10 to NULL
	for(int i = 0; i < NUMBER_BUCKETS; i++) {
		hashmap->array[i]= NULL;
		pthread_mutex_init(&hashmap->bucket_mutex[i], NULL); //initialise
	}
	hashmap->hash=hash;
	hashmap->cmp=cmp;
	hashmap->key_destruct=key_destruct;
	hashmap->value_destruct=value_destruct;

	return hashmap;
}
/**
Puts an entry(key and value) into the hashmap.
If the key exists, the old entry is removed and 
replaced with new value.
*/
void hash_map_put_entry_move(struct hash_map* map, void* k, void* v) {
	if(map == NULL || k == NULL || v == NULL) {
		return;
	}

	int idx=map->hash(k); //Get the hashcode(index)

	pthread_mutex_lock(&(map->bucket_mutex[idx]));
	
	//Check if any entry exists
	if(map->array[idx] == NULL) {
		struct node* new=malloc(sizeof(struct node)); //create new node 
		new->key=k;
		new->value=v;
		new->next= NULL;
		map->array[idx]= new; //put into the hash structure 
		
		pthread_mutex_unlock(&(map->bucket_mutex[idx]));
		return;
	}

	//Check if key already exists 
	struct node *temp = map->array[idx];

	while(temp != NULL) {
		int result= map->cmp(k, temp->key);
		
		if(result == 1) { //the same key
			//free the value & key of old node then replace with new one
			value_destroy(temp->value); 
			key_destroy(temp->key); 
			temp->value=v;
			temp->key=k;
			pthread_mutex_unlock(&(map->bucket_mutex[idx]));
			return; //exit function
		}else{
			temp = temp->next; //get next node
		}
	}
	
	//If there already exists nodes in the bucket, find last position.
	struct node *curr = map->array[idx];

	while(curr->next != NULL) {
		curr = curr->next;
	}
	struct node* new=malloc(sizeof(struct node)); //create new node 
	new->key= k;
	new->value= v;
	new->next= NULL;
	curr->next= new; //'link' the 2 nodes togther
	pthread_mutex_unlock(&(map->bucket_mutex[idx]));

	return;
}

/**
Remove an entry from the hash map using the key.
If the key doesn't exist, there will be no changes to the hash map.
If the key exists, entry(key and value) is removed
*/
void hash_map_remove_entry(struct hash_map* map, void* k) {
	if(map == NULL || k == NULL ) {
		return;
	}
	int idx=map->hash(k); //Get the hashcode(index) 

	pthread_mutex_lock(&(map->bucket_mutex[idx]));
	//Check if any entry exists using NULL.
	if(map->array[idx] == NULL) {
		pthread_mutex_unlock(&(map->bucket_mutex[idx]));
		return;
	}

	struct node* prev= NULL;
	struct node* curr= map->array[idx]; //current 
	while(curr != NULL) {
		int result= map->cmp(k, curr->key);
		if(result == 1) { //the same key
			if(prev == NULL) { //the target was the first entry/node
				//target node is head, need to remove it
				struct node* next = curr->next;
				key_destroy(curr->key);
				value_destroy(curr->value);
				free(curr); 
				curr=NULL;
				map->array[idx]= next; //set new first entry 
			}else{
				prev->next = curr->next; //linking 2 nodes that are not adjacent
				key_destroy(curr->key);
				value_destroy(curr->value);
				free(curr); 
				curr=NULL;
			}
			pthread_mutex_unlock(&(map->bucket_mutex[idx]));
			return;
		}else{
			prev = curr; 
			curr = curr->next; //get next node 
		}
	}

	pthread_mutex_unlock(&(map->bucket_mutex[idx]));
	return;
}

/**
Get the value based on the key in the map.
If the key does not exist, return NULL.
*/
void* hash_map_get_value_ref(struct hash_map* map, void* k) {
	if(k == NULL || map == NULL) { 
		return NULL;
	}

	int idx=map->hash(k); //Get the hashcode(index)

	pthread_mutex_lock(&(map->bucket_mutex[idx]));
	//Check if any entry exists using NULL. 
	if(map->array[idx] == NULL) {
		pthread_mutex_unlock(&(map->bucket_mutex[idx]));
		return NULL;
	}
	//Check if key already exists
	struct node *temp = map->array[idx];
	while(temp != NULL) {
		int result= map->cmp(k, temp->key);
		
		if(result == 1) { 
			pthread_mutex_unlock(&(map->bucket_mutex[idx]));
			return temp->value; 
		}else{
			temp = temp->next; //get next node
		}
	}
	pthread_mutex_unlock(&(map->bucket_mutex[idx]));
	return NULL;
}

/**
Destroy all entries in the map and the map itself.
*/
void hash_map_destroy(struct hash_map* map) {
	if(map == NULL) {
		return;
	}

	for(int i = 0 ; i < NUMBER_BUCKETS; i++) {
		if(map->array[i] == NULL) {
			free(map->array[i]); 
			continue;
		}
		//start iterating through buckets
		struct node *curr = map->array[i];
		while(curr != NULL) {
			//free 3 things: K, V & node itself 
			struct node *tmp = curr->next; //get next node
			key_destroy(curr->key);
			value_destroy(curr->value);
			free(curr);
			curr = tmp;
		}
	}
	//free array
	free(map->array);
	//free the hashmap itself 
	free(map);
	return;
}

/**
Prints out all contents in the hashtable 
*/
void print_all(struct hash_map* hashmap) {
	for(int i=0;i<NUMBER_BUCKETS;i++) {
		printf("Bucket number[%d]\n", i);
		if(hashmap->array[i] == NULL) {
			printf("No entries in bucket\n");
			continue;
		}
		struct node *curr = hashmap->array[i];
		while(curr != NULL) {
			printf("%s, %s\n", (char*)curr->key, (char*)curr->value);
			curr = curr->next;
		}
	}
}

int main() {

	struct hash_map* hashmap;
	hashmap=hash_map_new(&hash, &compare, &key_destroy, &value_destroy);

	//Create example entries to be put into hashtable 
	char* p1_k=malloc(20* sizeof(char));
	strcpy(p1_k, "Mike Ross");
	char* p1_v=malloc(20* sizeof(char));
	strcpy(p1_v, "Associate");
	
	char* p2_k=malloc(20* sizeof(char));
	strcpy(p2_k, "Rachel Zane");
	char* p2_v=malloc(20* sizeof(char));
	strcpy(p2_v, "Paralegal");
	
	char* p3_k=malloc(20* sizeof(char));
	strcpy(p3_k, "Harvey Spectre");
	char* p3_v=malloc(20* sizeof(char));
	strcpy(p3_v, "Closer");
	
	char* p4_k=malloc(20* sizeof(char));
	strcpy(p4_k, "Donna");
	char* p4_v=malloc(20* sizeof(char));
	strcpy(p4_v, "Secretary");
	
	char* p5_k=malloc(20* sizeof(char));
	strcpy(p5_k, "Jessica Pearson");
	char* p5_v=malloc(20* sizeof(char));
	strcpy(p5_v, "The Boss");
	
	char* p6_k=malloc(20* sizeof(char));
	strcpy(p6_k, "Rory");
	char* p6_v=malloc(20* sizeof(char));
	strcpy(p6_v, "Ace");

	hash_map_put_entry_move(hashmap, p1_k, p1_v); //1st entry 
	hash_map_put_entry_move(hashmap, p2_k, p2_v); //2nd entry 
	hash_map_put_entry_move(hashmap, p3_k, p3_v); //3rd entry 
	hash_map_put_entry_move(hashmap, p4_k, p4_v); //4th entry 
	hash_map_put_entry_move(hashmap, p5_k, p5_v); //5th entry 
	hash_map_put_entry_move(hashmap, p6_k, p6_v); //6th entry 

	print_all(hashmap);
	
	void* return_val= hash_map_get_value_ref(hashmap, p4_k); //Secretary
	printf("Getting the value of Donna: %s\n", (char*)return_val);
	
	printf("Removing entries: \n");
	hash_map_remove_entry(hashmap, p6_k); //remove Rory

	hash_map_destroy(hashmap); //free all dynamic memory

	return 0;
}