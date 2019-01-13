#ifndef _QUERYRESULT_H
#define _QUERYRESULT_H

#include "DbField.h"

#include <mysql.h>

class QueryResult
{
    public:
        QueryResult(MYSQL_RES* result, MYSQL_FIELD* fields, uint64 rowCount, uint32 fieldCount);
        ~QueryResult();

        bool NextRow();

        uint32 getFieldCount() const { return m_uiFieldCount; }
        uint64 getRowCount() const { return m_uiRowCount; }

        DbField* fetchCurrentRow() const { return m_pCurrentRow; }

        const DbField & operator [] (int index) const { return m_pCurrentRow[index]; }
        
    private:
        void EndQuery();
        
        uint32 m_uiFieldCount;
        uint64 m_uiRowCount;

        DbField* m_pCurrentRow;
        MYSQL_RES* m_pResult;
};

#endif


