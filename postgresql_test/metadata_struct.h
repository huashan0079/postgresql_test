#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>


namespace my_project {

    // 列的详细信息
    struct ColumnMetadata {
        std::string name;                    // 列名
        std::string pg_type_name;             // PostgreSQL原始类型名 (如 "int4", "varchar")
        int type_oid;                          // PostgreSQL类型OID

        // 长度/精度信息
        int max_length = 0;                    // VARCHAR(n) 的长度n
        int numeric_precision = 0;              // NUMERIC精度
        int numeric_scale = 0;                  // NUMERIC小数位

        // 约束信息
        bool is_nullable = true;                // 是否可为NULL
        bool is_primary_key = false;            // 是否主键
        bool is_unique = false;                  // 是否唯一约束
        std::optional<std::string> default_value; // 默认值

        // 继承信息（PostgreSQL特有）
        bool is_inherited = false;               // 是否从父表继承
        std::string origin_table;                 // 来源表（如果是继承）

        // 注释
        std::string comment;

        // 权限信息（用于校验）
        bool can_select = true;                   // 当前用户能否查询
        bool can_insert = true;                    // 能否插入
        bool can_update = true;                     // 能否更新

        // 统计信息（用于性能预警）
        int64_t null_count = 0;                     // NULL值数量
        int distinct_count = 0;                      // 不同值数量
        double null_frac = 0.0;                      // NULL比例
    };

    // 表的元数据
    struct TableMetadata {
        std::string schema;                       // schema名
        std::string name;                          // 表名
        std::string full_name;                     // schema.table
        std::string comment;                        // 表注释

        // 列信息
        std::vector<ColumnMetadata> columns;
        std::unordered_map<std::string, size_t> column_index;  // 列名 -> 索引

        // 索引信息
        struct IndexInfo {
            std::string name;
            std::vector<std::string> columns;
            bool is_primary;
            bool is_unique;
            std::string index_def;                   // CREATE INDEX语句
        };
        std::vector<IndexInfo> indexes;

        // 继承信息
        std::vector<std::string> parent_tables;      // 父表列表
        std::vector<std::string> child_tables;       // 子表列表（需要额外查询）

        // 表的基本信息
        int64_t row_estimate = 0;                     // 估计行数（reltuples）
        int64_t total_size = 0;                        // 表大小（字节）
        int64_t table_size = 0;                         // 表数据大小
        int64_t index_size = 0;                          // 索引大小

        // 权限信息
        bool can_select = true;
        bool can_insert = true;
        bool can_update = true;
        bool can_delete = true;

        // 获取列
        const ColumnMetadata* getColumn(const std::string& name) const {
            auto it = column_index.find(name);
            if (it != column_index.end()) {
                return &columns[it->second];
            }
            return nullptr;
        }
    };

} // namespace my_db
