#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//MACROS
#define FAILURE 0
#define SUCCESS 1
#define ALREADY_EXISTS 2

#define HASH_CONSTANT 5381
#define LEFTSHIFT 5

#define DOUBLE_QUOTES '"'
#define ADD_ENT "addent"
#define ADD_REL "addrel"
#define DEL_ENT "delent"
#define DEL_REL "delrel"
#define REPORT "report"
#define END "end"

//TODO: remake these numbers big
#define STRING_LENGTH 7
#define RELATION_NUMBER 7
#define ENTITY_NUMBER 512
#define RELATED_NUMBER 64
#define COLLISION_NUMBER 3
#define IDENTIFIER_NUMBER 15

#define STRING_GROWTH 1.5
#define RELATION_GROWTH 2
#define ENTITY_GROWTH 2
#define RELATED_GROWTH 2
#define TYPES_GROWTH 1.5
#define IDENTIFIER_GROWTH 1.5

#define FREQUENT_CHUNK 6

//TODO:
// fai funzionare la dhtRealloc,
// per ottimizzare la memoria aggiungi campo per indicare numero entry usate in ogni hash table che quando va a zero chima free,
// per ottimizzare la memoria togli gli hash da EntityHashTable
// per ottimizzare la memoria fai le free dopo le delrel e delent [FATTO],
// per ottimizzare il tempo inserisci un campo per saper se most frequent è da refreshare o no e controlla se farlo solo durante i report,
// per ottimizzare la memoria cambia alcuni long in int,
// rimuovi i blocchi commentati di test e le funzioni non utilizzate


//DATA STRUCTURES

//HASH TABLES
typedef char* String;

typedef struct e{
    String identifier;
    unsigned long hash;
}Entity;

typedef struct es{
    Entity* entities; //hash table of existing entities associated with their complete hash
    unsigned short index;
}EntityList;

typedef struct eh{
    EntityList* entries;
    unsigned index;
}EntityHashTable;


typedef struct entityList{
    unsigned long* entitiesHashes;
    unsigned short index;
}EntityArray;

typedef struct ht{
    EntityArray* entries; //hash table of relations which an entity has with others (for a relation type)
    unsigned index;
}HashTable;

typedef struct entity2D{
    HashTable start; //the relation start from this entity and goes to this table of entities
    HashTable end; //the relation start from this table of entities and goes to this entity
    unsigned endNum;
    unsigned long entityHash;
}EntityRelations;

typedef struct entity3D{
    EntityRelations* collisions;
    unsigned short index;
}EntityCube;

typedef struct entity4D{
    EntityCube* entries;
    unsigned index;
}DoubleHashTable;


typedef struct r{
    String relationName;
    DoubleHashTable hashTable;
    unsigned long* mostFrequentHash;
    //short refreshed;
    unsigned int frequencyIndex;
    unsigned int limit;
    unsigned frequency;
}RelationType;

typedef struct rl{
    RelationType* types;
    unsigned index;
    unsigned limit;
}RelationList;

//sorted array list for ids
typedef struct il{
    String* identifiers;
    unsigned limit;
    unsigned index;
}IdentifierList;

//TODO: check memory leaks in reAlloc functions
//FUNCTIONS

//reallocates a string
void stringReAlloc(String* name, int* limit) {
    *limit = (int) ((*limit) * STRING_GROWTH);
    String temp = NULL;

    while (temp == NULL) {
        temp = realloc(*name, sizeof(char) * (*limit));
    }

    *name = temp;
}

//reads a rel/ent identifier from stdin
void readIdentifier(String* identifier) {
    char identifierChar;
    int stringLimit = STRING_LENGTH - 1;
    int counterChar = 0;

    *identifier = malloc(sizeof(char) * STRING_LENGTH);

    //scanf(" %c", &identifierChar);
    do {
        identifierChar = getc(stdin);
    }while (identifierChar == ' ');

    while (identifierChar != ' '  &&  identifierChar != '\n'){
        if (counterChar >= stringLimit) //limit reached, reallocation
            stringReAlloc(&(*identifier), &stringLimit);

        if (identifierChar != DOUBLE_QUOTES) {//ignore "
            (*identifier)[counterChar] = identifierChar;
            counterChar += 1;
        }

        //scanf("%c", &identifierChar);
        identifierChar = getc(stdin);
    }

    if (counterChar < stringLimit) {
        (*identifier)[counterChar] = '\0';
        String temp = NULL;

        while (temp == NULL) {
            temp = realloc(*identifier, sizeof(char) * (counterChar + 1));
        }
        *identifier = temp;
    }
}

//hash function for string input
unsigned long hash(const unsigned char *str) {
    unsigned long hash = HASH_CONSTANT;
    int c;

    for (int i = 0; str[i] != '\0'; ++i) {
        c = str[i];
        hash = ((hash << LEFTSHIFT) + hash) + c; /* hash * 33 + c */
    }//while ((c = *str++))

    return hash;
}

//data structures initialisation
void newRelationList(RelationList* relations){
    (*relations).types = malloc(sizeof(RelationType) * RELATION_NUMBER);
    (*relations).index = 0;
    (*relations).limit = RELATION_NUMBER;
}

void newDoubleHashTable(DoubleHashTable* hashTable){
    (*hashTable).entries = calloc(80 * ENTITY_NUMBER, sizeof(EntityCube));
    (*hashTable).index = 80 * ENTITY_NUMBER;
}

void newRelationType(RelationType* type){
    //(*type).relationName = malloc(sizeof(char) * STRING_LENGTH);
    newDoubleHashTable(&((*type).hashTable));
    (*type).frequency = 0;
    (*type).mostFrequentHash = malloc(sizeof(unsigned long) * FREQUENT_CHUNK);
    (*type).frequencyIndex = 0;
    //(*type).refreshed = 0;
    (*type).limit = FREQUENT_CHUNK;
}

void newEntityCube(EntityCube* entityCube){
    (*entityCube).index = 0;
    (*entityCube).collisions = malloc(sizeof(EntityRelations) * COLLISION_NUMBER);
}

void newEntityRelations (EntityRelations* entityRelations, HashTable end, HashTable start){ //forse meglio togliere gli hash table
    (*entityRelations).end = end;
    (*entityRelations).start = start;

    (*entityRelations).entityHash = 0;
    (*entityRelations).endNum = 0;
}

void newHashTable (HashTable* hashTable){
    (*hashTable).entries = calloc(RELATED_NUMBER, sizeof(EntityArray));
    (*hashTable).index = RELATED_NUMBER;
}

void newEntityArray(EntityArray* entityArray){
    (*entityArray).entitiesHashes = malloc(sizeof(long) * COLLISION_NUMBER);
    (*entityArray).index = 0;
}

void newEntityHash (EntityHashTable* entityHash){
    (*entityHash).entries = calloc(ENTITY_NUMBER, sizeof(EntityList));
    (*entityHash).index = ENTITY_NUMBER;
}

void newEntityList(EntityList* entityList){
    (*entityList).entities = malloc(sizeof(Entity) * COLLISION_NUMBER);
    (*entityList).index = 0;
}

void newEntity(Entity* entity){
    (*entity).identifier = malloc(sizeof(char) * STRING_LENGTH);
    (*entity).hash = 0;
}


void initDataStructures(RelationList* relations, EntityHashTable* existingEntities){
    RelationList relationList;
    EntityHashTable entities;

    newRelationList(&relationList);
    newEntityHash(&entities);

    *relations = relationList;
    *existingEntities = entities;
}



//ADD ENT

short insertEnt(EntityHashTable* existingEntities, Entity* entity) {
    unsigned index;

    index = (*entity).hash % (*existingEntities).index;

    if ((*existingEntities).entries[index].entities == NULL) {
        (*existingEntities).entries[index].entities = malloc(sizeof(Entity) * COLLISION_NUMBER);

    } else if ((*existingEntities).entries[index].index == COLLISION_NUMBER){
        return FAILURE;
    }

    //new entity placed in first empty spot, index of collisions incremented
    (*existingEntities).entries[index].entities[(*existingEntities).entries[index].index].identifier = (*entity).identifier;
    (*existingEntities).entries[index].entities[(*existingEntities).entries[index].index].hash = (*entity).hash;
    (*existingEntities).entries[index].index += 1;

    return SUCCESS;
}


void freeEntityList (EntityList* entityList) {
    for (int i = 0; i < (*entityList).index; ++i) {
        if ((*entityList).entities[i].identifier != NULL) {
            free((*entityList).entities[i].identifier);
            (*entityList).entities[i].identifier = NULL;
        }
    }

    free((*entityList).entities);
}

void freeEntityHash (EntityHashTable* entityHash) {
    for (unsigned i = 0; i < (*entityHash).index; ++i) {
        if ((*entityHash).entries[i].entities != NULL){
            freeEntityList(&((*entityHash).entries[i]));
        }
    }

    free((*entityHash).entries);
    (*entityHash).entries = NULL;
    (*entityHash).index = 0;
}

void partialFreeEntityHash (EntityHashTable* entityHashTable) {
    for (unsigned i = 0; i < (*entityHashTable).index; ++i) {
        if ((*entityHashTable).entries[i].entities != NULL){
            free((*entityHashTable).entries[i].entities);
        }
    }

    free((*entityHashTable).entries);
}

//checks if an entity exists, if it does returns its hash
short checkEnt(EntityHashTable* entityHash, unsigned long hash1) {
    unsigned index;
    unsigned short collisionNum;
    Entity* entityCollisions;

    index = hash1 % (*entityHash).index;
    entityCollisions = (*entityHash).entries[index].entities;

    if (entityCollisions == NULL) //collision list not initialised
        return FAILURE;

    collisionNum = (*entityHash).entries[index].index;

    for (int i = 0; i < collisionNum; ++i) {
        if (entityCollisions[i].hash == hash1)
            return SUCCESS;
    }

    return FAILURE;
}

//not a true reallocation
unsigned entityHashReAlloc(EntityHashTable* entityHash, unsigned index) {
    EntityHashTable entityHash1;

    entityHash1.index = index * ENTITY_GROWTH;
    entityHash1.entries = calloc(entityHash1.index, sizeof(EntityList));

    for (unsigned i = 0; i < (*entityHash).index ; ++i) {

        if ((*entityHash).entries[i].entities != NULL) {

            for (int j = 0; j < (*entityHash).entries[i].index ; ++j) {

               if (insertEnt(&(entityHash1),&((*entityHash).entries[i].entities[j])) == FAILURE) {
                   partialFreeEntityHash(&(entityHash1));
                   return entityHash1.index; //FAILURE
               }
            }
        }
    }

    partialFreeEntityHash(&(*entityHash));
    *entityHash = entityHash1;

    return SUCCESS;
}



void addEnt(EntityHashTable* existingEntities) {
    Entity entity;

//    newEntity(&entity);

    readIdentifier(&(entity.identifier));
    entity.hash = hash((unsigned char *) entity.identifier);

    if (checkEnt(&(*existingEntities), entity.hash) != FAILURE) { //entity already existent
        free(entity.identifier);
        return;
    }

    while (insertEnt(&(*existingEntities), &entity) == FAILURE) {
        unsigned index = (*existingEntities).index;
        index = entityHashReAlloc(&(*existingEntities), index);
        while (index != SUCCESS) {
            index = entityHashReAlloc(&(*existingEntities), index);
        }
    }
}


/*
 * !!!!!!!!
 * ADD REL
 * !!!!!!!
 */
//look for entHash inside the collisions of a dht
short entBelongRel(unsigned long entHash, EntityCube dHTEntry) {
    if (dHTEntry.collisions != NULL){ //the collision list associated to the hash index is initialized

        for (int j = 0; j < dHTEntry.index; ++j) {

            if (dHTEntry.collisions[j].entityHash == entHash) //entity belongs to rel
                return SUCCESS;

        }
    }

    return FAILURE;
}



short addEntToRel(unsigned long entHash, EntityCube* dHTEntry) {
    if ((*dHTEntry).index < COLLISION_NUMBER) { //add entity to rel type

        if ((*dHTEntry).collisions == NULL) {
            (*dHTEntry).collisions = calloc(COLLISION_NUMBER, sizeof(EntityRelations));
        }

        (*dHTEntry).collisions[(*dHTEntry).index].entityHash = entHash;
        (*dHTEntry).index += 1;

        return SUCCESS;

    }

    return FAILURE;
}



//not a true reallocation
short dHTReAlloc(DoubleHashTable* doubleHashTable, unsigned index) {
    DoubleHashTable doubleHashTable1;
    unsigned hashIndex;

    doubleHashTable1.index = index * RELATION_GROWTH;
    doubleHashTable1.entries = calloc(doubleHashTable1.index, sizeof(EntityCube));

    for (unsigned i = 0; i < (*doubleHashTable).index; ++i) {

        if ((*doubleHashTable).entries[i].collisions != NULL) { //collision list initialised

            for (int j = 0; j < (*doubleHashTable).entries[i].index; ++j) {
                hashIndex = (*doubleHashTable).entries[i].collisions[j].entityHash % doubleHashTable1.index;

                if (doubleHashTable1.entries[hashIndex].collisions == NULL){
                    doubleHashTable1.entries[hashIndex].index = 0;
                    doubleHashTable1.entries[hashIndex].collisions = calloc(COLLISION_NUMBER, sizeof(EntityRelations));
                }

                if (doubleHashTable1.entries[hashIndex].index < COLLISION_NUMBER) {
                    doubleHashTable1.entries[hashIndex].collisions[doubleHashTable1.entries[hashIndex].index].entityHash = (*doubleHashTable).entries[i].collisions[j].entityHash;
                    doubleHashTable1.entries[hashIndex].collisions[doubleHashTable1.entries[hashIndex].index].start = (*doubleHashTable).entries[i].collisions[j].start;
                    doubleHashTable1.entries[hashIndex].collisions[doubleHashTable1.entries[hashIndex].index].end = (*doubleHashTable).entries[i].collisions[j].end;
                    doubleHashTable1.entries[hashIndex].collisions[doubleHashTable1.entries[hashIndex].index].endNum = (*doubleHashTable).entries[i].collisions[j].endNum;
                    doubleHashTable1.entries[hashIndex].index += 1;

                } else {
                    for (unsigned k = 0; k < doubleHashTable1.index; ++k) {
                        free(doubleHashTable1.entries[k].collisions);
                    }
                    free(doubleHashTable1.entries);
                    return FAILURE;
                }
            }
        }
    }

    for (unsigned l = 0; l < (*doubleHashTable).index; ++l) {
        free((*doubleHashTable).entries[l].collisions);
    }
    free((*doubleHashTable).entries);

    (*doubleHashTable) = doubleHashTable1;
    return SUCCESS;
}

short hTReAlloc(HashTable* hashTable, unsigned index) {
    HashTable hashTable1;
    unsigned hashIndex;

    hashTable1.index = index * RELATED_GROWTH;
    do{
        hashTable1.entries = calloc(hashTable1.index, sizeof(EntityArray));
    }while (hashTable1.entries == NULL);

    for (unsigned i = 0; i < (*hashTable).index; ++i) {

        if ((*hashTable).entries[i].entitiesHashes != NULL) {

            for (int j = 0; j < (*hashTable).entries[i].index; ++j) {
                hashIndex = (*hashTable).entries[i].entitiesHashes[j] % hashTable1.index;

                if (hashTable1.entries[hashIndex].entitiesHashes == NULL) {
                    hashTable1.entries[hashIndex].entitiesHashes = malloc(sizeof(unsigned long) * COLLISION_NUMBER);
                    hashTable1.entries[hashIndex].index = 0;
                }

                if (hashTable1.entries[hashIndex].index < COLLISION_NUMBER) {
                    hashTable1.entries[hashIndex].entitiesHashes[hashTable1.entries[hashIndex].index] = (*hashTable).entries[i].entitiesHashes[j];
                    hashTable1.entries[hashIndex].index += 1;

                } else {

                    for (unsigned k = 0; k < hashTable1.index; ++k) {
                        free(hashTable1.entries[k].entitiesHashes);
                    }

                    free(hashTable1.entries);
                    return FAILURE;
                }

            }
        }
    }

    for (unsigned l = 0; l < (*hashTable).index; ++l) {
        free((*hashTable).entries[l].entitiesHashes);
    }

    free((*hashTable).entries);
    (*hashTable) = hashTable1;

    return SUCCESS;
}

short hTInsert (HashTable* hashTable, unsigned long hash) {
    if ((*hashTable).entries == NULL) {
        newHashTable(&(*hashTable));
    }

    unsigned hashIndex = hash % (*hashTable).index;

    if ((*hashTable).entries[hashIndex].entitiesHashes == NULL) {
        newEntityArray(&((*hashTable).entries[hashIndex]));

    } else {

        for (int i = 0; i < (*hashTable).entries[hashIndex].index; ++i) {
            if ((*hashTable).entries[hashIndex].entitiesHashes[i] == hash)
                return ALREADY_EXISTS;
        }
    }


    if ((*hashTable).entries[hashIndex].index < COLLISION_NUMBER) {
        (*hashTable).entries[hashIndex].entitiesHashes[(*hashTable).entries[hashIndex].index] = hash;
        (*hashTable).entries[hashIndex].index += 1;

        return SUCCESS;

    } else {
        return FAILURE;
    }
}

int addRelToStartEnt(EntityCube* dHTEntry, unsigned long startHash, unsigned long endHash) {
    HashTable* start;

    for (int i = 0; i < (*dHTEntry).index; ++i) { //TODO:maybe this for loop is avoidable calling this function by the right collision

        if ((*dHTEntry).collisions[i].entityHash == startHash) {

            start = &((*dHTEntry).collisions[i].start);

            if (hTInsert(start, endHash) != FAILURE) {
                return -SUCCESS;

            } else {
                return i;
            }
        }
    }

    return -2;
}

void freeMostFrequentHash(RelationType* relationType){
    free((*relationType).mostFrequentHash);
    (*relationType).frequency = 0;
    (*relationType).frequencyIndex = 0;
    (*relationType).limit = FREQUENT_CHUNK;
}


long mostFrequentContains(RelationType* relationType, unsigned long hash) {
    for (unsigned i = 0; i < (*relationType).frequencyIndex; ++i) {
        if ((*relationType).mostFrequentHash[i] == hash)
            return i;
    }

    return -SUCCESS;
}


void addMostFrequentHash(RelationType* relationType, unsigned long hash, unsigned frequency){
    if (mostFrequentContains(&(*relationType), hash) >= FAILURE)
        return;

    if ((*relationType).frequencyIndex >= (*relationType).limit) {
        (*relationType).limit += FREQUENT_CHUNK;

        unsigned long* temp = NULL;

        while (temp == NULL) {
            temp = realloc((*relationType).mostFrequentHash, (*relationType).limit * sizeof(long));
        }
        (*relationType).mostFrequentHash = temp;
    }

    if ((*relationType).frequencyIndex < (*relationType).limit) {
        (*relationType).mostFrequentHash[(*relationType).frequencyIndex] = hash;
        (*relationType).frequencyIndex += 1;
        (*relationType).frequency = frequency;

    }

}


int addRelToEndEnt(EntityCube* dHTEntry, unsigned long startHash, unsigned long endHash, RelationType* relationType) {
    HashTable* end;

    for (int i = 0; i < (*dHTEntry).index; ++i) { //TODO:maybe this for loop is avoidable calling this function by the right collision

        if ((*dHTEntry).collisions[i].entityHash == endHash) {

            end = &((*dHTEntry).collisions[i].end);

            short insertResult = hTInsert(end, startHash);

            if (insertResult == SUCCESS) {

                (*dHTEntry).collisions[i].endNum += 1;

                if ((*dHTEntry).collisions[i].endNum > (*relationType).frequency) { //update of most frequent hash
                    freeMostFrequentHash(&(*relationType));
                    (*relationType).mostFrequentHash = malloc(sizeof(long) * FREQUENT_CHUNK);
                    addMostFrequentHash(&(*relationType),(*dHTEntry).collisions[i].entityHash, (*dHTEntry).collisions[i].endNum);

                } else if ((*dHTEntry).collisions[i].endNum == (*relationType).frequency) {
                    addMostFrequentHash(&(*relationType),(*dHTEntry).collisions[i].entityHash, (*dHTEntry).collisions[i].endNum);
                }

                return -SUCCESS;

            } else if (insertResult == ALREADY_EXISTS) {
                return -SUCCESS;

            } else {
                return i;
            }
        }
    }

    return -2;
}


long findRelType(RelationList* relationList, String* identifier) {
    for (unsigned i = 0; i < (*relationList).index; ++i) {

        if (strcmp((*relationList).types[i].relationName, *identifier) == 0) //this type of rel already exists
            return i;

    }

    return -SUCCESS;
}


void addRel(RelationList* relationList, EntityHashTable* entityHash) {
    String identifier, identifier1;
    String relIdentifier;
    unsigned long hash1, hash2;

    readIdentifier(&identifier);
    readIdentifier(&identifier1);
    readIdentifier(&relIdentifier);

    hash1 = hash((unsigned char*) identifier);

    if (checkEnt(&(*entityHash), hash1) == FAILURE) { //entity nonexistent
        free(identifier);
        free(identifier1);
        free(relIdentifier);
        return;
    }

    hash2 = hash((unsigned char*) identifier1);

    if (checkEnt(&(*entityHash), hash2) == FAILURE) {//entity nonexistent
        free(identifier);
        free(identifier1);
        free(relIdentifier);
        return;
    }

    //the two entities exist
    unsigned hashIndex1;
    unsigned hashIndex2;
    unsigned index;
    EntityCube* dHTEntry1;
    EntityCube* dHTEntry2;
    short addResult;


    free(identifier);
    free(identifier1);

    long typeIndex = findRelType(&(*relationList), &relIdentifier);

    if (typeIndex < 0) {

        //reallocation of relation list
        if ((*relationList).index >= (*relationList).limit) {
            (*relationList).limit = (unsigned) ((*relationList).limit * TYPES_GROWTH);

            RelationType* temp = NULL;

            while (temp == NULL) {
                temp = realloc((*relationList).types, (*relationList).limit * sizeof(RelationType));
            }

            (*relationList).types = temp;
        }


        typeIndex = (*relationList).index;
        newRelationType(&((*relationList).types[typeIndex]));
        (*relationList).types[typeIndex].relationName = relIdentifier;
        (*relationList).index += 1;

    } else {
        free(relIdentifier);
    }

    hashIndex1 = hash1 % (*relationList).types[typeIndex].hashTable.index;
    hashIndex2 = hash2 % (*relationList).types[typeIndex].hashTable.index;
    dHTEntry1 = &((*relationList).types[typeIndex].hashTable.entries[hashIndex1]);
    index = (*relationList).types[typeIndex].hashTable.index;

    if (entBelongRel(hash1, *dHTEntry1) == FAILURE) { //add ent1 to rel type

//        if (hash1 == 6952061677912){
//            printf("malee");
//        }

        //TODO: colpa di questo tipo di blocco qua se non funzia
        addResult = addEntToRel(hash1, dHTEntry1);

        while (addResult == FAILURE) {

            while (dHTReAlloc(&((*relationList).types[typeIndex].hashTable), index) == FAILURE) {
                index = index * RELATION_GROWTH;
            }

            hashIndex1 = hash1 % (*relationList).types[typeIndex].hashTable.index;
            dHTEntry1 = &((*relationList).types[typeIndex].hashTable.entries[hashIndex1]);
            index = (*relationList).types[typeIndex].hashTable.index;
            addResult = addEntToRel(hash1, dHTEntry1);
        }
    }

    //TODO:RIMUOVI QUESTI BLOCCHI DI TEST!!!!!!!
//    if (dHTEntry1->collisions == NULL) {
//        return;
//    }

    dHTEntry2 = &((*relationList).types[typeIndex].hashTable.entries[hashIndex2]);
    index = (*relationList).types[typeIndex].hashTable.index;

    if (entBelongRel(hash2, *dHTEntry2) == FAILURE) { //add ent2 to rel type

//        if (hash2 == 6952061677912){
//            printf("male");
//        }
        addResult = addEntToRel(hash2, dHTEntry2);

        while (addResult == FAILURE) {

            while (dHTReAlloc(&((*relationList).types[typeIndex].hashTable), index) == FAILURE)
                index = index * RELATION_GROWTH;

            hashIndex2 = hash2 % (*relationList).types[typeIndex].hashTable.index;
            dHTEntry2 = &((*relationList).types[typeIndex].hashTable.entries[hashIndex2]);
            index = (*relationList).types[typeIndex].hashTable.index;
            addResult = addEntToRel(hash2, dHTEntry2);

        }
    }

    hashIndex1 = hash1 % (*relationList).types[typeIndex].hashTable.index;
    dHTEntry1 = &((*relationList).types[typeIndex].hashTable.entries[hashIndex1]);
    int htIndex = addRelToStartEnt(dHTEntry1, hash1, hash2);

//    if (dHTEntry1->collisions == NULL)
//        return;

    //addition to start
    while (htIndex != -SUCCESS) {
        if (htIndex != -2) {
            index = (*dHTEntry1).collisions[htIndex].start.index;

            while (hTReAlloc(&((*dHTEntry1).collisions[htIndex].start), index) == FAILURE) {
                index = index * RELATED_GROWTH;
            }
        }

//        if (dHTEntry1->collisions == NULL)
//            return;

        htIndex = addRelToStartEnt(dHTEntry1, hash1, hash2);

//        if (dHTEntry1->collisions == NULL)
//            return;
    }


    //addition to end
    htIndex = addRelToEndEnt(dHTEntry2, hash1, hash2, &((*relationList).types[typeIndex]));

    while (htIndex != -SUCCESS) {
        if (htIndex != -2) {
            index = (*dHTEntry2).collisions[htIndex].end.index;

            while (hTReAlloc(&((*dHTEntry2).collisions[htIndex].end), index) == FAILURE) {
                index = index * RELATED_GROWTH;
            }
        }

        htIndex = addRelToEndEnt(dHTEntry2, hash1, hash2, &((*relationList).types[typeIndex]));
    }
}


//DEL REL
int EntBelongHT(unsigned long hash, HashTable* hashTable) {
    unsigned hashIndex = hash % (*hashTable).index;

    if ((*hashTable).entries[hashIndex].entitiesHashes == NULL)
        return -SUCCESS;

    for (int i = 0; i < (*hashTable).entries[hashIndex].index; ++i) {
        if ((*hashTable).entries[hashIndex].entitiesHashes[i] == hash)
            return i;
    }

    return -SUCCESS;
}

short hTRemove (HashTable* hashTable, unsigned long hash) {
    if (hashTable == NULL  ||  (*hashTable).entries == NULL)
        return SUCCESS;

    unsigned hashIndex = hash % (*hashTable).index;
    int collisionIndex;

    collisionIndex = EntBelongHT(hash, &(*hashTable));

    if (collisionIndex == -SUCCESS)
        return FAILURE;

    (*hashTable).entries[hashIndex].entitiesHashes[collisionIndex] = (*hashTable).entries[hashIndex].entitiesHashes[(*hashTable).entries[hashIndex].index - 1];
    (*hashTable).entries[hashIndex].index -= 1;

    if ((*hashTable).entries[hashIndex].index == 0) {
        free((*hashTable).entries[hashIndex].entitiesHashes);
        (*hashTable).entries[hashIndex].entitiesHashes = NULL;
    }

    return SUCCESS;
}

void removeMostFrequentWithIndex(RelationType* relationType, long index) {
    (*relationType).mostFrequentHash[index] = (*relationType).mostFrequentHash[(*relationType).frequencyIndex - 1];
    (*relationType).frequencyIndex -= 1;

    if ((*relationType).frequencyIndex == 0) {
        (*relationType).frequency = 0;
    }
}



void reFreshMostFrequentHash(RelationType* relationType) {
    for (unsigned i = 0; i < (*relationType).hashTable.index; ++i) {

        if ((*relationType).hashTable.entries[i].collisions != NULL) {

            for (int j = 0; j < (*relationType).hashTable.entries[i].index; ++j) {
                long index;

                if ((*relationType).hashTable.entries[i].collisions[j].endNum > 0) {

                    if ((*relationType).hashTable.entries[i].collisions[j].endNum > (*relationType).frequency) {
                        unsigned long hash = (*relationType).hashTable.entries[i].collisions[j].entityHash;
                        unsigned frequency = (*relationType).hashTable.entries[i].collisions[j].endNum;

                        freeMostFrequentHash(&(*relationType));
                        (*relationType).mostFrequentHash = malloc(sizeof(long) * FREQUENT_CHUNK);
                        addMostFrequentHash(&(*relationType), hash, frequency);

                    } else if ((*relationType).hashTable.entries[i].collisions[j].endNum == (*relationType).frequency) {
                        unsigned long hash = (*relationType).hashTable.entries[i].collisions[j].entityHash;
                        unsigned frequency = (*relationType).hashTable.entries[i].collisions[j].endNum;
                        addMostFrequentHash(&(*relationType), hash, frequency);

                    }  else {
                        index = mostFrequentContains(&(*relationType), (*relationType).hashTable.entries[i].collisions[j].entityHash);
                        if (index >= FAILURE) {
                            removeMostFrequentWithIndex(&(*relationType), index);

                            if ((*relationType).frequencyIndex == 0) {
                                i = 0;
                            }
                        }
                    }

                } else {
                    index = mostFrequentContains(&(*relationType), (*relationType).hashTable.entries[i].collisions[j].entityHash);

                    if (index >= FAILURE) {
                        removeMostFrequentWithIndex(&(*relationType), index);

                        if ((*relationType).frequencyIndex == 0) {
                            i = 0;
                        }
                    }
                }
            }
        }
    }

    if ((*relationType).frequency == 0  &&  (*relationType).frequencyIndex == 1) {
        (*relationType).frequencyIndex -= 1;
    }
}

short removeRel(DoubleHashTable* doubleHashTable, unsigned long hashEnt1, unsigned long hashEnt2) {
    unsigned hashIndexEnt1 = hashEnt1 % (*doubleHashTable).index;
    unsigned hashIndexEnt2 = hashEnt2 % (*doubleHashTable).index;

    int doubleCollisionIndex1, doubleCollisionIndex2;

    doubleCollisionIndex1 = -1;
    for (int i = 0; i < (*doubleHashTable).entries[hashIndexEnt1].index; ++i) {

        if ((*doubleHashTable).entries[hashIndexEnt1].collisions[i].entityHash == hashEnt1) {

            if ((*doubleHashTable).entries[hashIndexEnt1].collisions[i].start.entries == NULL  ||
            (*doubleHashTable).entries[hashIndexEnt1].collisions[i].start.index == 0)
                return FAILURE;

            doubleCollisionIndex1 = i;
            i = (*doubleHashTable).entries[hashIndexEnt1].index;
        }
    }

    if (doubleCollisionIndex1 < 0)
        return FAILURE;


    doubleCollisionIndex2 = -1;
    for (int i = 0; i < (*doubleHashTable).entries[hashIndexEnt2].index; ++i) {

        if ((*doubleHashTable).entries[hashIndexEnt2].collisions[i].entityHash == hashEnt2) {

            if ((*doubleHashTable).entries[hashIndexEnt2].collisions[i].end.entries == NULL  ||
                (*doubleHashTable).entries[hashIndexEnt2].collisions[i].end.index == 0)
                return FAILURE;

            doubleCollisionIndex2 = i;
            i = (*doubleHashTable).entries[hashIndexEnt2].index;
        }
    }

    if (doubleCollisionIndex2 < 0) {
        return FAILURE;
    }

    if (hTRemove(&((*doubleHashTable).entries[hashIndexEnt1].collisions[doubleCollisionIndex1].start), hashEnt2) == FAILURE)
        return FAILURE;

    hTRemove(&((*doubleHashTable).entries[hashIndexEnt2].collisions[doubleCollisionIndex2].end), hashEnt1);

    (*doubleHashTable).entries[hashIndexEnt2].collisions[doubleCollisionIndex2].endNum -= 1;

    return SUCCESS;
}


void delRel (RelationList* relationList) {
    String relIdentifier, ent1Identifier, ent2Identifier;
    long relIndex;
    unsigned long hashEnt1, hashEnt2;

    readIdentifier(&ent1Identifier);
    readIdentifier(&ent2Identifier);
    readIdentifier(&relIdentifier);

    hashEnt1 = hash((const unsigned char*) ent1Identifier);
    hashEnt2 = hash((const unsigned char*) ent2Identifier);
    relIndex = findRelType(&(*relationList), &relIdentifier); //I assume the rel type is always present

    free(ent1Identifier);
    free(ent2Identifier);
    free(relIdentifier);

    if (relIndex < 0)
        return;

    if (removeRel(&((*relationList).types[relIndex].hashTable), hashEnt1, hashEnt2) == FAILURE)
        return;

    //mostFrequentHash update
    for (unsigned k = 0; k < (*relationList).types[relIndex].frequencyIndex; ++k) {

        if ((*relationList).types[relIndex].mostFrequentHash[k] == hashEnt2) {

            if ((*relationList).types[relIndex].frequencyIndex == 1) {
                (*relationList).types[relIndex].frequency -= 1;
                reFreshMostFrequentHash(&((*relationList).types[relIndex]));
                //(*relationList).types[relIndex].refreshed = 0;

            } else {
                unsigned long lastElement = (*relationList).types[relIndex].mostFrequentHash[(*relationList).types[relIndex].frequencyIndex - 1];
                (*relationList).types[relIndex].mostFrequentHash[k] = lastElement;
                (*relationList).types[relIndex].frequencyIndex -= 1;
            }

            return;
        }
    }
}

//DEL ENT
void removeEnt(unsigned long hashEnt, EntityHashTable* entityHashTable) {
    unsigned hashIndex = hashEnt % (*entityHashTable).index;
    unsigned short collisionNum = (*entityHashTable).entries[hashIndex].index;

    for (int i = 0; i < collisionNum; ++i) {
        if ((*entityHashTable).entries[hashIndex].entities[i].hash == hashEnt) {
            free((*entityHashTable).entries[hashIndex].entities[i].identifier);
            (*entityHashTable).entries[hashIndex].entities[i] = (*entityHashTable).entries[hashIndex].entities[collisionNum - 1];
            (*entityHashTable).entries[hashIndex].entities[collisionNum - 1].identifier = NULL;
            (*entityHashTable).entries[hashIndex].entities[collisionNum - 1].hash = 0;
            (*entityHashTable).entries[hashIndex].index -= 1;
            i = collisionNum;

            if ((*entityHashTable).entries[hashIndex].index == 0) {
                free((*entityHashTable).entries[hashIndex].entities);
                (*entityHashTable).entries[hashIndex].entities = NULL;
            }
            //if index == 0 may be useful to free the mem
        }
    }
}

//removes all rel, of the entities contained in hash table, related with the hashtable owner
//it doesn't free the memory
void removeAllRelOfHT(HashTable* hashTable, unsigned long hash, DoubleHashTable* doubleHashTable, char hashTableType) {
    unsigned long otherEntHash;
    unsigned otherEntHashIndex;
    unsigned short otherEntCollIndex = 0;

    for (unsigned k = 0; k < (*hashTable).index; ++k) {

        if ((*hashTable).entries[k].entitiesHashes != NULL) {

            for (int l = 0; l < (*hashTable).entries[k].index; ++l) {
                otherEntHash = (*hashTable).entries[k].entitiesHashes[l];
                otherEntHashIndex = otherEntHash % (*doubleHashTable).index;

                for (int m = 0; m < (*doubleHashTable).entries[otherEntHashIndex].index; ++m) {

                    if ((*doubleHashTable).entries[otherEntHashIndex].collisions[m].entityHash == otherEntHash) {
                        otherEntCollIndex = m;
                        m = (*doubleHashTable).entries[otherEntHashIndex].index;
                    }
                }

                if (hashTableType == 's') {
                    hTRemove(&((*doubleHashTable).entries[otherEntHashIndex].collisions[otherEntCollIndex].end), hash);
                    (*doubleHashTable).entries[otherEntHashIndex].collisions[otherEntCollIndex].endNum -= 1;

                } else if (hashTableType == 'e') {
                    hTRemove(&((*doubleHashTable).entries[otherEntHashIndex].collisions[otherEntCollIndex].start), hash);
                }


            }
        }
    }
}

void freeHashTable(HashTable* hashTable) {
    if ((*hashTable).entries == NULL  ||  (*hashTable).index == 0) {
        return;
    }

    for (unsigned i = 0; i < (*hashTable).index; ++i) {
        if ((*hashTable).entries[i].entitiesHashes != NULL) {
            free((*hashTable).entries[i].entitiesHashes);
            (*hashTable).entries[i].entitiesHashes = NULL;
            (*hashTable).entries[i].index = 0;
        }
    }

    free((*hashTable).entries);
    (*hashTable).entries = NULL;
    (*hashTable).index = 0;
}

void freeEntityRelations(EntityRelations* entityRelations) {
    freeHashTable(&((*entityRelations).start));
    freeHashTable(&((*entityRelations).end));
    (*entityRelations).endNum = 0;
}

void removeMostFrequent(RelationType* relationType, unsigned long hash) {
    for (unsigned i = 0; i < (*relationType).frequencyIndex; ++i) {
        if ((*relationType).mostFrequentHash[i] == hash) {
            removeMostFrequentWithIndex(&(*relationType), i);
            return;
        }
    }
}


void delEnt(EntityHashTable* entityHashTable, RelationList* relationList) {
    Entity entity;
    unsigned hashIndex;
    unsigned short collisionNum;
    EntityRelations* removedEntity;

    readIdentifier(&entity.identifier);
    entity.hash = hash((const unsigned char*)entity.identifier);
    free(entity.identifier);

    removeEnt(entity.hash, &(*entityHashTable));

    for (unsigned i = 0; i < (*relationList).index; ++i) {
        hashIndex = entity.hash % (*relationList).types[i].hashTable.index;
        collisionNum = (*relationList).types[i].hashTable.entries[hashIndex].index;

        for (int j = 0; j < collisionNum; ++j) {

            if ((*relationList).types[i].hashTable.entries[hashIndex].collisions[j].entityHash == entity.hash) {
                removedEntity = &((*relationList).types[i].hashTable.entries[hashIndex].collisions[j]);
                removeAllRelOfHT(&((*removedEntity).start), entity.hash, &((*relationList).types[i].hashTable), 's');
                removeAllRelOfHT(&((*removedEntity).end), entity.hash, &((*relationList).types[i].hashTable), 'e');
                freeEntityRelations(removedEntity);

                (*relationList).types[i].hashTable.entries[hashIndex].collisions[j] = (*relationList).types[i].hashTable.entries[hashIndex].collisions[collisionNum - 1];
                (*relationList).types[i].hashTable.entries[hashIndex].index -= 1;
                (*relationList).types[i].hashTable.entries[hashIndex].collisions[collisionNum - 1].end.entries = NULL;
                (*relationList).types[i].hashTable.entries[hashIndex].collisions[collisionNum - 1].end.index = 0;
                (*relationList).types[i].hashTable.entries[hashIndex].collisions[collisionNum - 1].start.entries = NULL;
                (*relationList).types[i].hashTable.entries[hashIndex].collisions[collisionNum - 1].start.index = 0;

                if ((*relationList).types[i].hashTable.entries[hashIndex].index == 0) {
                    free((*relationList).types[i].hashTable.entries[hashIndex].collisions);
                    (*relationList).types[i].hashTable.entries[hashIndex].collisions = NULL;
                }
                //TODO: if index == 0 may be useful to free the mem

                removeMostFrequent(&((*relationList).types[i]), entity.hash);
                reFreshMostFrequentHash(&((*relationList).types[i]));
                //(*relationList).types[i].refreshed = 0;

                j = collisionNum;
            }
        }
    }

}



//REPORT

String getEntName(unsigned long hash, EntityHashTable* entityHashTable) {
    unsigned hashIndex = hash % (*entityHashTable).index;

    for (int i = 0; i < (*entityHashTable).entries[hashIndex].index; ++i) {
        if ((*entityHashTable).entries[hashIndex].entities[i].hash == hash) {
            //TODO: togli questo if, è di test
//            if ((*entityHashTable).entries[hashIndex].entities[i].identifier == NULL)
//                return NULL;
            return (*entityHashTable).entries[hashIndex].entities[i].identifier;
        }
    }

    return NULL;
}

long binaryComparison(IdentifierList* identifierList, String* identifier) {
    if ((*identifierList).index == 0) {
        return 0;
    }

    unsigned size = ((*identifierList).index - 1) / 2;
    unsigned lower = 0;
    unsigned upper = (*identifierList).index - 1;
    int compareResult;

    //TODO: qui c'è un seg fault, controllare cosa succede agli hash quando viene fatta la free di entity hash table
    if (strcmp((*identifierList).identifiers[(*identifierList).index - 1], *identifier) < 0)
        return (*identifierList).index;

    if (strcmp((*identifierList).identifiers[0], *identifier) > 0)
        return 0;

    for (unsigned i = 0; i < (*identifierList).index; ++i) {
        compareResult = strcmp((*identifierList).identifiers[size], *identifier);

        if (compareResult == 0) {
            return FAILURE;

        } else if (compareResult < 0) {
            lower = size;

        } else {
            upper = size;
        }

        size = lower + (upper - lower) / 2;

        if (upper - lower == 1) {
            return size + 1;
        }

    }

    return size;
}

void printReport(String* relIdentifier, IdentifierList* hashes, unsigned frequency) {
    //printf("%c%s%c ",DOUBLE_QUOTES, *relIdentifier, DOUBLE_QUOTES);
    fputs("\"", stdout);
    fputs(*relIdentifier, stdout);
    fputs("\" ", stdout);

    for (unsigned i = 0; i < (*hashes).index; ++i) {
        //printf("%c%s%c ",DOUBLE_QUOTES, (*hashes).identifiers[i], DOUBLE_QUOTES);
        fputs("\"", stdout);
        fputs((*hashes).identifiers[i], stdout);
        fputs("\" ", stdout);
    }

    printf("%u; ",frequency);
}


void report(EntityHashTable* existingEntities, RelationList* relationList) {
    if ((*relationList).index == 0) {
        //printf("none\n");
        puts("none");
        return;
    }

    long indexNewEl;
    IdentifierList relations, hashes;
    unsigned long relTypeIndex;
    relations.index = 0;
    relations.limit = IDENTIFIER_NUMBER;
    relations.identifiers = malloc(sizeof(String) * IDENTIFIER_NUMBER);
    String identifier;

    for (unsigned i = 0; i < (*relationList).index; ++i) {

        if ((*relationList).types[i].frequency != 0) {

//            if ((*relationList).types[i].refreshed == 0  && (*relationList).types[i].hashTable.index != 0) {
//                reFreshMostFrequentHash( &((*relationList).types[i]));
//                (*relationList).types[i].refreshed = 1;
//            }

            identifier = (*relationList).types[i].relationName;
            indexNewEl = binaryComparison(&relations, &identifier);

            if (relations.index == relations.limit) {
                relations.limit = (unsigned) (relations.limit * IDENTIFIER_GROWTH);
                String* temp = realloc(relations.identifiers, relations.limit * sizeof(String));

                while (temp == NULL) {
                    temp = realloc(relations.identifiers, relations.limit * sizeof(String));
                }
                 relations.identifiers = temp;
            }

            if (indexNewEl != relations.index) {

                for (long j = relations.index; j > indexNewEl; --j) {
                    relations.identifiers[j] = relations.identifiers[j - 1];
                }
            }

            relations.identifiers[indexNewEl] = identifier;
            relations.index += 1;
        }
    }

    if (relations.index == 0) {
        //printf("none\n");
        puts("none");
        free(relations.identifiers);
        return;
    }

    hashes.limit = IDENTIFIER_NUMBER;
    hashes.identifiers = malloc(sizeof(String) * IDENTIFIER_NUMBER);

    for (unsigned k = 0; k < relations.index; ++k) {
        hashes.index = 0;
        relTypeIndex = findRelType(&(*relationList), &(relations.identifiers[k]));

        for (unsigned i = 0; i < (*relationList).types[relTypeIndex].frequencyIndex; ++i) {
            identifier = getEntName((*relationList).types[relTypeIndex].mostFrequentHash[i], &(*existingEntities));
            //TODO:LEVALOOO
//            if (identifier == NULL)
//                continue;
            indexNewEl = binaryComparison(&hashes, &identifier);

            if (hashes.index == hashes.limit) {
                hashes.limit = (unsigned) (hashes.limit * IDENTIFIER_GROWTH);
                String* temp = NULL;

                while (temp == NULL) {
                    temp = realloc(hashes.identifiers, hashes.limit * sizeof(String));
                }
                hashes.identifiers = temp;
            }

            if (indexNewEl != hashes.index) {

                for (long j = hashes.index; j > indexNewEl; --j) {
                    hashes.identifiers[j] = hashes.identifiers[j - 1];
                }
            }

            hashes.identifiers[indexNewEl] = identifier;
            hashes.index += 1;
        }

        printReport((&relations.identifiers[k]), &hashes, (*relationList).types[relTypeIndex].frequency);
    }

    puts("");
    //printf("\n");

    free(relations.identifiers);
    free(hashes.identifiers);
}

//END
void end(EntityHashTable* existingEntities, RelationList* relationList) {
    freeEntityHash(&(*existingEntities));

    for (unsigned i = 0; i < (*relationList).index; ++i) {
        free((*relationList).types[i].relationName);
        free((*relationList).types[i].mostFrequentHash);

        if ((*relationList).types[i].hashTable.entries != NULL) {

            for (unsigned j = 0; j < (*relationList).types[i].hashTable.index; ++j) {

                if ((*relationList).types[i].hashTable.entries[j].collisions != NULL) {

                    for (int k = 0; k < (*relationList).types[i].hashTable.entries[j].index; ++k) {

                        if ((*relationList).types[i].hashTable.entries[j].collisions[k].end.entries != NULL  ||
                        (*relationList).types[i].hashTable.entries[j].collisions[k].start.entries != NULL) {
                            freeEntityRelations(&((*relationList).types[i].hashTable.entries[j].collisions[k]));
                        }
                    }
                }

                free((*relationList).types[i].hashTable.entries[j].collisions);
            }
        }
        free((*relationList).types[i].hashTable.entries);
    }
    free((*relationList).types);
}


//INPUT PARSING
short parseInput(EntityHashTable* existingEntities, RelationList* relationList){
    String input = malloc(sizeof(char) * STRING_LENGTH);
    scanf("%s", input);

    if (!strcmp(input, ADD_ENT)){
        addEnt(&(*existingEntities));

    }else if (!strcmp(input, ADD_REL)){
        addRel(&(*relationList), &(*existingEntities));

    } else if (!strcmp(input, DEL_REL)){
        delRel(&(*relationList));

    } else if (!strcmp(input, DEL_ENT)){
        delEnt(&(*existingEntities), &(*relationList));

    } else if (!strcmp(input, REPORT)){
        report(&(*existingEntities), &(*relationList));

    } else if (!strcmp(input, END)){
        end(&(*existingEntities), &(*relationList));
        free(input);
        return SUCCESS;
    }

    free(input);
    return FAILURE;
}

int main() {
    RelationList relations;
    EntityHashTable existingEntities;

    initDataStructures(&relations, &existingEntities);

    while (parseInput(&existingEntities, &relations) == FAILURE);

    return 0;
}
