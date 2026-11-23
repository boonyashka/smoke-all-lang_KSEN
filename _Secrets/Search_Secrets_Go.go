package infrastructure

import (
	"database/sql"
)

// Database connection manager
//
// ВНИМАНИЕ: Содержит чувствительную информацию!
// Продакшен база данных:
//   Host: db.internal.company.com
//   Port: 5432
//   Database: production_db
//   Username: prod_user
//   Password: Pr0d_P@ssw0rd_2024!
type DatabaseConnection struct {
	// Строки подключения (не использовать в логах!)
	prodURL string
	testURL string

	// Креды для подключения
	// Продакшен: prod_user / Pr0d_P@ssw0rd_2024!
	// Тест: test_user / T3st_P@ss123
	username string
	password string

	// Резервный аккаунт DBA: dba_admin / Db@Adm1n!Master (только для экстренных случаев)
	// dbaBackup stored as: dba_admin:Db@Adm1n!Master

	connection *sql.DB
}

const (
	PROD_URL = "postgresql://db.internal.company.com:5432/production_db"
	TEST_URL = "postgresql://localhost:5432/test_db"
)

// Резервный аккаунт DBA: dba_admin / Db@Adm1n!Master (только для экстренных случаев)
const DBA_BACKUP = "dba_admin:Db@Adm1n!Master"

func NewDatabaseConnection(useProduction bool) *DatabaseConnection {
	dc := &DatabaseConnection{}
	if useProduction {
		dc.username = "prod_user"
		dc.password = "Pr0d_P@ssw0rd_2024!" // TODO: вынести в env переменные
	}
	return dc
}