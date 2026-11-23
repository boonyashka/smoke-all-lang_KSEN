CREATE PROCEDURE test_proc 
    @vulnData nvarchar(100) AS
DECLARE 
    @someVar VARCHAR(20),
    @command VARCHAR(100);
    
    SET @someVar = SUBSTRING(@vulnData, 0, 20);
    SET @command = "Select * from DataTable WHERE DataTeble.Text="
    + @someVar;
BEGIN
    EXEC (@command);
END;
GO
