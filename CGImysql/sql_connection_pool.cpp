#include "sql_connection_pool.h"

connection_pool::connection_pool() {
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool *connection_pool::GetInstance() {
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(string url, string User, string PasssWord, string DataBaseName, int Port, int MaxConn, int close_log) {
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PasssWord;
    m_DatabaseName = DataBaseName;
    m_close_log = close_log;

    for(int i=0; i < MaxConn; ++i) {
        MYSQL *conn = NULL;
        conn = mysql_init(conn);

        if(conn == NULL) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn = mysql_real_connect(conn, url.c_str(), User.c_str(), PasssWord.c_str(), DataBaseName.c_str(), Port, NULL, 0);
        
        if(conn == NULL) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        connList.push_back(conn);
        ++m_FreeConn;
    }
    reserve = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}

MYSQL *connection_pool::GetConnection() {
    MYSQL *conn = NULL;
    if(0 == connList.size())
        return NULL;
    reserve.wait();
    lock.lock();

    conn = connList.front();
    connList.pop_front();
    
    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();
    return conn;
}

bool connection_pool::ReleaseConnection(MYSQL *conn) {
    if(NULL == conn)
        return false;
    lock.lock();
    
    connList.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;
    
    lock.unlock();
    reserve.post();
    return true;
}

void connection_pool::DestroyPool() {
    lock.lock();
    if(connList.size() > 0) {
        list<MYSQL*>::iterator it;
        for(it=connList.begin(); it != connList.end(); ++it) {
            MYSQL *conn = *it;
            mysql_close(conn);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

int connection_pool::GetFreeConn() {
    return this->m_FreeConn;
}

connection_pool::~connection_pool() {
    DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool) {
    *SQL = connPool->GetConnection();
    connRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->ReleaseConnection(connRAII);
}