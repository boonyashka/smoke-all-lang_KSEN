
class OsCommanding {
    fun test(cmd: String): String {
        Runtime.getRuntime().exec(cmd)
    }
}

