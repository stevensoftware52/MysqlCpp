#include "Database.h"

#include <ctime>
#include <iostream>
#include <fstream>

// Keep track of how many database connections the 
size_t Database::m_stDatabaseCount = 0;

Database::Database() : 
    m_pMYSQL(nullptr),
    m_bQueriesTransaction(false),
    m_bCancelToken(false),
    m_bInit(false)
{
    
}

Database::~Database()
{
    Uninitialise();
}

bool Database::Uninitialise()
{
    if (!m_bInit)
        return false;

    m_bCancelToken = true;

    // Wait for the work thread to finish.
    m_threadWorker.join();

    if (m_pMYSQL)
        mysql_close(m_pMYSQL);

    // Free MYSQL library pointers for last ~DB
    if (--m_stDatabaseCount == 0)
        mysql_library_end();

    m_pMYSQL = nullptr;
    m_bInit = false;
    return true;
}

bool Database::Initialize(const char* infoString)
{
    m_bInit = true;

    // Before first connection
    if (m_stDatabaseCount++ == 0)
    {
        mysql_library_init(-1, NULL, NULL);

        if (!mysql_thread_safe())
        {
            printf("Database::Initialize - Used MySQL library isn't thread-safe.");
            return false;
        }
    }

    m_threadWorker = std::thread(&Database::WorkerThread, this);

    MYSQL* pMyqlInit = mysql_init(NULL);

    if (!pMyqlInit)
    {
        printf("Database::Initialize - Could not initialize Mysql connection");
        return false;
    }
        
    std::string strHost;
    std::string strPortOrSocket;
    std::string strUser;
    std::string strPassword;
    std::string strDbName;

    std::istringstream ss(infoString);

    if (!std::getline(ss, strHost, ';') ||
        !std::getline(ss, strPortOrSocket, ';') ||
        !std::getline(ss, strUser, ';') ||
        !std::getline(ss, strPassword, ';') ||
        !std::getline(ss, strDbName, ';'))

    {
        printf("Database::Initialize - Bad infoString, format should be 'host;port;user;pw;dbname'.");
        return false;
    }

    mysql_options(pMyqlInit, MYSQL_SET_CHARSET_NAME, "utf8");
    
    int32 port = 0;

    // Named pipe use option (Windows)
    if (strHost == ".") 
    {
        uint32 opt = MYSQL_PROTOCOL_PIPE;
        mysql_options(pMyqlInit, MYSQL_OPT_PROTOCOL, (char const*)&opt);
        port = 0;
    }

    // Generic case
    else
    {
        port = atoi(strPortOrSocket.c_str());
    }

    m_pMYSQL = mysql_real_connect(pMyqlInit, strHost.c_str(), strUser.c_str(), strPassword.c_str(), strDbName.c_str(), port, NULL, 0);

    if (m_pMYSQL)
    {
        mysql_autocommit(m_pMYSQL, 1);

        QueueExecuteQuery("SET NAMES `utf8`");
        QueueExecuteQuery("SET CHARACTER SET `utf8`");

        static uint32 minMysqlVersion = 50003;

        if (MYSQL_VERSION_ID < minMysqlVersion)
        {
            printf("Database::Initialize - Your MySQL is out of date. Your have %d when a minimum of %d is required.", MYSQL_VERSION_ID, minMysqlVersion);
            return false;
        }

        my_bool my_true = (my_bool)1;
        mysql_options(m_pMYSQL, MYSQL_OPT_RECONNECT, &my_true);
        return true;
    }
    else
    {
        printf("Database::Initialize - Could not connect to MySQL database %s at %s\n", strDbName.c_str(),strHost.c_str());
        mysql_close(pMyqlInit);
        return false;
    }
}

void Database::WorkerThread()
{
    // Cycle until m_bCancelToken variable is set to false.
    //  However, we will also wait until we've finished emptying m_queueQueries. 
    //  Anything in that queue expected itself to be finished.

    while (true)
    {
        // New list every loop, don't store outside of scope.
        std::vector<std::shared_ptr<QueryObj>> queries;

        // Grab all pending queries.
        if (m_queueQueries.popAll(queries))
        {
            std::lock_guard<std::mutex> lock(m_mutexMysql);

            // Do every query.
            while (!queries.empty())
            {
                QueryObj* pObj = (*queries.begin()).get();
                pObj->RunQuery(*this);
                queries.erase(queries.begin());
            }
        }
        else
        {
            if (m_bCancelToken)
                break;

            // Don't be a racing thread.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    printf("Database::WorkerThread end.\n");
}

std::shared_ptr<QueryResult> Database::Query(const char* format, ...)
{
    if (!format || !m_pMYSQL)
        return std::shared_ptr<QueryResult>(NULL);

    std::string strQuery;
    FORMAT_STRING_ARGS(format, strQuery, MAX_QUERY_LEN);
    return LockedPerformQuery(strQuery);
}

int32 Database::QueryInt32(const char* format, ...)
{
    if (!format || !m_pMYSQL)
        return 0;

    std::string strQuery;
    FORMAT_STRING_ARGS(format, strQuery, MAX_QUERY_LEN);

    if (std::shared_ptr<QueryResult> result = LockedPerformQuery(strQuery))
    {
        DbField* pFields = result->fetchCurrentRow();
        return pFields[0].getInt32();
    }

    return 0;
}

std::shared_ptr<QueryResult> Database::LockedPerformQuery(const std::string strQuery)
{
    std::lock_guard<std::mutex> lock(m_mutexMysql);
    return PerformQuery(strQuery);    
}

std::shared_ptr<QueryResult> Database::PerformQuery(const std::string strQuery)
{
    ASSERT(m_pMYSQL);
    
    if (!RawMysqlQueryCall(strQuery))
        return nullptr;

    MYSQL_RES* pResult = mysql_store_result(m_pMYSQL);

    if (!pResult)
        return nullptr;

    uint64 uiNumRows = mysql_affected_rows(m_pMYSQL);

    if (!uiNumRows)
    {
        mysql_free_result(pResult);
        return nullptr;
    }

    uint32 uiNumFields = mysql_field_count(m_pMYSQL);

    if (!uiNumFields)
    {
        mysql_free_result(pResult);
        return nullptr;
    }

    return std::make_shared<QueryResult>(pResult, mysql_fetch_fields(pResult), uiNumRows, uiNumFields);
}

void Database::BeginManyQueries()
{
    ASSERT(!m_bQueriesTransaction);
    m_bQueriesTransaction = true;
}

void Database::CommitManyQueries()
{
    std::vector<std::shared_ptr<QueryObj>> result;

    for (size_t i = 0; i < m_vTransactionQueries.size(); ++i)
        result.push_back(std::make_shared<QueryObj>(m_vTransactionQueries[i]));

    // We anticipate that 
    m_queueQueries.pushMany(result);

    m_vTransactionQueries.clear();
    m_bQueriesTransaction = false;
}

void Database::CancelManyQueries()
{
    m_vTransactionQueries.clear();
    m_bQueriesTransaction = false;
}

bool Database::ExecuteQueryInstant(const char* format, ...)
{
    if (!format || !m_pMYSQL)
        return false;
    
    std::string strQuery;
    FORMAT_STRING_ARGS(format, strQuery, MAX_QUERY_LEN);

    std::lock_guard<std::mutex> lock(m_mutexMysql);
    return RawMysqlQueryCall(strQuery, true);
}

bool Database::QueueExecuteQuery(const char*  format,...)
{
    if (!format || !m_pMYSQL)
        return false;
    
    std::string strQuery;
    FORMAT_STRING_ARGS(format, strQuery, MAX_QUERY_LEN);

    if (m_bQueriesTransaction)
    {
        m_vTransactionQueries.push_back(strQuery);
    }
    else
    {
        ASSERT(!strQuery.empty());
        m_queueQueries.push(std::make_shared<QueryObj>(strQuery));
    }

    return true;
}

bool Database::RawMysqlQueryCall(const std::string strQuery, const bool bDeleteGatheredData)
{    
    ASSERT(m_pMYSQL);

    if (mysql_query(m_pMYSQL, strQuery.c_str()))
    {
        printf("SQL Error: '%s'.", mysql_error(m_pMYSQL));
        printf("Query: '%s'.", strQuery.c_str());
        return false;
    }

    if (bDeleteGatheredData)
    {
        if (MYSQL_RES* pResult = mysql_store_result(m_pMYSQL))
            mysql_free_result(pResult);
    }

    return true;
}

void Database::Ping()
{
    QueueExecuteQuery("SELECT 1");
}

void Database::EscapeString(std::string& str)
{
    if (str.empty() || !m_pMYSQL)
        return;

    char strResult[MAX_QUERY_LEN];
    ASSERT(str.size() < MAX_QUERY_LEN);
    
    mysql_real_escape_string(m_pMYSQL, strResult, str.c_str(), str.size());
    
    // Copy result.
    str = strResult;
}

void Database::CallbackResult(const uint64 id, std::shared_ptr<CallbackQueryObj::ResultQueryHolder> result)
{
    std::lock_guard<std::mutex> lock(m_mutexCallbackQueries);
    
    auto itr = m_uoCallbackQueries.find(id);

    if (itr != m_uoCallbackQueries.end())
    {
        printf("Database::CallbackResult - Id already exists, deleting old one.");
        m_uoCallbackQueries.erase(itr);
    }

    m_uoCallbackQueries[id] = result;
}

void Database::GrabAndClearCallbackQueries(std::unordered_map<uint64, std::shared_ptr<CallbackQueryObj::ResultQueryHolder>>& result)
{
    std::lock_guard<std::mutex> lock(m_mutexCallbackQueries);
    result = m_uoCallbackQueries;
    m_uoCallbackQueries.clear();
}