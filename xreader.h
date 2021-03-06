﻿/*
* Copyright (C) 2017 YY Inc. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/


#ifndef __X_READER_H
#define __X_READER_H


#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <iostream>

#include "util.h"

namespace x2struct {

/*
  DOC need implement
  bool has(const std::string)
  const std::string& type()
*/
template<typename DOC>
class XReader {
protected:
    typedef DOC doc_type;
    typedef XReader<DOC> xdoc_type;
public:
    // only c++0x support reference initialize, so use pointer
    XReader(const doc_type *parent, const char* key):_parent(parent), _key(key), _index(-1), _set_has(false){}
    XReader(const doc_type *parent, size_t index):_parent(parent), _key(0), _index(int(index)), _set_has(false){}
    ~XReader(){}
public:
    template <typename TYPE>
    bool convert(const char*key, std::vector<TYPE> &val) {
        doc_type tmp;
        doc_type *obj = get_obj(key, &tmp);
        if (NULL == obj) {
            return false;
        }

        size_t s = obj->size();                // [implement] size_t size(bool to_vec=true)
        val.resize(s);
        for (size_t i=0; i<s; ++i) {
            (*obj)[i].convert(NULL, val[i]);   // [implement] doc_type operator[](size_t)
        }
        return true;
    }

    template <typename TYPE>
    bool convert(const char*key, std::set<TYPE> &val) {
        doc_type tmp;
        doc_type *obj = get_obj(key, &tmp);
        if (NULL == obj) {
            return false;
        }

        size_t s = obj->size();
        for (size_t i=0; i<s; ++i) {
            TYPE _t;
            (*obj)[i].convert(NULL, _t);
            val.insert(_t);
        }
        return true;
    }

    template <typename TYPE>
    bool convert(const char*key, std::map<std::string,TYPE> &val) {
        doc_type tmp;
        doc_type *obj = get_obj(key, &tmp);
        if (NULL == obj) {
            return false;
        }

        for (doc_type d=obj->begin(); d; d=d.next()) { // [implement] doc_type begin(); doc_type next(); operator bool() const;
            TYPE _t;
            d.convert(NULL, _t);
            val[d.key()] = _t;
        }
        return true;
    }

    template <typename KEYTYPE, typename TYPE>
    bool convert(const char*key, std::map<KEYTYPE, TYPE> &val) {
        doc_type tmp;
        doc_type *obj = get_obj(key, &tmp);
        if (NULL == obj) {
            return false;
        }

        for (doc_type d=obj->begin(); d; d=d.next()) {
            TYPE _t;
            d.convert(NULL, _t);
            KEYTYPE _k;
            std::string key = d.key();
            if (key[0]!='x') {
                _k = Util::tonum<KEYTYPE>(key);
            } else { // libconfig/xml不支持数字作为key，所以用x开头，比如x11
                _k = Util::tonum<KEYTYPE>(key.substr(1));
            }
            val[_k] = _t;
        }
        return true;
    }

    template <typename TYPE>
    bool convert(const char*key, TYPE& val) {
        doc_type tmp;
        doc_type *obj = get_obj(key, &tmp);
        if (NULL == obj) {
            return false;
        }

        size_t len = obj->size(false);
        if (0==len) {
            val.__x_to_struct(*obj);
        } else {
            for (size_t i=0; i<len; ++i) {
                doc_type sub = (*obj)[i];
                if (val.__x_condition(sub, this->key_char())) {
                    val.__x_to_struct(sub);
                    break;
                }
            }
        }
        return true;
    }

    std::string attribute(const char* key) {
        std::string val;
        (static_cast<doc_type*>(this))->convert(key, val);
        return val;
    }

    const std::string key() const {
        if (0 != _key) {
            return _key;
        } else {
            return Util::tostr(_index);
        }
    }

    const char* key_char() const {
        if (0 != _key) {
            return _key;
        } else {
            return "";
        }
    }

    std::string hasa(const std::string&key, const std::string&alias, bool *me) {
        return Util::alias_parse(key, alias, static_cast<doc_type*>(this)->type(), me);
    }
    std::string path() {
        std::vector<std::string> nodes;
        const doc_type* tmp = static_cast<doc_type*>(this);
        while (tmp) {
            std::string k;
            k.reserve(32);
            if (0 != tmp->_key) {
                if (0!=tmp->_parent && 0!=tmp->_parent->_parent) {
                    k.append(".");
                }
                k.append(tmp->_key);
            } else {
                k.append("[").append(Util::tostr(tmp->_index)).append("]");
            }
            nodes.push_back(k);
            tmp = tmp->_parent;
        }
        std::string p;
        p.reserve(64);
        for (int i=(int)nodes.size()-1; i>=0; --i) {
            p.append(nodes[i]);
        }
        return p;
    }
    void me_exception(const std::string&key) {
        std::string err;
        err.reserve(128);
        err.append("miss ");
        std::string p = path();
        if (!p.empty()) {
            err.append(p).append(".");
        }
        err.append(key);
        throw std::runtime_error(err);
    }

    bool set_has() const {
        return _set_has;
    }
    void set_has(bool set) {
        _set_has = set;
    }
protected:
    doc_type* get_obj(const char *key, doc_type *tmp) {
        doc_type *obj = static_cast<doc_type*>(this);
        if (NULL != key) {
            obj = obj->child(key, tmp);   // [implement] doc_type* child(const char*, doc_type*)
        }
        return obj;
    }
    const doc_type* _parent;
    const char* _key;
    int _index;
    bool _set_has;
};

}

#endif
