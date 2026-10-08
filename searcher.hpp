#pragma once

#include "index.hpp"
#include "util.hpp"
#include "log.hpp"
#include <algorithm>
#include <unordered_map>
#include <jsoncpp/json/json.h>

namespace ns_searcher{

    struct InvertedElemPrint{
        uint64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint():doc_id(0), weight(0){}
    };

    class Searcher{
        private:
            ns_index::Index *index; //供系统进行查找的索引
        public:
            Searcher(){}
            ~Searcher(){}
        public:
            void InitSearcher(const std::string &input)
            {
                //1. 获取或者创建index对象
                index = ns_index::Index::GetInstance();
                //std::cout << "获取index单例成功..." << std::endl;
                LOG(NORMAL, "获取index单例成功...");
                //2. 根据index对象建立索引
                index->BuildIndex(input);
                //std::cout << "建立正排和倒排索引成功..." << std::endl;
                LOG(NORMAL, "建立正排和倒排索引成功...");
            }
            //query: 搜索关键字
            //json_string: 返回给用户浏览器的搜索结果
            void Search(const std::string &query, std::string *json_string)
            {
                //1.[分词]:对我们的query进行按照searcher的要求进行分词
                std::vector<std::string> words;
                ns_util::JiebaUtil::CutString(query, &words);
                //2.[触发]:就是根据分词的各个"词"，进行index查找,建立index是忽略大小写，所以搜索，关键字也需要
                //ns_index::InvertedList inverted_list_all; //内部InvertedElem
                std::vector<InvertedElemPrint> inverted_list_all;

                std::unordered_map<uint64_t, InvertedElemPrint> tokens_map;

                for(std::string word : words){
                    boost::to_lower(word);

                    ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                    if(nullptr == inverted_list){
                        continue;
                    }
                    //不完美的地方：暂时可以交给大家 , 你/是/一个/好人 100
                    //inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());
                    for(const auto &elem : *inverted_list){
                        auto &item = tokens_map[elem.doc_id]; //[]:如果存在直接获取，如果不存在新建
                        //item一定是doc_id相同的print节点
                        item.doc_id = elem.doc_id;
                        item.weight += elem.weight;
                        item.words.push_back(elem.word);
                    }
                }
                for(const auto &item : tokens_map){
                    inverted_list_all.push_back(std::move(item.second));
                }

                //3.[合并排序]：汇总查找结果，按照相关性(weight)降序排序
                //std::sort(inverted_list_all.begin(), inverted_list_all.end(),\
                //      [](const ns_index::InvertedElem &e1, const ns_index::InvertedElem &e2){
                //        return e1.weight > e2.weight;
                //        });
                  std::sort(inverted_list_all.begin(), inverted_list_all.end(),\
                          [](const InvertedElemPrint &e1, const InvertedElemPrint &e2){
                          return e1.weight > e2.weight;
                          });
                //4.[构建]:根据查找出来的结果，构建json串 -- jsoncpp --通过jsoncpp完成序列化&&反序列化
                Json::Value root;
                for(auto &item : inverted_list_all){
                    ns_index::DocInfo * doc = index->GetForwardIndex(item.doc_id);
                    if(nullptr == doc){
                        continue;
                    }
                    Json::Value elem;
                    elem["title"] = doc->title;
                    elem["desc"] = GetDesc(doc->content, item.words[0]); //content是文档的去标签的结果，但是不是我们想要的，我们要的是一部分 TODO
                    elem["url"]  = doc->url;
                    //for deubg, for delete
                    elem["id"] = (int)item.doc_id;
                    elem["weight"] = item.weight; //int->string

                    root.append(elem);
                }

                //Json::StyledWriter writer;
                Json::FastWriter writer;
                *json_string = writer.write(root);
            }

            std::string GetDesc(const std::string &html_content, const std::string &word)
            {
                //找到word在html_content中的首次出现，然后往前找50字节(如果没有，从begin开始)，往后找100字节(如果没有，到end就可以的)
                //截取出这部分内容
                const int prev_step = 50;
                const int next_step = 100;
                //1. 找到首次出现
                auto iter = std::search(html_content.begin(), html_content.end(), word.begin(), word.end(), [](int x, int y){
                        return (std::tolower(x) == std::tolower(y));
                        });
                if(iter == html_content.end()){
                    return "None1";
                }
                int pos = std::distance(html_content.begin(), iter);

                //2. 获取start，end , std::size_t 无符号整数
                int start = 0; 
                int end = html_content.size() - 1;
                //如果之前有50+字符，就更新开始位置
                if(pos > start + prev_step) start = pos - prev_step;
                if(pos < end - next_step) end = pos + next_step;

                //3. 截取子串,return
                if(start >= end) return "None2";
                std::string desc = html_content.substr(start, end - start);
                desc += "...";
                return desc;
            }
    };
}




















