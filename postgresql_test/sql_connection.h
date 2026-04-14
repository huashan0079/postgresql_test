#pragma once

class Connection {

	friend class SelectBuilder;
	friend class InsertBuilder;
	friend class UpdateBuilder;
	friend class DeleteBuilder;
	Connection(const Connection& obj) = delete;
	Connection& operator=(const Connection& obj) = delete;
public:
	Connection(Connection_Pool* pool);
	~Connection();

	bool connect(std::string host, std::string user, std::string pwd, std::string db, unsigned int port);

	void disconnect();

	void refreshAliveTime();

	long long getAliveTime();

	bool switchDatabase(const std::string& db_name);

	MYSQL* getConn() { return conn; }

	const MYSQL_STMT* getStmt() { return stmt; }

	void fullResetStmt();

	void putSqlStream(const std::string& sql);

	void putSqlParam(const mysql_toolkit::SqlParam& param);

	SelectBuilder select(const std::string& main_table, const std::initializer_list<std::shared_ptr<Expr>>& exprs);

private:

	double pow10(unsigned int n);

	//void clear_table();
	//bool clear_table(const std::string& table);
	bool get_table(const std::string& table, std::unordered_map<std::string, FieldMeta>& value);

	bool execute();

	bool bindParams(MYSQL_BIND* bind);

	void clearSqlAndParams();



private:
	std::chrono::steady_clock::time_point aliveTime;
	MYSQL* conn;
	Connection_Pool* pool;
	MYSQL_STMT* stmt;
	bool is_null_true;
	bool is_null_false;
	std::string db_name;
	std::string table_name;
	std::stringstream sql_buf;
	std::vector<SqlParam> params;
	std::vector<SqlParam> where_params;
	std::vector<unsigned long> bind_lengths;
	std::unordered_map<std::string, FieldMeta> table_cache;
	std::unique_ptr<my_cache::Cache<std::string, std::unordered_map<std::string, FieldMeta>>> table_metadata;
};