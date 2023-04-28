from __future__ import print_function
import unittest
from ge import *

# Unit Test to Check functionality

class Output(IGEProgramOutput):
    def __init__(self):
        super(Output, self).__init__()
        print("Creating output class")

    def invoke(self, output):
        print(output, end='')

    def __del__(self):
        print("Deleting output class")

class FlushOutput(IGEProgramFlushOutput):
    def __init__(self):
        super(FlushOutput, self).__init__()

    def invoke(self):
        print("A flush was requested.")

class StringInput(IGEProgramInputString):
    def __init__(self):
        super(StringInput, self).__init__()

    # length is the maximum accepted string length
    def invoke(self, length):
        self.setValue("Hello World!")

class CharInput(IGEProgramInputChar):
    def __init__(self):
        super(CharInput, self).__init__()

    def invoke(self):
        return ord('a')

class InputCheck(IGEProgramInputCheck):
    def __init__(self):
        super(InputCheck, self).__init__()

    def invoke(self):
        # We pretend we have character input available.
        print("Input check requested")
        return 1

eng = GAUSS()
eng.setOutputModeManaged(False)

#thisown = 0

# Set output callback
out = Output().__disown__()
eng.setProgramOutputAll(out)

# Set output flush callback
flush = FlushOutput().__disown__()

eng.setProgramFlushOutput(flush)

# Set string input callback
inString = StringInput().__disown__()

eng.setProgramInputString(inString)

# Set character input callback
inChar = CharInput().__disown__()

eng.setProgramInputCharBlocking(inChar)
eng.setProgramInputChar(inChar)

# Set character input check callback
inputCheckCallback = InputCheck().__disown__()
#inputCheckCallback.thisown = 0;
eng.setProgramInputCheck(inputCheckCallback)

eng.initialize()

class TestGAUSSEngine(unittest.TestCase):
    ge = eng

#    def setUp(self):
#        self.ge = GAUSS()
#        self.ge.setOutputModeManaged(False)
#
#        self.assertTrue(self.ge is not None)
#
#        #thisown = 0
#
#        # Set output callback
#        self.out = Output() #.__disown__()
#        self.ge.setProgramOutputAll(self.out)
#
#        # Set output flush callback
#        flush = FlushOutput().__disown__()
#        
#        self.ge.setProgramFlushOutput(flush)
#
#        # Set string input callback
#        inString = StringInput().__disown__()
#        
#        self.ge.setProgramInputString(inString)
#
#        # Set character input callback
#        inChar = CharInput().__disown__()
#        
#        self.ge.setProgramInputCharBlocking(inChar)
#        self.ge.setProgramInputChar(inChar)
#
#        # Set character input check callback
#        inputCheckCallback = InputCheck().__disown__()
#        #inputCheckCallback.thisown = 0;
#        self.ge.setProgramInputCheck(inputCheckCallback)
#
#        self.assertTrue(self.ge.initialize())

    def testWorkspace(self):
        wh1 = self.ge.createWorkspace("wh1")
        wh11 = self.ge.createWorkspace("wh1")
        wh111 = self.ge.createWorkspace("wh1")
        wh1111 = self.ge.createWorkspace("wh1")
        self.assertEqual("wh1", wh1.name())
        self.assertTrue(self.ge.destroyWorkspace(wh1))
        self.assertEqual(None, wh1.workspace())

        self.assertFalse(self.ge.destroyWorkspace(wh1))

        wh2 = self.ge.createWorkspace("wh2")
        self.ge.setActiveWorkspace(wh2)
        self.assertEqual(wh2.name(), self.ge.getActiveWorkspace().name())

        wh3 = self.ge.createWorkspace("wh3")
        wh4 = self.ge.createWorkspace("wh4")

        wh2.setName("wh222")
        self.ge.updateWorkspaceName(wh2)
        self.assertEqual("wh222", wh2.name())

        count = 1000

        for i in range(count):
            t = self.ge.createWorkspace("temp{}".format(i))
            #self.ge.executeString("print \"my random number: \" rndu(1,1) \" for \" {}".format(i), t)

        for i in range(count):
            self.ge.destroyWorkspace(self.ge.getWorkspace("temp{}".format(i)))


    def testMatrices(self):
        self.ge.executeString("x = 5")
        x = self.ge.getMatrix("x")
        self.assertEqual(5, x.getElement())
        self.assertEqual(5, x[0])

#        try:
#            print("x[1] == {}".format(x[1]))
#            self.assertTrue(False)
#        except IndexError:
#            self.assertTrue(True)
#        except Exception:
#            self.assertTrue(False)

        self.ge.executeString("x = { 1 2, 3 4 }")
        x = self.ge.getMatrixAndClear("x")
        self.assertEqual(2, x.getRows())
        self.assertEqual(2, x.getCols())
        self.assertEqual([1, 2, 3, 4], list(x.getData()))
        self.assertEqual(4, len(x))

        for i in range(1, 5):
            self.assertEqual(i, x[i - 1])
            self.assertEqual(5 - i, x[-i])

        x2 = self.ge.getMatrix("x")
        self.assertEqual(1, x2.getRows())
        self.assertEqual(1, x2.getCols())
        self.assertEqual(0, x2.getElement())

        self.ge.executeString("x = complex(seqa(1,1,8), seqa(9,1,8))")
        x = self.ge.getMatrix("x")
        self.assertNotEquals(None, x)
        self.assertEqual(16, len(x.getData(True)))
        self.assertEqual(8, len(x))

        self.assertEqual([1, 2, 3, 4, 5, 6, 7, 8], list(x.getData()))

        x_d = self.ge.getMatrixDirect("x")

        num = 1

        for i in x_d:
            self.assertEqual(num, i)
            num += 1

        num = 1
        for i in x:
            self.assertEqual(num, i)
            num += 1

        self.assertEqual([9, 10, 11, 12, 13, 14, 15, 16], list(x.getImagData()))

        self.ge.setSymbol(GEMatrix(range(0, 10)), "y")
        y = self.ge.getMatrix("y")
        self.assertEqual([float(i) for i in range(0, 10)], list(y.getData()))

        y = self.ge["y"]
        self.assertEqual([float(i) for i in range(0, 10)], list(y.getData()))

        x_d = doubleArray(10)

        for i in range(10):
            x_d[i] = i * 5

        self.ge["x"] = x_d

        self.ge.executeString("print x")

        self.ge["x"] = 10
        self.assertEqual(10, self.ge["x"][0])

        self.ge["x"] = 11.0
        self.assertEqual(11.0, self.ge["x"][0])

    def testArrays(self):
        self.ge.executeString("ai = seqa(1,1,24); aj = seqa(25,1,24);")

        # Create a complex 3x3x3 array
        self.ge.executeString("as = areshape(ai, 2|3|4); at = areshape(aj, 2|3|4); au = complex(as,at)")

        # Retrieve the array from the symbol table
        auArray = self.ge.getArray("au")

        self.assertEqual([2, 3, 4], list(auArray.getOrders()))
        self.assertEqual(3, auArray.getDimensions())

        self.assertEqual(24, auArray.size()) # complex
        self.assertEqual(24, len(auArray)) # complex
        self.assertEqual([float(i) for i in range(1, 25)], list(auArray.getData()))
        self.assertEqual([float(i) for i in range(25, 49)], list(auArray.getImagData()))

        # Extract a plane from the array
        self.assertEqual([5, 6, 7, 8, 17, 18, 19, 20], list(auArray.getPlane([0, 2, 0]).getData()))
        self.assertEqual([6, 18], list(auArray.getVector([0, 2, 2])))

        # We are directly passing in the orders, followed by the data
        a2 = GEArray([2, 2, 2], [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0])

        # Add it to the symbol table
        self.ge.setSymbol(a2, "a2")

        a3 = self.ge.getArrayAndClear("a2")
        self.assertEqual([2, 2, 2], list(a3.getOrders()))
        self.assertEqual(3, a3.getDimensions())

        a2 = self.ge.getArray("a2")
        self.assertEqual([1], list(a2.getOrders()))
        self.assertEqual(1, a2.getDimensions())
        self.assertEqual([0], list(a2.getData()))

    def testStrings(self):
        geStr = "Hello World"

        # Add it to the symbol table
        self.ge.setSymbol(geStr, "s")

        s = self.ge.getString("s")
        self.assertEqual("Hello World", s)

        self.ge["r"] = "testing"
        self.assertEqual("testing", self.ge["r"][0])

        # create a temp workspace
        tempWh = self.ge.createWorkspace("temp")

        # Change the value of 's' in GAUSS
        self.ge.executeString("s = \"Goodbye World\"", tempWh)
        self.ge.executeString("print s", tempWh)

        s = self.ge.getString("s")
        self.assertEqual("Hello World", s)

        s = self.ge.getString("s", tempWh)
        self.assertEqual("Goodbye World", s)

        # Here we can verify that the symbol type of 's' is in fact a string
        self.assertEqual(self.ge.getSymbolType("s"), GESymType.STRING)
        self.assertEqual(self.ge.getSymbolType("s", tempWh), GESymType.STRING)

        # Retrieve it from the symbol table
        s = self.ge.getString("s")
        self.assertEqual(11, len(s))

    def testStringArrays(self):
        # Create an example string array using GAUSS code.
        self.ge.executeString("string sa = { this is a test, one two three four, five six seven eight }")

        # Retrieve the string array from the symbol table
        sa = self.ge.getStringArray("sa")

        self.assertNotEquals(None, sa)

        self.assertEqual(12, sa.size())
        self.assertEqual(12, len(sa))

        self.assertEqual("EIGHT", sa.getElement(2,3))
        self.assertEqual("EIGHT", sa[11]) # 2*4 + 3

        # Create a string array in PHP
        sa2 = ["one", "two", "three", "four"]

        gesa2 = GEStringArray(sa2, 2, 2)

        # Add it to the symbol table
        self.assertTrue(self.ge.setSymbol(gesa2, "sa2"))

        sa = self.ge.getStringArray("sa2")
        self.assertNotEquals(None, sa)
        self.assertEqual(4, sa.size())
        self.assertEqual(sa2, list(sa.getData()))

        for i in range(1, len(sa2) + 1):
            self.assertEqual(sa2[i - 1], sa[i - 1])
            self.assertEqual(sa2[-i], sa[-i])
#    def tearDown(self):
#        self.ge.shutdown()

suite = unittest.TestLoader().loadTestsFromTestCase(TestGAUSSEngine)
unittest.TextTestRunner(verbosity=3).run(suite)

