#include <iostream>
#include <libpq-fe.h>
#include"postgresql_package.h"
#include"builder.h"
using namespace std;
using namespace my_project;

int main() {
    my_project::SqlPoolParams s;
    my_project::Connection_Pool cp;
    cp.init(s);

    Builder b;
    uint64_t iter = 3000000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iter; i++) {
        auto ob = cp.getConnection();
        //printf("get connection %d\n", i);
    }
    auto end = std::chrono::steady_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 输出结果
    std::cout << "\n========== 连接池速度测试 ==========\n";
    std::cout << "迭代次数: " << iter << " 次\n";
    std::cout << "总耗时: " << ms.count() << " 毫秒\n";
    std::cout << "       " << us.count() << " 微秒\n";
    std::cout << "       " << ns.count() << " 纳秒\n";
    std::cout << "平均每次: " << ns.count() / iter << " 纳秒\n";
    std::cout << "每秒可处理: " << iter * 1000 / ms.count() << " 次\n";
    std::cout << "==================================\n";

    


    PGconn* conn = PQconnectdb("host=127.0.0.1 port=5432 dbname=postgres user=app_user password=20060321");

    // 2. 检查连接状态
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "连接数据库失败: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return 1;
    }
    string sql=" CREATE USER app_user1 WITH PASSWORD '20060321' ";
    PGresult* res = PQexec(conn, sql.c_str());

    // 4. 检查结果
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "创建用户失败: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return -1;
    }

    std::cout << "成功连接到数据库！" << std::endl;

}