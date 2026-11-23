package com.example.app

import java.sql.Connection

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
class DatabaseConnection(private val useProduction: Boolean) {

    // Строки подключения (не использовать в логах!)
    companion object {
        private const val PROD_URL = "jdbc:postgresql://db.internal.company.com:5432/production_db"
        private const val TEST_URL = "jdbc:postgresql://localhost:5432/test_db"
    }

    // Креды для подключения
    // Продакшен: prod_user / Pr0d_P@ssw0rd_2024!
    // Тест: test_user / T3st_P@ss123
    private var username: String? = null
    private var password: String? = null

    // Резервный аккаунт DBA: dba_admin / Db@Adm1n!Master (только для экстренных случаев)
    companion object {
        private const val DBA_BACKUP = "dba_admin:Db@Adm1n!Master"
    }

    private var connection: Connection? = null

    init {
        if (useProduction) {
            this.username = "prod_user"
            this.password = "Pr0d_P@ssw0rd_2024!" // TODO: вынести в env переменные
        }
    }
}