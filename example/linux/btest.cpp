//g++ -DSQLITE_HAS_CODEC -o nttest nttest.cpp -I . libsqlite3.a -lssl -lcrypto


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

// 测试数据库文件名
#define TEST_DB "test.db"
#define TEST_DB_COPY "test_encrypted_copy.db"
#define PLAINTEXT_DB "test_plaintext.db"

// 测试密钥
#define TEST_KEY "123456789"
#define WRONG_KEY "WrongPassword123"
#define NEW_KEY "MyNewSuperSecretKey456"

// 测试数据量
#define TEST_DATA_COUNT 1000

// 颜色定义
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// 函数声明
int test_basic_operations();
int test_key_management();
int test_error_handling();
int test_performance();
int test_database_conversion();
int test_concurrency();
int test_backup_restore();

// 辅助函数
int execute_sql(sqlite3 *db, const char *sql);
int compare_databases(const char *db1_path, const char *db2_path, const char *key1, const char *key2);
void print_test_result(const char *test_name, int result);
sqlite3* open_database(const char *db_path, const char *key);
void close_database(sqlite3 *db);

int main() {
    printf("=== SQLCipher 测试套件 ===\n\n");
    
    int all_passed = 1;
    int result;
    
    // 测试基本操作
    result = test_basic_operations();
    print_test_result("基本操作测试", result);
    all_passed &= result;
    
	
	return 0;
	
    // 测试密钥管理
    result = test_key_management();
    print_test_result("密钥管理测试", result);
    all_passed &= result;
    
    // 测试错误处理
    result = test_error_handling();
    print_test_result("错误处理测试", result);
    all_passed &= result;
    
    // 测试数据库转换
    result = test_database_conversion();
    print_test_result("数据库转换测试", result);
    all_passed &= result;
    
    // 测试备份恢复
    result = test_backup_restore();
    print_test_result("备份恢复测试", result);
    all_passed &= result;
    
    // 测试性能
    result = test_performance();
    print_test_result("性能测试", result);
    all_passed &= result;
    
    // 测试并发（注意：此测试需要多线程支持）
    // result = test_concurrency();
    // print_test_result("并发测试", result);
    // all_passed &= result;
    
    printf("\n=== 测试完成 ===\n");
    if (all_passed) {
        printf(ANSI_COLOR_GREEN "所有测试通过！\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_RED "部分测试失败！\n" ANSI_COLOR_RESET);
    }
    
    // 清理测试文件
    remove(TEST_DB);
    remove(TEST_DB_COPY);
    remove(PLAINTEXT_DB);
    
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

/**
 * 测试基本数据库操作
 */
int test_basic_operations() {
    printf("\n--- 基本操作测试x %s ---\n", TEST_KEY);
    
    sqlite3 *db = NULL;
    int rc = 0;
    
    // 打开数据库并设置密钥
    db = open_database(TEST_DB, TEST_KEY);
    if (!db) {
        fprintf(stderr, "无法打开数据库\n");
        return 0;
    }
    
    // 创建表
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS users ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "name TEXT NOT NULL,"
                                  "email TEXT NOT NULL UNIQUE,"
                                  "age INTEGER,"
                                  "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)";
    
    rc = execute_sql(db, create_table_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "创建表失败\n");
        close_database(db);
        return 0;
    }
    
    // 插入数据
    const char *insert_sql = "INSERT INTO users (name, email, age) VALUES ('Alice Smith', 'alice@example.com', 30)";
    rc = execute_sql(db, insert_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "插入数据失败\n");
        close_database(db);
        return 0;
    }
    
    // 查询数据
    sqlite3_stmt *stmt;
    const char *query_sql = "SELECT id, name, email, age FROM users WHERE name = 'Alice Smith'";
    rc = sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "查询准备失败: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        const char *email = (const char *)sqlite3_column_text(stmt, 2);
        int age = sqlite3_column_int(stmt, 3);
        
        printf("查询结果: ID=%d, Name=%s, Email=%s, Age=%d\n", id, name, email, age);
        
        // 验证数据
        if (strcmp(name, "Alice Smith") == 0 && strcmp(email, "alice@example.com") == 0 && age == 30) {
            found = 1;
        }
    }
    
    sqlite3_finalize(stmt);
    
    if (!found) {
        fprintf(stderr, "未找到预期的数据\n");
        close_database(db);
        return 0;
    }
    
    // 更新数据
    const char *update_sql = "UPDATE users SET age = 31 WHERE name = 'Alice Smith'";
    rc = execute_sql(db, update_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "更新数据失败\n");
        close_database(db);
        return 0;
    }
    
    // 删除数据
    const char *delete_sql = "DELETE FROM users WHERE name = 'Alice Smith'";
    rc = execute_sql(db, delete_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "删除数据失败\n");
        close_database(db);
        return 0;
    }
    
    // 关闭数据库
    close_database(db);
    
    return 1;
}

/**
 * 测试密钥管理功能
 */
int test_key_management() {
    printf("\n--- 密钥管理测试 ---\n");
    
    sqlite3 *db = NULL;
    int rc = 0;
    
    // 打开数据库并设置初始密钥
    db = open_database(TEST_DB, TEST_KEY);
    if (!db) {
        fprintf(stderr, "无法打开数据库\n");
        return 0;
    }
    
    // 创建测试表
    rc = execute_sql(db, "CREATE TABLE IF NOT EXISTS test_keys (id INTEGER PRIMARY KEY, value TEXT)");
    if (rc != SQLITE_OK) {
        close_database(db);
        return 0;
    }
    
    // 插入测试数据
    rc = execute_sql(db, "INSERT INTO test_keys (value) VALUES ('test data for key change')");
    if (rc != SQLITE_OK) {
        close_database(db);
        return 0;
    }
    
    // 关闭数据库
    close_database(db);
    
    // 重新打开数据库，验证原始密钥有效
    db = open_database(TEST_DB, TEST_KEY);
    if (!db) {
        fprintf(stderr, "使用原始密钥无法打开数据库\n");
        return 0;
    }
    
    // 更改密钥
    char rekey_sql[128];
    snprintf(rekey_sql, sizeof(rekey_sql), "PRAGMA rekey = '%s'", NEW_KEY);
    rc = execute_sql(db, rekey_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "更改密钥失败: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    printf("密钥已从 '%s' 更改为 '%s'\n", TEST_KEY, NEW_KEY);
    
    // 关闭数据库
    close_database(db);
    
    // 验证使用旧密钥无法打开
    sqlite3 *db_old = NULL;
    rc = sqlite3_open(TEST_DB, &db_old);
    if (rc == SQLITE_OK) {
        rc = sqlite3_key(db_old, TEST_KEY, strlen(TEST_KEY));
        if (rc == SQLITE_OK) {
            // 尝试执行查询验证
            char *err_msg = NULL;
            rc = sqlite3_exec(db_old, "SELECT * FROM test_keys", NULL, NULL, &err_msg);
            if (rc == SQLITE_OK) {
                fprintf(stderr, "错误：使用旧密钥仍能访问数据库\n");
                sqlite3_close(db_old);
                return 0;
            }
            sqlite3_free(err_msg);
        }
        sqlite3_close(db_old);
    }
    
    // 验证使用新密钥可以打开
    db = open_database(TEST_DB, NEW_KEY);
    if (!db) {
        fprintf(stderr, "使用新密钥无法打开数据库\n");
        return 0;
    }
    
    // 验证数据仍然存在
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT value FROM test_keys", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "查询准备失败: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *value = (const char *)sqlite3_column_text(stmt, 0);
        if (strcmp(value, "test data for key change") == 0) {
            found = 1;
        }
    }
    
    sqlite3_finalize(stmt);
    close_database(db);
    
    if (!found) {
        fprintf(stderr, "更改密钥后数据丢失\n");
        return 0;
    }
    
    printf("密钥管理测试通过\n");
    return 1;
}

/**
 * 测试错误处理功能
 */
int test_error_handling() {
    printf("\n--- 错误处理测试 ---\n");
    
    sqlite3 *db = NULL;
    int rc = 0;
    
    // 测试1: 使用错误的密钥打开数据库
    rc = sqlite3_open(TEST_DB, &db);
    if (rc == SQLITE_OK) {
        rc = sqlite3_key(db, WRONG_KEY, strlen(WRONG_KEY));
        if (rc == SQLITE_OK) {
            // 尝试执行查询
            char *err_msg = NULL;
            rc = sqlite3_exec(db, "SELECT * FROM users", NULL, NULL, &err_msg);
            if (rc == SQLITE_OK) {
                fprintf(stderr, "错误：使用错误密钥仍能访问数据库\n");
                sqlite3_free(err_msg);
                sqlite3_close(db);
                return 0;
            }
            printf("错误处理测试1通过：使用错误密钥得到预期错误\n");
            sqlite3_free(err_msg);
        }
        sqlite3_close(db);
    }
    
    // 测试2: 数据库文件损坏
    FILE *f = fopen(TEST_DB, "ab");
    if (f) {
        // 向文件末尾添加一些垃圾数据
        const char *garbage = "this is garbage data to corrupt the database";
        fwrite(garbage, 1, strlen(garbage), f);
        fclose(f);
        
        // 尝试打开损坏的数据库
        rc = sqlite3_open(TEST_DB, &db);
        if (rc == SQLITE_OK) {
            rc = sqlite3_key(db, TEST_KEY, strlen(TEST_KEY));
            if (rc == SQLITE_OK) {
                char *err_msg = NULL;
                rc = sqlite3_exec(db, "SELECT * FROM users", NULL, NULL, &err_msg);
                if (rc == SQLITE_OK) {
                    fprintf(stderr, "错误：损坏的数据库仍能正常访问\n");
                    sqlite3_free(err_msg);
                    sqlite3_close(db);
                    return 0;
                }
                printf("错误处理测试2通过：损坏的数据库得到预期错误\n");
                sqlite3_free(err_msg);
            }
            sqlite3_close(db);
        }
    }
    
    // 测试3: 执行无效的SQL语句
    db = open_database(TEST_DB, TEST_KEY);
    if (db) {
        char *err_msg = NULL;
        rc = sqlite3_exec(db, "INVALID SQL STATEMENT", NULL, NULL, &err_msg);
        if (rc == SQLITE_OK) {
            fprintf(stderr, "错误：无效的SQL语句未报错\n");
            sqlite3_free(err_msg);
            sqlite3_close(db);
            return 0;
        }
        printf("错误处理测试3通过：无效SQL语句得到预期错误\n");
        sqlite3_free(err_msg);
        sqlite3_close(db);
    }
    
    printf("错误处理测试通过\n");
    return 1;
}

/**
 * 测试数据库转换功能（明文 <-> 加密）
 */
int test_database_conversion() {
    printf("\n--- 数据库转换测试 ---\n");
    
    sqlite3 *db_plain = NULL;
    sqlite3 *db_encrypted = NULL;
    int rc = 0;
    
    // 创建明文数据库
    rc = sqlite3_open(PLAINTEXT_DB, &db_plain);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法创建明文数据库: %s\n", sqlite3_errmsg(db_plain));
        return 0;
    }
    
    // 在明文数据库中创建表和数据
    rc = execute_sql(db_plain, "CREATE TABLE IF NOT EXISTS plain_data (id INTEGER PRIMARY KEY, value TEXT)");
    if (rc != SQLITE_OK) {
        sqlite3_close(db_plain);
        return 0;
    }
    
    rc = execute_sql(db_plain, "INSERT INTO plain_data (value) VALUES ('plain text data')");
    if (rc != SQLITE_OK) {
        sqlite3_close(db_plain);
        return 0;
    }
    
    sqlite3_close(db_plain);
    
    // 将明文数据库转换为加密数据库
    rc = sqlite3_open(TEST_DB, &db_encrypted);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法创建加密数据库: %s\n", sqlite3_errmsg(db_encrypted));
        return 0;
    }
    
    // 设置加密密钥
    rc = sqlite3_key(db_encrypted, TEST_KEY, strlen(TEST_KEY));
    if (rc != SQLITE_OK) {
        fprintf(stderr, "设置密钥失败: %s\n", sqlite3_errmsg(db_encrypted));
        sqlite3_close(db_encrypted);
        return 0;
    }
    
    // 附加明文数据库
    char attach_sql[128];
    snprintf(attach_sql, sizeof(attach_sql), "ATTACH DATABASE '%s' AS plaintext KEY ''", PLAINTEXT_DB);
    rc = execute_sql(db_encrypted, attach_sql);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "附加明文数据库失败: %s\n", sqlite3_errmsg(db_encrypted));
        sqlite3_close(db_encrypted);
        return 0;
    }
    
    // 导出数据到加密数据库
    rc = execute_sql(db_encrypted, "SELECT sqlcipher_export('main', 'plaintext')");
    if (rc != SQLITE_OK) {
        fprintf(stderr, "导出数据失败: %s\n", sqlite3_errmsg(db_encrypted));
        sqlite3_close(db_encrypted);
        return 0;
    }
    
    // 分离明文数据库
    rc = execute_sql(db_encrypted, "DETACH DATABASE plaintext");
    if (rc != SQLITE_OK) {
        fprintf(stderr, "分离数据库失败: %s\n", sqlite3_errmsg(db_encrypted));
        sqlite3_close(db_encrypted);
        return 0;
    }
    
    sqlite3_close(db_encrypted);
    
    // 验证加密数据库
    db_encrypted = open_database(TEST_DB, TEST_KEY);
    if (!db_encrypted) {
        fprintf(stderr, "无法打开加密数据库\n");
        return 0;
    }
    
    // 验证数据
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db_encrypted, "SELECT value FROM plain_data", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "查询准备失败: %s\n", sqlite3_errmsg(db_encrypted));
        sqlite3_close(db_encrypted);
        return 0;
    }
    
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *value = (const char *)sqlite3_column_text(stmt, 0);
        if (strcmp(value, "plain text data") == 0) {
            found = 1;
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db_encrypted);
    
    if (!found) {
        fprintf(stderr, "转换后数据丢失\n");
        return 0;
    }
    
    printf("数据库转换测试通过\n");
    return 1;
}

/**
 * 测试备份和恢复功能
 */
int test_backup_restore() {
    printf("\n--- 备份恢复测试 ---\n");
    
    sqlite3 *db = NULL;
    sqlite3 *backup_db = NULL;
    int rc = 0;
    
    // 创建源数据库并添加测试数据
    db = open_database(TEST_DB, TEST_KEY);
    if (!db) {
        fprintf(stderr, "无法打开源数据库\n");
        return 0;
    }
    
    rc = execute_sql(db, "CREATE TABLE IF NOT EXISTS backup_test (id INTEGER PRIMARY KEY, data TEXT)");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    rc = execute_sql(db, "INSERT INTO backup_test (data) VALUES ('backup test data')");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    // 创建备份数据库
    rc = sqlite3_open(TEST_DB_COPY, &backup_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法创建备份数据库: %s\n", sqlite3_errmsg(backup_db));
        sqlite3_close(db);
        return 0;
    }
    
    // 设置备份数据库的密钥
    rc = sqlite3_key(backup_db, TEST_KEY, strlen(TEST_KEY));
    if (rc != SQLITE_OK) {
        fprintf(stderr, "设置备份数据库密钥失败: %s\n", sqlite3_errmsg(backup_db));
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    // 执行备份
    sqlite3_backup *backup = sqlite3_backup_init(backup_db, "main", db, "main");
    if (!backup) {
        fprintf(stderr, "备份初始化失败: %s\n", sqlite3_errmsg(backup_db));
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    rc = sqlite3_backup_step(backup, -1);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "备份执行失败: %d\n", rc);
        sqlite3_backup_finish(backup);
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    rc = sqlite3_backup_finish(backup);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "备份完成失败: %d\n", rc);
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    // 关闭数据库
    sqlite3_close(db);
    sqlite3_close(backup_db);
    
    // 验证备份
    db = open_database(TEST_DB, TEST_KEY);
    backup_db = open_database(TEST_DB_COPY, TEST_KEY);
    
    if (!db || !backup_db) {
        fprintf(stderr, "无法打开数据库进行验证\n");
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    // 比较两个数据库的内容
    if (compare_databases(TEST_DB, TEST_DB_COPY, TEST_KEY, TEST_KEY)) {
        printf("备份恢复测试通过\n");
    } else {
        fprintf(stderr, "备份与源数据库不一致\n");
        sqlite3_close(db);
        sqlite3_close(backup_db);
        return 0;
    }
    
    sqlite3_close(db);
    sqlite3_close(backup_db);
    
    return 1;
}

/**
 * 测试性能
 */
int test_performance() {
    printf("\n--- 性能测试 ---\n");
    
    sqlite3 *db = NULL;
    int rc = 0;
    clock_t start, end;
    double time_taken;
    
    // 打开数据库
    db = open_database(TEST_DB, TEST_KEY);
    if (!db) {
        fprintf(stderr, "无法打开数据库\n");
        return 0;
    }
    
    // 创建测试表
    rc = execute_sql(db, "CREATE TABLE IF NOT EXISTS performance_test ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "data TEXT NOT NULL,"
                      "value INTEGER NOT NULL)");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    // 测试插入性能
    start = clock();
    
    // 使用事务批量插入
    rc = execute_sql(db, "BEGIN TRANSACTION");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    char insert_sql[128];
    for (int i = 0; i < TEST_DATA_COUNT; i++) {
        snprintf(insert_sql, sizeof(insert_sql), 
                "INSERT INTO performance_test (data, value) VALUES ('test data %d', %d)", 
                i, i * 2);
        rc = execute_sql(db, insert_sql);
        if (rc != SQLITE_OK) {
            execute_sql(db, "ROLLBACK");
            sqlite3_close(db);
            return 0;
        }
    }
    
    rc = execute_sql(db, "COMMIT");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("插入 %d 条记录耗时: %.3f 秒\n", TEST_DATA_COUNT, time_taken);
    
    // 测试查询性能
    start = clock();
    
    sqlite3_stmt *stmt;
    const char *query_sql = "SELECT id, data, value FROM performance_test WHERE value > ?";
    rc = sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "查询准备失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    
    // 绑定参数
    int param_value = TEST_DATA_COUNT / 2;
    rc = sqlite3_bind_int(stmt, 1, param_value);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "参数绑定失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    // 执行查询
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    
    sqlite3_finalize(stmt);
    
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("查询 %d 条记录耗时: %.3f 秒\n", count, time_taken);
    
    // 测试更新性能
    start = clock();
    
    rc = execute_sql(db, "BEGIN TRANSACTION");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    rc = execute_sql(db, "UPDATE performance_test SET value = value * 2 WHERE id % 2 = 0");
    if (rc != SQLITE_OK) {
        execute_sql(db, "ROLLBACK");
        sqlite3_close(db);
        return 0;
    }
    
    rc = execute_sql(db, "COMMIT");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("更新记录耗时: %.3f 秒\n", time_taken);
    
    // 测试删除性能
    start = clock();
    
    rc = execute_sql(db, "DELETE FROM performance_test WHERE id % 3 = 0");
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("删除记录耗时: %.3f 秒\n", time_taken);
    
    // 关闭数据库
    sqlite3_close(db);
    
    printf("性能测试完成\n");
    return 1;
}

/**
 * 测试并发访问（需要多线程支持）
 */
int test_concurrency() {
    printf("\n--- 并发测试 ---\n");
    printf("注意：此测试需要多线程支持，当前未实现\n");
    return 1;
}

/**
 * 执行SQL语句
 */
int execute_sql(sqlite3 *db, const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL执行失败: %s\nSQL语句: %s\n", err_msg, sql);
        sqlite3_free(err_msg);
    }
    
    return rc;
}

/**
 * 比较两个数据库的内容
 */
int compare_databases(const char *db1_path, const char *db2_path, const char *key1, const char *key2) {
    sqlite3 *db1 = open_database(db1_path, key1);
    sqlite3 *db2 = open_database(db2_path, key2);
    
    if (!db1 || !db2) {
        fprintf(stderr, "无法打开数据库进行比较\n");
        sqlite3_close(db1);
        sqlite3_close(db2);
        return 0;
    }
    
    // 获取表列表
    sqlite3_stmt *stmt;
    const char *sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
    
    if (sqlite3_prepare_v2(db1, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "获取表列表失败: %s\n", sqlite3_errmsg(db1));
        sqlite3_close(db1);
        sqlite3_close(db2);
        return 0;
    }
    
    int tables_match = 1;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *table_name = (const char *)sqlite3_column_text(stmt, 0);
        
        // 检查表是否存在于第二个数据库
        sqlite3_stmt *stmt2;
        char check_sql[128];
        snprintf(check_sql, sizeof(check_sql), 
                "SELECT name FROM sqlite_master WHERE type='table' AND name='%s'", 
                table_name);
        
        if (sqlite3_prepare_v2(db2, check_sql, -1, &stmt2, NULL) != SQLITE_OK) {
            fprintf(stderr, "检查表存在失败: %s\n", sqlite3_errmsg(db2));
            tables_match = 0;
            break;
        }
        
        if (sqlite3_step(stmt2) != SQLITE_ROW) {
            fprintf(stderr, "表 %s 在第二个数据库中不存在\n", table_name);
            tables_match = 0;
            sqlite3_finalize(stmt2);
            break;
        }
        
        sqlite3_finalize(stmt2);
        
        // 比较表内容
        char count_sql[128];
        snprintf(count_sql, sizeof(count_sql), "SELECT COUNT(*) FROM %s", table_name);
        
        sqlite3_stmt *count_stmt1, *count_stmt2;
        
        if (sqlite3_prepare_v2(db1, count_sql, -1, &count_stmt1, NULL) != SQLITE_OK ||
            sqlite3_prepare_v2(db2, count_sql, -1, &count_stmt2, NULL) != SQLITE_OK) {
            fprintf(stderr, "准备计数查询失败\n");
            tables_match = 0;
            break;
        }
        
        int count1 = 0, count2 = 0;
        
        if (sqlite3_step(count_stmt1) == SQLITE_ROW) {
            count1 = sqlite3_column_int(count_stmt1, 0);
        }
        
        if (sqlite3_step(count_stmt2) == SQLITE_ROW) {
            count2 = sqlite3_column_int(count_stmt2, 0);
        }
        
        sqlite3_finalize(count_stmt1);
        sqlite3_finalize(count_stmt2);
        
        if (count1 != count2) {
            fprintf(stderr, "表 %s 记录数不匹配: %d vs %d\n", table_name, count1, count2);
            tables_match = 0;
            break;
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db1);
    sqlite3_close(db2);
    
    return tables_match;
}

/**
 * 打印测试结果
 */
void print_test_result(const char *test_name, int result) {
    if (result) {
        printf(ANSI_COLOR_GREEN "[PASS] %s\n" ANSI_COLOR_RESET, test_name);
    } else {
        printf(ANSI_COLOR_RED "[FAIL] %s\n" ANSI_COLOR_RESET, test_name);
    }
}

/**
 * 打开数据库并设置密钥
 */
sqlite3* open_database(const char *db_path, const char *key) {
    sqlite3 *db = NULL;
    int rc = sqlite3_open(db_path, &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库 %s: %s\n", db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    
    // 设置密钥
    rc = sqlite3_key(db, key, strlen(key));
    if (rc != SQLITE_OK) {
        fprintf(stderr, "设置密钥失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    
    return db;
}

/**
 * 关闭数据库
 */
void close_database(sqlite3 *db) {
    if (db) {
        sqlite3_close(db);
    }
}
