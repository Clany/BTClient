#ifndef METAINFO_H
#define METAINFO_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "clany/clany_macros.h"

_CLANY_BEGIN
using ByteArray = string;

struct MetaInfo {
    string announce;
    string name;
    llong length;
    int piece_length;
    int num_pieces;
    vector<string> sha1_sums;
};

class ParseError : public runtime_error {
public:
    ParseError(const string& msg) : runtime_error(msg) {}
};

class MetaInfoParser {
public:
    bool parse(const ByteArray& data, MetaInfo& meta_info);
    void clear();

private:
    bool parseString(string& name);
    bool parseInteger(llong& number);
    bool parseList();
    bool parseDictionry(map<string, string>& dict);

    void fillMetaInfo(const map<string, string>& info_dict,
                      MetaInfo& meta_info);

private:
    ByteArray file_data;
    ByteArray info;
    size_t idx;

    map<string, string> meta_dict;
};
_CLANY_END

#endif // METAINFO_H