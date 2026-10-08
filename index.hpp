#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include "util.hpp"
#include "log.hpp"

namespace ns_index{

    struct DocInfo{
        std::string title;   //文档的标题
        std::string content; //文档对应的去标签之后的内容
        std::string url;     //官网文档url
        uint64_t doc_id;          //文档的ID，暂时先不做过多理解
    };

    struct InvertedElem{
        uint64_t doc_id;
        std::string word;
        int weight;
        InvertedElem():weight(0){}
    };

    //倒排拉链
    typedef std::vector<InvertedElem> InvertedList;

    class Index{
        private:
            //正排索引的数据结构用数组，数组的下标天然是文档的ID
            std::vector<DocInfo> forward_index; //正排索引
            //倒排索引一定是一个关键字和一组(个)InvertedElem对应[关键字和倒排拉链的映射关系]
            std::unordered_map<std::string, InvertedList> inverted_index;
        private:
            Index(){} //但是一定要有函数体，不能delete
            Index(const Index&) = delete;
            Index& operator=(const Index&) = delete;

            static Index* instance;
            static std::mutex mtx;
        public:
            ~Index(){}
        public:
            static Index* GetInstance()
            {
                if(nullptr == instance){
                    mtx.lock();
                    if(nullptr == instance){
                        instance = new Index();
                    }
                    mtx.unlock();
                }
                return instance;
            }
            //根据doc_id找到找到文档内容
            DocInfo *GetForwardIndex(uint64_t doc_id)
            {
                if(doc_id >= forward_index.size()){
                    std::cerr << "doc_id out range, error!" << std::endl;
                    return nullptr;
                }
                return &forward_index[doc_id];
            }

            //根据关键字string，获得倒排拉链
            InvertedList *GetInvertedList(const std::string &word)
            {
                auto iter = inverted_index.find(word);
                if(iter == inverted_index.end()){
                    std::cerr << word << " have no InvertedList" << std::endl;
                    return nullptr;
                }
                return &(iter->second);
            }
            //根据去标签，格式化之后的文档，构建正排和倒排索引
            //data/raw_html/raw.txt
            bool BuildIndex(const std::string &input) //parse处理完毕的数据交给我
            {
                std::ifstream in(input, std::ios::in | std::ios::binary);
                if(!in.is_open()){
                    std::cerr << "sorry, " << input << " open error" << std::endl;
                    return false;
                }

                std::string line;
                int count = 0;
                while(std::getline(in, line)){
                    DocInfo * doc = BuildForwardIndex(line);
                    if(nullptr == doc){
                        std::cerr << "build " << line << " error" << std::endl; //for deubg
                        continue;
                    }

                    BuildInvertedIndex(*doc);
                    count++;
                    //if(count % 50 == 0){
                        //std::cout <<"当前已经建立的索引文档: " << count <<std::endl;
                        LOG(NORMAL, "当前的已经建立的索引文档: " + std::to_string(count));
                    //}
                }
                return true;
            }
        private:
            DocInfo *BuildForwardIndex(const std::string &line)
            {
                //1. 解析line，字符串切分
                //line -> 3 string, title, content, url
                std::vector<std::string> results;
                const std::string sep = "\3";   //行内分隔符
                ns_util::StringUtil::Split(line, &results, sep);
                //ns_util::StringUtil::CutString(line, &results, sep);
                if(results.size() != 3){
                    return nullptr;
                }
                //2. 字符串进行填充到DocIinfo
                DocInfo doc;
                doc.title = results[0]; //title
                doc.content = results[1]; //content
                doc.url = results[2];   ///url
                doc.doc_id = forward_index.size(); //先进行保存id，在插入，对应的id就是当前doc在vector中的下标!
                //3. 插入到正排索引的vector
                forward_index.push_back(std::move(doc)); //doc,html文件内容
                return &forward_index.back();
            }

            bool BuildInvertedIndex(const DocInfo &doc)
            {
                //DocInfo{title, content, url, doc_id}
                //word -> 倒排拉链
                struct word_cnt{
                    int title_cnt;
                    int content_cnt;

                    word_cnt():title_cnt(0), content_cnt(0){}
                };
                std::unordered_map<std::string, word_cnt> word_map; //用来暂存词频的映射表

                //对标题进行分词
                std::vector<std::string> title_words;
                ns_util::JiebaUtil::CutString(doc.title, &title_words);

                //if(doc.doc_id == 1572){
                //    for(auto &s : title_words){
                //        std::cout << "title: " << s << std::endl;
                //    }
                //}

                //对标题进行词频统计
                for(std::string s : title_words){
                    boost::to_lower(s); //需要统一转化成为小写
                    word_map[s].title_cnt++; //如果存在就获取，如果不存在就新建
                }

                //对文档内容进行分词
                std::vector<std::string> content_words;
                ns_util::JiebaUtil::CutString(doc.content, &content_words);
                //if(doc.doc_id == 1572){
                //    for(auto &s : content_words){
                //        std::cout << "content: " << s << std::endl;
                //    }
                //}

                //对内容进行词频统计
                for(std::string s : content_words){
                    boost::to_lower(s);
                    word_map[s].content_cnt++;
                }

#define X 10
#define Y 1
                //Hello,hello,HELLO
                for(auto &word_pair : word_map){
                    InvertedElem item;
                    item.doc_id = doc.doc_id;
                    item.word = word_pair.first;
                    item.weight = X*word_pair.second.title_cnt + Y*word_pair.second.content_cnt; //相关性
                    InvertedList &inverted_list = inverted_index[word_pair.first];
                    inverted_list.push_back(std::move(item));
                }

                return true;
            }
    };
    Index* Index::instance = nullptr;
    std::mutex Index::mtx;
}







