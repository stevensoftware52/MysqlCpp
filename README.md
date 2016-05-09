# MysqlCpp
C++ framework for interacting with MySQL in a multi-threaded aplication

# Example

```
Database GameDb;    

// Initialize the world database
if (!GameDb.Initialize("host;port;user;pw;dbname"))
{
    printf("GameServer::StartDB - Cannot connect to world database");
    return false;
}

// Example query
if (std::shared_ptr<QueryResult> result = GameDb.Query("SELECT entry, name FROM table"))
{
    do
    {
        DbField* pFields = result->fetchCurrentRow();    
        const unsigned int entry = pFields[0].getUInt32();
        const std::string name = pFields[1].getCppString();
    }
    while (result->NextRow());
}

// Cleanup
GameDb.Uninitialise();
```
