#include "Database.h"

#include <ctime>
#include <iostream>
#include <fstream>

// It's assumed that Database::m_mutexMysql will already be locked in scope when any of these functions are called
//

void QueryObj::RunQuery(Database& db)
{
    // Would be nonsensical for this to be empty.
    ASSERT(!m_strQuery.empty());
    db.RawMysqlQueryCall(m_strQuery, true);
}

void CallbackQueryObj::RunQuery(Database& db)
{
    std::shared_ptr<ResultQueryHolder> result(new ResultQueryHolder(m_strMsgToSelf));
    
    // Would be nonsensical for this to be empty.
    ASSERT(!m_uoQueries.empty());

    for (auto itr = m_uoQueries.begin(); itr != m_uoQueries.end(); ++itr)
        result->setResult(itr->first, db.PerformQuery(itr->second));

    db.CallbackResult(m_uiId, result);
}