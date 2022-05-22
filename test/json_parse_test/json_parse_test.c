#include "../../json_parse.h"

int main(int argc, char** argv){
    const char* config_file_path;
    struct nvdimm_config config;

    assert(argc == 2);

    config_file_path = argv[1];
    parse_json(config_file_path, &config);

    return 0;
}