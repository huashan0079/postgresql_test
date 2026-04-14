#pragma once
#include<iostream>
#include<string>
#include<vector>
#include"postgresql_package.h"
namespace my_project {

	enum class OpType {
		EQ, NE, LT, LE, GT, GE, ADD, SUB, MUL, DIV, MOD, BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_LEFT, BIT_RIGHT, LIKE
	};

	enum class CompositeType {
		AND, OR
	};

	enum class JoinType {
        INNER, LEFT, RIGHT, FULL
	};

	inline std::string op_to_string(OpType op) {
		switch (op) {
		case OpType::EQ: return "=";
		case OpType::NE: return "<>";
		case OpType::LT: return "<";
		case OpType::LE: return "<=";
		case OpType::GT: return ">";
		case OpType::GE: return ">=";
		case OpType::ADD: return "+";
		case OpType::SUB: return "-";
		case OpType::MUL: return "*";
		case OpType::DIV: return "/";
		case OpType::MOD: return "%";
		case OpType::BIT_AND: return "&";
		case OpType::BIT_OR: return "|";
		case OpType::BIT_XOR: return "^";
		case OpType::BIT_NOT: return "~";
		case OpType::BIT_LEFT: return "<<";
		case OpType::BIT_RIGHT: return ">>";
		case OpType::LIKE: return "LIKE";
		}
	}

	class Expr {
	public:
		virtual ~Expr() = default; // 虚析构：确保子类析构正确
		virtual void collect_params(std::vector<SqlParam>& params, std::ostringstream& oss) const = 0; // 收集该表达式的参数
		std::string bind_field = "";
		std::string bind_table = "";
	};

	class ColumnExpr :public Expr {
	public:
		ColumnExpr(const std::string& schema_name, const std::string& table_name,
			const std::string& column_name, const std::string& alias = "")
			: table_name(table_name), field_name(field_name), alias(alias) {
			bind_table = table_name;
			bind_field = field_name;
		}
		inline void collect_params(std::vector<SqlParam>& params, std::ostringstream& oss) const override {}
	private:
		const std::string table_name;
		std::string field_name;
		std::string alias;
	};

	class FromItem {

	};

	class Table :public FromItem, public std::enable_shared_from_this<Table> {
		std::string table_name;
		std::string schema_name;
		std::string alias;
	public:
		Table(const std::string& schema_name, const std::string& table_name, const std::string& alias = "")
			:table_name(table_name), schema_name(schema_name), alias(alias) {
		}
		std::shared_ptr<FromItem> get_table() {
			return shared_from_this();
		}
		std::shared_ptr<ColumnExpr> col(const std::string& column_name) {
			return std::make_shared<ColumnExpr>(schema_name, table_name, column_name, alias);
		}
	};

	class ParamExpr : public Expr {
		SqlParam value;
		ParamExpr(const SqlParam& val) : value(val) {
			bind_field = "";
			bind_table = "";
		}

		void collect_params(std::vector<SqlParam>& params, std::ostringstream& oss) const override {
			params.emplace_back(value); // 收集参数值
		}

	};

	class BinaryExpr : public Expr {
		std::shared_ptr<Expr> left;  // 左表达式（通常是ColumnExpr）
		OpType op;                   // 运算符
		std::shared_ptr<Expr> right; // 右表达式（ColumnExpr/ParamExpr/子查询）

		BinaryExpr(std::shared_ptr<Expr> l, OpType o, std::shared_ptr<Expr> r)
			: left(l), op(o), right(r) {
			bind_field = left->bind_field;
		}

		void collect_params(std::vector<SqlParam>& params, std::ostringstream& oss) const override {
			// 递归收集左右表达式的参数
			left->collect_params(params, oss);
			right->collect_params(params, oss);
		}
	};

	class CompositeExpr : public Expr {
		CompositeType op; // 只能是AND/OR
		std::vector<std::shared_ptr<Expr>> children; // 子表达式列表

		CompositeExpr(CompositeType o, std::vector<std::shared_ptr<Expr>> c) : op(o), children(std::move(c)) {}

		void collect_params(std::vector<SqlParam>& params, std::ostringstream& oss) const override {
			// 递归收集所有子表达式的参数
			for (const auto& child : children) {
				child->collect_params(params, oss);
			}
		}
	};

	class JoinExpr {
        std::shared_ptr<Expr> expr;
		std::shared_ptr<FromItem> source;
		JoinType op;
	public:
        JoinExpr(JoinType op, std::shared_ptr<FromItem> source, std::shared_ptr<Expr> expr) : 
		op(op), source(source), expr(expr) {}
	};

	class DML {
		using Value = std::variant<SqlParam, std::shared_ptr<Expr>>;
		std::vector<std::shared_ptr<FromItem>> source;// 来源
		std::shared_ptr<Expr> condition;
		std::vector<std::shared_ptr<Expr>> order_by;
        std::vector<std::shared_ptr<Expr>> returning;
		int limit_value;

		//Select 专用
		std::vector<std::shared_ptr<JoinExpr>> join_exprs;
		std::vector<std::shared_ptr<Expr>> group_by;
		std::vector<std::shared_ptr<Expr>> having;
		//Insert 专用
		std::vector<std::shared_ptr<Expr>> insert_columns;
        std::vector<std::vector<std::shared_ptr<Expr>>> values;
		std::vector<std::vector<Value>> rows;

		//Update 专用
        std::vector<std::shared_ptr<Expr>> update_columns;
        std::vector<Value> update_values;
	};

	template<typename Derived>
	class FromMixin {
    protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& FROM(std::initializer_list<std::shared_ptr<FromItem>> tables) {
			for (auto table : tables) {
                derived().source.push_back(table);
			}
			return derived();
		}
	};


	// WHERE 模块
	template<typename Derived>
	class WhereMixin {
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& WHERE(std::shared_ptr<Expr> condition) {
			derived().condition = condition;
			return derived();
		}
	};

	template<typename Derived>
	class LimitMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& LIMIT(int limit) {
			derived().limit_value = limit;
			return derived();
		}
	};

	// ORDER BY 模块
	template<typename Derived>
	class OrderByMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& ORDER_BY(std::shared_ptr<Expr> order) {
			derived().order_by.emplace_back(order);
			return derived();
		}
	};

	// RETURNING 模块
	template<typename Derived>
	class ReturningMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& RETURNING(std::initializer_list<std::shared_ptr<Expr>> cols) {
			derived().returning.clear();
			for (auto col : cols) {
                derived().returning.emplace_back(col);
			}
			return derived();
		}
	};

	// WITH 模块
	template<typename Derived>
	class WithMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		Derived& WITH(const std::string& name, const std::string& query) {
			std::stringstream ss;
			ss << name << " AS (" << query << ")";
			derived().with_ctes.push_back(ss.str());
			return derived();
		}
	};

	template<typename Derived>
	class SelectMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		
		Derived& JOIN(JoinType op, std::shared_ptr<FromItem> source, std::shared_ptr<Expr> expr) {
			derived().join_exprs.push_back(std::make_shared<JoinExpr>(op, source, expr));
			return derived();
		}

		Derived& GROUP_BY(std::shared_ptr<Expr> col) {
			derived().group_by.push_back(col);
			return derived();
		}

		Derived& HAVING(std::shared_ptr<Expr> expr) {
			derived().having.emplace_back(expr);
			return derived();
		}
	};

	// INSERT 专用模块
	template<typename Derived>
	class InsertMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		

		Derived& COLUMNS(std::initializer_list<std::shared_ptr<Expr>> cols) {
			for (auto col : cols) {
                derived().insert_columns.empalce_back(col);
			}
			return derived();
		}
		template<typename... Args>
		Derived& VALUES(Args&&... args) {
			derived().rows.emplace_back();
			auto& row = derived().rows.back();
			(row.emplace_back(std::forward<Args>(args)), ...);
			return derived();
		}
	};

	// UPDATE 专用模块
	template<typename Derived>
	class UpdateMixin {
	protected:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
	public:
		template<typename Val>
		Derived& SET(std::shared_ptr<Expr> col, Val&& value) {
			derived().update_columns.push_back(col);
            derived().update_values.emplace_back(std::forward<Val>(value));
			return derived();
		}
	};

	

	class SelectBuilder :public DML,
		public SelectMixin<SelectBuilder>,
		public FromMixin<SelectBuilder>,
		public WhereMixin<SelectBuilder>,
		public LimitMixin<SelectBuilder>,
		public OrderByMixin<SelectBuilder>,
		public WithMixin<SelectBuilder>
	{
		friend class Builder;
		SelectBuilder(std::initializer_list<std::shared_ptr<Expr>>& cols) {

		}
		public:

	};

	class InsertBuilder :public DML,
	    public InsertMixin<InsertBuilder>,
		public ReturningMixin<InsertBuilder>
	{
		friend class Builder;
		InsertBuilder(std::shared_ptr<FromItem>& table) {

		}
        public:

	};

	class UpdateBuilder :public DML,
		public UpdateMixin<UpdateBuilder>,
		public WhereMixin<UpdateBuilder>,
        public ReturningMixin<UpdateBuilder>
	{
		friend class Builder;
		UpdateBuilder(std::shared_ptr<FromItem>& table) {

		}
        public:

	};

	class DeleteBuilder :public DML,
		public WhereMixin<DeleteBuilder>,
        public ReturningMixin<DeleteBuilder>
	{
		friend class Builder;
		DeleteBuilder(std::shared_ptr<FromItem>& table) {

		}
		public:
		
	};

	class Builder {

	public:
		SelectBuilder Select(std::initializer_list<std::shared_ptr<Expr>>& cols) {
			return SelectBuilder(cols);
		}
		InsertBuilder Insert(std::shared_ptr<FromItem>& table) {
            return InsertBuilder(table);
		}
		UpdateBuilder Update(std::shared_ptr<FromItem>& table) {
			return UpdateBuilder(table);
		}
		DeleteBuilder Delete(std::shared_ptr<FromItem>& table) {
			return DeleteBuilder(table);
		}
	};

};

