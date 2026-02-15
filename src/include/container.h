#pragma once

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define AS_INT(PTR) (*(int*)(PTR))
#define AS_DOUBLE(PTR) (*(double*)(PTR))
#define AS_FLOAT(PTR) (*(float*)(PTR))
#define AS_UINT(PTR) (*(unsigned int*)(PTR))
#define AS_OP_CODE(PTR) (*(enum OP_CODE*)(PTR))

typedef struct {
	uint8_t* data;
	size_t elementSize;
	size_t len;
	size_t capacity;
} Vector;

Vector newVector(size_t initialCapacity, size_t elementSize);
void VecFree(Vector*);
void VecPush(Vector*, void* value);
void VecAdd(Vector*, void* value);
void* VecTop(Vector*);
void* VecPopBack(Vector*);
void VecClear(Vector*);

static inline void VecSet(Vector* ar, size_t i, void* value) {
	assert(i < ar->capacity);
	memcpy(ar->data + ar->elementSize * i, value, ar->elementSize);
}

static inline void* VecAt(Vector* ar, size_t i) {
	assert(i < ar->capacity);
	return ar->data + ar->elementSize * i;
}

//-----------------------------------------------------------------------------
typedef struct {
	void** data;
	size_t len;
	size_t capacity;
} PtrVec;

PtrVec newPtrVec(size_t initialCapacity);
void PtrVecFree(PtrVec*);
void PtrVecPush(PtrVec*, void*);
void* PtrVecTop(PtrVec*);
void* PtrVecPopBack(PtrVec*);
void PtrVecClear(PtrVec*);

static inline void PtrVecSet(PtrVec* ar, size_t i, void* value) {
	assert(i < ar->capacity);
	ar->data[i] = value;
}

static inline void* PtrVecAt(PtrVec* ar, size_t i) {
	return ar->data[i];
}
//-----------------------------------------------------------------------------

typedef struct NumVec {
	double* data;
	size_t len;
	size_t capacity;
} NumVec;

NumVec newNumVec(size_t initialCapacity);
void NumVecFree(NumVec*);
void NumVecPush(NumVec*, double);
double NumVecTop(NumVec*);
double NumVecPopBack(NumVec*);

static inline void NumVecSet(NumVec* ar, size_t i, double value) {
	assert(i < ar->capacity);
	ar->data[i] = value;
}

static inline double NumVecAt(NumVec* ar, size_t i) {
	return ar->data[i];
}

void NumVecClear(NumVec*);
