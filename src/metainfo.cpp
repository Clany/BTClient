#include <openssl/sha.h>
#include "metainfo.h"

using namespace std;
using namespace clany;

bool MetaInfoParser::parse(const string& data, MetaInfo& meta_info)
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

bool MetaInfoParser::parseString(string& name)
{
    char c = file_data[idx];
    if (c < '0' || c > '9') return false;

    string name_size_str;
    while ((c = file_data[idx++]) != ':') {
        name_size_str.push_back(c);
    };

    int name_size = stoi(name_size_str);
    name = static_cast<string>(file_data).substr(idx, name_size);
    idx += name_size;

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

    number = stoll(num_str);
    if (is_negative) number = -number;

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
            info_data = file_data.substr(info_begin, info_len);

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
    meta_info.announce = info_dict.at("announce");
    meta_info.length = stoll(info_dict.at("length"));
    meta_info.name = info_dict.at("name");
    meta_info.num_pieces = info_dict.at("pieces").size() / 20;
    meta_info.piece_length = stoi(info_dict.at("piece length"));
    SHA1((uchar*)info_data.c_str(), info_data.length(), (uchar*)meta_info.info_hash.c_str());

    string sha1_str = info_dict.at("pieces");
    for (auto i = 0u; i < sha1_str.size(); i += SHA1_LENGTH) {
        meta_info.sha1_sums.push_back(sha1_str.substr(i, SHA1_LENGTH));
    }
}