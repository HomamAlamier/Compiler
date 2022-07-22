#ifndef PATH_H
#define PATH_H

#include <include/strings.h>

string_t* path_get_dir(string_t* filename);
string_t* path_make_path(string_t* dir, string_t* filename);
#endif // PATH_H
