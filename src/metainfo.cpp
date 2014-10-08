#include <openssl/sha.h>
#include "metainfo.h"

using namespace std;
using namespace clany;

bool MetaInfoParser::parse(const ByteArray& data, MetaInfo& meta_info)
{
    clear();
    file_data = data;
    idx = 0;

    if (parseDictionry(meta_dict)) {
        fillMetaInfo(meta_dict, meta_info);
        return true;
    }
    return false;
}

void MetaInfoParser::clear()
{
    file_data.clear();
    info_data.clear();
    meta_dict.clear();
}

bool MetaInfoParser::parseString(string& str)
{
    char c = file_data[idx];
    if (c < '0' || c > '9') return false;

    string size_str;
    while ((c = file_data[idx++]) != ':') {
        size_str.push_back(c);
    };

    int size = stoi(size_str);
    str = file_data.sub(idx, size);
    idx += size;

    return true;
}

bool MetaInfoParser::parseInteger(llong& number)
{
    if (file_data[idx] != 'i') return false;
    ++idx;

    bool is_negative = false;
    if (file_data[idx] == '-') {
        is_negative = true;
        ++idx;
    }

    string num_str;
    char c;
    while ((c = file_data[idx++]) != 'e') {
        if (c < '0' || c > '9') return false;
        num_str.push_back(c);
    };
    number = is_negative ? -stoll(num_str) : stoll(num_str);

    return true;
}

bool MetaInfoParser::parseList()
{
    // TODO
    return true;
}

bool MetaInfoParser::parseDictionry(Dict& dict)
{
    if (file_data[idx] != 'd') return false;

    ++idx;
    do {
        if (file_data[idx] == 'e') {
            ++idx;
            break;
        }

        string key;
        if(!parseString(key)) return false;

        if (key == "info") {
            size_t info_begin = idx;
            Dict info_dict;
            parseDictionry(info_dict);
            size_t info_len = idx - info_begin;
            info_data = file_data.sub(info_begin, info_len);

            if (file_data[idx] != 'e') return false;
            dict.insert(info_dict.begin(), info_dict.end());

            return true;
        }

        string name;
        llong  number;
        if (parseString(name)) {
            dict.insert({key, name});
        } else if (parseInteger(number)) {
            dict.insert({key, to_string(number)});
        } else {
            return false;
        }
    } while (idx < file_data.size());

    return true;
}

void MetaInfoParser::fillMetaInfo(const Dict& info_dict,
                                  MetaInfo& meta_info)
{
    meta_info.announce     = info_dict.at("announce");
    meta_info.length       = stoll(info_dict.at("length"));
    meta_info.name         = info_dict.at("name");
    meta_info.num_pieces   = info_dict.at("pieces").size() / 20;
    meta_info.piece_length = stoi(info_dict.at("piece length"));
    SHA1((uchar*)info_data.data(), info_data.size(), (uchar*)meta_info.info_hash.data());

    ByteArray sha1(info_dict.at("pieces"));
    for (auto i = 0u; i < sha1.size(); i += SHA1_LENGTH) {
        meta_info.sha1_vec.push_back(sha1.sub(i, SHA1_LENGTH));
    }
}