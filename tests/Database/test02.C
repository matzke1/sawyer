// Test that a statement in a non-executing state can continue to exist after the database connection is closed, and that
// operating on the statement later will throw an exception.

#include <Sawyer/DatabaseSqlite.h>

using namespace Sawyer::Database;

Statement globalStmt;

static void initGlobalStmt() {
    boost::filesystem::path dbName = "test02.db";

    boost::system::error_code ec;
    boost::filesystem::remove(dbName, ec);
    Sqlite db(dbName.native());

    globalStmt = db.stmt("create table table_1 (name text)");
}

static void useGlobalStmt() {
    globalStmt.run();
}

static void destroyGlobalStmt() {
    globalStmt = Statement();
}

int main() {
    initGlobalStmt();

    try {
        useGlobalStmt();
        ASSERT_not_reachable("test failed");
    } catch (const Exception &e) {
        ASSERT_always_require2(std::string(e.what()) == "connection is closed", e.what());
    }

    destroyGlobalStmt();
}
