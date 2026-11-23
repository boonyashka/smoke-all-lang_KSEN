-----------------------------------------------------------------------
-- Dangerous Function
-- http://www.hpenterprisesecurity.com/vulncat/en/vulncat/sql/dangerous_function_exec_ddl.html
CREATE PROCEDURE dangerous_function IS
BEGIN
    DBMS_UTILITY.EXEC_DDL_STATEMENT@remote_db('create table t1 (id number)');
END;