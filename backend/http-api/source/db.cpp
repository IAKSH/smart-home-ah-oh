#include "db.h"
#include <stdexcept>

namespace ahohs::db {

PostgresDB::PostgresDB(const std::string& connstr) {
    try {
        conn = std::make_unique<pqxx::connection>(connstr);
        if (!conn || !conn->is_open()) {
            throw std::runtime_error("Failed to open PostgreSQL connection");
        }
        PostgresDB::logger->info("PostgreSQL connection established.");
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("PostgreSQL connection error: {}", e.what());
        throw;
    }
}

PostgresDB::~PostgresDB() {
    if (conn && conn->is_open()) {
        conn->close();
        PostgresDB::logger->info("PostgreSQL connection closed.");
    }
}

bool PostgresDB::create(const std::string& sql) {
    try {
        pqxx::work txn(*conn);
        txn.exec(sql);
        txn.commit();
        PostgresDB::logger->info("Create operation executed: {}", sql);
        return true;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Create operation failed: {}. Error: {}", sql, e.what());
        return false;
    }
}

std::optional<pqxx::result> PostgresDB::read(const std::string& sql) {
    try {
        pqxx::work txn(*conn);
        pqxx::result res = txn.exec(sql);
        txn.commit();
        PostgresDB::logger->info("Read operation executed: {}", sql);
        return res;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Read operation failed: {}. Error: {}", sql, e.what());
        return std::nullopt;
    }
}

bool PostgresDB::update(const std::string& sql) {
    try {
        pqxx::work txn(*conn);
        txn.exec(sql);
        txn.commit();
        PostgresDB::logger->info("Update operation executed: {}", sql);
        return true;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Update operation failed: {}. Error: {}", sql, e.what());
        return false;
    }
}

bool PostgresDB::remove(const std::string& sql) {
    try {
        pqxx::work txn(*conn);
        txn.exec(sql);
        txn.commit();
        PostgresDB::logger->info("Delete operation executed: {}", sql);
        return true;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Delete operation failed: {}. Error: {}", sql, e.what());
        return false;
    }
}

std::unique_ptr<pqxx::work> PostgresDB::begin_transaction() {
    try {
        return std::make_unique<pqxx::work>(*conn);
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Failed to begin transaction. Error: {}", e.what());
        return nullptr;
    }
}

// 注册和调用预处理语句的 API
void PostgresDB::register_prepared_statement(const std::string& stmt_name, const std::string& sql) {
    try {
        conn->prepare(stmt_name, sql);
        PostgresDB::logger->info("Prepared statement '{}' registered.", stmt_name);
    }
    catch (const std::exception& e) {
        std::string err_msg = e.what();
        if (err_msg.find("already exists") != std::string::npos) {
            PostgresDB::logger->warn("Prepared statement '{}' already exists. Overwriting it.", stmt_name);
            conn->unprepare(stmt_name);
            conn->prepare(stmt_name, sql);
            PostgresDB::logger->warn("Prepared statement '{}' has been overwritten.", stmt_name);
        } else {
            PostgresDB::logger->error("Failed to register prepared statement '{}': {}", stmt_name, e.what());
            throw;
        }
    }
}

bool PostgresDB::exec_prepared(const std::string& stmt_name, const std::vector<std::string>& params) {
    try {
        pqxx::work txn(*conn);
        pqxx::params pq_params;
        pq_params.reserve(params.size());
        for (const auto& param : params) {
            pq_params.append(param);
        }
        txn.exec(pqxx::prepped(stmt_name), pq_params);
        txn.commit();
        PostgresDB::logger->info("Prepared statement '{}' executed successfully.", stmt_name);
        return true;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Execution of prepared statement '{}' failed: {}", stmt_name, e.what());
        return false;
    }
}

std::optional<pqxx::result> PostgresDB::query_prepared(const std::string& stmt_name,
                                                       const std::vector<std::string>& params) {
    try {
        pqxx::work txn(*conn);
        pqxx::params pq_params;
        pq_params.reserve(params.size());
        for (const auto& param : params) {
            pq_params.append(param);
        }
        pqxx::result res = txn.exec(pqxx::prepped(stmt_name), pq_params);
        txn.commit();
        PostgresDB::logger->info("Prepared statement '{}' queried successfully.", stmt_name);
        return res;
    }
    catch (const std::exception& e) {
        PostgresDB::logger->error("Query using prepared statement '{}' failed: {}", stmt_name, e.what());
        return std::nullopt;
    }
}

}  // namespace ahohs::db
