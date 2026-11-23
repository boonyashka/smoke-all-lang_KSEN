-----------------------------------------------------------------------
-- Poor Error Handling: Empty Default Exception Handler
-- http://www.hpenterprisesecurity.com/vulncat/en/vulncat/sql/poor_error_handling_empty_default_exception_handler.html
-- Ignoring an exception can cause the program to overlook unexpected states and conditions.

CREATE PROCEDURE empty_default_exception_handler IS
BEGIN
    INSERT INTO table1 VALUES(1, 2, 3, 4);
    COMMIT;
  EXCEPTION
    WHEN OTHERS THEN NULL; -- empty state block
END;