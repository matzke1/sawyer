#include <Sawyer/Database.h>

#include <boost/filesystem.hpp>
#include <iostream>

using namespace Sawyer::Database;

// Basic API
static void test01(Connection db) {
    db.run("create table testing (id integer primary key, name text)");
    db.run("insert into testing (name) values ('alice')");
}

// Iteration
static void test02(Connection db) {
    Statement stmt = db.stmt("select name from testing limit 1");
    Iterator iter = stmt.begin();
    Row row = *iter;
    std::string name = *row.get<std::string>(0);
    ASSERT_always_require2(name == "alice", name);
}

// Binding
static void test03(Connection db) {
    Statement stmt = db.stmt("insert into testing (name) values (?name)");
    stmt.bind("name", "betty");
    stmt.run();
}

// Multi-row, multi-column results
static void test04(Connection db) {
    Statement stmt = db.stmt("select id, name from testing order by name");
    Iterator iter = stmt.begin();
    ASSERT_always_require(iter != stmt.end());
    int id = *iter->get<int>(0);
    std::string name = *iter->get<std::string>(1);
    ASSERT_always_require2(id == 1, boost::lexical_cast<std::string>(id));
    ASSERT_always_require2(name == "alice", name);

    ++iter;
    ASSERT_always_require(iter != stmt.end());
    id = *iter->get<int>(0);
    name = *iter->get<std::string>(1);
    ASSERT_always_require2(id == 2, boost::lexical_cast<std::string>(id));
    ASSERT_always_require2(name == "betty", name);

    ++iter;
    ASSERT_always_require(iter == stmt.end());
}

int main() {
    boost::filesystem::path dbName = "x.db";

    boost::system::error_code ec;
    boost::filesystem::remove(dbName, ec);
    Connection db(dbName.native());

    test01(db);
    test02(db);
    test03(db);
    test04(db);
}
