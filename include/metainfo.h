#ifndef METAINFO_H
#define METAINFO_H

#include <map>
#include "clany/byte_array.hpp"

_CLANY_BEGIN
const int SHA1_LENGTH = 20;

struct MetaInfo {
    string announce     = "";
    string name         = "";
    llong length        = 0;
    int piece_length    = 0;
    int num_pieces      = 0;
    ByteArray info_hash = ByteArray(20, '0');
    vector<ByteArray> sha1_vec = {};
};

class MetaInfoParser {
    using Dict = map<string, string>;

public:
    bool parse(const ByteArray& data, MetaInfo& meta_info);
    void clear();

    const Dict& getDictionary() { return meta_dict; }

private:
    bool parseString(string& str);
    bool parseInteger(llong& number);
    bool parseList();
    bool parseDictionry(Dict& dict);

    void fillMetaInfo(const Dict& info_dict, MetaInfo& meta_info);

private:
    ByteArray file_data;
    ByteArray info_data;
    size_t idx;

    Dict meta_dict;
};
_CLANY_END

#endif // METAINFO_H