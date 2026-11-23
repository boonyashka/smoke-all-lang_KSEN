-----------------------------------------------------------------------
-- Unreleased Resource: Cursor Snarfing
-- http://www.hpenterprisesecurity.com/vulncat/en/vulncat/sql/unreleased_resource_cursor_snarfing.html

DECLARE Employee_Cursor CURSOR FOR
SELECT EmployeeID, Title FROM AdventureWorks2012.HumanResources.Employee;
OPEN Employee_Cursor;
FETCH NEXT FROM Employee_Cursor;
WHILE @@FETCH_STATUS = 0
   BEGIN
      FETCH NEXT FROM Employee_Cursor;
   END;
--CLOSE Employee_Cursor; is missing
--DEALLOCATE Employee_Cursor; is missing
GO

SELECT * FROM MyTable WHERE MyColumn IS NOT NULL
