#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "container.h"

Vector newVector(size_t initialCapacity, size_t elementSize) {
	return (Vector) {
		.data = (uint8_t*) malloc(initialCapacity * elementSize),
		.elementSize = elementSize,
		.len = 0u,
		.capacity = initialCapacity
	};
}

void VecFree(Vector* vec) {
	free(vec->data);
	vec->capacity = 0u;
	vec->data = NULL;
	vec->len = 0u;
}

void* VecTop(Vector* vec) {
	assert(vec->len != 0);
	return vec->data + (vec->elementSize * (vec->len - 1));
}

void VecPush(Vector* vec, void* value) {
	if (vec->len >= vec->capacity) {
		vec->capacity *= 2;
		void* newPtr = realloc(vec->data, vec->capacity * vec->elementSize);
		vec->data = newPtr;
	}
	memcpy(vec->data + (vec->elementSize * vec->len), value, vec->elementSize);
	vec->len++;
}

void* VecPopBack(Vector* vec) {
	assert(vec->len != 0u);
	return vec->data + (vec->elementSize * --vec->len);
}

void VecClear(Vector* vec) {
	vec->len = 0;
}
//------------------------------------------------------------------------

PtrVec newPtrVec(size_t initialCapacity) {
	return (PtrVec) {
		.data = malloc(initialCapacity * sizeof(void*)),
		.len = 0,
		.capacity = initialCapacity,
	};
}

void PtrVecFree(PtrVec* pv) {
	free(pv->data);
	pv->capacity = 0;
	pv->len = 0;
	pv->data = NULL;
}

void PtrVecPush(PtrVec* pv, void* value) {
	if (pv->len >= pv->capacity) {
		pv->capacity *= 2;
		void* newPtr = realloc(pv->data, pv->capacity * sizeof(void**));
		pv->data = newPtr;
	}
	pv->data[pv->len++] = value;
}

void* PtrVecTop(PtrVec* pv) {
	assert(pv->len > 0);
	return pv->data[pv->len - 1];
}

void* PtrVecPopBack(PtrVec* pv) {
	assert(pv->len > 0);
	return pv->data[--pv->len];
}

void PtrVecClear(PtrVec* pv) {
	pv->len = 0;
}

//------------------------------------------------------------------------

NumVec newNumVec(size_t initialCapacity) {
	return (NumVec) {
		.data = (double*) malloc(initialCapacity * sizeof(double)),
		.len = 0,
		.capacity = initialCapacity,
	};
}

void NumVecFree(NumVec* nv) {
	free(nv->data);
	nv->capacity = 0;
	nv->len = 0;
	nv->data = NULL;
}

void NumVecPush(NumVec* vec, double n) {
	if (vec->len >= vec->capacity) {
		vec->capacity *= 2;
		void* newPtr = realloc(vec->data, vec->capacity * sizeof(double));
		vec->data = newPtr;
	}
	vec->data[vec->len++] = n;
}

double NumVecTop(NumVec* vec) {
	assert(vec->len > 0);
	return vec->data[vec->len - 1];
}

double NumVecPopBack(NumVec* vec) {
	assert(vec->len > 0);
	return vec->data[--vec->len];
}

void NumVecClear(NumVec * vec) {
	vec->len = 0;
}