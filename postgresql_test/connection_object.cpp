#pragma once
#include "postgresql_package.h"
#include"connection_object.h"
#include"connection_pool.h"
#include"metadata_struct.h"
#include"cache.h"
namespace my_project {

    

    Connection::Connection(Connection_Pool::CPImpl* pool):Impl(std::make_unique<CImpl>(pool)) {}

    Connection::CImpl::~CImpl() = default;

    Connection::~Connection() {
        //printf("?? Connection %p 被销毁\n", this);
    }

    Connection::CImpl::CImpl(Connection_Pool::CPImpl* pool):pool(pool),
        is_null_true(true), is_null_false(false) {
        table_metadata = std::make_unique<my_cache::Cache<std::string, TableMetadata>>(my_cache::CacheType::LRU, 100);
    }

    bool Connection::CImpl::connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& port="5432"
        , const std::string& dbname="postgres") {

        conn.reset();

        const char* keywords[] = { "host", "port", "user", "password","dbname",nullptr};
        const char* values[] = { host.c_str(), port.c_str(), user.c_str(), pwd.c_str(), dbname.c_str(),nullptr};

        conn.reset(
            PQconnectdbParams(keywords, values, 0),
            [](PGconn* c) {
                if (c) {
                    PQfinish(c);  // 只会在这里释放一次
                }
            }
        );
        refreshAliveTime();
        if (PQstatus(conn.get()) != CONNECTION_OK) {
           
            std::cerr << "PostgreSQL 连接失败: " << PQerrorMessage(conn.get()) << std::endl;
            conn.reset();  // 清空智能指针即可
            return false;
        }
        std::cout << "PostgreSQL 连接成功" << std::endl;
        return true;
    }

    void Connection::CImpl::refreshAliveTime() {
        aliveTime = std::chrono::steady_clock::now();
    }

    long long Connection::CImpl::getAliveTime() {
        std::chrono::nanoseconds res = std::chrono::steady_clock::now() - aliveTime;
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(res);
        return ms.count();
    }

    

    /*void Connection::refreshAliveTime() {
        Impl->refreshAliveTime();
    }

    long long Connection::getAliveTime() {
        return Impl->getAliveTime();
    }
    bool Connection::connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& port) {
        return Impl->connect(host, user, pwd, port);
    }*/
}