# Boost 搜索引擎

基于 C++ 实现的 Boost C++ 库官方文档**垂直搜索引擎**。
涵盖从 HTML 文档采集、文本解析、正排/倒排索引构建、中文分词、相关性排序到 Web 检索服务的完整链路

## 功能特性

-  **文档解析**：递归扫描 HTML，提取标题 / 正文 / URL，简易状态机去标签
-  **双索引结构**：正排索引（`vector`，doc_id→文档）+ 倒排索引（`unordered_map`，词→倒排拉链）
-  **中文分词**：集成 cppjieba 结巴分词（搜索引擎模式），支持停用词过滤
-  **相关性排序**：标题权重 ×10 + 正文权重 ×1，大小写不敏感
-  **摘要生成**：关键词前后 50/100 字节截取，支持不区分大小写定位
-  **Web 服务**：基于 cpp-httplib 提供 HTTP 接口，jQuery 前端页面

## 系统架构

<img width="870" height="963" alt="image" src="https://github.com/user-attachments/assets/21c1c6f3-e0d1-4f23-b7ef-820a0d6029ab" />

## 技术栈

<img width="1524" height="462" alt="image" src="https://github.com/user-attachments/assets/b340f496-847e-4b10-84f0-21e5cc98b935" />

## 目录结构

<img width="1323" height="822" alt="image" src="https://github.com/user-attachments/assets/bad41d5d-9885-4f5f-afa9-eb40151b150a" />

## 依赖安装

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y libboost-all-dev libjsoncpp-dev
```

### cppjieba & cpp-httplib

本项目通过头文件引入，需将源码放到项目根目录：

```bash
# 结巴分词
git clone https://github.com/yanyiwu/cppjieba.git
# cpp-httplib
git clone https://github.com/yhirose/cpp-httplib.git
```

并确保结巴词典目录 `dict/` 位于运行目录下。

## 编译

```bash
make all
```

将生成三个可执行文件：`parser`、`debug`、`http_server`。

## 运行

### 1. 准备数据

将 Boost 官方文档 HTML 文件放入 `data/input/` 目录。

### 2. 数据预处理

```bash
./parser
```

递归解析所有 HTML，输出到 `data/raw_html/raw.txt`。

### 3. 命令行搜索（调试用）

```bash
./debug
```

### 4. 启动 Web 服务

```bash
./http_server
```

浏览器访问 `http://localhost:8081/` 即可使用。

## 核心原理

### 正排索引

以 `doc_id`（数组下标）为键，存储文档的完整信息：

```cpp
struct DocInfo {
    std::string title;    // 文档标题
    std::string content;  // 去标签后的正文
    std::string url;      // 官方文档 URL
    uint64_t doc_id;      // 文档 ID = vector 下标
};
```

### 倒排索引

以关键词为键，存储包含该词的文档列表（倒排拉链）：

```cpp
struct InvertedElem {
    uint64_t doc_id;
    std::string word;
    int weight;  // 相关性权重
};
typedef std::vector<InvertedElem> InvertedList;
```

### 相关性权重

```
weight = 10 × title_cnt + 1 × content_cnt
```

标题中出现的词权重为正文的 10 倍，因为标题更能代表文档主题。

### 检索流程

1. **分词**：对用户 query 调用结巴分词，统一转小写
2. **触发**：逐个词查倒排索引，获取相关文档
3. **合并**：相同 doc_id 的权重累加，收集命中词列表
4. **排序**：按 weight 降序排列
5. **回表**：通过 doc_id 查正排索引，取 title / content / url
6. **摘要**：截取关键词前后内容作为搜索摘要
7. **输出**：序列化为 JSON 返回前端
