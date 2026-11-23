namespace dslClassTest:
    class Factory:
        def PvoClass create():
            return

    class PvoClass:
        def PvoFunc(self, param):
            Detect(param, SQLInjection, SqlCommon)
            
    class PvoInheritor(PvoClass):
        pass

    #class PvoInheritor2(dslParentClassTest.PvoClass):
    #    pass   
        
namespace dslTest:
    def PvoFunc(param):
        Detect(param, SQLInjection, SqlCommon)
       
    def String TaintFunc():
        return Taint
        
    def ReturnTaintTuple():
        return Something, Taint
        
namespace dslFilter:
    def String FilterSqlCommon(String foo):
        return foo.filter(SqlCommon)
        
    def String FilterSQLInjection(String foo):
        return foo.filter(SQLInjection)

namespace github/com/gofrs/uuid:
    def PvoFunc(param):
        Detect(param, SQLInjection, SqlCommon)

namespace dslParentClassTest:
    class PvoClass:
        def PvoFunc(self, param):
            Detect(param, SQLInjection, SqlCommon)