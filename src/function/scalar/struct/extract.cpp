#include "duckdb/function/scalar/struct_functions.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/common/string_util.hpp"

using namespace std;

namespace duckdb {

static void struct_extract_fun(DataChunk &input, ExpressionState &state, Vector &result) {
	auto &func_expr = (BoundFunctionExpression &)state.expr;
	auto &info = (StructExtractBindData &)*func_expr.bind_info;

	// this should be guaranteed by the binder
	assert(input.column_count() == 1);
	auto &vec = input.data[0];

	if (info.index >= vec.GetChildren().size()) {
		throw Exception("Not enough struct entries for struct_extract");
	}

	auto &child = vec.GetChildren()[info.index];
	if (child.first != info.key || child.second->type != info.type) {
		throw Exception("Struct key or type mismatch");
	}
	result.Reference(*child.second.get());
	assert(result.size() == vec.size());
}

static unique_ptr<FunctionData> struct_extract_bind(BoundFunctionExpression &expr, ClientContext &context) {
	// the binder should fix this for us.
	assert(expr.children.size() == 2);
	assert(expr.arguments.size() == expr.children.size());

	auto &struct_child = expr.children[0];

	assert(expr.arguments[0].id == SQLTypeId::STRUCT);
	assert(struct_child->return_type == TypeId::STRUCT);
	if (expr.arguments[0].child_type.size() < 1) {
		throw Exception("Can't extract something from an empty struct");
	}

	auto &key_child = expr.children[1];

	if (expr.arguments[1].id != SQLTypeId::VARCHAR || key_child->return_type != TypeId::VARCHAR ||
	    !key_child->IsScalar()) {
		throw Exception("Key name for struct_extract needs to be a constant string");
	}
	Value key_val = ExpressionExecutor::EvaluateScalar(*key_child.get());
	assert(key_val.type == TypeId::VARCHAR);
	if (key_val.is_null || key_val.str_value.length() < 1) {
		throw Exception("Key name for struct_extract needs to be neither NULL nor empty");
	}
	string key = StringUtil::Lower(key_val.str_value);

	SQLType return_type;
	idx_t key_index = 0;
	bool found_key = false;

	for (size_t i = 0; i < expr.arguments[0].child_type.size(); i++) {
		auto &child = expr.arguments[0].child_type[i];
		if (child.first == key) {
			found_key = true;
			key_index = i;
			return_type = child.second;
			break;
		}
	}
	if (!found_key) {
		throw Exception("Could not find key in struct");
	}

	expr.return_type = GetInternalType(return_type);
	expr.sql_return_type = return_type;
	expr.children.pop_back();
	return make_unique<StructExtractBindData>(key, key_index, GetInternalType(return_type));
}

void StructExtractFun::RegisterFunction(BuiltinFunctions &set) {
	// the arguments and return types are actually set in the binder function
	ScalarFunction fun("struct_extract", {SQLType::STRUCT, SQLType::VARCHAR}, SQLType::ANY, struct_extract_fun, false,
	                   struct_extract_bind);
	set.AddFunction(fun);
}

} // namespace duckdb
