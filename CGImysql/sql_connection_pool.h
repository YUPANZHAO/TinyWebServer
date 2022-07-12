#ifndef _SQL_CONNECTION_POOL_H_
#define _SQL_CONNECTION_POOL_H_

#include <string>
#include "../lock/locker.h"
#include <list>
#include "../log/log.h"
#include <mysql/mysql.h>

using namespace std;

class connection_pool {
public:
    MYSQL *GetConnection();                 // 获取数据库连接
    bool ReleaseConnection(MYSQL *conn);    // 释放连接
    int GetFreeConn();                      // 获取连接
    void DestroyPool();                     // 销毁所有连接

    // 单例模式
    static connection_pool *GetInstance();
    
    void init(string url, string User, string PasssWord, string DataBaseName, int Port, int MaxConn, int close_log);

private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn;  // 最大连接数
    int m_CurConn;  // 当前已使用连接数
    int m_FreeConn; // 当前空闲的连接数
    locker lock;
    list<MYSQL*> connList; // 连接池
    sem reserve;

public:
    string m_url;   // 主机地址
    string m_Port;  // 数据库端口号
    string m_User;  // 数据库用户名
    string m_PassWord; // 密码
    string m_DatabaseName; // 数据库名
    int m_close_log;// 日志开关
};

// 保管从连接池中获取的连接，自动归还资源
class connectionRAII {
public:
    connectionRAII(MYSQL **conn, connection_pool *connPool);
    ~connectionRAII();
private:
    MYSQL *connRAII;
    connection_pool *poolRAII;
};

#endif