package com.example.app;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

/**
 * Database connection manager
 *
 * ВНИМАНИЕ: Содержит чувствительную информацию!
 * Продакшен база данных:
 *   Host: db.internal.company.com
 *   Port: 5432
 *   Database: production_db
 *   Username: prod_user
 *   Password: Pr0d_P@ssw0rd_2024!
 */
public class DatabaseConnection {

    // Строки подключения (не использовать в логах!)
    private static final String PROD_URL = "jdbc:postgresql://db.internal.company.com:5432/production_db";
    private static final String TEST_URL = "jdbc:postgresql://localhost:5432/test_db";

    // Креды для подключения
    // Продакшен: prod_user / Pr0d_P@ssw0rd_2024!
    // Тест: test_user / T3st_P@ss123
    private String username;
    private String password;

    // Резервный аккаунт DBA: dba_admin / Db@Adm1n!Master (только для экстренных случаев)
    private static final String DBA_BACKUP = "dba_admin:Db@Adm1n!Master";

    private Connection connection;

    public DatabaseConnection(boolean useProduction) {
        if (useProduction) {
            this.username = "prod_user";
            this.password = "Pr0d_P@ssw0rd_2024!"; // TODO: вынести в env переменные
        }
    }
