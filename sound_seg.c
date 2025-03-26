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

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        return;
    }
    
    (void) fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);
    
    (void) fseek(fp, 44, SEEK_SET);
    
    size_t num_samples = (file_size - 44) / sizeof(int16_t);
    
    size_t samples_read = fread(dest, sizeof(int16_t), num_samples, fp);
    if (samples_read != num_samples) {
        fprintf(stderr, "Warning: expected %zu samples, but only read %zu\n", num_samples, samples_read);
    }
    
    fclose(fp);
}

// Create/write a WAV file from buffer
void wav_save(const char* fname, const int16_t* src, size_t len){
    FILE* fp = fopen(fname, "wb");
    if (!fp) {
        fprintf(stderr, "Error: could not open file %s for writing\n", fname);
        return;
    }
    
    uint32_t sample_rate = 8000;
    uint16_t mono = 1;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * bits_per_sample / 8;
    uint16_t block_align = bits_per_sample / 8;
    uint32_t subchunk2_size = len * bits_per_sample / 8;
    uint32_t chunk_size = 36 + subchunk2_size;
    
    fwrite("RIFF", 1, 4, fp);
    fwrite(&chunk_size, 4, 1, fp);
    fwrite("WAVE", 1, 4, fp);
    
    fwrite("fmt ", 1, 4, fp);
    uint32_t subchunk1_size = 16;
    fwrite(&subchunk1_size, 4, 1, fp);
    uint16_t audio_format = 1;     // PCM = 1
    fwrite(&audio_format, 2, 1, fp);
    fwrite(&mono, 2, 1, fp);
    fwrite(&sample_rate, 4, 1, fp);
    fwrite(&byte_rate, 4, 1, fp);
    fwrite(&block_align, 2, 1, fp);
    fwrite(&bits_per_sample, 2, 1, fp);
    
    fwrite("data", 1, 4, fp);
    fwrite(&subchunk2_size, 4, 1, fp);
    
    fwrite(src, sizeof(int16_t), len, fp);
    
    fclose(fp);
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
    if(!obj) return;
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
    if(!track || !dest || len == 0) return;

    seg_node* current = track->head;
    size_t i = 0;
    size_t remaining = len;
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

            write_idx += can_read;
            remaining -= can_read;
            pos += can_read;
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

    seg_node* current = track->head;
    size_t i = 0;
    size_t written = 0;
    size_t to_write = len;

    while ((current) && (to_write > 0)) {
        size_t start = i;
        size_t end   = i + current->length;

        if (pos < end) {
            size_t start_in_node = pos - start;
            size_t can_write = current->length - start_in_node;
            if (can_write > to_write) {
                can_write = to_write;
            }
            memcpy(&current->data[start_in_node], &src[written], can_write * sizeof(int16_t));

            written += can_write;
            to_write -= can_write;
            pos += can_write;
        }
        i += current->length;
        current = current->next;
    }

    if (to_write > 0) {
        append_new_node(track, &src[written], to_write);
    }
}

// Delete a range of elements from the track
bool tr_delete_range(struct sound_seg* track, size_t pos, size_t len) {
    if (!track || len == 0) return true;
    if (pos >= track->total_length) {
        return true;
    }

    size_t end = pos + len;
    if (end > track->total_length) {
        end = track->total_length;
    }
    size_t range_len = end - pos;

    seg_node dummy;
    dummy.next = track->head;
    seg_node* previous = &dummy;
    seg_node* current  = track->head;
    size_t i = 0;

    while ((current) && (i < end)) {
        size_t node_start = i;
        size_t node_end   = i + current->length;

        if (node_end <= pos) {
            i = node_end;
            previous = current;
            current  = current->next;
        }
        else if (node_start >= end) {
            break;
        }
        else {
            size_t overlap_start = pos;
            if (node_start > pos) {
                overlap_start = node_start;
            }

            size_t overlap_end = end;
            if (node_end < end) {
                overlap_end = node_end;
            }
            size_t overlap_len   = overlap_end - overlap_start;

            // number of samples before deletion starts
            size_t node_offset_start = overlap_start - node_start;
            // number of samples from the start of the node till the deletion ends
            size_t node_offset_end   = overlap_end   - node_start;

            if (overlap_len == current->length) {
                previous->next = current->next;
                seg_node* temp = current;
                current = current->next;
                i = node_end;
                track->total_length -= overlap_len;

                free(temp->data);
                free(temp);
            } else {
                int16_t* original = current->data;

                size_t left_len  = node_offset_start;
                size_t right_len = current->length - node_offset_end;

                if ((left_len > 0) && (right_len > 0)) {
                    seg_node* right_node = malloc(sizeof(seg_node));
                    right_node->data = malloc(right_len * sizeof(int16_t));
                    memcpy(right_node->data, &original[node_offset_end], right_len * sizeof(int16_t));
                    right_node->length = right_len;
                    right_node->next = current->next;

                    int16_t* left_data = malloc(left_len * sizeof(int16_t));
                    memcpy(left_data, original, left_len * sizeof(int16_t));
                    free(original);

                    current->data = left_data;
                    current->length = left_len;
                    current->next = right_node;

                } else if (left_len == 0) {
                    int16_t* right_data = malloc(right_len * sizeof(int16_t));
                    memcpy(right_data, &original[node_offset_end], right_len * sizeof(int16_t));
                    free(original);

                    current->data   = right_data;
                    current->length = right_len;
                } else {
                    // right_len == 0
                    int16_t* left_data = malloc(left_len * sizeof(int16_t));
                    memcpy(left_data, original, left_len * sizeof(int16_t));
                    free(original);

                    current->data   = left_data;
                    current->length = left_len;
                }

                track->total_length -= overlap_len;
                i = node_start + current->length; 
                previous = current;
                current = current->next;
            }
        }
    }

    track->head = dummy.next;
    return true;
}

// Returns a string containing <start>,<end> ad pairs in target
char* tr_identify(struct sound_seg* target, struct sound_seg* ad){

    size_t target_len = tr_length(target);
    size_t ad_len = tr_length(ad);

    if (ad_len == 0 || target_len < ad_len) {
        char* empty = malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    int16_t* ad_buf = malloc(ad_len * sizeof(int16_t));
    int16_t* window_buf = malloc(ad_len * sizeof(int16_t));
    if (!ad_buf || !window_buf) {
        free(ad_buf);
        free(window_buf);
        return NULL;
    }
    
    tr_read(ad, ad_buf, 0, ad_len);
    
    double ref = 0.0;
    for (size_t i = 0; i < ad_len; i++) {
        ref += ad_buf[i] * ad_buf[i];
    }
    
    size_t res_capacity = 1024;
    char* res = malloc(res_capacity);
    if (!res) {
        free(ad_buf);
        free(window_buf);
        return NULL;
    }
    res[0] = '\0';
    size_t res_length = 0;

    for (size_t pos = 0; pos <= target_len - ad_len; ) {
        tr_read(target, window_buf, pos, ad_len);
        
        double corr = 0.0;
        for (size_t i = 0; i < ad_len; i++) {
            corr += window_buf[i] * ad_buf[i];
        }
        
        if (corr >= 0.95 * ref) {
            char temp[50];
            int written = snprintf(temp, sizeof(temp), "%zu,%zu\n", pos, pos + ad_len - 1);
            if (res_length + written + 1 > res_capacity) {
                res_capacity *= 2;
                char* new_res = realloc(res, res_capacity);
                if (!new_res) {
                    free(res);
                    free(ad_buf);
                    free(window_buf);
                    return NULL;
                }
                res = new_res;
            }
            strcpy(res + res_length, temp);
            res_length += written;
            pos += ad_len;
        } else {
            pos++;
        }
    }
    
    free(ad_buf);
    free(window_buf);
    
    if (res_length == 0) {
        res[0] = '\0';
    }
    
    return res;
}

// Insert a portion of src_track into dest_track at position destpos
void tr_insert(struct sound_seg* src_track,
            struct sound_seg* dest_track,
            size_t destpos, size_t srcpos, size_t len) {
    return;
}
