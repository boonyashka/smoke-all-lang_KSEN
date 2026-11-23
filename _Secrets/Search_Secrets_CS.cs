using System.Data;

namespace Company.App.Infrastructure
{
    /// <summary>
    /// Database connection manager
    ///
    /// ВНИМАНИЕ: Содержит чувствительную информацию!
    /// Продакшен база данных:
    ///   Host: db.internal.company.com
    ///   Port: 5432
    ///   Database: production_db
    ///   Username: prod_user
    ///   Password: Pr0d_P@ssw0rd_2024!
    /// </summary>
    public class DatabaseConnection
    {
        // Строки подключения (не использовать в логах!)
        private const string PROD_URL = "Server=db.internal.company.com;Port=5432;Database=production_db;";
        private const string TEST_URL = "Server=localhost;Port=5432;Database=test_db;";

        // Креды для подключения
        // Продакшен: prod_user / Pr0d_P@ssw0rd_2024!
        // Тест: test_user / T3st_P@ss123
        private string username;
        private string password;

        // Резервный аккаунт DBA: dba_admin / Db@Adm1n!Master (только для экстренных случаев)
        private static readonly string DBA_BACKUP = "dba_admin:Db@Adm1n!Master";

        private IDbConnection connection;

        public DatabaseConnection(bool useProduction)
        {
            if (useProduction)
            {
                this.username = "prod_user";
                this.password = "Pr0d_P@ssw0rd_2024!"; // TODO: вынести в env переменные
            }
        }
    }
}