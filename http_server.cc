#include "cpp-httplib/httplib.h"
#include "searcher.hpp"

const std::string input = "data/raw_html/raw.txt";
const std::string root_path = "./wwwroot";

int main()
{
    ns_searcher::Searcher search;
    search.InitSearcher(input);

    httplib::Server svr;
    svr.set_base_dir(root_path.c_str());
    svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &rsp){
            if(!req.has_param("word")){
                rsp.set_content("必须要有搜索关键字!", "text/plain; charset=utf-8");
                return;
            }
            std::string word = req.get_param_value("word");
            //std::cout << "用户在搜索：" << word << std::endl;
            LOG(NORMAL, "用户搜索的: " + word);
            std::string json_string;
            search.Search(word, &json_string);
            rsp.set_content(json_string, "application/json");
            //rsp.set_content("你好,世界!", "text/plain; charset=utf-8");
            });

    LOG(NORMAL, "服务器启动成功...");
    svr.listen("0.0.0.0", 8081);
    return 0;
}
