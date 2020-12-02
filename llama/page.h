#ifndef PAGE_H_
#define PAGE_H_

#include "vertex_record.h"

const int M = 1;
const int PAGE_SIZE = 1 << M;

struct page{
    int id;
    vertex_record* vertices;
    page(int i) : id(i) { vertices = new vertex_record[PAGE_SIZE];}
    page(page &other, int i) : page(i){ 
        for(int i=0;i<PAGE_SIZE;++i) 
            vertices[i] = other[i]; 
    }
    ~page(){ delete[] vertices;}
    vertex_record& operator[](size_t i){ return vertices[i];}
};

#endif