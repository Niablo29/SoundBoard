#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct seg_node {
    int16_t* data;
    size_t length;
    struct seg_node* next;

} seg_node;

struct sound_seg {
    //TODO
    seg_node* head;
    size_t total_length;
};


// Load a WAV file into buffer
void wav_load(const char* filename, int16_t* dest){
    return;
}

// Create/write a WAV file from buffer
void wav_save(const char* fname, int16_t* src, size_t len){
    return;
}

// Initialize a new sound_seg object
struct sound_seg* tr_init() {
    struct sound_seg* seg = malloc(sizeof(struct sound_seg));
    if (!seg) return NULL;
    seg->head = NULL;
    seg->total_length = 0;
    return seg;
}

// Destroy a sound_seg object and free all allocated memory
void tr_destroy(struct sound_seg* obj) {
    if(!obj) return NULL;
    seg_node* current = obj->head;
    seg_node* next;

    while (current != NULL){
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    free(obj);

    return;
}

// Return the length of the segment
size_t tr_length(struct sound_seg* seg) {
    if(!seg) return 0;
    return seg->total_length;
}

static void append_new_node(struct sound_seg* track, const int16_t* src, size_t len){

    seg_node* new_node = malloc(sizeof(seg_node));
    new_node->data = malloc(len * sizeof(int16_t));
    memcpy(new_node->data, src, len * sizeof(int16_t));
    new_node->length = len;
    new_node->next = NULL;

    track->total_length += len;

    if (!track->head) {
        track->head = new_node;
    } else {
        seg_node* current = track->head;
        while (current->next) {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Read len elements from position pos into dest
void tr_read(struct sound_seg* track, int16_t* dest, size_t pos, size_t len) {
    if(!track || !dest || len == 0) return 0;

    seg_node* current = track->head;
    size_t i = 0;
    size_t remaining = 0;
    size_t write_idx = 0;

    while ((current) && (remaining > 0)){
        size_t start = i;
        size_t end = i + current->length;

        if (pos < end){
            size_t start_in_node;
            if (pos > start) {
                start_in_node = pos - start;
            } else {
                start_in_node = 0;
            }
            size_t can_read = current->length - start_in_node;

            if (can_read > remaining){
                can_read = remaining;
            }

            memcpy(&dest[write_idx], &current->data[start_in_node], can_read * sizeof(int16_t));

            write_idx   += can_read;
            remaining   -= can_read;
            pos         += can_read;
        }

        i += current->length;
        current = current->next;

    }

    return;
}

// Write len elements from src into position pos
void tr_write(struct sound_seg* track, int16_t* src, size_t pos, size_t len) {

    if (!track || !src || len == 0) return;

    if (pos >= track->total_length) {
        append_new_node(track, src, len);
        return;
    }
    
    seg_node* cur = track->head;
    size_t offset = 0;
    size_t written = 0;
    size_t to_write = len;

    while (cur && to_write > 0) {
        size_t node_start = offset;
        size_t node_end   = offset + cur->length;

        if (pos < node_end) {
            size_t start_in_node = pos - node_start;
            size_t can_write = cur->length - start_in_node;
            if (can_write > to_write) {
                can_write = to_write;
            }
            memcpy(&cur->data[start_in_node],
                   &src[written],
                   can_write * sizeof(int16_t));

            written += can_write;
            to_write -= can_write;
            pos      += can_write;
        }
        offset += cur->length;
        cur = cur->next;
    }

    if (to_write > 0) {
        append_new_node(track, &src[written], to_write);
    }
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    return true;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad){
    return NULL;
}

// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {
    return;
}
