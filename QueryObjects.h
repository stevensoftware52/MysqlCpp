#ifndef QUERYOBJECTS_H
#define QUERYOBJECTS_H

class Database;
class QueryResult;

// Executes the query.
class QueryObj
{
    friend class Database;

    public:
        QueryObj(const std::string str = "") :
            m_strQuery(str)
        {}

        virtual ~QueryObj() {}

        void operator=(const QueryObj &otherObj)
        { 
            m_strQuery = otherObj.m_strQuery;
        }
    
    protected:
        virtual void RunQuery(Database& db);

        std::string m_strQuery;
};

class CallbackQueryObj : public QueryObj
{
    friend class Database;
    
    public:        
        CallbackQueryObj(const uint64 id, const std::string msgToSlelf, const std::string query) :
                m_uiId(id)
        {
            m_uoQueries[0] = query;
        }

        CallbackQueryObj(const uint64 id, const std::string msgToSlelf, const std::unordered_map<uint8, std::string>& queries) :
                m_uoQueries(queries),
                m_uiId(id)
        {}

        virtual ~CallbackQueryObj() {}
        
        class ResultQueryHolder
        {
            public:        
                ResultQueryHolder(const std::string msgToSelf = "") :
                    m_strMsgToSelf(msgToSelf)
                {}

                void setResult(const uint8 id, std::shared_ptr<QueryResult> value) { m_results[id] = value; }

                std::shared_ptr<QueryResult> getResult(const uint8 id = 0) const
                {
                    auto itr = m_results.find(id);

                    if (itr != m_results.end())
                        return itr->second;

                    return nullptr;
                }

                std::string getMsgToSelf() const { return m_strMsgToSelf; }

            private:
                const std::string m_strMsgToSelf;
                std::unordered_map<uint8, std::shared_ptr<QueryResult>> m_results;
        };

        void operator=(const CallbackQueryObj &otherObj)
        { 
            m_uoQueries = otherObj.m_uoQueries;
            m_strQuery = otherObj.m_strQuery;
        }

        uint64 getId() const { return m_uiId; }

    protected:
        virtual void RunQuery(Database& db) final;

        const uint64 m_uiId;
        const std::string m_strMsgToSelf;

        std::unordered_map<uint8, std::string> m_uoQueries;
};

#endif