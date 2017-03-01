#ifndef GESYMTYPE_H
#define GESYMTYPE_H

/**
 * GESymType stores GAUSS constant values for different symbol types.
 * Access these in a static fashion.
 *
 * Example:
 *
 * #### Python ####
 * ~~~{.py}
def checkType(x):
    return {
        GESymType.SCALAR:
            "Symbol is a scalar.",
        GESymType.SPARSE:
            "Symbol is a sparse matrix.",
        GESymType.MATRIX:
            "Symbol is a matrix.",
        GESymType.STRING:
            "Symbol is a string.",
        GESymType.STRUCT:
            "Symbol is a struct.",
        GESymType.STRING_ARRAY:
            "Symbol is a string array.",
        GESymType.ARRAY_GAUSS:
            "Symbol is an array.",
        GESymType.PROC:
            "Symbol is a proc.",
        GESymType.OTHER:
            "Symbol is of another type."

        }.get(x, "Unknown symbol type")

ge.executeString("x = 1.0")
ge.executeString("s = \"test\"")
ge.executeString("a = areshape(seqa(1, 1, 24), 2|3|4)")

xType = ge.getSymbolType("x")
sType = ge.getSymbolType("s")
aType = ge.getSymbolType("a")

print checkType(xType)
print checkType(sType)
print checkType(aType)
 * ~~~
 * #### PHP ####
 * ~~~{.php}
function checkType($type) {
    switch ($type) {
        case GESymType::SCALAR:
            return "Symbol is a scalar.";
            break;
        case GESymType::SPARSE:
            return "Symbol is a sparse matrix.";
            break;
        case GESymType::MATRIX:
            return "Symbol is a matrix.";
            break;
        case GESymType::STRING:
            return "Symbol is a string.";
            break;
        case GESymType::STRUCT:
            return "Symbol is a struct.";
            break;
        case GESymType::STRING_ARRAY:
            return "Symbol is a string array.";
            break;
        case GESymType::ARRAY_GAUSS:
            return "Symbol is an array.";
            break;
        case GESymType::PROC:
            return "Symbol is a proc.";
            break;
        case GESymType::OTHER:
            return "Symbol is of another type.";
            break;
        default:
            return "Unknown symbol type";
    }
}

$ge->executeString("x = 1.0;");
$ge->executeString("s = \"test\";");
$ge->executeString("a = areshape(seqa(1, 1, 24), 2|3|4);");

$xType = $ge->getSymbolType("x");
$sType = $ge->getSymbolType("s");
$aType = $ge->getSymbolType("a");

echo checkType($xType) . PHP_EOL;
echo checkType($sType) . PHP_EOL;
echo checkType($aType) . PHP_EOL;
 * ~~~
 * results in output:
 * ~~~
Symbol is a matrix.
Symbol is a string.
Symbol is an array.
 * ~~~
 *
 */
struct GESymType
{
public:
    static const int SCALAR = 16;        /**< Scalar (i.e. 1, 1.5); */
    static const int SPARSE = 38;            /**< Sparse Matrix */
    static const int MATRIX = 6;            /**< Matrix */
    static const int STRING = 13;            /**< String */
    static const int STRUCT = 19;            /**< Structure */
    static const int PSTRUCT = 23;            /**< PStruct */
    static const int STRING_ARRAY = 15;  /**< String Array */
    static const int ARRAY_GAUSS = 17;   /**< N-Dimensional Array */
    static const int PROC = 8;            /**< Proc */
    static const int OTHER = 99;            /**< Other */
};

//class GESymType
//{
//    /**
//      * Enumeration of all symbol types known to GAUSS
//      */
//    
//    enum {
//        SCALAR = 16,        /**< Scalar (i.e. 1, 1.5); */
//        SPARSE = 38,            /**< Sparse Matrix */
//        MATRIX = 6,            /**< Matrix */
//        STRING = 13,            /**< String */
//        STRUCT = 19,            /**< Structure */
//        PSTRUCT = 23,            /**< PStruct */
//        STRING_ARRAY = 15,  /**< String Array */
//        ARRAY_GAUSS = 17,   /**< N-Dimensional Array */
//        PROC = 8,            /**< Proc */
//        OTHER = 99            /**< Other */
//    };
//};

#endif // GESYMTYPE_H
