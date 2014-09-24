#ifndef CLANY_FILE_HANDLE_HPP
#define CLANY_FILE_HANDLE_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <stdexcept>
#include <limits>

#ifdef _WIN32
// #  define NOMINMAX
// #  include <windows.h>
// #  include <tchar.h>
#else
#  include <dirent.h>
#endif

#include "clany_macros.h"

_CLANY_BEGIN
typedef istreambuf_iterator<char> ifsbuf_iter;

class FileExcept :public runtime_error
{
public:
    FileExcept(const string& err_msg) :runtime_error(err_msg) {};
};


inline string fileToString(const string& file_name)
{
    ifstream ifs(file_name);
    if (!ifs.is_open()) {
        throw FileExcept("Could not open file " + file_name);
    }

    return string(ifsbuf_iter(ifs), ifsbuf_iter());
}


inline string readFile(const string& file_name)
{
    return fileToString(file_name);
}


inline ifstream& gotoLine(ifstream& file, int num)
{
    file.seekg(ios::beg);
    for (int i = 0; i < num; ++i) {
        file.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    return file;
}


inline string getLineStr(ifstream& file, int num)
{
    gotoLine(file, num);

    string curr_line;
    getline(file, curr_line);

    return curr_line;
}


inline int countLine(const string& file_name)
{
    ifstream ifs (file_name);

    int count = 0;
    string tmp;
    while (getline(ifs, tmp)) {
        ++count;
    }

    return count;
}
_CLANY_END

#endif // CLANY_FILE_HANDLE_HPP
