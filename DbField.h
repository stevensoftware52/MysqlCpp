#ifndef _DBFIELD_H
#define _DBFIELD_H

#include <string>

typedef signed __int64 int64;
typedef int int32;
typedef short int16;
typedef char int8;

typedef unsigned __int64 uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

class DbField
{
    public:
        DbField();
        DbField(DbField &f);
        DbField(const char* value);
        ~DbField();

        void SetValue(const char* value);
        
        const char* getString() const { return m_pData; }        
        bool getBool() const { return m_pData ? atoi(m_pData) > 0 : false; }        
        float getFloat() const { return m_pData ? static_cast<float>(atof(m_pData)) : 0.0f; }
        double getDouble() const { return m_pData ? static_cast<double>(atof(m_pData)) : 0.0f; }     
        int16 getInt16() const { return m_pData ? static_cast<int16>(atol(m_pData)) : int16(0); }        
        int32 getInt32() const { return m_pData ? static_cast<int32>(atol(m_pData)) : int32(0); }        
        uint8 getUInt8() const { return m_pData ? static_cast<uint8>(atol(m_pData)) : uint8(0); }        
        uint16 getUInt16() const {  return m_pData ? static_cast<uint16>(atol(m_pData)) : uint16(0); }
        uint32 getUInt32() const { return m_pData ? static_cast<uint32>(atol(m_pData)) : uint32(0); }
        
        uint64 getUInt64() const
        {
            if (m_pData)
            {
                uint64 value;
                sscanf(m_pData, "%llu", &value);
                return value;
            }
            
            return 0;
        }

        std::string getCppString() const { return m_pData ? m_pData : ""; }

    private:
        char* m_pData;
};

#endif


