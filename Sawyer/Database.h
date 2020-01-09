#ifndef Sawyer_Database_H
#define Sawyer_Database_H

#include <boost/iterator/iterator_facade.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cctype>
#include <memory.h>
#include <Sawyer/Assert.h>
#include <Sawyer/Map.h>
#include <Sawyer/Optional.h>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace Sawyer {

/** SQL database interface.
 *
 *  This interface is similar to the SQLite3 and PostgreSQL C APIs but using C++ idioms, such as:
 *
 *  @li The parameters in a prepared statement are numbered starting at zero instead of one, just as rows and columns are
 *  numbered starting at zero. The underlying APIs for SQLite3 and PostgreSQL are both inconsistent in this regard.
 *
 *  @li Unexpected errors are handled by throwing exceptions rather than returning a success or error code or by requiring
 *  a call to another error-query function.
 *
 *  @li The resulting rows of a query are accessed by single-pass iterators in the usual C++ way.
 *
 *  @li Result iterators report (by exception) when they're accessed after being invalidated by some operation on the statement
 *  or database.
 *
 *  @li This API uses namespaces and classes for encapsulation rather than using free functions in the global scope.
 *
 *  @li Quantities that are logically non-negative use unsigned types.
 *
 *  @li All objects are reference counted and destroyed automatically.
 *
 *  @li A prepared statement that's not currently executing doesn't count as a reference to the database, allowing prepared
 *  statements to be cached in places (global variables, objects, etc.) that might make them difficult to locate and destroy
 *  in order to remove all references to a database.
 *
 *  Here's a basic example to open or create a database:
 *
 * @code
 *  #include <Sawyer/Database.h>
 *  using namespace Sawyer::Database;
 *
 *  Connection db("the_database_file.db");
 * @endcode
 *
 *  Once a database is opened you can run queries. The simplest API is for queries that don't produce any output. This is a
 *  special case of a more general mechanism which we'll eventually get to:
 *
 * @code
 *  db.run("create table testing (id integer primary key, name text)");
 *  db.run("insert into testing (name) values ('Allan')");
 * @endcode
 *
 *  The next most complicated API is for queries that return a single value. Specifically this is any query that returns at
 *  least one row of results, where the first column of the first row is the value of interest.  Since the return value could
 *  be either a value or null, the return type is a @ref Sawyer::Optional, which allows you to do things like provide a
 *  default:
 *
 * @code
 *  std::string name = db.get<std::string>("select name from testing limit 1").orElse("Burt");
 * @endcode
 *
 *  Then there are queries that return multiple rows each having multiple columns with zero-origin indexing. Again, the "get"
 *  functions return a @ref Sawyer::Optional in order to handle null values.
 *
 * @code
 *  for (auto row: db.stmt("select id, name from testing"))
 *      std::cout <<"id=" <<row.get<int>(0).orElse(0) <<", name=" <<row.get<std::string>(1).orDefault() <<"\n";
 * @endcode
 *
 *  Finally, statements can have named parameters that get filled in later, possibly repeatedly:
 *
 * @code
 *  std::vector<std::pair<std::string, int>> data = ...;
 *  Statement stmt = db.stmt("insert into testing (id, name) values (?id, ?name)");
 *  for (auto record: data) {
 *      stmt.bind("name", record.first);
 *      stmt.bind("id", record.second);
 *      stmt.run();
 *  }
 * @endcode
 *
 *  Chaining of the member functions works too. Here's the same example again:
 *
 * @code
 *  std::vector<std::pair<std::string, int>> data = ...;
 *  Statement stmt = db.stmt("insert into testing (id, name) values (?id, ?name)");
 *  for (auto record: data)
 *      stmt.bind("name", record.first).bind("id", record.second).run();
 * @endcode
 *
 *  Here's a query that uses parameters:
 *
 * @code
 *  // parse the SQL once
 *  Statement stmt = db.stmt("select name from testing where id = ?id");
 *
 *  // later, bind the parameters to actual values and execute
 *  stmt.bind("id", 123);
 *  for (auto row: stmt)
 *      std::cout <<"name=" <<row.get<std::string>(0) <<"\n";
 * @endcode
 *
 *  Of course you can also iterate over results by hand too. And if you decide to stop early then the statement is automatically
 *  canceled:
 *
 * @code
 *  Statement stmt = db.stmt("select name from testing order by name");
 *
 *  for (Statement::Iterator row = stmt.begin(); row != stmt.end(); ++row) {
 *      std::string name = row->get<std::string>(0);
 *      std::cout <<"name = " <<name <<"\n";
 *      if ("foobar" == name)
 *          break;
 *  }
 * @endcode
 *
 *  Here's a small example showing how using idiomatic C++ instead of the lower-level C API can make programs easier to read.
 *  Both versions abort with an error message on any kind of SQLite error, although the second one does so by throwing
 *  exceptions. The original code:
 *
 * @code
 *  ASSERT_not_null(connection);
 *  std::string q = "insert into interesting (url, parent_page, reason, discovered) values (?,?,?,?)";
 *  sqlite3_stmt *stmt = nullptr;
 *  int status;
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_prepare_v2(connection_, q.c_str(), q.size()+1, &stmt, nullptr)),
 *                         sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_TRANSIENT)),
 *                         sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_bind_int64(stmt, 2, parentPage)),
 *                         sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_bind_text(stmt, 3, reason.c_str(), -1, SQLITE_STATIC)),
 *                         sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_bind_text(stmt, 4, timeStamp().c_str(), -1, SQLITE_TRANSIENT)),
 *                         sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_DONE == (status = sqlite3_step(stmt)), sqlite3_errstr(status));
 *  ASSERT_always_require2(SQLITE_OK == (status = sqlite3_finalize(stmt)), sqlite3_errstr(status)); stmt = nullptr;
 * @endcode
 *
 *  And here's the idiomatic C++ version:
 *
 * @code
 *  connection.stmt("insert into interesting (url, parent_page, reason, discovered)"
 *                  " values (?url, ?parent_page, ?reason, ?discovered)")
 *      .bind("url", url.str())
 *      .bind("parent_page", parentPage)
 *      .bind("reason", reason)
 *      .bind("discovered", timeStamp())
 *      .run();
 * @endcode
 *
 * */
namespace Database {

class Connection;
class Statement;
class Row;
class Iterator;

namespace Detail {
    class Connection;
    class Statement;
}

class Exception: public std::runtime_error {
public:
    Exception(const std::string &what)
        : std::runtime_error(what) {}

    ~Exception() noexcept {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Connection
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** %Connection to an underlying database.
 *
 *  A @ref Connection is a lightweight object that can be copied. All copies initially point to the same underlying RDBMS
 *  connection, but individuals can have their connection changed with member functions such as @ref open and @ref close. The
 *  connections are destroyed automatically only after no databases or executing statements reference them. */
class Connection {
    std::shared_ptr<Detail::Connection> pimpl_;

public:
    /** Construct an instance not connected to any RDBMS. */
    Connection();

    /** Construct an instance by opening a connection to a RDBMS. */
    explicit Connection(const std::string&);

private:
    friend class Statement;
    explicit Connection(const std::shared_ptr<Detail::Connection> &pimpl);

public:
    /** Destructor.
     *
     *  The destructor calls @ref close before destroying this object. */
    ~Connection() = default;

    /** Opens a connection to a RDBMS.
     *
     *  If this object has an open connection to a RDBMS then that connection is broken, possibly causing it to close, before
     *  the new connection is established.  An exception is thrown if the new connection cannot be established. */
    Connection& open(const std::string &url);

    /** Tests whether an underlying RDBMS connection is established. */
    bool isOpen() const;

    /** Close a database if open.
     *
     *  Any established connection to a RDBMS is removed. If this was the last active reference to the connection then the
     *  connection is closed.  Statements that are in an "executing" state count as references to the connection, but other
     *  statement states are not counted, and they will become invalid (operating on them will throw an exception). */
    Connection& close();

    /** Creates a statement to be executed.
     *
     *  Takes a string containing a single SQL statement, parses and compiles it, and prepares it to be executed. Any parsing or
     *  compiling errors will throw an exception. The returned statement will be in either an "unbound" or "ready" state
     *  depending on whether it has parameters or not, respectively.  A statement in these states does not count as a reference
     *  to the underlying RDBMS connection and closing the connection will invalidate the statement (it will throw an exception
     *  if used).  This allows prepared statements to be constructed and yet still be possible to close the connection and
     *  release any resources it's using.
     *
     *  Statements may have parameters that need to be bound to actual values before the statement can start
     *  executing. Parameters are represented by "?name" occurring outside any string literals, where "name" is a sequence of
     *  one or more alpha-numeric characters. The same name may appear more than once within a statement, in which case binding
     *  an actual value to the parameter will cause all occurrances of that parameter to bound to the specified value.
     *  Parameters can only appear at certain positions within the SQL where a value rather than a key word is expected. */
    Statement stmt(const std::string &sql);

    /** Shortcut to execute a statement with no result. */
    Connection& run(const std::string &sql);

    /** Shortcut to execute a statement returning a single result.
     *
     *  The SQL statement is expected to return at least one row, and the first column of that row becomes the return value of
     *  this member function. The return type is a @ref Sawyer::Optional in order to represent database null values. */
    template<typename T>
    Sawyer::Optional<T> get(const std::string &sql);

    /** Row number for the last SQL "insert".
     *
     *  This has all the same caveats as the sqlite3_last_insert_rowid function. */
    size_t lastInsert() const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Statement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** SQL statement.
 *
 *  Statements are lightweight objects that can be copied and assigned. Statements in the EXECUTING state hold a reference
 *  the the underlying RDBMS connection. */
class Statement {
    std::shared_ptr<Detail::Statement> pimpl_;

public:
    /** A statement can be in one of many states. */
    enum State {
        UNBOUND,                                        /**< Statement contains parameters not bound to actual values. */
        READY,                                          /**< All parameters are bound and statement is ready to execute. */
        EXECUTING,                                      /**< Statement has started executing and is pointing to a result row. */
        FINISHED,                                       /**< Statement has finished executing. Parameters remain bound. */
        DEAD                                            /**< Synchronization with lower layer has been lost. */
    };

private:
    friend class Detail::Connection;
    explicit Statement(const std::shared_ptr<Detail::Statement> &stmt)
        : pimpl_(stmt) {}

public:
    /** Connection associated with this statement. */
    Connection connection() const;

    /** Bind an actual value to a statement parameter.
     *
     *  The parameter with the specified @p name is bound (or rebound) to the specified actual @p value. An exception is thrown
     *  if the statement doesn't have a parameter with the given name.
     *
     *  If the statement was in the finished state, then binding a parameter causes all other parameters to become unbound. This
     *  helps ensure that all parameters are bound to the correct values each time a prepared statement is executed. If you don't
     *  want this behavior, use @ref rebind instead.  Regardless, the statement is transitioned to either the @ref UNBOUND or
     *  @ref READY state. */
    template<typename T>
    Statement& bind(const std::string &name, const T &value);

    /** Re-bind an actual value to a statement parameter.
     *
     *  This function's behavior is identical to @ref bind except one minor detail: no other parameters are unbound if the
     *  statement was in the finished state. */
    template<typename T>
    Statement& rebind(const std::string &name, const T &value);

    /** Begin execution of a statement.
     *
     *  Begins execution of a statement and returns an iterator to the result.  If the statement returns at least one row
     *  of a result then the returned iterator points to this result and the statement will be in the @ref EXECUTING state
     *  until the iterator is destroyed.  If the statement doesn't produce at least one row then an end iterator is returned
     *  and the statement is in the @ref FINISHED state. Errors are indicated by throwing an exception. */
    Iterator begin();

    /** The end iterator. */
    Iterator end();

    /** Shortcut to run a statement and ignore results.
     *
     *  This is here mainly for constency with @ref Connection::run. It's identical to @ref begin except we ignore the returned
     *  iterator, which causes the statement to be canceled after its first row of output. */
    Statement& run();
    
    /** Shortcut to run a statement returning a single result.
     *
     *  The result that's returned is the first column of the first row. The statement must produce at least one row of result,
     *  although the first column of that row can be null. */
    template<typename T>
    Sawyer::Optional<T> get();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Row
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Refers to a row of the result of an executed statement.
 *
 *  This is the type obtained by dereferencing an iterator. Rows are lightweight objects that can be copied. A row is
 *  automatically invalidated when the iterator is incremented. */
class Row {
    friend class Iterator;
    std::shared_ptr<Detail::Statement> stmt_;
    size_t sequence_;                                   // for checking validity

private:
    Row()
        : sequence_(0) {}

    explicit Row(const std::shared_ptr<Detail::Statement> &stmt);

public:
    /** Get a particular column. */
    template<typename T>
    Sawyer::Optional<T> get(size_t columnIdx) const;

    /** Get the row number.
     *
     *  Rows are numbered starting at zero. */
    size_t rowNumber() const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Iterator
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Single-pass iterator over the result rows of an executed statement.
 *
 *  An iterator is either the special "end" iterator, or points to a row of a result from a statement currently in the
 *  @ref Statement::EXECUTING "EXECUTING" state. Incrementing the iterator advances to either the next row of
 *  the statement or the end iterator. When the end iterator is reached the associated statement is is transitioned to
 *  the @ref Statement::FINISHED "FINISHED" state. */
class Iterator: public boost::iterator_facade<Iterator, const Row, boost::forward_traversal_tag> {
    Row row_;

public:
    /** An end iterator. */
    Iterator() {}

private:
    friend class Detail::Statement;
    explicit Iterator(const std::shared_ptr<Detail::Statement> &stmt);

public:
    /** Test whether this is an end iterator. */
    bool isEnd() const {
        return !row_.stmt_;
    }

    /** Test whether this is not an end iterator. */
    explicit operator bool() const {
        return !isEnd();
    }
    
private:
    friend class boost::iterator_core_access;
    const Row& dereference() const;
    bool equal(const Iterator&) const;
    void increment();
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Only implementation details beyond this point.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Detail {

namespace Outer = ::Sawyer::Database;

// Reference counted
class Connection: public std::enable_shared_from_this<Connection> {
    friend class Outer::Connection;
    friend class Statement;

    sqlite3 *connection = nullptr;

private:
    Connection() {}
    explicit Connection(const std::string &url) {
        open(url);
    }

public:
    ~Connection() {
        try {
            close();
        } catch (...) {
        }
    }

private:
    void open(const std::string &url) {
        close();
        if (strlen(url.c_str()) != url.size())
            throw Exception("invalid database name");
        int status = sqlite3_open(url.c_str(), &connection);
        if (SQLITE_OK != status)
            throw Exception(sqlite3_errstr(status));
        sqlite3_busy_timeout(connection, 1000 /*milliseconds*/);
    }

    void close() {
        if (connection) {
            int status = sqlite3_close(connection);
            connection = nullptr;
            if (SQLITE_OK != status)
                throw Exception(sqlite3_errstr(status));
        }
    }

    Outer::Statement stmt(const std::string &sql);

    size_t lastInsert() const {
        ASSERT_not_null(connection);
        return boost::numeric_cast<size_t>(sqlite3_last_insert_rowid(connection));
    }
};

class Parameter {
    friend class Statement;

    std::vector<size_t> locations;                      // 1-origin "?" indexes
    bool isBound = true;

    void append(int location) {
        ASSERT_require(location > 0);
        locations.push_back(location);
    }
};

// Reference counted
class Statement: public std::enable_shared_from_this<Statement> {
    friend class Connection;
    friend class Outer::Statement;
    friend class Outer::Iterator;
    friend class Outer::Row;
    using Parameters = Sawyer::Container::Map<std::string, Parameter>;
    
    std::shared_ptr<Connection> db;                     // non-null while statement is executing
    std::weak_ptr<Connection> weakDb;                   // refers to the originating connection
    sqlite3_stmt *stmt = nullptr;                       // underlying SQLite3 statement
    Parameters params;                                  // mapping from param names to question marks
    Outer::Statement::State state_ = Outer::Statement::DEAD; // don't set directly; use "state" member function
    size_t sequence = 0;                                // sequence number for invalidating row iterators
    size_t rowNumber = 0;                               // result row number

    // FIXME[Robb Matzke 2020-01-06]: use UTF-8 when parsing
    explicit Statement(const std::shared_ptr<Connection> &db, const std::string &sql)
        : weakDb(db) {                                  // save only a weak pointer, no shared pointer
        ASSERT_not_null(db);
        ASSERT_not_null(db->connection);

        // Parse the input SQL to find named parameters and create the low-level SQL having only "?" parameters
        std::string lowSql;
        bool inString = false;
        state(Outer::Statement::READY);
        for (size_t i=0; i<sql.size(); ++i) {
            if ('\'' == sql[i]) {
                inString = !inString;                   // works for "''" escape too
                lowSql += sql[i];
            } else if ('?' == sql[i] && !inString) {
                lowSql += '?';
                std::string paramName;
                while (i+1 < sql.size() && (::isalnum(sql[i+1]) || '_' == sql[i+1]))
                    paramName += sql[++i];
                if (paramName.empty())
                    throw Exception("invalid parameter name at character position " + boost::lexical_cast<std::string>(i));
                Parameter &param = params.insertMaybeDefault(paramName);
                param.append(params.size());            // 1-origin parameter numbers
                state(Outer::Statement::UNBOUND);
            } else {
                lowSql += sql[i];
            }
        }
        if (inString) {
            state(Outer::Statement::DEAD);
            throw Exception("mismatched quotes in SQL statement");
        }

        // Create the low-level prepared statement
        const char *rest = nullptr;
        int status = sqlite3_prepare_v2(db->connection, lowSql.c_str(), lowSql.size()+1, &stmt, &rest);
        if (SQLITE_OK != status)
            throw Exception(sqlite3_errstr(status));
        while (rest && ::isspace(*rest))
            ++rest;
        if (rest && *rest) {
            sqlite3_finalize(stmt);                     // clean up if possible; ignore error otherwise
            throw Exception("extraneous text after end of SQL statement");
        }
    }

    void invalidateIteratorsAndRows() {
        ++sequence;
    }

    bool lockDatabase() {
        return (db = weakDb.lock()) != nullptr;
    }

    void unlockDatabase() {
        db.reset();
    }

    bool isDatabaseLocked() const {
        return db != nullptr;
    }

    std::shared_ptr<Connection> database() const {
        return weakDb.lock();
    }
    
    Outer::Statement::State state() const {
        return state_;
    }
    
    void state(Outer::Statement::State newState) {
        switch (newState) {
            case Outer::Statement::DEAD:
            case Outer::Statement::FINISHED:
            case Outer::Statement::UNBOUND:
            case Outer::Statement::READY:
                invalidateIteratorsAndRows();
                unlockDatabase();
                break;
            case Outer::Statement::EXECUTING:
                ASSERT_require(isDatabaseLocked());
                break;
        }
        state_ = newState;
    }

    bool hasUnboundParameters() const {
        ASSERT_forbid(state() == Outer::Statement::DEAD);
        for (const Parameter &param: params.values()) {
            if (!param.isBound)
                return true;
        }
        return false;
    }

    void unbindAllParams() {
        ASSERT_forbid(state() == Outer::Statement::DEAD);
        for (Parameter &param: params.values())
            param.isBound = false;
        state(params.isEmpty() ? Outer::Statement::READY : Outer::Statement::UNBOUND);
    }
    
    void reset(bool doUnbind) {
        ASSERT_forbid(state() == Outer::Statement::DEAD);
        int status = SQLITE_OK;
        if (Outer::Statement::EXECUTING == state() || Outer::Statement::FINISHED == state())
            status = sqlite3_reset(stmt);               // doesn't actually unbind parameters

        invalidateIteratorsAndRows();
        if (doUnbind) {
            unbindAllParams();
        } else {
            state(hasUnboundParameters() ? Outer::Statement::UNBOUND : Outer::Statement::READY);
        }

        if (SQLITE_OK != status) {
            state(Outer::Statement::DEAD);              // we no longer know the SQLite3 state
            throw Exception(sqlite3_errstr(status));
        }
    }

    // FIXME[Robb Matzke 2020-01-06]: check that we can bind null, perhaps using Sawyer::Nothing
    template<typename T>
    void bind(const std::string &name, const T &value, bool isRebind) {
        switch (state()) {
            case Outer::Statement::DEAD:
                throw Exception("statement is dead");
            case Outer::Statement::FINISHED:
            case Outer::Statement::EXECUTING:
                reset(!isRebind);
                // fall through
            case Outer::Statement::READY:
            case Outer::Statement::UNBOUND: {
                if (!params.exists(name))
                    throw Exception("no such parameter \"" + name + "\" in statement");
                Parameter &param = params[name];
                bool wasUnbound = param.isBound;
                for (size_t location: param.locations) {
                    int status = bind(location, value);
                    if (SQLITE_OK != status) {
                        if (param.locations.size() > 1)
                            state(Outer::Statement::DEAD); // parameter might be only partly bound now
                        throw Exception(sqlite3_errstr(status));
                    }
                }
                param.isBound = true;

                if (wasUnbound && !hasUnboundParameters())
                    state(Outer::Statement::READY); 
                break;
            }
        }
    }

    int bind(size_t location, int value) {
        return sqlite3_bind_int(stmt, location, value);
    }

    int bind(size_t location, int64_t value) {
        return sqlite3_bind_int64(stmt, location, value);
    }

    int bind(size_t location, size_t value) {
        return bind(location, boost::numeric_cast<int64_t>(value));
    }
    
    int bind(size_t location, double value) {
        return sqlite3_bind_double(stmt, location, value);
    }

    int bind(size_t location, const std::string &value) {
        return sqlite3_bind_text(stmt, location, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    int bind(size_t location, const char *value) {
        return value ? bind(location, std::string(value)) : bind_null(location);
    }

    int bind_null(size_t location) {
        return sqlite3_bind_null(stmt, location);
    }
    
    Outer::Iterator begin() {
        switch (state()) {
            case Outer::Statement::DEAD:
                throw Exception("statement is dead");
            case Outer::Statement::UNBOUND: {
                std::string s;
                for (Parameters::Node &param: params.nodes()) {
                    if (!param.value().isBound)
                        s += (s.empty() ? "" : ", ") + param.key();
                }
                ASSERT_forbid(s.empty());
                throw Exception("unbound parameters: " + s);
            }
            case Outer::Statement::FINISHED:
            case Outer::Statement::EXECUTING: {
                invalidateIteratorsAndRows();
                int status = sqlite3_reset(stmt);
                if (SQLITE_OK != status) {
                    state(Outer::Statement::DEAD);
                    throw Exception(sqlite3_errstr(status));
                }
            }
                // fall through
            case Outer::Statement::READY: {
                if (!lockDatabase())
                    throw Exception("connection has been closed");
                state(Outer::Statement::EXECUTING);
                Outer::Iterator iter = next();
                rowNumber = 0;
                return iter;
            }
        }
        ASSERT_not_reachable("invalid state");
    }

    // Advance an executing statement to the next row
    Outer::Iterator next() {
        ASSERT_require(state() == Outer::Statement::EXECUTING); // no other way to get here
        ++rowNumber;
        int status = sqlite3_step(stmt);
        if (SQLITE_ROW == status) {
            invalidateIteratorsAndRows();
            return Iterator(shared_from_this());
        } else if (SQLITE_DONE == status) {
            state(Outer::Statement::FINISHED);
            return Iterator();
        } else {
            state(Outer::Statement::DEAD);
            throw Exception(sqlite3_errstr(status));
        }
    }

    // Get a column value from the current row of result
    template<typename T>
    Sawyer::Optional<T> get(size_t columnIdx) {
        ASSERT_require(state() == Outer::Statement::EXECUTING); // no other way to get here
        size_t nCols = boost::numeric_cast<size_t>(sqlite3_column_count(stmt));
        if (columnIdx >= nCols)
            throw Exception("column index " + boost::lexical_cast<std::string>(columnIdx) + " is out of range");
        if (SQLITE_NULL == sqlite3_column_type(stmt, columnIdx))
            return Sawyer::Nothing();
        size_t nBytes = sqlite3_column_bytes(stmt, columnIdx);
        const unsigned char *s = sqlite3_column_text(stmt, columnIdx);
        ASSERT_not_null(s);
        std::string str(s, s+nBytes);
        return boost::lexical_cast<T>(str);
    }

    // FIXME[Robb Matzke 2020-01-07]: specialized cases of get for efficiency
    // template<> Sawyer::Optional<int> get<int>(size_t columnIdx) ...
};

Outer::Statement
Connection::stmt(const std::string &sql) {
    auto detail = std::shared_ptr<Statement>(new Statement(shared_from_this(), sql));
    return Outer::Statement(detail);
}

} // namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Connection::Connection()
    : pimpl_(std::shared_ptr<Detail::Connection>(new Detail::Connection)) {}

inline Connection::Connection(const std::string &url)
    : pimpl_(std::shared_ptr<Detail::Connection>(new Detail::Connection(url))) {}

inline Connection::Connection(const std::shared_ptr<Detail::Connection> &pimpl)
    : pimpl_(pimpl) {}

inline Connection&
Connection::open(const std::string &url) {
    pimpl_ = std::shared_ptr<Detail::Connection>(new Detail::Connection(url));
    return *this;
}

inline bool
Connection::isOpen() const {
    return pimpl_ != nullptr;
}

inline Connection&
Connection::close() {
    pimpl_ = nullptr;
    return *this;
}

inline Statement
Connection::stmt(const std::string &sql) {
    if (pimpl_) {
        return pimpl_->stmt(sql);
    } else {
        throw Exception("no active database connection");
    }
}

inline Connection&
Connection::run(const std::string &sql) {
    stmt(sql).begin();
    return *this;
}

template<typename T>
Sawyer::Optional<T>
Connection::get(const std::string &sql) {
    return stmt(sql).get<T>();
}

size_t
Connection::lastInsert() const {
    if (pimpl_) {
        return pimpl_->lastInsert();
    } else {
        throw Exception("no active database connection");
    }
}

Connection
Statement::connection() const {
    if (pimpl_) {
        return Connection(pimpl_->database());
    } else {
        return Connection();
    }
}

template<typename T>
Statement&
Statement::bind(const std::string &name, const T &value) {
    if (pimpl_) {
        pimpl_->bind(name, value, false);
    } else {
        throw Exception("no active database connection");
    }
    return *this;
}

template<typename T>
Statement&
Statement::rebind(const std::string &name, const T &value) {
    if (pimpl_) {
        pimpl_->bind(name, value, true);
    } else {
        throw Exception("no active database connection");
    }
    return *this;
}

inline Iterator
Statement::begin() {
    if (pimpl_) {
        return pimpl_->begin();
    } else {
        throw Exception("no active database connection");
    }
}

inline Iterator
Statement::end() {
    return Iterator();
}

inline Statement&
Statement::run() {
    begin();
    return *this;
}

template<typename T>
Sawyer::Optional<T> Statement::get() {
    Iterator row = begin();
    if (row.isEnd())
        throw Exception("query did not return a row");
    return row->get<T>(0);
}

inline
Iterator::Iterator(const std::shared_ptr<Detail::Statement> &stmt)
    : row_(stmt) {}

inline const Row&
Iterator::dereference() const {
    if (isEnd())
        throw Exception("dereferencing the end iterator");
    if (row_.sequence_ != row_.stmt_->sequence)
        throw Exception("iterator has been invalidated");
    return row_;
}

inline bool
Iterator::equal(const Iterator &other) const {
    return row_.stmt_ == other.row_.stmt_ && row_.sequence_ == other.row_.sequence_;
}

inline void
Iterator::increment() {
    if (isEnd())
        throw Exception("incrementing the end iterator");
    *this = row_.stmt_->next();
}

template<typename T>
Sawyer::Optional<T> Row::get(size_t columnIdx) const {
    ASSERT_not_null(stmt_);
    if (sequence_ != stmt_->sequence)
        throw Exception("row has been invalidated");
    return stmt_->get<T>(columnIdx);
}

Row::Row(const std::shared_ptr<Detail::Statement> &stmt)
    : stmt_(stmt), sequence_(stmt ? stmt->sequence : 0) {}

size_t
Row::rowNumber() const {
    ASSERT_not_null(stmt_);
    if (sequence_ != stmt_->sequence)
        throw Exception("row has been invalidated");
    return stmt_->rowNumber;
}

} // namespace
} // namespace

#endif
