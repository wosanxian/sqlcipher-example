//g++ -DSQLITE_HAS_CODEC -o nttest nttest.cpp -I . libsqlite3.a -lssl -lcrypto

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <string>

#define TEST_DB  "test.db"
#define TEST_KEY "123456789"

sqlite3* open_database(const char *db_path, const char *key) 
{
    sqlite3 *db = NULL;
    int rc = sqlite3_open(db_path, &db);

    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "无法打开数据库 %s: %s\n", db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    rc = sqlite3_key(db, key, strlen(key));
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "设置密钥失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    return db;
}

void close_database(sqlite3 *db) 
{
    if (db) 
    {
        sqlite3_close(db);
    }
}

int execute_sql(sqlite3 *db, const char *sql) 
{
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL执行失败: %s\nSQL语句: %s\n", err_msg, sql);
        sqlite3_free(err_msg);
    }
    
    return rc;
}

int test_basic_operations() 
{
    sqlite3 * db = open_database(TEST_DB, TEST_KEY);
    if (!db) 
    {
        fprintf(stderr, "无法打开数据库\n");
        return 0;
    }

    // 创建表
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS msg ("
                                  "id interger PRIMARY KEY, name text NOT NULL, age interger, school text)";

    if (execute_sql(db, create_table_sql) != SQLITE_OK) 
    {
        fprintf(stderr, "创建表失败\n");
        close_database(db);
        return 0;
    }

    // 插入数据
    const char *insert_sql = "INSERT INTO msg(id, name, age, school) VALUES(20250102, 'lishi', 19, 'pku')";
    if (execute_sql(db, insert_sql) != SQLITE_OK) 
    {
        fprintf(stderr, "插入数据失败\n");
        close_database(db);
        return 0;
    }

    // 查询数据
    sqlite3_stmt *stmt;
    const char *query_sql = "SELECT id, name, age, school FROM msg";
    if (sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL) != SQLITE_OK) 
    {
        fprintf(stderr, "查询准备失败: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        int age = sqlite3_column_int(stmt, 2);
        const char *school = (const char *)sqlite3_column_text(stmt, 3);

        printf("查询结果: ID=%d, Name=%s, Age=%d, School=%s\n", id, name, age, school);
    }

    sqlite3_finalize(stmt);

    // 关闭数据库
    close_database(db);

    return 1;
}

int main() 
{
    test_basic_operations();

    return 0;
}
