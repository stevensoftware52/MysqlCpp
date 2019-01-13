#include "DbField.h"
#include "Database.h"

DbField::DbField() : 
    m_pData(nullptr)
{

}

DbField::DbField(DbField &f)
{
    const char* value = nullptr;
    value = f.getString();

    if (value && (m_pData = new char[strlen(value) + 1]))
        strcpy(m_pData, value);
    else
        m_pData = nullptr;
}

DbField::DbField(const char* value) 
{
    if (value && (m_pData = new char[strlen(value) + 1]))
        strcpy(m_pData, value);
    else
        m_pData = nullptr;
}

DbField::~DbField()
{
    if (m_pData)
        delete [] m_pData;
}

void DbField::SetValue(const char* value)
{
    if (m_pData)
        delete [] m_pData;

    if (value)
    {
        m_pData = new char[strlen(value) + 1];
        strcpy(m_pData, value);
    }
    else
    {
        m_pData = nullptr;
    }
}


