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

class MetaInfoError : public runtime_error {
public:
    MetaInfoError(const string& msg) : runtime_error(msg) {}
};

class MetaInfoParser {
    using Dict = map<string, string>;

public:
    bool parse(const ByteArray& data, MetaInfo& meta_info);
    void clear();

    const Dict& getDictionary() { return meta_dict; }

private:
    bool parseString(string& name);
    bool parseInteger(llong& number);
    bool parseList();
    bool parseDictionry(Dict& dict);

    void fillMetaInfo(const Dict& info_dict,
                      MetaInfo& meta_info);

private:
    ByteArray file_data;
    ByteArray info;
    size_t idx;

    Dict meta_dict;
};
_CLANY_END

#endif // METAINFO_H