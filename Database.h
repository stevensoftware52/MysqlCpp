/*
** gummy52
*/

#ifndef DATABASE_H
#define DATABASE_H

#include "SafeQueue.h"
#include "QueryResult.h"
#include "QueryObjects.h"

#include <mysql.h>
#include <unordered_map>
#include <thread>

#define MAX_QUERY_LEN 8192

#define _LIKE_           "LIKE"
#define _TABLE_SIM_      "`"
#define _CONCAT3_(A,B,C) "CONCAT( " A " , " B " , " C " )"
#define _OFFSET_         "LIMIT %d,1"

// Callback results are in the same queue as QueueExecuteQuery and CommitManyQueries
// ::Query and ::ExecuteQueryInstant are asynchronous with m_queueQueries
class Database
{
    friend class QueryObj;
    friend class CallbackQueryObj;

    public:
        Database();
        ~Database();        
        
        void Ping();
        void EscapeString(std::string& str);
        void GrabAndClearCallbackQueries(std::unordered_map<uint64, std::shared_ptr<CallbackQueryObj::ResultQueryHolder>>& result);
        
        void BeginManyQueries();
        void CommitManyQueries();
        void CancelManyQueries();
        
        void queueCallbackQuery(const uint64 id, const std::unordered_map<uint8, std::string> queries, const std::string msgToSelf = "") 
        { 
            m_queueQueries.push(std::shared_ptr<CallbackQueryObj>(new CallbackQueryObj(id, msgToSelf, queries)));
        }

        void queueCallbackQuery(const uint64 id, const std::string query, const std::string msgToSelf = "") 
        { 
            m_queueQueries.push(std::shared_ptr<CallbackQueryObj>(new CallbackQueryObj(id, msgToSelf, query)));
        }

        bool Uninitialise();
        bool Initialize(const char* infoString);
        bool QueueExecuteQuery(const char* format, ...) ATTR_PRINTF(2, 3);
        bool ExecuteQueryInstant(const char* format, ...) ATTR_PRINTF(2, 3);    
        
        int32 QueryInt32(const char* format, ...);

        std::shared_ptr<QueryResult> Query(const char* format, ...) ATTR_PRINTF(2, 3);

        operator bool () const { return m_pMYSQL != NULL; }
        
    private:        
        void WorkerThread();
        void CallbackResult(const uint64 id, std::shared_ptr<CallbackQueryObj::ResultQueryHolder> result);

        // Returns true if success, false if fail.
        bool RawMysqlQueryCall(const std::string strQuery, const bool bDeleteGatheredData = false);

        std::shared_ptr<QueryResult> LockedPerformQuery(const std::string strQuery);
        std::shared_ptr<QueryResult> PerformQuery(const std::string strQuery);
        
        MYSQL* m_pMYSQL;

        // When true, the queue thread ends.
        bool m_bCancelToken;
        bool m_bInit;

        // When true, execute queries get added to m_vTransactionQueries.
        bool m_bQueriesTransaction;

        static size_t m_stDatabaseCount;
        
        std::mutex m_mutexMysql;
        std::mutex m_mutexCallbackQueries;
        std::thread m_threadWorker;
                
        SafeQueue<std::shared_ptr<QueryObj>> m_queueQueries;

        // Begin -> Commit, a way to do a bunch of queries at the same time without waiting in queue.
        std::vector<std::string> m_vTransactionQueries;

        // The results of queued queries with callbacks.
        std::unordered_map<uint64, std::shared_ptr<CallbackQueryObj::ResultQueryHolder>> m_uoCallbackQueries;
};

#endif


