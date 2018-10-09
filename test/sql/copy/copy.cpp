
#include "catch.hpp"
#include "test_helpers.hpp"

#include <fstream>

using namespace duckdb;
using namespace std;

TEST_CASE("Test Copy statement", "[copystatement]") {
	unique_ptr<DuckDBResult> result;
	DuckDB db(nullptr);
	DuckDBConnection con(db);

	// Generate CSV file With ; as delimiter and complex strings
	ofstream from_csv_file("test.csv");
	for (int i = 0; i < 5000; i++)
		from_csv_file << i << "," << i << ", test" << endl;
	from_csv_file.close();

	// Loading CSV into a table
	result =
	    con.Query("CREATE TABLE test (a INTEGER, b INTEGER,c VARCHAR(10));");
	result = con.Query("COPY test FROM 'test.csv';");
	REQUIRE(CHECK_COLUMN(result, 0, {5000}));

	result = con.Query("SELECT COUNT(a), SUM(a) FROM test;");
	REQUIRE(CHECK_COLUMN(result, 0, {5000}));
	REQUIRE(CHECK_COLUMN(result, 1, {12497500}));

	//  Creating CSV from table
	result = con.Query("COPY test to 'test2.csv';");
	REQUIRE(CHECK_COLUMN(result, 0, {5000}));

	//  Creating CSV from Query
	result =
	    con.Query("COPY (select a,b from test where a < 4000) to 'test3.csv';");
	REQUIRE(CHECK_COLUMN(result, 0, {4000}));

	// Exporting selected columns from a table to a CSV.
	result = con.Query("COPY test(a,c) to 'test4.csv';");
	REQUIRE(CHECK_COLUMN(result, 0, {5000}));

	// Importing CSV to Selected Columns
	result =
	    con.Query("CREATE TABLE test2 (a INTEGER, b INTEGER,c VARCHAR(10));");
	result = con.Query("COPY test2(a,c) from 'test4.csv';");

	remove("test.csv");
	remove("test2.csv");
	remove("test3.csv");
	remove("test4.csv");
}