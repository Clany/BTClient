#ifndef METAINFO_H
#define METAINFO_H

#include <string>
#include <vector>
#include <map>
#include "clany/clany_defs.h"

_CLANY_BEGIN
const int SHA1_LENGTH = 20;

struct MetaInfo {
    string announce  = "";
    string name      = "";
    llong length     = 0;
    int piece_length = 0;
    int num_pieces   = 0;
    string info_hash = string(20, '0');
    vector<string> sha1_sums = {};
};

class MetaInfoParser {
    using Dict = map<string, string>;

public:
    bool parse(const string& data, MetaInfo& meta_info);
    void clear();

    const Dict& getDictionary() { return meta_dict; }

private:
    bool parseString(string& name);
    bool parseInteger(llong& number);
    bool parseList();
    bool parseDictionry(Dict& dict);

    void fillMetaInfo(const Dict& info_dict, MetaInfo& meta_info);

private:
    string file_data;
    string info_data;
    size_t idx;

    Dict meta_dict;
};
_CLANY_END

#endif // METAINFO_H