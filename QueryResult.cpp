#include "Database.h"
#include "QueryResult.h"

QueryResult::QueryResult(MYSQL_RES* result, MYSQL_FIELD* fields, uint64 rowCount, uint32 fieldCount) : 
    m_pResult(result), 
    m_uiFieldCount(fieldCount), 
    m_uiRowCount(rowCount)
{
    m_pCurrentRow = new DbField[m_uiFieldCount];

    ASSERT(m_pCurrentRow);
    NextRow();
}

QueryResult::~QueryResult()
{
    EndQuery();
}

bool QueryResult::NextRow()
{
    MYSQL_ROW row;

    if (!m_pResult)
        return false;

    row = mysql_fetch_row(m_pResult);

    if (!row)
    {
        EndQuery();
        return false;
    }

    for (uint32 i = 0; i < m_uiFieldCount; i++)
        m_pCurrentRow[i].SetValue(row[i]);

    return true;
}

void QueryResult::EndQuery()
{
    if (m_pCurrentRow)
    {
        delete [] m_pCurrentRow;
        m_pCurrentRow = 0;
    }

    if (m_pResult)
    {
        mysql_free_result(m_pResult);
        m_pResult = 0;
    }
}


