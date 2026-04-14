#pragma once
#include <atomic>
#include "postgresql_package.h"
//#include"connection_object.h"
#include "cache.h"
#include"metadata_struct.h"

namespace my_project {

    class Connection_Pool::CPImpl {
    public:
        friend class ConnectionDeleter;
    private:

        struct SqlPoolParams params;
        std::mutex pool_mutex;
        std::condition_variable cv;
        std::queue<std::unique_ptr<Connection>> pool;
        std::unique_ptr<my_cache::Cache<std::string, TableMetadata>> table_metadata;
        bool is_initialized;

        std::atomic<bool> is_running = true;
    public:

        ~CPImpl();
        bool init(const struct SqlPoolParams& params);
        void clear_table();
        void clear_table(const std::string& key);
        bool put_table(const std::string& key,TableMetadata& value);
        auto get_table(const std::string& key);
        std::unique_ptr < Connection, ConnectionDeleter> getConnection();

    private:
        bool addConnection();

        void produceConnection();

        void recycleConnection();

        void returnConnection(std::unique_ptr<Connection> conn);

        void clearPool();
    };

}