#include <include/path.h>

const char* __path_sep__ =
#ifdef _WIN32
"\\"
#else
"/"
#endif
;


string_t* path_get_dir(string_t* filename) {
    size_t ind = string_last_index_of(filename, __path_sep__);
    return string_substr(filename, 0, ind);
}

string_t* path_make_path(string_t* dir, string_t* filename) {
    if (dir->buffer[dir->size] == __path_sep__[0]) {
        return string_format("%S%S", dir, filename);
    }
    return string_format("%S%s%S", dir, __path_sep__, filename);
}
