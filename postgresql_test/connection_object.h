#pragma once
#include "postgresql_package.h"
#include "metadata_struct.h"
#include "cache.h"
namespace my_project {

    class Connection::CImpl {
        //friend struct Connection_Pool::ConnectionDeleter;
    public:
        CImpl(Connection_Pool::CPImpl* pool);
        //~Connection();
        ~CImpl();
        bool connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& port,const std::string& dbname);

        void refreshAliveTime();

        long long getAliveTime();

        //void fullResetStmt();

        //void putSqlStream(const std::string& sql);

        //void putSqlParam(const SqlParam& param);


    private:

        double pow10(unsigned int n);

        bool get_table(const std::string& table, std::unordered_map<std::string, TableMetadata>& value);

        bool execute();

        void clearSqlAndParams();

    private:
        std::chrono::steady_clock::time_point aliveTime;
        std::shared_ptr<PGconn> conn;
        const Connection_Pool::CPImpl* pool;

        bool is_null_true;
        bool is_null_false;
        std::string db_name;
        std::string table_name;
        std::vector<SqlParam> params;
        std::vector<SqlParam> where_params;
        std::vector<unsigned long> bind_lengths;
        std::unordered_map<std::string, TableMetadata> table_cache;
        std::unique_ptr<my_cache::Cache<std::string, TableMetadata>> table_metadata;
    };

	

}