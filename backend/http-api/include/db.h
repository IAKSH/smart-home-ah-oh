#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <functional>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ahohs::db {

class PostgresDB {
 public:
    // 构造与析构
    explicit PostgresDB(const std::string& connstr);  // 初始化数据库连接
    ~PostgresDB();

    // 禁止拷贝，支持移动
    PostgresDB(const PostgresDB&) = delete;
    PostgresDB& operator=(const PostgresDB&) = delete;
    PostgresDB(PostgresDB&&) noexcept = default;
    PostgresDB& operator=(PostgresDB&&) noexcept = default;

    // 基础 CRUD 操作
    bool create(const std::string& sql);         // 执行插入语句
    std::optional<pqxx::result> read(const std::string& sql);  // 执行查询语句
    bool update(const std::string& sql);           // 执行更新语句
    bool remove(const std::string& sql);           // 执行删除语句

    // 启动事务
    std::unique_ptr<pqxx::work> begin_transaction();

    /**
     * 模板化查询函数
     *
     * 此函数允许将 SQL 查询结果（pqxx::row）转换为任意需要的数据类型，并返回所有转换后的结果。
     * 用户可以传递一个自定义的转换函数（converter），用来将每一行转换为目标类型。
     *
     * @tparam T 转换后的数据类型
     * @param sql SQL 查询语句
     * @param converter 自定义转换函数
     * @return 包含转换后结果的向量
     */
    template <typename T>
    std::vector<T> execute_query(const std::string& sql, std::function<T(const pqxx::row&)> converter) {
        std::vector<T> results;
        try {
            pqxx::work txn(*conn);               // 开启事务
            pqxx::result res = txn.exec(sql);      // 执行 SQL 查询
            txn.commit();                        // 提交事务
            for (const auto& row : res) {
                results.push_back(converter(row));  // 使用转换器将结果转换为目标类型
            }
            // 使用静态 logger 进行记录
            PostgresDB::logger->info("Query executed successfully: {}", sql);
        }
        catch (const std::exception& e) {
            PostgresDB::logger->error("Query execution failed: {}. Error: {}", sql, e.what());
        }
        return results;
    }

    // 注册和调用预处理语句
    void register_prepared_statement(const std::string& stmt_name, const std::string& sql);  // 注册预处理语句
    bool exec_prepared(const std::string& stmt_name, const std::vector<std::string>& params);    // 执行无结果预处理语句
    std::optional<pqxx::result> query_prepared(const std::string& stmt_name, const std::vector<std::string>& params);  // 查询预处理语句

 private:
    std::unique_ptr<pqxx::connection> conn;  // 数据库连接对象

    // 独立的静态日志对象，所有日志均使用该对象输出
    inline static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("postgres_db");
};

}  // namespace ahohs::db
