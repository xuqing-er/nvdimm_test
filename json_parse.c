#include "json_parse.h"

// must correspond to type
const char* test_op_type_strings[] = {
    "invalid",
    "write_rnd",
    "write_seq",
    "read_rnd",
    "read_seq"
};

static enum test_op_type
string_to_optype(char* str){
    if(strcmp("read_rnd", str) == 0){
        return READ_RANDOM;
    }else if(strcmp("read_seq", str) == 0){
        return READ_SEQUENTIAL;
    }else if(strcmp("write_rnd", str) == 0){
        return WRITE_RANDOM;
    }else if(strcmp("write_seq", str) == 0){
        return WRITE_SEQUENTIAL;
    }else{
        return INVALID_TYPE;
    }
}

static inline const char*
optype_to_string(enum test_op_type type){
    return test_op_type_strings[type];
}

static void
nvdimm_config_init(struct nvdimm_config *config){
    config->pmem_space_size = 64ul<<30;
    config->element_size = 8;
    config->stride = 256;
    config->offset = 0;
    config->repetition = 1;
    config->preload_pagetable = 0;

    return ;
}

static void
nvdimm_config_print(struct nvdimm_config *config){ 
    printf("test config\n");
    printf("\tfile_path: %s,\n \
        \r\toptype: %s\n \
        \r\tpmem_space_size: %lu,\n \
        \r\telement_size: %lu,\n \
        \r\tloop_num: %lu,\n \
        \r\tstride: %d,\n \
        \r\toffset: %d,\n \
        \r\trepetion: %d\n \
        \r\tpreload_pagetable: %d\n", config->file_path,optype_to_string(config->type), config->pmem_space_size, config->element_size, \
        config->loop_num, config->stride, config->offset, config->repetition, config->preload_pagetable);
    return ;
}

static size_t
parse_size_item(const char* str){
    size_t tmp_val;
    size_t ret;
    size_t i;

    tmp_val = 0; i =0;
    if(str[0]<'0' || str[0]>'9'){
        perror("invalid size");
        return -1;
    }
    while(str[i]>='0' && str[i]<='9'){
        tmp_val = tmp_val*10 + str[i]-'0';
        i ++;
    }
    if(str[i] == 'G'){
        ret = GB(tmp_val);
    }else if(str[i] == 'M'){
        ret = MB(tmp_val);
    }else if(str[i] == 'K'){
        ret = KB(tmp_val);
    }else if(str[i] == 'B'){
        ret = tmp_val;
    }else{
        ret = tmp_val;
    }

    return ret;
}

int parse_json(const char *path, struct nvdimm_config *config){
    int fd;
    int ret;
    struct stat statbuf;
    size_t file_size;
    char* config_buf;
    cJSON* root;
    cJSON* item;
    size_t tmp_val;
    int i;

    nvdimm_config_init(config);

    if (stat(path, &statbuf) < 0) {
        perror("stat file");
        exit(1);
    }

    file_size = statbuf.st_size;
    assert(file_size > 0);
    config_buf = (char*) malloc(file_size);

    if((fd = open(path, O_RDONLY)) < 0){
        perror("open file");
        exit(1);
    }
    if(read(fd, config_buf, file_size) < 0){
        perror("read file");
        exit(1);
    }

    root = cJSON_Parse(config_buf);
    if(root == NULL){
        perror("parse json config");
        exit(1);
    }

    item = cJSON_GetObjectItem(root,"file_path");
    if(item != NULL){
        memcpy(config->file_path, item->valuestring, strlen(item->valuestring));
    }else{
        exit(1);
    }

    item = cJSON_GetObjectItem(root,"type");
    if(item == NULL){
        perror("have to define optype: read_rnd, read_seq, write_rnd, write_seq");
        exit(1);
    }
    if((config->type = string_to_optype(item->valuestring)) == INVALID_TYPE){
        perror("invalid optype");
        exit(1);
    }

    item = cJSON_GetObjectItem(root,"pmem_space_size");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->pmem_space_size = tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"element_size");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->element_size = tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"loop_num");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->loop_num= tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"stride");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->stride = tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"offset");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->offset = tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"repetition");
    if(item != NULL){
        tmp_val = parse_size_item(item->valuestring);
        if(tmp_val >= 0){
            config->repetition = tmp_val;
        }else{
            exit(1);
        }
    }

    item = cJSON_GetObjectItem(root,"preload_pagetable");
    if(item != NULL){
        config->preload_pagetable = item->valueint;
    }

    cJSON_Delete(root);
    free(config_buf);
    close(fd);

    nvdimm_config_print(config);
    return ret;
}