#ifndef VERTEX_RECORD_H_
#define VERTEX_RECORD_H_

#include <cstdio>

class vertex_record{
    int64_t snapshot_id_;
    size_t offset_;
    size_t fragment_length_;
    // int64_t degree; //optional
public:
    vertex_record() : snapshot_id_(-1), offset_(0), fragment_length_(0) {}
    vertex_record(int64_t s_id, size_t o, size_t f_l) : snapshot_id_(s_id), offset_(o), fragment_length_(f_l){}
    ~vertex_record(){}
    
    void set_record(int64_t s_id, size_t o, size_t f_l){
        snapshot_id_ = s_id;
        offset_ = o;
        fragment_length_ = f_l;
    }

    int64_t snapshot_id(){return snapshot_id_;}
    size_t offset(){return offset_;}
    void printrecord(){
        printf("id: %ld | offset: %ld | fragment length: %ld\n", snapshot_id_, offset_, fragment_length_);
    }
};

#endif