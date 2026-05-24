#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cctype>

// ============= 工具函数：JSON 字符串转义 =============
std::string escape_json(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"':  result += "\\\""; break;
        case '\\': result += "\\\\";  break;
        case '/':  result += "\\/";   break;
        case '\b': result += "\\b";   break;
        case '\f': result += "\\f";   break;
        case '\n': result += "\\n";   break;
        case '\r': result += "\\r";   break;
        case '\t': result += "\\t";   break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                result += "\\u00" + std::string(1, "0123456789abcdef"[c % 16]);
            else
                result += c;
        }
    }
    return result;
}

// ============= 命令参数解析：支持双引号字符串 =============
std::vector<std::string> parse_args(const std::string& line) {
    std::vector<std::string> args;
    size_t i = 0;
    while (i < line.size()) {
        // 跳过空白
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
        if (i >= line.size()) break;

        std::string token;
        if (line[i] == '"') {           // 带引号的字符串
            ++i; // 跳过左引号
            while (i < line.size() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.size()) { // 处理转义
                    ++i; // 跳过反斜线
                    switch (line[i]) {
                    case 'n': token += '\n'; break;
                    case 't': token += '\t'; break;
                    case '\\': token += '\\'; break;
                    case '"': token += '"'; break;
                    default: token += line[i]; break;
                    }
                }
                else {
                    token += line[i];
                }
                ++i;
            }
            if (i < line.size()) ++i; // 跳过右引号
        }
        else {                       // 普通单词
            while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))) {
                token += line[i++];
            }
        }
        args.push_back(token);
    }
    return args;
}

// ============= 学生成绩管理核心类 =============
class StudentManager {
public:
    struct Student {
        std::string id;
        std::string name;
        std::map<std::string, double> scores; // 科目 -> 成绩
    };

    // ---------- 添加学生 ----------
    std::string add_student(const std::string& id, const std::string& name) {
        if (students.find(id) != students.end())
            return json_error("student already exists");
        Student s;
        s.id = id;
        s.name = name;
        students[id] = s;
        return json_ok("student added");
    }

    // ---------- 添加/更新成绩 ----------
    std::string add_score(const std::string& id, const std::string& subject, double score) {
        auto it = students.find(id);
        if (it == students.end())
            return json_error("student not found");
        if (score < 0 || score > 100)
            return json_error("score must be in 0..100");
        it->second.scores[subject] = score;
        return json_ok("score recorded");
    }

    // ---------- 查询学生信息 ----------
    std::string query_student(const std::string& id) {
        auto it = students.find(id);
        if (it == students.end())
            return json_error("student not found");

        std::ostringstream data;
        data << "{\"id\":\"" << escape_json(it->second.id)
            << "\",\"name\":\"" << escape_json(it->second.name)
            << "\",\"scores\":{";
        bool first = true;
        for (const auto& kv : it->second.scores) {
            if (!first) data << ",";
            data << "\"" << escape_json(kv.first) << "\":" << kv.second;
            first = false;
        }
        data << "}}";
        return json_ok(data.str());
    }

    // ---------- 修改成绩（若科目不存在则自动添加） ----------
    std::string modify_score(const std::string& id, const std::string& subject, double score) {
        return add_score(id, subject, score); // 行为一致
    }

    // ---------- 删除学生 ----------
    std::string delete_student(const std::string& id) {
        auto it = students.find(id);
        if (it == students.end())
            return json_error("student not found");
        students.erase(it);
        return json_ok("student deleted");
    }

    // ---------- 列出所有学生 ----------
    std::string list_students() {
        std::ostringstream arr;
        arr << "[";
        bool first = true;
        for (const auto& kv : students) {
            if (!first) arr << ",";
            arr << "{\"id\":\"" << escape_json(kv.second.id)
                << "\",\"name\":\"" << escape_json(kv.second.name) << "\"}";
            first = false;
        }
        arr << "]";
        return json_ok(arr.str());
    }

    // ---------- 计算某学生平均分 ----------
    std::string average(const std::string& id) {
        auto it = students.find(id);
        if (it == students.end())
            return json_error("student not found");
        if (it->second.scores.empty())
            return json_error("no scores for this student");
        double sum = 0.0;
        for (const auto& kv : it->second.scores)
            sum += kv.second;
        double avg = sum / it->second.scores.size();
        return json_ok(std::to_string(avg));
    }

private:
    std::map<std::string, Student> students;

    // 构造成功响应 JSON
    std::string json_ok(const std::string& data) {
        std::ostringstream out;
        // data 若不是对象，需要加引号处理
        if (data.empty() || data[0] != '{' && data[0] != '[')
            out << "{\"status\":\"ok\",\"message\":\"" << escape_json(data) << "\"}";
        else
            out << "{\"status\":\"ok\",\"data\":" << data << "}";
        return out.str();
    }

    // 构造错误响应 JSON
    std::string json_error(const std::string& msg) {
        std::ostringstream out;
        out << "{\"status\":\"error\",\"message\":\"" << escape_json(msg) << "\"}";
        return out.str();
    }
};

// ============= 主命令处理 =============
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    StudentManager mgr;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;           // 忽略空行

        std::vector<std::string> args = parse_args(line);
        if (args.empty()) continue;

        std::string response;
        try {
            const std::string& cmd = args[0];

            if (cmd == "ADD_STUDENT" && args.size() == 3) {
                response = mgr.add_student(args[1], args[2]);
            }
            else if (cmd == "ADD_SCORE" && args.size() == 4) {
                double score = std::stod(args[3]);
                response = mgr.add_score(args[1], args[2], score);
            }
            else if (cmd == "QUERY" && args.size() == 2) {
                response = mgr.query_student(args[1]);
            }
            else if (cmd == "MODIFY_SCORE" && args.size() == 4) {
                double score = std::stod(args[3]);
                response = mgr.modify_score(args[1], args[2], score);
            }
            else if (cmd == "DELETE_STUDENT" && args.size() == 2) {
                response = mgr.delete_student(args[1]);
            }
            else if (cmd == "LIST_STUDENTS" && args.size() == 1) {
                response = mgr.list_students();
            }
            else if (cmd == "AVERAGE" && args.size() == 2) {
                response = mgr.average(args[1]);
            }
            else {
                response = std::string("{\"status\":\"error\",\"message\":\"unknown command or wrong arguments\"}");
            }
        }
        catch (const std::exception& e) {
            response = std::string("{\"status\":\"error\",\"message\":\"") + escape_json(e.what()) + "\"}";
        }

        std::cout << response << std::endl;   // 一行 JSON，方便 Python 按行读取
    }

    return 0;
}