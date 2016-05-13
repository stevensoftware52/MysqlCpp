# MysqlCpp
C++ framework for interacting with MySQL in a multi-threaded application.
My personal coding style is to lower-case functions that are entirely written in a class header.

# Example

```
Database GameDb;    

// Initialize the world database
if (!GameDb.Initialize("host;port;user;pw;dbname"))
{
    printf("GameServer::StartDB - Cannot connect to world database");
    return false;
}

// Example blocking query
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

// If you wan't to issue a non blocking query without concern for the result, you queue it as such.
// The queue is executed in the order it's given queries, but is asynchronous to executions outside of that queue.
GameDb.QueueExecuteQuery("UPDATE table SET name = ''");

// Executes a blocking query without concern for the result.
GameDb.ExecuteQueryInstant("UPDATE table SET );

// If you want to set-up adding many queries to the queue at once with the option to cancel before you've finished adding them all in.
GameDb.BeginManyQueries();

// Would have many calls of 'QueueExecuteQuery' inside SaveAllPlayers().
if (Game::SaveAllPlayers())
{
    GameDb.CommitManyQueries();
}
else
{
    printf("Failed to save all players!");
    GameDb.CancelTransaction();
}
    
// If you want to get data without blocking, you queue up what I called a "Callback" by providing an ID and a query string.
// Then, later, check and process any results.
// GameDb.queueCallbackQuery(GET_PLAYER_DATA_QUERY, "SELECT * FROM players WHERE name = ''");
// GameDb.GrabAndClearCallbackQueries(uoPlaceToPutResults);
// ProcessResults(uoPlaceToPutResults);

// Cleanup
GameDb.Uninitialise();
```
