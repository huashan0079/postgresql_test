#pragma once
#include <libpq-fe.h>
#include <variant> 
#include <iostream>
#include<queue>
#include<vector>
#include<unordered_map>
#include<thread>
#include<mutex>
#include<chrono>
//#include"connection_object.h"
//#include"Connection_Pool.h"
//#include"cache.h"
namespace my_project {
	class Connection_Pool;

	enum class TableFieldType {
		UNKNOWN,

		// 数值类型
		SMALLINT,           // int2
		INTEGER,            // int4
		BIGINT,            // int8

		// 浮点类型
		REAL,              // float4
		DOUBLE_PRECISION,  // float8

		// 精确数值
		NUMERIC,           // decimal/numeric

		// 字符类型
		VARCHAR,           // character varying
		CHAR,              // character
		TEXT,              // text

		// 二进制类型
		BYTEA,             // 二进制数据

		// 日期时间
		DATE,
		TIME,              // time without time zone
		TIMETZ,            // time with time zone
		TIMESTAMP,         // timestamp without time zone
		TIMESTAMPTZ,       // timestamp with time zone
		INTERVAL,          // 时间间隔

		// 布尔类型
		BOOLEAN,           // bool

		// 网络地址
		INET,
		CIDR,
		MACADDR,

		// JSON类型
		JSON,
		JSONB,

		// 其他
		UUID,
		XML,
		MONEY,

		NULL_TYPE
	};

	enum class SqlParamType {
		BOOL, LONG_LONG, DOUBLE, STRING, NULL_TYPE
	};

	struct SqlParam {

		using ValueType = std::variant<
			std::monostate,  // 表示 NULL
			bool,
			long long,
			unsigned long long,
			double,
			std::string
		>;
		SqlParam() : value(std::monostate{}), type(SqlParamType::NULL_TYPE) {}

		SqlParam(std::nullptr_t) : value(std::monostate{}), type(SqlParamType::NULL_TYPE) {}

		SqlParam(bool val) : value(val), type(SqlParamType::BOOL) {}

		SqlParam(int val) : value(static_cast<long long>(val)),
			type(SqlParamType::LONG_LONG) {
		}

		SqlParam(long long val) : value(val), type(SqlParamType::LONG_LONG) {}

		SqlParam(unsigned int val) : value(static_cast<unsigned long long>(val)),
			type(SqlParamType::LONG_LONG), is_unsigned(true) {
		}

		SqlParam(unsigned long long val) : value(val),
			type(SqlParamType::LONG_LONG), is_unsigned(true) {
		}

		SqlParam(float val) : value(static_cast<double>(val)),
			type(SqlParamType::DOUBLE) {
		}

		SqlParam(double val) : value(val), type(SqlParamType::DOUBLE) {}

		SqlParam(const std::string& val) : value(val), type(SqlParamType::STRING) {}

		SqlParam(const char* val) : value(std::string(val)), type(SqlParamType::STRING) {}

		ValueType value;
		
		SqlParamType type;
		bool is_unsigned = false;
	};

	struct SqlPoolParams {
		std::string host = "127.0.0.1";
		std::string user = "postgres";
		std::string passwd = "20060321";
		std::string dbname = "postgres";
		std::string port = "5432";
		std::string unix_socket = "";
		unsigned long client_flag = 0;

		int minSize = 16;
		int maxSize = 32;
		int retry_count = 3;
		int timeout = 30;
		int maxIdleTime = 300;

		int max_table_cache_capacity = 200;
	};

	class Connection;
	class Connection_Pool;
	

	class Connection_Pool {
		friend class Connection;
		class CPImpl;
		std::unique_ptr<CPImpl> Impl;
	public:
		class ConnectionDeleter {
		public:
			void operator()(Connection* conn) const;
			CPImpl* impl_;
		};
		friend class ConnectionDeleter;

	public:
		Connection_Pool(const Connection_Pool& obj) = delete;
		Connection_Pool& operator=(const Connection_Pool& obj) = delete;
		Connection_Pool();
		~Connection_Pool();
		bool init(const struct SqlPoolParams& params);
		std::unique_ptr < Connection, Connection_Pool::ConnectionDeleter> getConnection();
	private:
	};



	class Connection {
		friend class Connection_Pool::CPImpl;
		//friend class Connection_Pool;
		class CImpl;
		std::unique_ptr<CImpl> Impl;
	public:
		Connection(const Connection& obj) = delete;
        Connection& operator=(const Connection& obj) = delete;
		~Connection();
	private:
		Connection(Connection_Pool::CPImpl* pool);
		CImpl* const getCImpl()const{ return Impl.get(); }
	};

}

